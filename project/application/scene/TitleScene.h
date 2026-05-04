#pragma once
#include "BaseScene.h"
#include "Sprite.h"

//=============================================================
// TitleSceneクラス
// タイトル画面を管理するシーンクラス。
//=============================================================
class TitleScene : public TKM::BaseScene {
public:
	TitleScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager) : dxCommon_(dxCommon), srvManager_(srvManager) {}

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;
	void Draw3D() override;
	void DrawSprite() override;
	void DrawBack() override;

private:
	//======================================================================
	// システム参照
	//======================================================================
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::SrvManager* srvManager_ = nullptr;
	TKM::Camera* camera_ = nullptr; // 今フレームのアクティブカメラ（CameraManagerから取得）
	std::unique_ptr<TKM::Sprite> backSprite_ = nullptr;
};