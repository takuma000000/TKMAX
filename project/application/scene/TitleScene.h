#pragma once
#include "BaseScene.h"

#include <memory>
#include <cmath>
#include <vector>
#include "engine/audio/AudioManager.h"
#include "TextureManager.h"
#include "DirectXCommon.h"
#include "srvManager.h"
#include "engine/2d/Sprite.h"
#include "SpriteCommon.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "Camera.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "Input.h"
#include "SceneManager.h"
#include "GameScene.h"
#include <SkyBox.h> 
#include <Easing.h>

class TitleScene : public BaseScene
{
public:
	TitleScene(DirectXCommon* dxCommon, SrvManager* srvManager) : dxCommon(dxCommon), srvManager(srvManager) {}

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

	// 既存
	std::unique_ptr<Sprite> sprite = nullptr;
	std::unique_ptr<Camera> camera = nullptr;

	// 背景用の自機（ヘリ）
	std::unique_ptr<Object3d> heli_ = nullptr;

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

	std::unique_ptr<Skybox> skybox_ = nullptr;
	float skyPitch_ = 0.0f;
	float skyRotSpeedX_ = 0.002f;

	std::unique_ptr<DirectionalLight> dirLight_ = nullptr;

	enum class EnemyMotion {
		EightXZ,  // XZの8の字
		SineStrafe, // 横振り＋前後スラローム
		Swoop      // たまに手前へ急降下→復帰
	};

	// 敵のパラメータ
	EnemyMotion enemyMotion_ = EnemyMotion::EightXZ;
	float enemyRadius_ = 8.0f;
	float enemyBaseY_ = 2.0f;
	float enemyBobAmp_ = 0.7f;
	float enemySpeed_ = 1.2f;
	float enemyScale_ = 1.2f;
	float enemyYawOffset_ = 0.0f;

	// 内部タイマー（ヘリ用とは別にして独立させる）
	float enemyTime_ = 0.0f;

	// ▼ 追加: 1体だけ置いてるコンテナ
	std::vector<std::unique_ptr<Object3d>> titleEnemies_;

	// Iris（白円）トランジション
	std::unique_ptr<Sprite> iris_ = nullptr;
	bool irisClosing_ = false;   // trueで「閉じる」演出中
	float irisScale_ = 0.2f;    // 開始スケール（小さめ）
	float irisSpeed_ = 2.8f;    // 拡大速度（好みで調整）
	float irisMax_ = 4.5f;    // これを超えたら画面を覆ったとみなす
	int irisHoldFrames_ = 0;

	float irisT_ = 0.0f;           // 進行度(0→1)
	float irisDuration_ = 0.8f;    // アニメ時間(秒)
	float irisStartScale_ = 10.0f; // 開始サイズ
	float irisEndScale_ = 0.0f;    // 目標（Initializeでセット）

	Ease::Tween irisTween_; // イージング関数

};
