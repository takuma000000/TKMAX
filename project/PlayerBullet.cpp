#include "PlayerBullet.h"
#include "MyMath.h"

void PlayerBullet::Initialize(DirectXCommon* dxCommon, const Vector3& startPos, const Vector3& targetPos) {
	dxCommon_ = dxCommon;
	position_ = startPos;

	ModelManager::GetInstance()->LoadModel("cube.obj", dxCommon_); // 弾のモデルをロード

	Vector3 direction = targetPos - startPos;
	direction = MyMath::Normalize(direction); // 自作Normalize関数

	velocity_ = direction * speed_;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	object3d_->SetModel("cube.obj"); // 弾のモデル
	object3d_->SetScale({ 0.2f, 0.2f, 0.2f }); // 小さくする
	object3d_->SetTranslate(position_);
}

void PlayerBullet::Update() {
	position_ += velocity_;
	object3d_->SetTranslate(position_);
	object3d_->Update();

	// 範囲外チェック（仮）
	if (MyMath::Length(position_) > 100.0f) {
		isDead_ = true;
	}

}

void PlayerBullet::Draw() {
	object3d_->Draw(Object3dCommon::GetInstance()->GetDxCommon());
}

void PlayerBullet::SetCamera(Camera* cam)
{
	if (object3d_) {
		object3d_->SetCamera(cam);
	}
}
