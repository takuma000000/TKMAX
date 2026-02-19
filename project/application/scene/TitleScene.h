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
	/// タイトルシーンを描画します。
	/// </summary>
	void Draw() override;

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

	float skyPitch_ = 0.0f; // スカイボックスのピッチ（X軸回転）角
	float skyRotSpeedX_ = 0.002f; // スカイボックスのX軸回転速度（ラジアン/フレーム）
	//======================================================================
	// タイトル用プレイヤー（見た目だけ）
	//======================================================================
	std::unique_ptr<Player> titlePlayer_ = nullptr; // タイトル用プレイヤー（見た目だけ）
	//======================================================================
	// タイトル用プレイヤーの周回モーション（ImGui調整）
	//======================================================================
	// 旋回タイマー
	float t_ = 0.0f;
	// ---- ImGui で調整するパラメータ（初期値は“見える”前提） ----
	float radius_ = 6.0f;   // 円運動の半径
	float baseY_ = 1.0f;   // 高さの基準
	float bobAmp_ = 0.5f;   // 上下ゆれ量
	float speed_ = 0.8f;   // 角速度（rad/sec のイメージ）
	float yawOffset_ = 0.0f;   // Yaw に任意オフセット
	float scale_ = 2.0f;   // モデル表示スケール
	// カメラ
	float camDist_ = 20.0f;  // カメラ距離（+Z側）
	float camY_ = 3.0f;   // カメラ高さ
	//======================================================================
	// タイトル敵（1体だけ / モーション切り替え）
	//======================================================================
	enum class EnemyMotion {
		EightXZ,     // XZの8の字
		SineStrafe,  // 横振り＋前後スラローム
		Swoop        // たまに手前へ急降下→復帰
	};
	// 敵のパラメータ
	EnemyMotion enemyMotion_ = EnemyMotion::EightXZ; // 現在のモーションパターン
	float enemyRadius_ = 8.0f; // 円運動の半径
	float enemyBaseY_ = 2.0f; // 高さの基準
	float enemyBobAmp_ = 0.7f; // 上下ゆれ量
	float enemySpeed_ = 1.2f; // 角速度（rad/sec のイメージ）
	float enemyScale_ = 1.2f; // モデル表示スケール
	float enemyYawOffset_ = 0.0f; // Yaw に任意オフセット
	// 内部タイマー（ヘリ用とは別にして独立させる）
	float enemyTime_ = 0.0f;
	// 1体だけ置いてるコンテナ
	std::vector<std::unique_ptr<TKM::Object3d>> titleEnemies_; // タイトル敵（1体だけ）
	std::vector<std::unique_ptr<TKM::Object3d>> titleTentacles_; // タイトル敵の触手（1体だけ）
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
	//======================================================================
	// 定数（π系）
	//======================================================================
	// タイトル敵の数式定義（π系）
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
};