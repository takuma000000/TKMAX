#pragma once
#include "BaseScene.h"

//=============================================================
// GameSceneクラス
// ゲーム本編を管理するシーンクラス。
//=============================================================
class GameScene : public TKM::BaseScene {
public:
	GameScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager)
		: dxCommon_(dxCommon),
		srvManager_(srvManager) {}

	~GameScene() = default;

	/// <summary>
	/// ゲーム本編シーンを初期化します。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// ゲーム本編シーンを終了します。
	/// </summary>
	void Finalize() override;
	/// <summary>
	/// ゲーム本編シーンを更新します。
	/// </summary>
	void Update() override;
	/// <summary>
	/// ゲーム本編シーンを描画します。
	/// </summary>
	void Draw() override;
	/// <summary>
	/// ゲーム本編シーンの3Dオブジェクトを描画します。
	/// </summary>
	void Draw3D() override;
	/// <summary>
	/// ゲーム本編シーンのSpriteを描画します。
	/// </summary>
	void DrawSprite() override;
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
	/// <returns></returns>
	TKM::Camera* UpdateActiveCamera();

private:
	//======================================================================
	// 基本システム
	//======================================================================
	TKM::DirectXCommon* dxCommon_ = nullptr; // DirectX共通（デバイス/コマンド等）
	TKM::SrvManager* srvManager_ = nullptr; // SRV管理
	//======================================================================
	// カメラ / ライト / スカイボックス
	//======================================================================
	std::unique_ptr<TKM::DirectionalLight> directionalLight_ = nullptr; // ディレクショナルライト
};