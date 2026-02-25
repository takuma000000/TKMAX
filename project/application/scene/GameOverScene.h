#pragma once
#include "BaseScene.h"
#include <memory>
#include "DirectXCommon.h"
#include "srvManager.h"
#include "camera/Camera.h"
#include "Player.h"
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

//=============================================================
// GameOverScene
// ゲームオーバー画面を管理するシーンクラス。
//=============================================================
class GameOverScene : public TKM::BaseScene{
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
	std::unique_ptr<Player> player_;
	std::unique_ptr<TKM::DirectionalLight> dirLight_;
	std::unique_ptr<TKM::Skybox> skybox_; // 背景スカイボックス
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
	// 墜落演出（炎 / 火花 / 失速スピン）
	//======================================================================
	// 墜落演出（発生位置 & タイマー）
	Vector3 crashOffset_ = { -0.6f, -0.9f, 0.2f }; // 機体原点からの出火ポイント
	float   flameTimer_ = 0.0f; // 炎バースト用のタイマー
	float   flameInterval_ = 0.6f; // 炎バースト間隔（秒）可変
	// 失速スピン用の角速度（ラジアン/秒）
	Vector3 tumbleSpeed_ = { 0.8f, 1.2f, 0.6f }; // x,y,z の回転速度
	bool tumbleActive_ = true; // 失速スピン中かどうか
	// 故障スポット（ローカル座標）と各スポットのクールダウン
	std::vector<Vector3> faultLocal_;       // 機体ローカル（翼/エンジン/尾など）
	std::vector<float>   faultCD_;          // 秒
	std::vector<float>   faultNext_;        // 次に噴くまでの残り秒
	// 1フレの炎/火花 発生総量の上限
	int perFrameFlameBudget_ = 40; // 墜落演出の炎は多めに
	int perFrameSparkBudget_ = 25; // 火花は炎より少なめに
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
	int fallFrameToggle_ = 0; // 落下中の炎/火花の発生を抑制するため、フレごとに交互に発生させるトグル
	int riseFrameToggle_ = 0; // 上昇中の炎/火花の発生を抑制するため、フレごとに交互に発生させるトグル
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
};