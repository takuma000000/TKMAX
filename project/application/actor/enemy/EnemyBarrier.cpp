#include "EnemyBarrier.h"
#include "Player.h"

void EnemyBarrier::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon_);
	object_->SetModel("sphere.obj");

	if (camera_) {
		object_->SetCamera(camera_);
	}

	// 初期状態をPlayerに同期させる
	UpdateVisual_();
}

void EnemyBarrier::Update() {
	UpdateVisual_();
	SyncToPlayer();
}

void EnemyBarrier::Draw(TKM::DirectXCommon* dxCommon) {
	/*if (!active_ || !visible_ || !object_ || !barrierCommon_) {
		return;
	}

	barrierCommon_->DrawSetCommon();
	object_->Draw(dxCommon);*/
	return;
}

void EnemyBarrier::SetCamera(TKM::Camera* camera) {
	camera_ = camera;
	if (object_) {
		object_->SetCamera(camera_);
	}
}

void EnemyBarrier::SetPlayer(Player* player) {
	player_ = player;
}

void EnemyBarrier::SetActive(bool active) {
	active_ = active;

	// 無効になった時はPlayer側のヒット情報も整理させる
	SyncToPlayer();
}

void EnemyBarrier::SetCenter(const Vector3& center) {
	center_ = center;
}

void EnemyBarrier::SetRadius(float radius) {
	radius_ = radius;
}

void EnemyBarrier::SyncToPlayer() {
	if (!player_) {
		return;
	}

	player_->SetWave1BarrierInfo(
		active_,
		center_,
		GetAABBSize()
	);
}

void EnemyBarrier::UpdateVisual_() {
	if (!object_) {
		return;
	}

	object_->SetTranslate(center_);
	object_->SetScale({ radius_, radius_, radius_ });
	object_->SetColor(color_);
	object_->Update();
}