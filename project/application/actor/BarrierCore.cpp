#define NOMINMAX
#include "BarrierCore.h"
#include "ParticleManager.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void BarrierCore::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");

	if (camera_) {
		object_->SetCamera(camera_);
	}
	if (parentScene_) {
		object_->SetParentScene(parentScene_);
	}

	object_->SetScale(baseScale_);
	object_->SetColor({ 1.0f, 0.35f, 0.8f, 1.0f });
}

void BarrierCore::Update(float dt) {
	if (!object_ || isDead_) {
		return;
	}

	if (isDying_) {
		deathTimer_ += dt;

		float t = deathTimer_ / deathDuration_;
		if (t < 0.0f) { t = 0.0f; }
		if (t > 1.0f) { t = 1.0f; }

		const float s = 1.0f - t * 0.35f;
		object_->SetScale({
			baseScale_.x * s,
			baseScale_.y * s,
			baseScale_.z * s
			});

		deathAlpha_ = 1.0f - t;
		object_->SetColor({ 1.0f, 0.35f, 0.8f, deathAlpha_ });
		object_->Update();

		if (deathTimer_ >= deathDuration_) {
			isDead_ = true;
		}
		return;
	}

#ifdef USE_IMGUI
	// ───────── BarrierCore 当たり判定ワイヤーボックス描画 ─────────
	{
		Vector3 center_ = GetWorldPosition();
		Vector3 size_ = colliderScale_;

		auto* lr_ = TKM::LineRenderer::GetInstance();
		if (lr_) {
			TKM::LineRenderer::Color normal_{ 1.0f, 0.2f, 1.0f, 1.0f };
			TKM::LineRenderer::Color dying_{ 1.0f, 0.5f, 0.2f, 1.0f };

			lr_->AddAABB(center_, size_, isDying_ ? dying_ : normal_);
		}
	}
#endif

	object_->Update();
}

void BarrierCore::Draw(TKM::DirectXCommon* dxCommon) {
	if (!object_ || isDead_) {
		return;
	}
	object_->Draw(dxCommon);
}

void BarrierCore::ImGuiDebug() {
#ifdef USE_IMGUI
	if (!object_) {
		return;
	}

	Vector3 pos_ = object_->GetTranslate();
	Vector3 scale_ = baseScale_;
	Vector3 collider_ = colliderScale_;
	int hp_ = this->hp_;

	if (ImGui::TreeNode("BarrierCore")) {
		ImGui::Text("Dead: %s", isDead_ ? "true" : "false");
		ImGui::Text("Dying: %s", isDying_ ? "true" : "false");

		if (ImGui::DragFloat3("位置", &pos_.x, 0.01f)) {
			object_->SetTranslate(pos_);
		}
		if (ImGui::DragFloat3("見た目スケール", &scale_.x, 0.01f, 0.01f, 50.0f)) {
			SetScale(scale_);
		}
		if (ImGui::DragFloat3("当たり判定サイズ", &collider_.x, 0.01f, 0.01f, 50.0f)) {
			SetColliderScale(collider_);
		}
		if (ImGui::DragInt("HP", &hp_, 1.0f, 1, 999)) {
			SetHP(hp_);
		}

		ImGui::TreePop();
	}
#endif
}

void BarrierCore::OnHitWithDamage(int damage) {
	if (isDead_ || isDying_) {
		return;
	}

	hp_ -= damage;
	if (hp_ > 0) {
		return;
	}

	hp_ = 0;
	isDying_ = true;
	defeated_ = true;
	deathTimer_ = 0.0f;
	deathAlpha_ = 1.0f;

	TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
	if (pm_) {
		const Vector3 pos = GetWorldPosition();
		pm_->Emit("enemyDeath_core", pos, 1);
		pm_->Emit("enemyDeath_shard", pos, 14);
		pm_->Emit("enemyDeath_smoke", pos, 3);
	}
}

Vector3 BarrierCore::GetWorldPosition() const {
	if (!object_) {
		return { 0.0f, 0.0f, 0.0f };
	}
	return object_->GetTranslate();
}

void BarrierCore::SetCamera(TKM::Camera* camera) {
	camera_ = camera;
	if (object_) {
		object_->SetCamera(camera);
	}
}

void BarrierCore::SetParentScene(TKM::BaseScene* scene) {
	parentScene_ = scene;
	if (object_) {
		object_->SetParentScene(scene);
	}
}

void BarrierCore::SetPosition(const Vector3& pos) {
	if (object_) {
		object_->SetTranslate(pos);
	}
}

void BarrierCore::SetScale(const Vector3& scale) {
	baseScale_ = scale;
	if (object_) {
		object_->SetScale(scale);
	}
}

void BarrierCore::SetColliderScale(const Vector3& scale) {
	colliderScale_ = scale;
}

void BarrierCore::SetModel(const std::string& modelName) {
	if (object_) {
		object_->SetModel(modelName);
	}
}

void BarrierCore::SetHP(int hp) {
	hp_ = hp;
	maxHP_ = hp;
}

void BarrierCore::SyncTransform() {
	if (object_) {
		object_->Update();
	}
}