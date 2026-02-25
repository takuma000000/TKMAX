#pragma once
#include "BaseScene.h"

#include <memory>
#include <cmath>
#include <vector>
#include "TextureManager.h"
#include "DirectXCommon.h"
#include "srvManager.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "camera/Camera.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "Input.h"
#include "SceneManager.h"
#include "GameScene.h"
#include <SkyBox.h> 
#include <Easing.h>
#include "GameOverScene.h"
#include "WaterRippleEffect.h"
#include "IrisUtil.h"
#include "TitleMenuController.h"
#include "Enemy.h"
#include <random>
#include "Player.h"
#include "BossEnemy.h"

//=============================================================
// TitleSceneクラス
// タイトル画面を管理するシーンクラス。
//=============================================================
class TitleScene : public TKM::BaseScene{
public:
	TitleScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager) : dxCommon_(dxCommon), srvManager_(srvManager) {}

	/// <summary>
	/// タイトルシーンを初期化します。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// タイトルシーンを終了処理します。
	/// </summary>
	void Finalize() override;
	/// <summary>
	/// タイトルシーンを更新します。
	/// </summary>
	void Update() override;
	/// <summary>
	/// タイトルシーンを描画します（互換用）。
	/// </summary>
	void Draw() override;
	/// <summary>
	/// 3Dを描画します（RenderTexture側）。
	/// </summary>
	void Draw3D() override;
	/// <summary>
	/// UI(Sprite)を描画します（Swapchain側）。
	/// </summary>
	void DrawSprite() override;
	/// <summary>
	/// 背景を描画します（Swapchain側）。タイトルシーンではスカイボックスを背景にしているため、ここで描画します。
	/// </summary>
	void DrawBack() override;

private:
	//======================================================================
	// システム参照
	//======================================================================
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::SrvManager* srvManager_ = nullptr;
	//======================================================================
	// シーン構成（2D / カメラ / ライト / 背景）
	//======================================================================
	std::unique_ptr<TKM::Sprite> sprite_ = nullptr; // 2Dスプライト共通
	std::unique_ptr<TKM::Camera> camera_ = nullptr; // カメラ
	std::unique_ptr<TKM::Skybox> skybox_ = nullptr;
	std::unique_ptr<TKM::DirectionalLight> dirLight_ = nullptr;
	// スカイボックスの回転制御
	float skyPitch_ = 0.0f; // スカイボックスのピッチ（X軸回転）角
	float skyRotSpeedX_ = 0.002f; // スカイボックスのX軸回転速度（ラジアン/フレーム）
	// カメラ
	float camDist_ = 20.0f;  // カメラ距離（+Z側）
	float camY_ = 3.0f;   // カメラ高さ
	//======================================================================
	// タイトル敵のシーケンス制御
	//======================================================================
	enum class Flow { // シーケンスの流れ
		IntroIrisOpen, // タイトル入場（アイリスオープン）
		Idle, // 待機
		Vanishing, // 消滅（UI非表示のまま）
		Ripple, // 波紋エフェクト発生
		IrisClose // タイトル退場（アイリスクローズ）
	};

	struct TitleEnemyUnit { // タイトル敵ユニット
		std::unique_ptr<Enemy> enemy_; // 敵オブジェクト
		float vanishDelay_ = 0.0f; // 消滅開始までの遅延時間（秒）
		bool alive_ = true; // 生存状態（trueで生きている、falseで消滅開始）
	};
	// タイトル敵の数式定義（π系）
	std::vector<TitleEnemyUnit> titleEnemies_; // タイトル敵ユニットのリスト
	Flow flow_ = Flow::IntroIrisOpen;
	std::mt19937 rng_{ std::random_device{}() }; // 乱数生成器
	bool showUi_ = true; // UI表示フラグ（trueで表示、falseで非表示）
	float seqTimer_ = 0.0f; // シーケンス全体の経過時間（秒）
	float vanishTimer_ = 0.0f; // 消滅シーケンスの経過時間（秒）
	float rippleTimer_ = 0.0f; // 波紋エフェクトの経過時間（秒）
	// シーケンスのタイミング定数
	static constexpr float kHideUiDelaySec_ = 0.10f; // UI非表示までの遅延時間（秒）
	static constexpr float kStartVanishDelaySec_ = 0.18f; // 消滅開始までの遅延時間（秒）
	static constexpr float kVanishDelayMaxSec_ = 0.65f; // 消滅遅延の最大時間（秒）
	static constexpr float kRippleWaitSec_ = 0.12f; // 波紋エフェクト発生までの待機時間（秒）
	int kEnemyCount = 100; // タイトル敵の数
	bool showMenuAfterVanish_ = false; // Vanishingが終わったあと、メニューを出すか？（A押しでtrue）
	/// <summary>
	/// タイトル敵を生成して配置します。
	/// </summary>
	void CreateTitleEnemies_();
	/// <summary>
	/// 消滅シーケンスを開始します。
	/// </summary>
	void ScheduleVanish_();
	/// <summary>
	/// 全てのタイトル敵が消滅したかチェックします。
	/// </summary>
	/// <returns>trueなら全て消滅、falseならまだ生きている敵がいる</returns>
	bool AllEnemiesGone_() const;
	/// <summary>
	/// タイトル敵の消滅エフェクトを発生させます。
	/// </summary>
	/// <param name="pos">エフェクトを発生させる位置</param>
	void EmitTitleExplode_(const Vector3& pos);
	//======================================================================
	// Iris（白円）トランジション
	//======================================================================
	std::unique_ptr<TKM::Sprite> iris_ = nullptr; // アイリス（白円）スプライト
	bool irisClosing_ = false;   // trueで「閉じる」演出中
	float irisScale_ = 0.2f;    // 開始スケール（小さめ）
	float irisSpeed_ = 2.8f;    // 拡大速度（好みで調整）
	float irisMax_ = 4.5f;      // これを超えたら画面を覆ったとみなす
	int irisHoldFrames_ = 0; // 閉じた状態を維持するフレーム数（0なら維持なし）
	float irisT_ = 0.0f;            // 進行度(0→1)
	float irisDuration_ = 0.8f;     // アニメ時間(秒)
	float irisStartScale_ = 10.0f;  // 開始サイズ
	float irisEndScale_ = 0.0f;     // 目標（Initializeでセット）
	Ease::Tween irisTween_; // イージング関数
	// アイリスのトランジション時間
	static constexpr float kIrisDurationSec_ = 0.8f;
	bool irisOpening_ = true; // trueで「開く」演出中
	static constexpr float kPi_ = 3.14159265358979323846f; // π
	static constexpr float kHalfPi_ = kPi_ * 0.5f;         // π/2
	static constexpr float kTwoPi_ = kPi_ * 2.0f;          // 2π
	//======================================================================
	// エフェクト / UI
	//======================================================================
	// 波紋エフェクト
	std::unique_ptr<TKM::WaterRippleEffect> rippleEffect_ = nullptr;
	// タイトルメニューコントローラー
	std::unique_ptr<TitleMenuController> titleMenu_ = nullptr; // タイトルメニューコントローラー
	//======================================================================
	// タイムステップ
	//======================================================================
	const float dt_ = 1.0f / 60.0f; // 固定フレームレート用デルタタイム
	//======================================================================
	// タイトル：メニュー中の見つめ合い（Player / Boss）
	//======================================================================
	std::unique_ptr<Player> titlePlayer_ = nullptr; // タイトル用プレイヤー（見た目だけ）
	std::unique_ptr<BossEnemy> titleBoss_ = nullptr; // タイトル用ボス（見た目だけ）
	Vector3 titlePlayerPos_ = { -12.0f, -3.8f, 13.3f }; // 左手前
	Vector3 titleBossPos_ = { 24.7f, 6.7f, 53.3f }; // 右奥
	// 回転（ラジアン想定）
	Vector3 titlePlayerRot_ = { 0.0f, 0.0f, 0.0f }; // 主にy(Yaw)を使う
	Vector3 titleBossRot_ = { 0.0f, 0.0f, 0.0f }; // 主にy(Yaw)を使う
	// 自動で見つめ合うか（Yaw自動）
	bool showdownAutoLook_ = true; // trueでPlayerがBossを見つめる（Yaw自動更新）、falseで両者とも正面向き固定
	// リセット用の初期値（今の値をそのまま固定したいならここを基準に）
	const Vector3 kShowdownDefaultPlayerPos_ = { -12.0f, -3.8f, 13.3f };
	const Vector3 kShowdownDefaultBossPos_ = { 24.7f,  6.7f, 53.3f };
	const Vector3 kShowdownDefaultPlayerRot_ = { 0.0f, 0.0f, 0.0f };
	const Vector3 kShowdownDefaultBossRot_ = { 0.0f, 0.0f, 0.0f };
	// 見つめ合い：回転調整（度）
	Vector3 titlePlayerRotDeg_ = { 0.0f, 0.0f, 0.0f }; // 手動オフセット（度）
	Vector3 titleBossRotDeg_ = { 0.0f, 0.0f, 0.0f }; // 手動オフセット（度）
	bool titleAutoLookAt_ = true; // trueなら自動で見つめ合う（Yaw/Pitch）
	/// <summary>
	/// タイトルの見つめ合い用のPlayerとBossを生成して配置します。
	/// </summary>
	void CreateShowdownActors_();
	/// <summary>
	/// タイトルの見つめ合い用のPlayerとBossを更新します。
	/// </summary>
	/// <param name="dt">デルタタイム</param>
	void UpdateShowdownActors_(float dt);
	/// <summary>
	/// タイトルの見つめ合い用のPlayerとBossを描画します。
	/// </summary>
	void DrawShowdownActors_();
	/// <summary>
	/// タイトルの見つめ合い用のPlayerとBossの、PlayerからBossへの向き（Yaw角）を計算します。
	/// </summary>
	/// <param name="from">Playerの位置</param>
	/// <param name="to">Bossの位置</param>
	/// <returns>PlayerからBossへの向き（Yaw角）</returns>
	float LookAtYaw_(const Vector3& from, const Vector3& to) const;
	//======================================================================
	// タイトル：ビーム撃ち合い
	//======================================================================
	bool  titleBeamActive_ = true;   // メニュー中にONにしたいならshowUi_と合わせて使う
	float titleClashEmitAcc_ = 0.0f; // 衝突エフェクトの発生レート調整用
	float titleBeamT_ = 0.5f;        // 衝突点（0=プレイヤー側, 1=ボス側）とりあえず0.5で中央
	int   titleBeamSegments_ = 18;   // 線上に置く粒の数（増やすほど“線”になる）
	int   titleBeamPerSeg_ = 1;      // 1セグメントに何粒置くか（重くなるので基本1）
	int   titleClashCore_ = 8;       // 衝突点のコア粒
	int   titleClashRays_ = 10;      // 衝突点のスパーク
	int   titleClashRing_ = 1;       // リング頻度
	/// <summary>
	/// タイトルのビーム撃ち合いの衝突点を更新します。
	/// </summary>
	/// <param name="dt">デルタタイム</param>
	void UpdateTitleBeamClash_(float dt);
	//======================================================================
	// Aボタン案内（A_title.png）
	//======================================================================
	std::unique_ptr<TKM::Sprite> aTitle_ = nullptr; // 「A」案内アイコン
	Vector2 aTitlePos_ = { 1280.0f * 0.5f, 720.0f - 90.0f }; // 画面中央下（中心座標）
	float aTitleScale_ = 0.16f; // サイズ倍率
	Vector2 aTitleTexSize_ = { 0.0f, 0.0f }; // テクスチャ元サイズ
	bool aTitleVisible_ = true; // 表示ON/OFF（ImGui用）
	/// <summary>
	/// 「A」案内アイコンのパラメータを適用します（位置・サイズ・透明度など）。ImGuiでaTitleVisible_をON/OFFするための関数。
	/// </summary>
	void ApplyATitleParams_();
	// A案内：フェード点滅
	float aTitleBlinkT_ = 0.0f;   // 経過時間
	float aTitleBlinkHz_ = 1.2f;  // 1秒あたりの往復回数（好みで）
	float aTitleAlphaMin_ = 0.25f; // 透明度の最小値（0.0fで完全に消える、1.0fで常に表示）
	float aTitleAlphaMax_ = 1.0f; // 透明度の最大値
	bool  aTitleBlink_ = true; // 点滅ON/OFF（ImGui用）
};