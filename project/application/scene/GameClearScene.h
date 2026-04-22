#pragma once
#include "BaseScene.h"

#include <memory>
#include "WindowsAPI.h"
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
#include "Enemy.h"
#include "BossEnemy.h"
#include "TimeScaleController.h"
#include "BossConfig.h"
#include "StateMachine.h"
#include <array>

//=============================================================
// GameClearSceneクラス
// ゲームクリア画面を管理するシーンクラス。
// 背景スカイボックス回転＋自機のジェットコースター演出。
//=============================================================
class GameClearScene : public TKM::BaseScene, public TKM::IStateContext {
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
	float skyRotSpeedX_ = 0.002f;  // X軸回転速度
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
	Vector3 clearParticleGlobalOffset_ = { -0.45f, -6.45f, 8.5f }; // クリアシーン全体のパーティクル発生位置補正
	Vector3 clearBannerBurstOffset_ = { 7.5f, 1.55f, 10.1f }; // GAME CLEARバースト位置補正
	bool debugEmitClearBannerBurst_ = false;                  // デバッグ用：常時発生ON/OFF
	float debugEmitClearBannerBurstTimer_ = 0.0f;             // デバッグ用：連続発生タイマー
	float debugEmitClearBannerBurstInterval_ = 0.15f;         // デバッグ用：発生間隔
	//======================================================================
	// UI（クリア表示）
	//======================================================================
	// 「GAME CLEAR」用スプライト
	std::unique_ptr<TKM::Sprite> clearSprite_;
	// クリアスプライト表示ON/OFFフラグ（カメラ演出が終わるまではOFF）
	bool isClearSpriteVisible_ = false;
	// クリアスプライト表示演出（アルファだけイージング）
	bool isClearSpriteFadePlaying_ = false; // フェード演出中か
	float clearSpriteFadeTime_ = 0.0f; // フェード演出経過時間
	float clearSpriteFadeDuration_ = 0.35f; // フェード演出時間
	Ease::Type clearSpriteFadeEaseType_ = Ease::Type::OutSine; // フェード演出のイージングタイプ

	// クリアスプライト位置ポップ演出
	bool isClearSpritePopPlaying_ = false; // ポップ演出中か
	float clearSpritePopTime_ = 0.0f; // 演出経過時間
	float clearSpritePopDuration_ = 0.35f; // 演出時間

	bool isClearMenuVisible_ = false; // GAME CLEAR と同時に出す

	Vector2 clearSpriteCenterPos_ = {
		static_cast<float>(TKM::WindowsAPI::GetClientWidth()) * 0.5f,
		static_cast<float>(TKM::WindowsAPI::GetClientHeight()) * 0.5f
	}; // 最終表示位置（画面中央）

	Vector2 clearSpriteStartPos_ = {
		static_cast<float>(TKM::WindowsAPI::GetClientWidth()) * 0.5f,
		static_cast<float>(TKM::WindowsAPI::GetClientHeight()) * 0.5f + 70.0f
	}; // 演出開始位置（少し下）
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
	std::unique_ptr<TKM::PostEffectController> postFx_ = nullptr;    // クリア画面用ポストエフェクト
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

	bool blurReleased_ = false;          // 一度終点に到達してブラー解除済みか
	float cameraBlurStrength_ = 0.85f;   // クリアシーン中の固定ブラー強度
	//======================================================================
	// クリア祝福パーティクル
	//======================================================================
	float celebrateCoreTimer_ = 0.0f; // 祝福の光の中心コア用タイマー
	float celebrateSparkTimer_ = 0.0f; // 祝福の光の中心スパーク用タイマー
	float celebrateRayTimer_ = 0.0f; // 祝福の光の線（レイ）用タイマー
	bool celebrateFinalBurstDone_ = false; // 最後の大きな爆発エフェクトを出したかどうか
	float clearStageFireTimer_ = 0.0f; // クリアステージの火エフェクト用タイマー
	bool clearStageFireActive_ = false; // クリアステージの火エフェクトを出すかどうか

	Vector3 clearCelebrateOffset_ = { 0.0f, 2.8f, 6.0f };     // 祝福演出の基準位置
	//Vector3 clearBannerBurstOffset_ = { 0.0f, 3.0f, 6.5f };   // GAME CLEAR表示時バースト

	// クリアステージの火エフェクトの位置（6箇所）
	std::array<Vector3, 6> clearStageFirePositions_ = {
		Vector3{ -18.0f, -6.5f, 18.0f },
		Vector3{ -10.5f, -6.5f, 20.0f },
		Vector3{ -3.0f,  -6.5f, 21.5f },
		Vector3{  4.5f,  -6.5f, 21.0f },
		Vector3{ 12.0f,  -6.5f, 19.5f },
		Vector3{ 19.0f,  -6.5f, 17.5f }
	};
	//======================================================================
	// クリア後コミカル逃走演出
	//======================================================================
	TKM::StateMachine clearComedySM_; // クリア後のコミカル逃走演出の状態遷移マシン
	float clearComedyTimer_ = 0.0f; // 演出の進行管理用タイマー

	bool clearComedyActorsSpawned_ = false; // ボスとザコを出現させたかどうか
	bool clearComedyFallSlowRequested_ = false; // 転ぶ瞬間のスローを開始したかどうか
	// ボスと雑魚敵
	std::unique_ptr<BossEnemy> clearComedyBoss_;
	std::unique_ptr<Enemy> clearComedyMobA_;
	std::unique_ptr<Enemy> clearComedyMobB_;
	bool clearComedyMobBFallEffectPlayed_ = false; // 雑魚敵Bの転ぶエフェクトを出したかどうか
	bool clearComedyMobBSlipEffectPlayed_ = false; // 雑魚敵Bの滑るエフェクトを出したかどうか
	bool clearComedyNoticeMarkPlayed_ = false; // 気づきマークを出したかどうか
	// 逃走演出用タイムスケールコントローラー
	TKM::TimeScaleController clearComedyTimeScale_;

	// ボス用設定
	BossEnemyConfig clearComedyBossConfig_{};

	// 出現位置
	Vector3 clearComedyBossStartPos_ = { -3.0f, 1.0f, 38.0f };
	Vector3 clearComedyMobAStartPos_ = { -13.0f, -2.2f, 34.0f };
	Vector3 clearComedyMobBStartPos_ = { 7.0f, -2.2f, 35.5f };
	// 逃走目標地点
	Vector3 clearComedyBossEscapePos_ = { 20.0f, 5.3f, 60.0f };
	Vector3 clearComedyMobAEscapePos_ = { 12.0f, 1.5f, 62.0f };
	Vector3 clearComedyMobBEscapePos_ = { 26.0f, 1.0f, 58.0f };
	// 転ぶ位置・回転
	Vector3 clearComedyMobBFallPos_ = { 15.0f, -0.2f, 45.0f };
	Vector3 clearComedyMobBFallRot_ = { 0.0f, -0.9f, 1.25f };
	// 最後に画面外まで逃がすための退場先
	Vector3 clearComedyBossExitPos_ = { 55.0f, 6.0f, 78.0f };
	Vector3 clearComedyMobAExitPos_ = { 55.0f, 1.7f, 82.0f };
	Vector3 clearComedyMobBExitPos_ = { 55.0f, 1.5f, 76.0f };
	// フェーズ切り替え時の位置を保持して瞬間移動を防ぐ
	Vector3 clearComedyBossRunStartPos_ = {};
	Vector3 clearComedyMobARunStartPos_ = {};
	Vector3 clearComedyMobBRunStartPos_ = {};
	// 転ぶ前の位置を保持して瞬間移動を防ぐ
	Vector3 clearComedyBossRecoverStartPos_ = {};
	Vector3 clearComedyMobARecoverStartPos_ = {};
	Vector3 clearComedyMobBRecoverStartPos_ = {};

	/// <summary>
	/// クリア後のコミカル逃走演出を更新します。
	/// </summary>
	void SetupClearComedyBossConfig_();
	/// <summary>
	/// クリア後のコミカル逃走演出でボスと雑魚敵を出現させます。
	/// </summary>
	void SpawnClearComedyActors_();

	// StateMachineの状態クラスをフレンド宣言して、状態クラスからシーンのprivateメンバにアクセスできるようにする
	friend class ClearComedyWaitAfterClearState;
	friend class ClearComedySpawnState;
	friend class ClearComedySlowNoticeState;
	friend class ClearComedyRunAwayState;
	friend class ClearComedyFallDownState;
	friend class ClearComedyStandUpState;
	friend class ClearComedyRecoverRunState;
	friend class ClearComedyDoneState;
};