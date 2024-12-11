#include "Skydome.h"
#include "ModelManager.h"

void Skydome::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	obj3dCo_ = object3dCommon;
	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(obj3dCo_, dxCommon_);
	//ModelManager::GetInstance()->LoadModel("SkyDome.obj", dxCommon_);
	object3d_->SetModel("SkyDome.obj");

	// 初期位置の設定
	object3d_->SetScale(scale_);
	object3d_->SetRotate(rotate_);
	object3d_->SetTranslate(translate_);
}

void Skydome::Update() {
	object3d_->SetScale(scale_);
	object3d_->SetRotate(rotate_);
	object3d_->SetTranslate(translate_);
	object3d_->Update();
}

void Skydome::Draw() {
	if (dxCommon_ == nullptr) {
		return; // 描画をスキップ
	}
	object3d_->Draw(dxCommon_); // 必要な引数を渡して描画
}

void Skydome::SetCamera(Camera* camera) {
	object3d_->SetCamera(camera);
}
