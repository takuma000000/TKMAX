#pragma once

#include "BaseScene.h"
#include "CameraManager.h"
#include "DirectionalLight.h"

#include <memory>

//=============================================================
// TitleSceneクラス
// タイトルシーンを管理するシーンクラス。
//=============================================================
class TitleScene : public TKM::BaseScene {
public:
	TitleScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager)
		: dxCommon_(dxCommon),
		srvManager_(srvManager) {}

	~TitleScene() = default;

	const char* GetSceneName() const override {
		return "TITLE";
	}

	/// <summary>
	/// タイトルシーンを初期化します。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// タイトルシーンを終了します。
	/// </summary>
	void Finalize() override;
	/// <summary>
	/// タイトルシーンを更新します。
	/// </summary>
	void Update() override;
	/// <summary>
	/// タイトルシーンを描画します。
	/// </summary>
	void Draw() override;
	/// <summary>
	/// 3Dオブジェクトを描画します。
	/// </summary>
	void Draw3D() override;
	/// <summary>
	/// スプライトを描画します。
	/// </summary>
	void DrawSprite() override;
	/// <summary>
	/// 背景を描画します。
	/// </summary>
	void DrawBack() override;
	/// <summary>
	/// ImGuiでデバッグ表示を行います。
	/// </summary>
	void ImGuiDebug();

	/// <summary>
	/// スプライトを初期化します。
	/// </summary>
	void InitializeSprite();
	/// <summary>
	/// オブジェクトを初期化します。
	/// </summary>
	void InitializeObjects();
	/// <summary>
	/// カメラを初期化します。
	/// </summary>
	void InitializeCamera();

	/// <summary>
	/// アクティブなカメラを更新します。
	/// </summary>
	TKM::Camera* UpdateActiveCamera();

private:
	//======================================================================
	// 基本システム
	//======================================================================
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::SrvManager* srvManager_ = nullptr;

	//======================================================================
	// ライト
	//======================================================================
	std::unique_ptr<TKM::DirectionalLight> directionalLight_ = nullptr;
};