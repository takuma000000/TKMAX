#include "Player.h"

void Player::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon, Camera* camera, Input* input) {
	dxCommon_ = dxCommon;
	obj3dCo_ = object3dCommon;
	camera_ = camera;
	input_ = input;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(obj3dCo_, dxCommon_);
	object3d_->SetModel("player.obj");

	// 初期位置の設定
	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, 0.0f, 0.0f };
}

void Player::Update() {
	// 入力による移動
	if (input_->PushKey(DIK_W)) {
		transform_.translate.y += 0.1f;
	}
	if (input_->PushKey(DIK_S)) {
		transform_.translate.y -= 0.1f;
	}
	if (input_->PushKey(DIK_A)) {
		transform_.translate.x -= 0.1f;
	}
	if (input_->PushKey(DIK_D)) {
		transform_.translate.x += 0.1f;
	}


	// オブジェクトの更新
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);
	object3d_->Update();
}

void Player::Draw() {
	object3d_->Draw(dxCommon_);
}
