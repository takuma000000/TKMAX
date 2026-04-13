#pragma once
#include "BaseScene.h"

#include <memory>
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Camera.h"
#include "Player.h"
#include "Object3dCommon.h"
#include "DirectionalLight.h"
#include <SkyBox.h>
#include "Vector3.h"
#include "MyMath.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include <Easing.h>
#include "IrisUtil.h"
#include "GameResultMenuController.h"
#include "PostEffectController.h"

//=============================================================
// GameClearSceneクラス
// ゲームクリア画面を管理するシーンクラス。
// 背景スカイボックス回転＋自機のジェットコースター演出。
//=============================================================
class GameClearScene : public TKM::BaseScene {
public:
	GameClearScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager)
		: dxCommon_(dxCommon), srvManager_(srvManager) {
	}
	~GameClearScene() = default;

	/// <summary>
	/// シーンを初期化します。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// シーンを終了します。
	/// </summary>
	void Finalize() override;
	/// <summary>
	/// シーンを更新します。
	/// </summary>
	void Update() override;
	/// <summary>
	/// シーンを描画します。
	/// </summary>
	void Draw() override;
	/// <summary>
	/// 3Dオブジェクトを描画します。
	/// </summary>
	void ImGuiDebug(); // デバッグUI

private:
	//======================================================================
	// システム参照
	//======================================================================
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::SrvManager* srvManager_ = nullptr;
	//======================================================================
	// カメラ・ライト
	//======================================================================
	// --- カメラ・ライト ---
	std::unique_ptr<TKM::Camera> camera_;
	std::unique_ptr<TKM::DirectionalLight> dirLight_;
	//======================================================================
	// 自機
	//======================================================================
	// --- 自機 ---
	std::unique_ptr<Player> player_;
	//======================================================================
	// スカイボックス
	//======================================================================
	// --- スカイボックス ---
	std::unique_ptr<TKM::Skybox> skybox_;
	float skyPitch_ = 0.0f;        // X軸回転量
	float skyRotSpeedX_ = 0.002f;  // X軸回転速度（GameOverSceneとほぼ同じ）
	//======================================================================
	// 自機クリア演出
	//======================================================================
	// --- 自機クリア演出用パラメータ ---
	const float dt_ = 1.0f / 60.0f;   // 固定フレーム（60fps想定）
	float planeTime_ = 0.0f;          // 経過時間(秒)
	float planeDuration_ = 6.0f;      // 左→右に抜けるまでの時間(秒)
	// 画面左外〜右外くらいの位置（ちょっと広めに取って完全に画面外スタート/ゴール）
	Vector3 planeStart_ = { -7.45f, -2.5f, 7.7f }; // 初期値は同じ（Updateで計算して上書き）
	Vector3 planeEnd_ = { -7.45f, -2.5f, 7.7f }; // 初期値は同じ（Updateで計算して上書き）
	Vector3 playerDisplayPos_ = { -7.45f, -2.5f, 7.7f }; // クリア画面でのプレイヤー表示位置（ImGui調整用）
	Vector3 playerDisplayRot_ = { 0.0f, 2.48f, 0.0f };    // クリア画面でのプレイヤー表示回転（ImGui調整用）
	//======================================================================
	// UI（クリア表示）
	//======================================================================
	// 「GAME CLEAR」用スプライト
	std::unique_ptr<TKM::Sprite> clearSprite_;
	//======================================================================
	// アイリス遷移
	//======================================================================
	// --- 画面遷移用アイリス（他シーンと同じ演出）---
	std::unique_ptr<TKM::Sprite> iris_;
	bool  irisOpening_ = true;     // 入場時は開き演出から
	bool  irisClosing_ = false;    // Aボタンで閉じ演出開始
	float irisScale_ = 0.0f;       // 現フレームのサイズ
	float irisMaxScale_ = 0.0f;    // 画面対角ベースの最大スケール
	Ease::Tween irisOpenTween_;    // 開き用（OutBack, 0.8s）
	Ease::Tween irisCloseTween_;   // 閉じ用（InBack, 0.8s）
	//======================================================================
	// メニュー
	//======================================================================
	std::unique_ptr<GameResultMenuController> clearMenu_; // クリア後のメニュー
	//======================================================================
	// シーン遷移状態
	//======================================================================
	enum class NextAction { // 次のアクション
		None,          // 何もしない
		Restart,       // リスタート
		ReturnToTitle  // タイトルへ戻る
	};
	NextAction nextAction_ = NextAction::None; // 次のアクション
	//======================================================================
	// ポストエフェクト
	//======================================================================
	std::unique_ptr<TKM::WaterRippleEffect> rippleEffect_ = nullptr; // 決定時の波紋
	//======================================================================
	// クリアシーン用カメラ演出
	//======================================================================
	Vector3 cameraStartPos_ = { 0.0f, 8.0f, -100.0f };   // 開始時：かなり引いた位置
	Vector3 cameraEndPos_ = { 0.0f, 3.0f, -20.0f };     // 終了時：今見せたい位置
	Vector3 cameraStartRot_ = { 0.28f, 0.0f, 0.0f };    // 開始時：少し見下ろし強め
	Vector3 cameraEndRot_ = { 0.1f, 0.0f, 0.0f };       // 終了時：今の角度

	float cameraMoveTime_ = 0.0f;                       // カメラ演出経過時間
	float cameraMoveDuration_ = 3.5f;                   // カメラ移動時間
	bool  enableCameraIntro_ = true;                    // カメラ導入演出ON/OFF

	Ease::Type cameraPosEaseType_ = Ease::Type::OutBack; // 位置：少し通り過ぎて戻る
	Ease::Type cameraRotEaseType_ = Ease::Type::OutSine; // 回転：自然に止める
};