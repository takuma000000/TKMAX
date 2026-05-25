#pragma once
#define NOMINMAX
#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "ModelManager.h"
#include "PlayerBullet.h"
#include "Input.h"
#include <algorithm>
#include <list>
#include <ParticleEmitter.h>
#include "Easing.h"
#include "reticle/Reticle.h"
#include "LineRenderer.h"
#include "TrailRibbonRenderer.h"
#include "HomingBullet.h"
#include "PlayerShotManager.h"
#include "PlayerShotConfig.h"
#include <functional>
#include <vector>
#include <array>
#include "PlayerDodge.h"
#include "PlayerHealth.h"

class BarrierCore;
class Enemy;
class BarrierCoreManager;

namespace TKM {
	class RadialBlurEffect;
}

//=============================================================
// Playerクラス
// プレイヤーの動作を制御するクラス。
//=============================================================
class Player {
public:
	//=============================================================
	// Wave1のバリアヒット情報構造体
	//=============================================================
	struct Wave1BarrierHit {
		Vector3 worldPos_ = { 0.0f, 0.0f, 0.0f }; // ヒットしたワールド座標
		float age_ = 0.0f; // ヒットしてからの経過時間
		float life_ = 0.35f; // エフェクトの寿命（秒）
	};

	//=============================================================
	// HUD状態通知（Observer）
	//=============================================================
	struct HudState {
		int currentHp_ = 0;                 // 現在HP
		int maxHp_ = 1;                     // 最大HP

		int rbAmmo_ = 0;                    // RB弾の残弾数
		int rbAmmoMax_ = 1;                 // RB弾の最大残弾数
		bool rbRefilling_ = false;          // RB弾が回復中かどうか

		int lbAmmo_ = 0;                    // LB弾の残弾数
		int lbAmmoMax_ = 1;                 // LB弾の最大残弾数

		bool rbRefillingFromEmpty_ = false; // RB弾が0発から回復中かどうか
	};

	/// <summary>
	/// プレイヤーオブジェクトを初期化します。
	/// </summary>
	/// <param name="common">Object3d の共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// プレイヤーの更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void Update(float dt);
	/// <summary>
	/// プレイヤーを描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// プレイヤーの弾のトレイル（軌跡）を描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void DrawTrails(TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// デバッグ用ImGui表示。
	/// </summary>
	void ImGuiDebug();

	/// <summary>
	/// HUDに表示するプレイヤー状態の構造体。
	/// </summary>
	using HudObserver = std::function<void(const HudState& state)>;
	/// <summary>
	/// HUDに表示するプレイヤー状態が変化したときに呼ばれる通知先を登録します。
	/// </summary>
	void AddHudObserver(const HudObserver& observer);
	/// <summary>
	/// 敵が死亡していたらターゲットを解除します。
	/// </summary>
	void RemoveEnemyIfDead();
	/// <summary>
	/// 一撃必殺を使用可能にします。
	/// </summary>
	void EnableSpecialAttack();
	/// <summary>
	/// 敵が破壊されたときの処理を行います。
	/// </summary>
	/// <param name="e">破壊された敵オブジェクト</param>
	void OnEnemyDestroyed(Enemy* e);
	/// <summary>
	/// プレイヤーが即死ダメージを受けたときの処理。
	/// </summary>
	/// <returns></returns>
	bool IsDead() const { return isDead_; }
	/// <summary>
	/// プレイヤーがダメージを受けたときの処理を行います。
	/// </summary>
	/// <param name="value">受けるダメージ量</param>
	void Damage(int value);
	/// <summary>
	/// プレイヤーが撃墜されたときの処理。
	/// </summary>
	void Death();
	/// <summary>
	/// 入力などのゲームプレイ処理を行わず、
	/// 見た目用に行列だけ更新したいとき（クリア演出用）
	/// </summary>
	void UpdateVisualOnly(float dt);
	/// <summary>
	/// 撃墜演出フェーズの更新。
	/// </summary>
	void StartBossDeathCameraZoom();
	/// <summary>
	/// RB弾が回復中かどうか。
	/// </summary>
	bool IsRbRefilling() const;
	/// <summary>
	/// RB弾が空の状態から回復中かどうか。
	/// </summary>
	/// <returns></returns>
	bool IsRbRefillingFromEmpty() const;
	/// <summary>
	/// LB弾が回復中かどうか。
	/// </summary>
	/// <returns></returns>
	bool IsWave1BarrierActive() const { return wave1BarrierActive_; }
	/// <summary>
	/// 攻撃ID付きでダメージを受ける処理を行います。
	/// 同一攻撃IDによる重複ダメージは無効化されます。
	/// </summary>
	/// <param name="damage">与えるダメージ量</param>
	/// <param name="attackId">攻撃を識別するID</param>
	/// <returns>ダメージが適用された場合 true、それ以外は false</returns>
	bool TryDamageFromAttack(int damage, int attackId);
	/// <summary>
	/// カメラシェイクを開始します。
	/// </summary>
	/// <param name="frameCount">シェイク継続フレーム数</param>
	void StartCameraShake(int frameCount);
	/// <summary>
	/// ゲームパッドの振動停止。
	/// </summary>
	void StopRumble();
	/// <summary>
	/// タイトルなど、入力/弾/移動を一切行わず「見た目だけ」動かす更新。
	/// </summary>
	void UpdateTitleIdle(float dt);
	/// <summary>
	/// ワンウェイバリア（LB弾）が敵の攻撃にヒットしたときの処理を追加します。
	/// </summary>
	/// <param name="worldPos">ヒットしたワールド座標</param>
	void AddWave1BarrierHit(const Vector3& worldPos);
	/// <summary>
	/// ミッドボスコアが破壊されたときの処理を行います。
	/// </summary>
	/// <param name="core"></param>
	void OnBarrierCoreDestroyed(BarrierCore* core);
	/// <summary>
	/// ワンウェイバリア（LB弾）が敵の攻撃にヒットしたときのフラッシュエフェクトをリクエストします。
	/// </summary>
	/// <param name="worldPos"></param>
	void RequestWave1BarrierFlash(const Vector3& worldPos);
	/// <summary>
	/// ワンウェイバリア（LB弾）が敵の攻撃にヒットしたときのフラッシュエフェクトのリクエストを消費します。
	/// </summary>
	/// <param name="outWorldPos"></param>
	/// <returns></returns>
	bool ConsumeWave1BarrierFlashRequest(Vector3& outWorldPos);
	/// <summary>
	/// カメラのズーム処理を行います。
	/// </summary>
	void ZoomCamera();
	/// <summary>
	/// 振動開始。
	/// </summary>
	/// <param name="sec">振動継続時間（秒）</param>
	/// <param name="leftMotor">左モーター強度（0〜65535）</param>
	/// <param name="rightMotor">右モーター強度（0〜65535）</param>
	void StartRumble(float sec, WORD leftMotor, WORD rightMotor);
	/// <summary>
	/// ゲーム開始時の前進演出を開始します。
	/// </summary>
	/// <param name="startOffsetZ">現在位置からどれだけ手前に置くか</param>
	/// <param name="durationSec">前進にかける時間</param>
	void StartIntroForwardMove(float startOffsetZ, float durationSec);
	/// <summary>
	/// ゲーム開始時の前進演出中かどうか。
	/// </summary>
	bool IsIntroForwardMoving() const { return introForwardActive_; }

	// Getter===================================
	/// <summary>
	/// プレイヤーの弾リストを取得します。
	/// </summary>
	/// <returns></returns>
	const std::list<std::unique_ptr<PlayerBullet>>& GetBullets() const;
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
	int GetHP() const { return health_ ? health_->GetHP() : 0; }
	/// <summary>
	/// プレイヤーの最大HPを取得します。
	/// </summary>
	/// <returns></returns>
	int GetMaxHP() const { return health_ ? health_->GetMaxHP() : 1; }
	/// <summary>
	/// プレイヤーのHP割合(0.0f〜1.0f)を取得します。
	/// </summary>
	/// <returns></returns>
	float GetHPRate() const { return health_ ? health_->GetHPRate() : 0.0f; }
	/// <summary>
	/// レティクルを取得します。
	/// </summary>
	/// <returns></returns>
	Reticle* GetReticle() { return reticle_.get(); }
	/// <summary>
	/// レティクルを取得します。(const版)
	/// </summary>
	/// <returns></returns>
	const Reticle* GetReticle() const { return reticle_.get(); }
	/// <summary>
	/// プレイヤーの回転を取得します。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetRotation() const { return object_->GetRotate(); }
	/// <summary>
	/// プレイヤーのコライダースケールを取得します。
	/// </summary>
	/// <returns></returns>
	Vector3 GetColliderScale() const { return colliderScale_; }
	/// <summary>
	/// RB弾の残数を取得します。
	/// </summary>
	int GetRbAmmo() const;
	/// <summary>
	/// RB弾の最大数を取得します。
	/// </summary>
	int GetRbAmmoMax() const;
	/// <summary>
	/// LB弾の残数を取得します。
	/// </summary>
	int GetLbAmmo() const;
	/// <summary>
	/// LB弾の最大数を取得します。
	/// </summary>
	int GetLbAmmoMax() const;
	/// <summary>
	/// ワンウェイバリア（LB弾）に関する情報を取得します。
	/// </summary>
	Vector3 GetWave1BarrierCenter() const { return wave1BarrierCenter_; }
	/// <summary>
	/// ワンウェイバリア（LB弾）に関する情報を取得します。
	/// </summary>
	Vector3 GetWave1BarrierSize() const { return wave1BarrierSize_; }
	/// <summary>
	/// ワンウェイバリア（LB弾）に関する情報を取得します。
	/// </summary>
	const std::vector<Wave1BarrierHit>& GetWave1BarrierHits() const {
		return wave1BarrierHits_;
	}
	// =========================================
	// Setter===================================
	/// <summary>
	/// ジェットスモークの有効/無効を設定します。
	/// </summary>
	/// <param name="enable">有効にする場合 true、それ以外は false</param>
	void SetEnableJetSmoke(bool enable) { enableJetSmoke_ = enable; }
	/// <summary>
	/// プレイヤーの HP を設定します。
	/// </summary>
	/// <param name="hp">設定する HP</param>
	void SetHP(int hp);
	/// <summary>
	/// 使用するカメラを設定します。
	/// プレイヤー本体およびレティクルにも同じカメラを適用します。
	/// </summary>
	/// <param name="camera">描画および判定に使用するカメラ</param>
	void SetCamera(TKM::Camera* camera);
	/// <summary>
	/// プレイヤーの位置を設定します。
	/// </summary>
	/// <param name="pos">設定する位置（ワールド座標）</param>
	void SetPosition(const Vector3& pos);
	/// <summary>
	/// 親シーンを設定します。
	/// </summary>
	/// <param name="parentScene">親シーン</param>
	void SetParentScene(TKM::BaseScene* parentScene);
	/// <summary>
	/// ターゲット敵を設定します。
	/// </summary>
	/// <param name="enemy">ターゲットとなる敵（nullptr 可）</param>
	void SetEnemy(Enemy* enemy);
	/// <summary>
	/// 全敵リスト参照を設定します。
	/// </summary>
	/// <param name="enemies">全敵リスト（外部所有）</param>
	void SetAllEnemies(std::vector<std::unique_ptr<Enemy>>* enemies);
	/// <summary>
	/// プレイヤー操作の有効/無効を設定します。
	/// </summary>
	/// <param name="enabled">操作を有効にする場合 true、それ以外は false</param>
	void SetControlEnabled(bool enabled);
	/// <summary>
	/// レティクルの表示/非表示を設定します。
	/// </summary>
	/// <param name="visible">表示する場合 true、それ以外は false</param>
	void SetReticleVisible(bool visible);
	/// <summary>
	/// ミッドボスコア参照を設定します。
	/// </summary>
	/// <param name="core">ミッドボスコア（nullptr 可）</param>
	void SetBarrierCore(BarrierCore* core);
	/// <summary>
	/// プレイヤーの当たり判定用スケールを設定します。
	/// </summary>
	/// <param name="s">当たり判定用スケール</param>
	void SetColliderScale(const Vector3& s);
	/// <summary>
	/// 放射状ブラーエフェクト参照を設定します。
	/// </summary>
	/// <param name="effect">放射状ブラーエフェクト（nullptr 可）</param>
	void SetRadialBlurEffect(TKM::RadialBlurEffect* effect);
	/// <summary>
	/// プレイヤーの回転を設定します。
	/// </summary>
	/// <param name="r">設定する回転角（度数法）</param>
	void SetRotation(const Vector3& r);
	/// <summary>
	/// プレイヤーの射撃の有効/無効を設定します。
	/// </summary>
	/// <param name="enabled">射撃を有効にする場合 true、それ以外は false</param>
	void SetShootingEnabled(bool enabled);
	/// <summary>
	/// ゲームパッドの振動の有効/無効を設定します。
	/// </summary>
	/// <param name="enabled">振動を有効にする場合 true、それ以外は false</param>
	void SetRumbleEnabled(bool enabled);
	/// <summary>
	/// 回転（Euler, rad）を直接セットします（タイトル等の演出用）。
	/// </summary>
	void SetRotate(const Vector3& rotRad);
	/// <summary>
	/// Yaw（Y回転）だけ設定します（ラジアン）。
	/// </summary>
	void SetYaw(float yawRad);
	/// <summary>
	/// ワンウェイバリア（LB弾）に関する情報を設定します。
	/// </summary>
	/// <param name="active">バリアがアクティブかどうか</param>
	/// <param name="center">バリアの中心位置（ワールド座標）</param>
	/// <param name="size">バリアのサイズ（幅・高さ・奥行）</param>
	void SetWave1BarrierInfo(bool active, const Vector3& center, const Vector3& size);
	/// <summary>
	/// BarrierCoreManager 参照を設定します。
	/// </summary>
	/// <param name="manager">BarrierCoreManager オブジェクト（nullptr 可）</param>
	void SetBarrierCoreManager(BarrierCoreManager* manager);
	// =========================================
private:
	//======================================================================
	// 内部メソッド
	//======================================================================
	/// <summary>
	/// ゲームパッド入力による移動を処理します。
	/// </summary>
	void HandleGamePadMove();
	/// <summary>
	/// カメラ追従処理を行います。
	/// </summary>
	void HandleFollowCamera();
	/// <summary>
	/// カメラの三人称視点追従処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateCameraFollowThirdPerson(float dt);
	/// <summary>
	/// HUD表示に必要な状態を通知します。
	/// </summary>
	void NotifyHudState_();
	/// <summary>
	/// HUD表示に必要な状態が前回通知から変化しているかどうかを判定します。
	/// </summary>
	/// <param name="state">現在のHUD状態</param>
	/// <returns>状態が変化している場合 true、それ以外は false</returns>
	bool IsHudStateChanged_(const HudState& state) const;
	//======================================================================
	// 参照ポインタ / 共通オブジェクト
	//======================================================================
	TKM::Camera* camera_ = nullptr;
	TKM::Object3dCommon* common_ = nullptr;
	TKM::DirectXCommon* dxCommon_ = nullptr;
	std::unique_ptr<PlayerShotManager> shotManager_ = nullptr;
	TKM::RadialBlurEffect* radialBlur_ = nullptr;
	TKM::BaseScene* parentScene_ = nullptr;
	std::unique_ptr<TKM::Object3d> object_; // プレイヤー本体の3Dオブジェクト
	std::unique_ptr<TKM::Object3d> flipper_; // プレイヤーの左右フリップ用オブジェクト
	std::unique_ptr<PlayerDodge> dodge_; // 回避行動管理クラス
	std::unique_ptr<PlayerHealth> health_; // HP、無敵、被弾状態管理クラス
	//======================================================================
	// カメラシェイク・バンク・移動範囲
	//======================================================================
	// --- カメラシェイク ---
	Vector3 cameraShakeOffset_ = { 0, 0, 0 }; // シェイクによるカメラ位置のオフセット
	int     cameraShakeFrame_ = 0; // シェイク残りフレーム数
	float   shakeBaseStrength_ = 1.8f;   // 基本のシェイク強度
	float   shakeZoomBoost_ = 8.0f;   // ズーム時の追加倍率
	float  bankAngle_ = 0.0f;                 // 現在の左右傾き（ロール）
	float  bankVel_ = 0.0f;                 // 左右傾き補間用
	float  pitchAngle_ = 0.0f; // 現在の上下傾き（ピッチ）
	float  pitchVel_ = 0.0f;   // 上下傾き補間用
	Vector3 moveMin_ = { -100.0f, -60.0f, 0.0f }; // 移動範囲（Zは固定）
	Vector3 moveMax_ = { 100.0f,  60.0f, 0.0f }; // 移動範囲（Zは固定）
	//======================================================================
	// 入力ラッチ / ジェット煙 / デバッグフラグ
	//======================================================================
	ParticleEmitter jetEmitter_; // ジェット煙エミッタ
	bool enableJetSmoke_ = true; // デフォルトON
	//======================================================================
	// プレイヤー状態 / 制御フラグ
	//======================================================================
	bool controlEnabled_ = true;  // trueなら通常操作、falseなら入力系を全部無視
	bool reticleVisible_ = true;  // trueならレティクル描画
	bool shootingEnabled_ = true; // trueなら射撃可能、falseなら射撃禁止
	bool rumbleEnabled_ = true; // true=振動OK / false=振動禁止
	//======================================================================
	// 撃墜管理
	//======================================================================
	bool   isDead_ = false;                    // 死亡状態か
	bool   deathStartHandled_ = false;         // 死亡開始時の一度きり処理用

	Vector3 deathVelocity_ = { 0.0f, 0.0f, 0.0f };     // 故障落下中の速度
	Vector3 deathAngularVelocity_ = { 0.0f, 0.0f, 0.0f }; // 故障落下中の角速度

	static constexpr float kDeathBackwardSpeed_ = 0.55f;   // 弱める
	static constexpr float kDeathFallStartSpeed_ = 0.01f;  // かなり弱く
	static constexpr float kDeathGravity_ = 0.006f;        // 超重要：めっちゃ弱く
	static constexpr float kDeathFallMaxSpeed_ = 0.25f;    // 落下速度を制限
	static constexpr float kDeathBackwardDamping_ = 0.992f; // 空気抵抗：かなり残す
	static constexpr float kDeathRotateDamping_ = 0.992f;   // 回転の慣性もゆっくり抜ける 
	static constexpr float kDeathMaxPitch_ = 1.20f;         // ピッチ（上下回転）の最大値（ラジアン）
	static constexpr float kDeathMaxRoll_ = 0.80f;          // ロール（左右回転）の最大値（ラジアン）
	Vector3 deathBackwardDir_ = { 0.0f, 0.0f, 0.0f };       // 後ろ反動の方向
	//======================================================================
	// 時ズーム（カメラ演出）
	//======================================================================
	// --- LT一時ズーム ---
	bool        ltZoomActive_ = false;   // ズーム中フラグ
	Ease::Tween ltZoomTween_;            // 0..1 の係数トゥイーン
	float       camZoom_ = 1.0f;    // 現在のズーム係数（1=通常）
	float       ltZoomHold_ = 0.0f;    // 最小倍率でホールドする秒数
	Vector3     camSavedPos_;            // カメラ位置保存用
	Vector3     camSavedRot_;            // カメラ回転保存用
	// --- ボス撃破時ズームアウト ---
	bool        bossZoomActive_ = false; // ボス撃破ズーム中か
	Ease::Tween bossZoomTween_;          // ボス用のズームトゥイーン
	float       bossZoom_ = 1.0f;        // ボス用ズーム係数（1=通常）
	//======================================================================
	// レティクル関連
	//======================================================================
	// 3Dレティクル関連
	std::unique_ptr<Reticle> reticle_;   // 3Dレティクル用Object3d
	float reticleDistance_ = 50.0f;      // 自機から前方への距離
	float reticleUpOffset_ = 0.0f;       // 必要なら少し上げる
	//======================================================================
	// 入力 & 弾共通パラメータ
	//======================================================================
	// 入力 & 弾共通の調整用定数
	static constexpr float kJetSmokeOffsetZ_ = 2.0f; // 機体後ろのジェット位置Zオフセット
	const float            dt = 1.0f / 60.0f; // 想定フレーム時間
	//======================================================================
	// 自機当たり判定 (AABB)
	//======================================================================
	// --- 自機当たり判定(AABB) ---
	Vector3 colliderScale_ = { 3.13f, 1.88f, 6.0f }; // 当たり判定用スケール
	//======================================================================
	// 振動（Rumble）
	//======================================================================
	float rumbleT_ = 0.0f; // 振動タイマー
	WORD  rumbleLeft_ = 0; // 左モーター強度
	WORD  rumbleRight_ = 0; // 右モーター強度
	float rumble2DelayT_ = 0.0f; // 2回目振動までの遅延タイマー
	float rumble2Sec_ = 0.0f; // 2回目振動継続時間
	WORD  rumble2Left_ = 0; // 左モーター強度
	WORD  rumble2Right_ = 0; // 右モーター強度
	bool  rumble2Pending_ = false; // 2回目振動保留フラグ
	/// <summary>
	/// 振動更新。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateRumble(float dt);
	//======================================================================
	// ひれパタパタ（常時アニメ）
	//======================================================================
	/// <summary>
	/// ひれのパタパタアニメーションを更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateFlipperAnim_(float dt);
	float  flipperAnimT_ = 0.0f;          // アニメ時間
	Vector3 flipperBaseRot_ = { 0,0,0 };  // ひれの基準回転（ローカル）
	float  flipperFlapAmp_ = 0.1f;       // 振り幅（ラジアン）
	float  flipperFlapHz_ = 1.0f;         // 周波数（1秒あたり何往復）
	float  flipperYawSwayAmp_ = 0.12f;    // ついでの横揺れ（ラジアン）
	//======================================================================
	// ぷかぷか（常時上下）
	//======================================================================
	/// <summary>
	/// プレイヤーの上下ぷかぷかアニメーションを更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateFloatBob_(float dt);
	float floatT_ = 0.0f;          // 経過時間
	float floatAmp_ = 0.1f;       // 振幅（上下の大きさ）
	float floatHz_ = 0.35f;        // 周波数（ゆっくり）
	bool  enableFloatBob_ = true;  // ON/OFF
	//======================================================================
	// LB弾（最大5・一定時間で満タン回復）
	//======================================================================
	bool debugUnlimitedLB_ = false;             // デバッグで無限（必要なら）
	//======================================================================
	// ワンウェイバリア
	//======================================================================
	bool wave1BarrierActive_ = false; // ワンウェイバリアがアクティブかどうか
	Vector3 wave1BarrierCenter_ = { 0.0f, 0.0f, 0.0f }; // ワンウェイバリアの中心位置（ワールド座標）
	Vector3 wave1BarrierSize_ = { 0.0f, 0.0f, 0.0f }; // ワンウェイバリアのサイズ（幅・高さ・奥行）
	static constexpr size_t kWave1BarrierHitMax_ = 8; // ワンウェイバリアヒットエフェクトの最大数
	std::vector<Wave1BarrierHit> wave1BarrierHits_; // ワンウェイバリアヒットエフェクトの情報リスト
	bool wave1BarrierFlashRequested_ = false; // ワンウェイバリアヒットフラッシュエフェクトのリクエストフラグ
	Vector3 wave1BarrierFlashPos_ = { 0.0f, 0.0f, 0.0f }; // ワンウェイバリアヒットフラッシュエフェクトのリクエスト情報
	//======================================================================
	// 弾の外部設定
	//======================================================================
	PlayerShotConfig shotConfig_; // プレイヤー弾設定(JSON読込結果)
	//======================================================================
	// HUD状態通知（Observer）
	//======================================================================
	std::vector<HudObserver> hudObservers_; // HUD状態通知の登録先リスト
	HudState lastHudState_{}; // 最後に通知したHUD状態
	bool hasLastHudState_ = false; // 最後に通知したHUD状態が有効かどうか
	//======================================================================
	// ゲーム開始時の前進演出
	//======================================================================
	bool introForwardActive_ = false;              // 前進演出中か
	float introForwardT_ = 0.0f;                   // 前進演出の経過時間
	float introForwardDuration_ = 0.6f;            // 前進演出時間
	Vector3 introForwardStartPos_ = { 0,0,0 };     // 開始位置
	Vector3 introForwardTargetPos_ = { 0,0,0 };    // 到達位置
	/// <summary>
	/// ゲーム開始時の前進演出を更新します。
	/// </summary>
	void UpdateIntroForwardMove_(float dt);
};