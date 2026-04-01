#include "BarrierCoreManager.h"
#include "Player.h"
#include "reticle/Reticle.h"
#include <algorithm>

void BarrierCoreManager::Initialize(
	TKM::Object3dCommon* common,
	TKM::DirectXCommon* dxCommon,
	TKM::Camera* camera,
	TKM::BaseScene* parent,
	Player* player
) {
	common_ = common;
	dxCommon_ = dxCommon;
	camera_ = camera;
	parent_ = parent;
	player_ = player;
}

void BarrierCoreManager::Update(float dt) {
	for (auto it = cores_.begin(); it != cores_.end();) {
		if (!(*it)) {
			it = cores_.erase(it);
			continue;
		}

		(*it)->Update(dt);

		if ((*it)->IsDead()) {
			if (player_) {
				player_->OnMidBossCoreDestroyed((*it).get());
			}
			it = cores_.erase(it);
		} else {
			++it;
		}
	}

	SyncPlayerTarget_();
}

void BarrierCoreManager::Draw(TKM::DirectXCommon* dxCommon) {
	for (auto& core : cores_) {
		if (!core) {
			continue;
		}
		core->Draw(dxCommon);
	}
}

void BarrierCoreManager::Clear() {
	cores_.clear();
	if (player_) {
		player_->SetMidBossCore(nullptr);
	}
}

void BarrierCoreManager::Spawn(const Vector3& center) {
	Clear();

	const float radius_ = kBarrierOuterRadius_ + kCoreOuterMargin_;
	const float stepDeg_ = 360.0f / static_cast<float>(kCoreCount_);

	for (int i = 0; i < kCoreCount_; ++i) {
		const float angleDeg_ = kCoreRingStartAngleDeg_ + stepDeg_ * static_cast<float>(i);
		const float angleRad_ = angleDeg_ * 3.1415926535f / 180.0f;

		Vector3 pos = center;
		pos.x += std::cos(angleRad_) * radius_;
		pos.y += std::sin(angleRad_) * radius_;
		pos.z += kCoreZOffset_;

		SpawnOne_(pos);
	}

	SyncPlayerTarget_();
}

bool BarrierCoreManager::IsAllDestroyed() const {
	for (const auto& core : cores_) {
		if (!core) {
			continue;
		}
		if (!core->IsDead() && !core->IsDying()) {
			return false;
		}
	}
	return true;
}

int BarrierCoreManager::GetAliveCount() const {
	int count_ = 0;

	for (const auto& core : cores_) {
		if (!core) {
			continue;
		}
		if (!core->IsDead() && !core->IsDying()) {
			++count_;
		}
	}

	return count_;
}

std::vector<MidBossCore*> BarrierCoreManager::GetAliveCores() const {
	std::vector<MidBossCore*> result;
	result.reserve(cores_.size());

	for (const auto& core : cores_) {
		if (!core) {
			continue;
		}
		if (core->IsDead() || core->IsDying()) {
			continue;
		}
		result.push_back(core.get());
	}

	return result;
}

void BarrierCoreManager::SetCamera(TKM::Camera* camera) {
	camera_ = camera;

	for (auto& core : cores_) {
		if (!core) {
			continue;
		}
		core->SetCamera(camera_);
	}
}

void BarrierCoreManager::SetParentScene(TKM::BaseScene* parent) {
	parent_ = parent;

	for (auto& core : cores_) {
		if (!core) {
			continue;
		}
		core->SetParentScene(parent_);
	}
}

void BarrierCoreManager::SetPlayer(Player* player) {
	player_ = player;

	for (auto& core : cores_) {
		if (!core) {
			continue;
		}
		core->SetReticle(player_ ? player_->GetReticle() : nullptr);
		core->SetPlayer([this]() {
			return player_ ? player_->GetPosition() : Vector3{ 0.0f, 0.0f, 0.0f };
			});
	}

	SyncPlayerTarget_();
}

void BarrierCoreManager::SpawnOne_(const Vector3& pos) {
	if (!common_ || !dxCommon_) {
		return;
	}

	auto core_ = std::make_unique<MidBossCore>();
	core_->Initialize(common_, dxCommon_);
	core_->SetCamera(camera_);
	core_->SetParentScene(parent_);
	core_->SetScale(kCoreScale_);
	core_->SetColliderScale(kCoreColliderScale_);
	core_->SetHP(kCoreHP_);
	core_->SetPosition(pos);
	core_->SetReticle(player_ ? player_->GetReticle() : nullptr);
	core_->SetPlayer([this]() {
		return player_ ? player_->GetPosition() : Vector3{ 0.0f, 0.0f, 0.0f };
		});
	core_->SyncTransform();

	cores_.push_back(std::move(core_));
}

MidBossCore* BarrierCoreManager::FindFirstAliveCore_() const {
	for (const auto& core : cores_) {
		if (!core) {
			continue;
		}
		if (!core->IsDead() && !core->IsDying()) {
			return core.get();
		}
	}
	return nullptr;
}

void BarrierCoreManager::SyncPlayerTarget_() {
	if (!player_) {
		return;
	}

	player_->SetMidBossCore(FindFirstAliveCore_());
}