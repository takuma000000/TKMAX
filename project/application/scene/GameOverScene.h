#pragma once
#include "BaseScene.h"
#include <memory>
#include "DirectXCommon.h"
#include "srvManager.h"
#include "Camera.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "DirectionalLight.h"
#include "ParticleManager.h"
#include <Easing.h>
#include "IrisUtil.h"
#include "GameResultMenuController.h"
#include "BossEnemy.h"
#include "NoiseEffect.h"

//=============================================================
// GameOverScene
// ゲームオーバー画面を管理するシーンクラス。
//=============================================================
class GameOverScene : public TKM::BaseScene {
public:
	GameOverScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager)
		: dxCommon_(dxCommon), srvManager_(srvManager) {
	}
	/// <summary>
	/// </summary>シーンを初期化します。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// </summary>シーンを終了します。
	/// </summary>
	void Finalize() override;
	/// <summary>
	/// </summary>シーンを更新します。
	/// </summary>
	void Update() override;
	/// <summary>
	/// </summary>シーンを描画します。
	/// </summary>
	void Draw() override;

private:
	//======================================================================
	// システム参照
	//======================================================================
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::SrvManager* srvManager_ = nullptr;
	//======================================================================
	// シーン構成（カメラ / ライト / 自機 / 背景）
	//======================================================================
	std::unique_ptr<TKM::Camera> camera_;
	std::unique_ptr<BossEnemy> boss_;
	std::unique_ptr<TKM::DirectionalLight> dirLight_;
	std::unique_ptr<TKM::Skybox> skybox_;
	float skyPitch_ = 0.0f;        // X軸回転量
	float skyRotSpeedX_ = 0.002f;  // X軸回転速度
	//======================================================================
	// 画面を覆う虹彩絞り演出用
	//======================================================================
	std::unique_ptr<TKM::Sprite> iris_;
	bool  irisOpening_ = true;     // 入場時は開き演出から
	bool  irisClosing_ = false;    // T押下で閉じ演出開始
	float irisScale_ = 0.0f;       // 現フレームのサイズ
	float irisMaxScale_ = 0.0f;    // 画面対角ベースの最大スケール
	Ease::Tween irisOpenTween_;    // 開き用（OutBack, 0.8s）
	Ease::Tween irisCloseTween_;   // 閉じ用（InBack, 0.8s）
	static constexpr float kIrisDuration_ = 0.8f; // 虹彩絞り演出時間
	//======================================================================
	// ゲームオーバー用ボス演出
	//======================================================================
	Vector3 bossBasePos_{ 0.0f, 2.0f, 42.0f };   // ボスの基準位置
	Vector3 bossPos_{ 0.0f, 2.0f, 42.0f };       // 現在位置
	Vector3 bossRot_{ 0.0f, 3.14f, 0.0f }; // 現在回転

	float bossAnimTimer_ = 0.0f;          // 常時アニメ用タイマー
	float bossJumpTimer_ = 0.0f;          // ジャンプ周期タイマー
	float bossJumpInterval_ = 2.4f;       // 次のジャンプまでの秒数
	float bossJumpDuration_ = 0.78f;      // 1回のジャンプ全体時間
	float bossJumpElapsed_ = 0.0f;        // ジャンプ開始からの経過
	float bossJumpHeight_ = 4.2f;         // ジャンプ高さ
	bool  bossJumping_ = false;           // ジャンプ中か
	bool  bossLandingShakeTriggered_ = false; // 着地シェイクを1回だけ出す

	Vector3 cameraBaseTranslate_{ 0.0f, 2.2f, -13.5f }; // シェイク前の基準カメラ位置
	float cameraShakeTimer_ = 0.0f;       // シェイク残り時間
	float cameraShakeDuration_ = 0.18f;   // シェイク継続時間
	float cameraShakeAmp_ = 0.45f;        // シェイク振幅
	//======================================================================
	// ジャンプ連動：上から崩れ落ちるストリーク
	//======================================================================
	float fallEmitTimer_ = 0.0f;          // 落下パーティクル発生タイマー
	float fallEmitInterval_ = 0.03f;      // 発生間隔
	int   fallFrameToggle_ = 0;           // 毎フレーム出しすぎ防止
	//======================================================================
	// 「GAME OVER」表示（フェード/スケール）
	//======================================================================
	// === 「GAME OVER」スプライト ===	
	std::unique_ptr<TKM::Sprite> overSprite_;
	// フェードイン（0→1）とスケール（0.8→1.0）
	Ease::Tween overAlphaTween_; // Alphaは単純に InOutQuad, 1.2s
	Ease::Tween overScaleTween_; // どちらも InOutBack, 1.2s
	bool overActive_ = false;   // アニメ進行フラグ
	float overAlpha_ = 0.0f;    // 現アルファ
	float overScale_ = 1.0f;    // 現スケール
	// --- 演出用（Update内 static を排除してカプセル化） ---
	float overGlowTimer_ = 0.0f; // 「GAME OVER」表示のグローエフェクト用タイマー
	//======================================================================
	// タイムステップ
	//======================================================================
	float dt_ = 1.0f / 60.0f; // 仮のデルタタイム
	//======================================================================
	// ゲームオーバーメニュー / 次アクション
	//======================================================================
	// --- ゲームオーバーメニュー ---
	std::unique_ptr<GameResultMenuController> overMenu_; // ゲームオーバーメニューコントローラー
	enum class NextAction { None, Restart, ReturnToTitle }; // 次のアクション（何もなし / リスタート / タイトルへ）
	NextAction nextAction_ = NextAction::None; // --- タイトルへ戻るためのフェードアウト ---
	//======================================================================
	// ノイズエフェクト
	//======================================================================
	std::unique_ptr<TKM::NoiseEffect> noiseEffect_; // ノイズエフェクト
	float noiseIntervalTimer_ = 0.0f;               // 次のノイズ発生までの経過
	float noiseNextInterval_ = 2.2f;                // 次にノイズが来るまでの時間
	float noiseDurationTimer_ = 0.0f;               // ノイズ発生中の経過
	float noiseCurrentDuration_ = 0.15f;            // 今回のノイズ継続時間
	bool isNoisePlaying_ = false;                   // 今ノイズ中か
};