#include "Skydome.h"
#include "ModelManager.h"

void Skydome::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	obj3dCo_ = object3dCommon;
	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(obj3dCo_, dxCommon_);
	//ModelManager::GetInstance()->LoadModel("SkyDome.obj", dxCommon_);
	object3d_->SetModel("SkyDome.obj");

	//初期位置の設定
	transform_.scale = { 80.0f,80.0f,80.0f };
	transform_.rotate = { 0.0f,0.0f,0.0f };
	transform_.translate = { 0.0f,0.0f,0.0f };
}

void Skydome::Update() {
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);

	object3d_->Update();
}

void Skydome::Draw() {
	object3d_->Draw(dxCommon_); // 必要な引数を渡して描画
}

void Skydome::SetCamera(Camera* camera) {
	object3d_->SetCamera(camera);
}
