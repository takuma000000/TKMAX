#pragma once
#include "BaseScene.h"
#include <memory>
#include "DirectXCommon.h"
#include "srvManager.h"
#include "Camera.h"
#include "application/player/Player.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "engine/effect/light/DirectionalLight.h"
#include "engine/effect/particle/ParticleManager.h"
#include <Easing.h>

//=============================================================
// GameOverScene
//=============================================================
class GameOverScene : public BaseScene{
public:
	GameOverScene(DirectXCommon* dxCommon, SrvManager* srvManager)
		: dxCommon_(dxCommon), srvManager_(srvManager) {
	}

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	std::unique_ptr<Camera> camera_;
	std::unique_ptr<Player> player_;
	std::unique_ptr<DirectionalLight> dirLight_;

	// 画面を覆う虹彩絞り演出用
	std::unique_ptr<Sprite> iris_;
	bool  irisOpening_ = true;     // 入場時は開き演出から
	bool  irisClosing_ = false;    // T押下で閉じ演出開始
	float irisScale_ = 0.0f;       // 現フレームのサイズ
	float irisMaxScale_ = 0.0f;    // 画面対角ベースの最大スケール

	Ease::Tween irisOpenTween_;    // 開き用（OutBack, 0.8s）
	Ease::Tween irisCloseTween_;   // 閉じ用（InBack, 0.8s）

	std::unique_ptr<Skybox> skybox_; // 背景スカイボックス
	float skyPitch_ = 0.0f;        // X軸回転量
	float skyRotSpeedX_ = 0.002f;  // X軸回転速度

	// 墜落演出（発生位置 & タイマー）
	Vector3 crashOffset_ = { -0.6f, -0.9f, 0.2f }; // 機体原点からの出火ポイント
	float   flameTimer_ = 0.0f;
	float   flameInterval_ = 0.6f;   // 炎バースト間隔（秒）可変

	// 失速スピン用の角速度（ラジアン/秒）
	Vector3 tumbleSpeed_ = { 0.8f, 1.2f, 0.6f }; // x,y,z の回転速度
	bool tumbleActive_ = true; // 失速スピン中かどうか

	// 故障スポット（ローカル座標）と各スポットのクールダウン
	std::vector<Vector3> faultLocal_;       // 機体ローカル（翼/エンジン/尾など）
	std::vector<float>   faultCD_;          // 秒
	std::vector<float>   faultNext_;        // 次に噴くまでの残り秒

	// 1フレの炎/火花 発生総量の上限
	int perFrameFlameBudget_ = 40;
	int perFrameSparkBudget_ = 25;

	// === 「GAME OVER」スプライト ===
	std::unique_ptr<Sprite> overSprite_;

	// フェードイン（0→1）とスケール（0.8→1.0）
	Ease::Tween overAlphaTween_;
	Ease::Tween overScaleTween_;

	bool overActive_ = false;   // アニメ進行フラグ
	float overAlpha_ = 0.0f;    // 現アルファ
	float overScale_ = 1.0f;    // 現スケール
};