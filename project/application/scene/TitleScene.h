#pragma once
#include "BaseScene.h"

#include <memory>
#include <cmath>
#include <vector>
#include "DirectXCommon.h"
#include "srvManager.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "CameraManager.h"
#include "Input.h"
#include "SceneManager.h"
#include <SkyBox.h> 
#include "WaterRippleEffect.h"
#include "TitleMenuController.h"
#include "Enemy.h"
#include <random>
#include "Player.h"
#include "BossEnemy.h"
#include "StateMachine.h"
#include "GameOverScene.h"
#include "GameScene.h"
#include "GameClearScene.h"
#include "TitleShowdownController.h"

//=============================================================
// TitleSceneクラス
// タイトル画面を管理するシーンクラス。
//=============================================================
class TitleScene : public TKM::BaseScene, public TKM::IStateContext {
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
	TKM::Camera* camera_ = nullptr; // 今フレームのアクティブカメラ（CameraManagerから取得）
	//======================================================================
	// シーン構成（2D / カメラ / ライト / 背景）
	//======================================================================
	std::unique_ptr<TKM::Sprite> sprite_ = nullptr; // 2Dスプライト共通
	std::unique_ptr<TKM::Skybox> skybox_ = nullptr;
	std::unique_ptr<TKM::DirectionalLight> dirLight_ = nullptr;
	// スカイボックスの回転制御
	float skyPitch_ = 0.0f; // スカイボックスのピッチ（X軸回転）角
	float skyRotSpeedX_ = 0.002f; // スカイボックスのX軸回転速度（ラジアン/フレーム）
	// カメラ
	float camDist_ = -30.0f;  // カメラ距離（+Z側）
	float camY_ = 3.0f;   // カメラ高さ
	//======================================================================
	// タイトル敵のシーケンス制御
	//======================================================================
	struct TitleEnemyUnit { // タイトル敵ユニット
		std::unique_ptr<Enemy> enemy_; // 敵オブジェクト
		float vanishDelay_ = 0.0f; // 消滅開始までの遅延時間（秒）
		bool alive_ = true; // 生存状態（trueで生きている、falseで消滅開始）
	};
	// タイトル敵の数式定義（π系）
	std::vector<TitleEnemyUnit> titleEnemies_; // タイトル敵ユニットのリスト
	std::mt19937 rng_{ std::random_device{}() }; // 乱数生成器
	bool showUi_ = true; // UI表示フラグ（trueで表示、falseで非表示）
	float seqTimer_ = 0.0f; // シーケンス全体の経過時間（秒）
	float vanishTimer_ = 0.0f; // 消滅シーケンスの経過時間（秒）
	float rippleTimer_ = 0.0f; // 波紋エフェクトの経過時間（秒）
	// シーケンスのタイミング定数
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
	float irisMax_ = 4.5f;      // これを超えたら画面を覆ったとみなす
	float irisStartScale_ = 10.0f;  // 開始サイズ
	float irisEndScale_ = 0.0f;     // 目標（Initializeでセット）
	Ease::Tween irisTween_; // イージング関数
	// アイリスのトランジション時間
	static constexpr float kIrisDurationSec_ = 0.8f;
	bool irisOpening_ = true; // trueで「開く」演出中
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
	// タイトルFlow（ステートマシン）
	//======================================================================
	TKM::StateMachine flowSM_; // タイトルのFlow制御用ステートマシン
	bool earlyExitUpdate_ = false; // Updateの早期抜けフラグ（シーン切り替えなどでUpdateの残り処理をスキップしたいときにtrueにする）
	// タイトルFlowの各ステートクラスをフレンド宣言
	friend class TitleFlowIntroIrisOpenState; // タイトルFlow：イントロのアイリス開きステート
	friend class TitleFlowIdleState; // タイトルFlow：アイドルステート（敵が出てきてない状態）
	friend class TitleFlowVanishingState; // タイトルFlow：消滅シーケンスステート
	friend class TitleFlowRippleState; // タイトルFlow：波紋エフェクトステート
	friend class TitleFlowIrisCloseState; // タイトルFlow：アイリス閉じステート
	//======================================================================
	// タイトル：見つめ合い演出
	//======================================================================
	std::unique_ptr<TitleShowdownController> titleShowdown_ = nullptr;
};