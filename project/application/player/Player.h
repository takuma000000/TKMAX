#pragma once

#define NOMINMAX
#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "ModelManager.h"
#include "PlayerBullet.h"
#include "Input.h"
#include "application/enemy/Enemy.h"
#include <algorithm>
#include <list>
#include <engine/effect/particle/ParticlerEmitter.h>
#include "Easing.h"
#include "Reticle.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

//=============================================================
// Playerクラス
// プレイヤーの動作を制御するクラス。
//=============================================================
class Player {
public:

	/// <summary>プレイヤーを初期化します。</summary>
	/// <param name="common">Object3d共通。</param>
	/// <param name="dxCommon">DirectX共通。</param>
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	/// <summary>プレイヤーを更新します。</summary>
	void Update();
	/// <summary>プレイヤーを描画します。</summary>
	void Draw(DirectXCommon* dxCommon);
	/// <summary>デバッグ用ImGui表示。</summary>
	void ImGuiDebug();

	/// <summary>敵が死亡していたらリストから削除します。</summary>
	void RemoveEnemyIfDead();
	/// <summary>一撃必殺を使用可能にします。</summary>
	void EnableSpecialAttack() { canUseSpecial_ = true; } // 一撃必殺を使用可能にする

	/// <summary>敵が破壊されたときの処理。</summary>
	void OnEnemyDestroyed(Enemy* e) {
		if (enemy_ == e) {
			enemy_ = nullptr;
		}
		for (auto& b : bullets_) {
			if (!b) continue;
			if (b->GetEnemy() == e) { // 弾が追従していた敵が破壊された
				b->SetEnemy(nullptr);
			}
		}
	}

	/// <summary>プレイヤーが撃墜されているかどうかを取得します。</summary>
	bool IsDead() const { return isDead_; }

	/// <summary>ダメージを与えます。</summary>
	/// <param name="value">ダメージ値。</param>
	void Damage(int value) {
		hp_ -= value;
		if (hp_ < 0) hp_ = 0;
	}

	/// <summary>撃墜関数</summary>
	void Death();

	/// <summary>弾のリストを取得します。</summary>
	const std::list<std::unique_ptr<PlayerBullet>>& GetBullets() const {
		return bullets_;
	}

	/// <summary>プレイヤーの位置を取得します。</summary>
	Vector3 GetPosition() const {
		return object_ ? object_->GetTranslate() : Vector3();
	}

	/// <summary>プレイヤーのHPを取得します。</summary>
	/// <returns>HP値。</returns>
	int GetHP() const { return hp_; }

	/// <summary>プレイヤーの回転を取得します。</summary>
	const Vector3& GetRotation() const { return object_->GetRotate(); }

	/// <summary>プレイヤーの回転を設定します。</summary>
	/// <param name="r">回転値。</param>
	void SetRotation(const Vector3& r) { object_->SetRotate(r); }

	/// <summary>ジェット噴射の有効/無効を設定します。</summary>
	/// <param name="enable">有効にする場合はtrue、無効にする場合はfalse。</param>
	void SetEnableJetSmoke(bool enable) { enableJetSmoke_ = enable; }

	/// <summary>プレイヤーのHPを設定します。</summary>
	/// <param name="hp">HP値。</param>
	void SetHP(int hp) { hp_ = hp; }

	/// <summary>カメラを設定します。</summary>
	void SetCamera(Camera* camera) 
	{
		this->camera = camera;
		if (object_) { object_->SetCamera(camera); }
		if (reticle_) { reticle_->SetCamera(camera); }
	}
	/// <summary>プレイヤーの位置を設定します。</summary>
	void SetPosition(const Vector3& pos);
	/// <summary>親シーンを設定します。</summary>
	void SetParentScene(BaseScene* parentScene);
	/// <summary>敵を設定します。</summary>
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }
	/// <summary>全敵リストを設定します。</summary>
	void SetAllEnemies(std::vector<std::unique_ptr<Enemy>>* enemies) {
		allEnemies_ = enemies;
	}
	/// <summary>カメラシェイクを開始します。</summary>
	void StartCameraShake(int frameCount);

	enum class DeathPhase { None, FaultSparks, FlyAway }; // 撃墜演出フェーズ

private:

	/// <summary>ゲームパッドの入力に基づいてプレイヤーを移動させます。</summary>
	void HandleGamePadMove();
	/// <summary>追従カメラを処理します。</summary>
	void HandleFollowCamera();
	/// <summary>射撃処理を行います。</summary>
	void HandleShooting();
	/// <summary>RB弾を更新します。</summary>
	void RBShoot();
	/// <summary>RT弾を更新します。</summary>
	void RTShoot();
	/// <summary>LB弾を更新します。</summary>
	void LBShoot();
	/// <summary>LT弾を更新します。</summary>
	void LTShoot();
	/// <summary>カメラの更新（第三者視点追従）を行います。</summary>
	void UpdateCameraFollowThirdPerson(float dt);
	/// <summary>カメラの更新（LTズーム）を行います。</summary>
	void ZoomCamera();

	Camera* camera = nullptr;
	Object3dCommon* common_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;
	Enemy* enemy_ = nullptr;

	BaseScene* parentScene_ = nullptr;
	std::unique_ptr<Object3d> object_;
	std::list<std::unique_ptr<PlayerBullet>> bullets_;
	std::vector<std::unique_ptr<Enemy>>* allEnemies_ = nullptr;

	Enemy* lastLockedEnemy_ = nullptr;  // 直前にロック表示していた敵
	bool rtHeld_ = false;  // RTをいま保持中か

	// --- カメラシェイク ---
	Vector3 cameraShakeOffset_ = { 0, 0, 0 };
	int cameraShakeFrame_ = 0;
	float shakeBaseStrength_ = 1.0f;   // 基本のシェイク強度
	float shakeZoomBoost_ = 8.0f;   // ズーム時の追加倍率

	bool canUseSpecial_ = false; // 一撃必殺が使用可能かどうか

	float bankAngle_ = 0.0f;      // 現在の傾き（ロール）
	float bankVel_ = 0.0f;      // 補間用
	Vector3 moveMin_ = { -20.0f, -3.0f, 0.0f }; // 移動範囲（Zは固定）
	Vector3 moveMax_ = { 20.0f,  8.0f, 0.0f };

	bool ltHeld_ = false; // LTの押下状態ラッチ

	ParticleEmitter jetEmitter_;

	bool debugUnlimitedSpecial_ = false; // ImGuiでONならRTを無制限発射

	bool enableJetSmoke_ = true; // デフォルトON

	int hp_ = 1; // 初期HP

	// 撃墜演出用
	bool   isDead_ = false;                // 撃墜モード中
	Vector3 deathVelocity_ = { 0,0,0 };      // 速度
	Vector3 deathRotateSpeed_ = { 0,0,0 };   // 回転速度
	float  deathTimer_ = 0.0f;             // 経過時間(秒想定)
	float  deathDuration_ = 2.6f;          // 強制演出の長さ（好みで）
	// デス演出ステート管理
	DeathPhase deathPhase_ = DeathPhase::None;
	// 故障スパーク段階の管理
	float faultTimer_ = 0.0f;
	float faultDuration_ = 1.3f;   // 何秒間スパークさせるか（ImGuiで調整可）
	int   faultBurstPerTick_ = 12; // 1回あたり粒の発生数（ImGuiで調整可）
	int   faultTickInterval_ = 2;  // 何フレームごとに出すか
	int   faultFrameCounter_ = 0;
	bool  flyInit_ = false; // FlyAway移行時の一度きり初期化フラグ

	// --- LT一時ズーム ---
	bool ltZoomActive_ = false;   // ズーム中フラグ
	Ease::Tween ltZoomTween_;            // 0..1 の係数トゥイーン
	float camZoom_ = 1.0f;         // 現在のズーム係数（1=通常）
	float ltZoomHold_ = 0.0f;      // 最小倍率でホールドする秒数
	Vector3 camSavedPos_; // カメラ位置保存用
	Vector3 camSavedRot_; // カメラ回転保存用

	std::unique_ptr<Reticle> reticle_;   // 3Dレティクル用Object3d
	float reticleDistance_ = 50.0f;         // 自機から前方への距離
	float reticleUpOffset_ = 0.0f;          // 必要なら少し上げる
};
