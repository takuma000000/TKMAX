#pragma once
#include "BaseScene.h"

#include <memory>

#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Camera.h"
#include "application/player/Player.h"
#include "Object3dCommon.h"
#include "engine/effect/light/DirectionalLight.h"
#include <SkyBox.h>
#include "engine/func/math/Vector3.h"
#include "MyMath.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include <Easing.h>

//=============================================================
// GameClearSceneクラス
// ゲームクリア画面を管理するシーンクラス。
// 背景スカイボックス回転＋自機のジェットコースター演出。
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

	// --- カメラ・ライト ---
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<DirectionalLight> dirLight_;

	// --- 自機 ---
	std::unique_ptr<Player> player_;

	// --- スカイボックス ---
	std::unique_ptr<Skybox> skybox_;
	float skyPitch_ = 0.0f;        // X軸回転量
	float skyRotSpeedX_ = 0.002f;  // X軸回転速度（GameOverSceneとほぼ同じ）

	// --- 自機クリア演出用パラメータ ---
	const float dt_ = 1.0f / 60.0f;   // 固定フレーム（60fps想定）
	float planeTime_ = 0.0f;          // 経過時間(秒)
	float planeDuration_ = 6.0f;      // 左→右に抜けるまでの時間(秒)

	// 画面左外〜右外くらいの位置（ちょっと広めに取って完全に画面外スタート/ゴール）
	Vector3 planeStart_ = { -35.0f, 0.0f, 8.0f };
	Vector3 planeEnd_ = { 35.0f, 0.0f, 8.0f };

	// 「GAME CLEAR」用スプライト
	std::unique_ptr<Sprite> clearSprite_;

	// --- 画面遷移用アイリス（他シーンと同じ演出）---
	std::unique_ptr<Sprite> iris_;
	bool  irisOpening_ = true;     // 入場時は開き演出から
	bool  irisClosing_ = false;    // Aボタンで閉じ演出開始
	float irisScale_ = 0.0f;       // 現フレームのサイズ
	float irisMaxScale_ = 0.0f;    // 画面対角ベースの最大スケール

	Ease::Tween irisOpenTween_;    // 開き用（OutBack, 0.8s）
	Ease::Tween irisCloseTween_;   // 閉じ用（InBack, 0.8s）
};