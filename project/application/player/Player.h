#pragma once
#define NOMINMAX
#include <memory>
#include "Object3d.h"
#include "engine/3d/camera/Camera.h"
#include "ModelManager.h"
#include "PlayerBullet.h"
#include "Input.h"
#include "application/enemy/Enemy.h"
#include <algorithm>
#include <list>
#include <engine/effect/particle/ParticlerEmitter.h>
#include "Easing.h"
#include "application/player/reticle/Reticle.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

class MidBossCore;

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
	/// <summary>
	/// 入力などのゲームプレイ処理を行わず、
	/// 見た目用に行列だけ更新したいとき（クリア演出用）
	/// </summary>
	void UpdateVisualOnly();

	// Getter===================================
	/// <summary>
	/// プレイヤーの弾リストを取得します。
	/// </summary>
	/// <returns></returns>
	const std::list<std::unique_ptr<PlayerBullet>>& GetBullets() const {
		return bullets_;
	}
	/// <summary>
	/// プレイヤーの位置を取得します。
	/// </summary>
	/// <returns></returns>
	Vector3 GetPosition() const {
		return object_ ? object_->GetTranslate() : Vector3();
	}
	/// <summary>
	/// プレイヤーのHPを取得します。
	/// </summary>
	/// <returns></returns>
	int GetHP() const { return hp_; }
	/// <summary>
	/// レティクルを取得します。
	/// </summary>
	/// <returns></returns>
	Reticle* GetReticle() const {
		return reticle_.get();
	}
	/// <summary>
	/// プレイヤーの回転を取得します。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetRotation() const { return object_->GetRotate(); }
	/// <summary>
	/// プレイヤーの回転を設定します。
	/// </summary>
	/// <param name="r"></param>
	void SetRotation(const Vector3& r) { object_->SetRotate(r); }
	// =========================================
	// Setter===================================
	/// <summary>
	/// ジェットスモークの有効/無効を設定します。
	/// </summary>
	/// <param name="enable"></param>
	void SetEnableJetSmoke(bool enable) { enableJetSmoke_ = enable; }
	/// <summary>
	/// プレイヤーのHPを設定します。
	/// </summary>
	/// <param name="hp"></param>
	void SetHP(int hp) { hp_ = hp; }
	/// <summary>
	/// カメラを設定します。
	/// </summary>
	/// <param name="camera"></param>
	void SetCamera(Camera* camera) 
	{
		this->camera = camera;
		if (object_) { object_->SetCamera(camera); }
		if (reticle_) { reticle_->SetCamera(camera); }
	}
	/// <summary>
	/// プレイヤーの位置を設定します。
	/// </summary>
	/// <param name="pos"></param>
	void SetPosition(const Vector3& pos);
	/// <summary>
	/// 親シーンを設定します。
	/// </summary>
	/// <param name="parentScene"></param>
	void SetParentScene(BaseScene* parentScene);
	/// <summary>
	/// ターゲット敵を設定します。
	/// </summary>
	/// <param name="enemy"></param>
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }
	/// <summary>
	/// 全敵リストを設定します。
	/// </summary>
	/// <param name="enemies"></param>
	void SetAllEnemies(std::vector<std::unique_ptr<Enemy>>* enemies) {
		allEnemies_ = enemies;
	}
	/// <summary>
	/// カメラシェイクを開始します。
	/// </summary>
	/// <param name="frameCount"></param>
	void StartCameraShake(int frameCount);
	/// <summary>
	/// プレイヤーの操作有効/無効を切り替えます。
	/// </summary>
	/// <param name="enabled"></param>
	void SetControlEnabled(bool enabled) { controlEnabled_ = enabled; }
	/// <summary>
	/// レティクルの表示/非表示を切り替えます。
	/// </summary>
	/// <param name="visible"></param>
	void SetReticleVisible(bool visible) { reticleVisible_ = visible; }
	/// <summary>
	/// ミッドボスコアを設定します。
	/// </summary>
	/// <param name="core"></param>
	void SetMidBossCore(MidBossCore* core) { core_ = core; }
	// =========================================
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
	MidBossCore* core_ = nullptr;

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
	Vector3 moveMin_ = { -100.0f, -20.0f, 0.0f }; // 移動範囲（Zは固定）
	Vector3 moveMax_ = { 100.0f,  20.0f, 0.0f };

	bool ltHeld_ = false; // LTの押下状態ラッチ

	ParticleEmitter jetEmitter_;

	bool debugUnlimitedSpecial_ = false; // ImGuiでONならRTを無制限発射

	bool enableJetSmoke_ = true; // デフォルトON

	int hp_ = 1; // 初期HP

	bool controlEnabled_ = true;   // trueなら通常操作、falseなら入力系を全部無視
	bool reticleVisible_ = true;   // trueならレティクル描画

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

	// 3Dレティクル関連
	std::unique_ptr<Reticle> reticle_;   // 3Dレティクル用Object3d
	float reticleDistance_ = 50.0f;         // 自機から前方への距離
	float reticleUpOffset_ = 0.0f;          // 必要なら少し上げる

	// 入力 & 弾共通の調整用定数
	static constexpr int   kTriggerThreshold = 128;  // LT/RT 判定しきい値
	float normalBulletSpeed_ = 2.2f; // RB/LB/RT の弾速
	static constexpr float kJetSmokeOffsetZ = 2.0f; // 機体後ろのジェット位置Zオフセット
	static constexpr float kHomingBulletSpeed = 0.6f; // LT弾の追尾速度
	const float dt = 1.0f / 60.0f; // 想定フレーム時間
};