#pragma once
#include "BaseScene.h"
#include "DirectXCommon.h"
#include "SrvManager.h"

//=============================================================
// GameClearSceneクラス
// ゲームクリア画面を管理するシーンクラス。
//=============================================================
class GameClearScene : public BaseScene {
public:
	GameClearScene(DirectXCommon* dxCommon, SrvManager* srvManager)
		: dxCommon(dxCommon), srvManager(srvManager) {
	}
	~GameClearScene() = default;

	/// <summary>シーンを初期化します。</summary>
	void Initialize() override;
	/// <summary>シーンを終了します。</summary>
	void Finalize() override;
	/// <summary>シーンを更新します。</summary>
	void Update() override;
	/// <summary>シーンを描画します。</summary>
	void Draw() override;

private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

	// UIなど必要に応じて
};
