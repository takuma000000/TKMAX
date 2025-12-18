#pragma once
#include <memory>
#include "Object3d.h"
#include "camera/Camera.h"
#include "BaseScene.h"
#include <ParticleManager.h>
#include "LineRenderer.h"
#include "reticle/Reticle.h"
#include "Easing.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

//=============================================================
// Enemyクラス
// 通常敵の挙動と当たり判定を管理するクラス。
//=============================================================

enum class EnemyBehavior {
	StraightStop,    // いまの「Z手前に進んでstopZで止まる」
	SineX,           // Xをサイン波で揺らしながら前進
	StrafeLtoR,      // Xを左右往復（矩形波）しながら前進
	ChasePlayer,     // プレイヤー方向にじわっと追尾
	PounceFromAbove, // 上空から急降下してくる
	FreeRoam,        // 自由に動き回る
};
// 死亡リアクションパターン
enum class EnemyDeathReaction {
	BlowAway,       // 吹っ飛んで消える
	RiseAbsorb,     // 上に吸い込まれるように消える
	Collapse,       // 崩れ落ちて潰れて消える
	BossFinal,      // その場で揺れながら爆散
};
// 敵の役割（通常 / Wave3中ボス / Wave3蘇生核）
enum class EnemyType {
	Normal,      // 通常ザコ
	Wave3MidBoss,// Wave3 中ボス
	Wave3Core,   // Wave3 蘇生用の「核」
	Boss,        // ボス
};

class Enemy {
public:
	/// <summary>
	/// 敵を初期化します。
	/// </summary>
	/// <param name="common"></param>
	/// <param name="dxCommon"></param>
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	/// <summary>
	/// 敵を更新します。
	/// </summary>
	void Update(float dt);
	/// <summary>
	/// 敵を描画します。
	/// </summary>
	/// <param name="dxCommon"></param>
	void Draw(DirectXCommon* dxCommon);
	/// <summary>
	/// ImGuiデバッグ表示。
	/// </summary>
	void ImGuiDebug();

	/// <summary>
	/// 敵がダメージを受けたときの処理。
	/// </summary>
	/// <param name="damage"></param>
	void OnHitWithDamage(int damage); // 特殊攻撃（ダメージ指定）
	/// <summary>
	/// 敵が即死ダメージを受けたときの処理。
	/// </summary>
	/// <returns></returns>
	bool IsDead() const { return isDead_; }
	/// <summary>
	/// 敵が位置ロック中かどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsLocked() const { return isLocked_; }
	/// <summary>
	/// 敵が死亡リアクション中かどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsDying() const { return isDying_; }
	/// <summary>
	/// 敵の死亡リアクションを開始します。
	/// </summary>
	/// <param name="hitDir"></param>
	void StartDeathReaction(const Vector3& hitDir);
	/// <summary>
	/// 敵のTransformを同期します。
	/// </summary>
	void SyncTransform();
	/// <summary>
	/// 敵が怒っているかどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsAngry() const { return isAngry_; }
	/// <summary>
	/// ボスの最終死亡リアクションを開始します。
	/// </summary>
	/// <param name="hitDir"></param>
	void StartBossDeathReaction(const Vector3& hitDir);

	// Getter===================================
	/// <summary>
	/// 当たり判定用スケールを取得します。
	/// </summary>
	/// <returns></returns>
	Vector3 GetColliderScale() const { return colliderScale_; }
	/// <summary>
	/// ワールド位置を取得します。
	/// </summary>
	/// <returns></returns>
	Vector3 GetWorldPosition() const;
	/// <summary>
	/// スケールを取得します。
	/// </summary>
	/// <returns></returns>
	Vector3 GetScale() const {
		return object_ ? object_->GetScale() : Vector3{ 1.0f, 1.0f, 1.0f };
	}
	/// <summary>
	/// HPを取得します。
	/// </summary>
	/// <returns></returns>
	int GetHP() const { return hp_; }
	/// <summary>
	/// 最大HPを取得します。
	/// </summary>
	/// <returns></returns>
	int GetMaxHP() const { return maxHP_; }
	/// <summary>
	/// プレイヤー位置取得関数を取得します。
	/// </summary>
	/// <returns></returns>
	const std::function<Vector3()>& GetPlayer() const { return playerGetter_; }
	/// <summary>
	/// 親シーンを取得します。
	/// </summary>
	/// <returns></returns>
	BaseScene* GetParentScene() const { return parentScene_; }
	/// <summary>
	/// 敵が撃破されたかどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool GetDefeated() const { return defeated_; }
	/// <summary>
	/// 敵が逃走したかどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool GetEscaped()  const { return escaped_; }
	/// <summary>
	/// 敵のタイプを取得します。
	/// </summary>
	/// <returns></returns>
	EnemyType GetType() const { return type_; }
	// =========================================
	// Setter===================================
	/// <summary>
	/// HPを設定します。
	/// </summary>
	/// <param name="hp"></param>
	void SetHP(int hp) {
		hp_ = hp;
		maxHP_ = hp;
	}
	/// <summary>
	/// モデルを設定します。
	/// </summary>
	/// <param name="modelName"></param>
	void SetModel(const std::string& modelName) {
		if (object_) object_->SetModel(modelName);
	}
	/// <summary>
	/// スケールを設定します。
	/// </summary>
	/// <param name="scale"></param>
	void SetScale(const Vector3& scale) {
		baseScale_ = scale; // 元のスケールを更新
		if (object_) object_->SetScale(scale); // Object3d にも反映
	}
	/// <summary>
	/// カメラを設定します。
	/// </summary>
	/// <param name="camera"></param>
	void SetCamera(Camera* camera);
	/// <summary>
	/// 位置を設定します。
	/// </summary>
	/// <param name="pos"></param>
	void SetPosition(const Vector3& pos);
	/// <summary>
	/// 親シーンを設定します。
	/// </summary>
	/// <param name="scene"></param>
	void SetParentScene(BaseScene* scene);
	/// <summary>
	/// 位置ロックフラグを設定します。
	/// </summary>
	/// <param name="v"></param>
	void SetLocked(bool v) { isLocked_ = v; if (!v) pulseT_ = 0.0f; }
	/// <summary>
	/// 当たり判定用スケールを設定します。
	/// </summary>
	/// <param name="s"></param>
	void SetColliderScale(const Vector3& s) { colliderScale_ = s; }
	/// <summary>
	/// 挙動パターンを設定します。
	/// </summary>
	/// <param name="b"></param>
	void SetBehavior(EnemyBehavior b) { behavior_ = b; }
	/// <summary>
	/// 毎フレームの移動量を設定します。
	/// </summary>
	/// <param name="v"></param>
	void SetVelocity(const Vector3& v) { velocity_ = v; }
	/// <summary>
	/// 停止Z座標を設定します。
	/// </summary>
	/// <param name="z"></param>
	void SetStopZ(float z) { stopZ_ = z; }
	/// <summary>
	/// SineX用のパラメータを設定します。
	/// </summary>
	/// <param name="ampX"></param>
	/// <param name="freq"></param>
	void SetSineParams(float ampX, float freq) { sineAmpX_ = ampX; sineFreq_ = freq; }
	/// <summary>
	/// StrafeX用のパラメータを設定します。
	/// </summary>
	/// <param name="left"></param>
	/// <param name="right"></param>
	/// <param name="speed"></param>
	void SetStrafeX(float left, float right, float speed) {
		strafeLeft_ = left; strafeRight_ = right; strafeSpeed_ = speed;
		if (strafePosX_ == 0.0f) strafePosX_ = left;
	}
	/// <summary>
	/// 撃てるかどうかを設定します。
	/// </summary>
	/// <param name="v"></param>
	/// <param name="interval"></param>
	void SetCanShoot(bool v, float interval) { canShoot_ = v; shootInterval_ = interval; }
	/// <summary>
	/// プレイヤー位置取得関数を設定します。
	/// </summary>
	/// <param name="getter"></param>
	void SetPlayer(std::function<Vector3()> getter) { playerGetter_ = std::move(getter); }
	/// <summary>
	/// Sine波の位相を設定します。
	/// </summary>
	/// <param name="rad"></param>
	void SetSinePhase(float rad) { sinePhase_ = rad; }
	/// <summary>
	/// レティクルを設定します。
	/// </summary>
	/// <param name="r"></param>
	void SetReticle(class Reticle* r) { reticle_ = r; }
	/// <summary>
	/// 飛び掛かり用のパラメータを設定します。
	/// </summary>
	/// <param name="start"></param>
	/// <param name="apex"></param>
	/// <param name="target"></param>
	/// <param name="duration"></param>
	void SetPounceParameters(const Vector3& start, const Vector3& apex, const Vector3& target, float duration = 1.6f) {
		pounceStart_ = start; // 開始位置
		pounceApex_ = apex; // 山の頂点
		pounceTarget_ = target; // 目標位置
		pounceDuration_ = duration; // 持続時間
		pounceTime_ = 0.0f; // 経過時間リセット
		pounceStarted_ = true; // フラグセット
		pounceDiving_ = false; // 急降下フェーズ前
	}
	/// <summary>
	/// 敵のタイプを設定します。
	/// </summary>
	/// <param name="t"></param>
	void SetType(EnemyType t) { type_ = t; }
	/// <summary>
	/// FreeRoam用のパラメータを設定します。
	/// </summary>
	/// <param name="min"></param>
	/// <param name="max"></param>
	/// <param name="normalSpeed"></param>
	/// <param name="angrySpeed"></param>
	void SetFreeRoamArea(const Vector3& min, const Vector3& max,
		float normalSpeed, float angrySpeed) {
		roamMin_ = min;
		roamMax_ = max;
		roamSpeedNormal_ = normalSpeed;
		roamSpeedAngry_ = angrySpeed;

		// 初期ターゲットは範囲の中心あたりにしておく
		roamTarget_ = {
			(min.x + max.x) * 0.5f,
			(min.y + max.y) * 0.5f,
			(min.z + max.z) * 0.5f,
		};
		hasRoamTarget_ = false;
	}
	/// <summary>
	/// 敵を怒り状態にします。
	/// </summary>
	/// <param name="duration"></param>
	void SetAngry(float duration) {
		isAngry_ = true;
		angryDuration_ = duration;
		angryTimer_ = 0.0f;
	}
	/// <summary>
	/// 移動凍結フラグを設定します。
	/// </summary>
	/// <param name="v"></param>
	void SetFreezeMove(bool v) { freezeMove_ = v; }
	/// <summary>
	/// 現在のHPを設定します。
	/// </summary>
	/// <param name="hp"></param>
	void SetCurrentHP(int hp) {
		if (hp < 0) { hp = 0; }
		if (hp > maxHP_) { hp = maxHP_; }
		hp_ = hp;
	}
	// =========================================
private:
	//--------------------------------------------------------------
	//  Enemy 内部データ（基本）
	//--------------------------------------------------------------
	std::unique_ptr<Object3d> object_;
	Camera* camera = nullptr;
	BaseScene* parentScene_ = nullptr;
	Reticle* reticle_ = nullptr;
	//--------------------------------------------------------------
	//  HP / 生存状態
	//--------------------------------------------------------------
	int   hp_ = 3;
	int   maxHP_ = 3;
	bool  isDead_ = false;  // 完全に死亡（描画/更新停止）
	bool  defeated_ = false; // プレイヤーに倒された
	bool  escaped_ = false; // 逃走扱い
	//--------------------------------------------------------------
	//  基本行動タイプ
	//--------------------------------------------------------------
	EnemyType    type_ = EnemyType::Normal;
	EnemyBehavior behavior_ = EnemyBehavior::StraightStop;
	float t_ = 0.0f;    // 各種挙動で使う汎用タイマー
	//--------------------------------------------------------------
	//  ロックオン演出
	//--------------------------------------------------------------
	bool   isLocked_ = false;
	float  pulseT_ = 0.0f;
	Vector3 baseScale_ = { 1.0f, 1.0f, 1.0f }; // 元のスケール
	Vector3 colliderScale_ = { 3.260f, 5.5f, 4.16f }; // AABBスケール
	//--------------------------------------------------------------
	//  移動（直進・停止）
	//--------------------------------------------------------------
	Vector3 velocity_ = { 0.0f, 0.0f, -0.1f }; // 基本前進
	float   stopZ_ = 30.0f;  // このZで止まる
	bool    stopMove_ = false;
	//--------------------------------------------------------------
	//  Sin 波移動（Wave2など）
	//--------------------------------------------------------------
	float sineAmpX_ = 0.0f;
	float sineFreq_ = 1.0f;
	float sinePhase_ = 0.0f; // ラジアン
	float startX_ = 0.0f;    // 初期位置保持
	//--------------------------------------------------------------
	//  ストレーフ左右移動
	//--------------------------------------------------------------
	float strafeLeft_ = -10.0f;
	float strafeRight_ = 10.0f;
	float strafeSpeed_ = 0.2f;
	float strafePosX_ = 0.0f;
	int   strafeDir_ = +1;
	//--------------------------------------------------------------
	//  プレイヤー追尾（Chase）
	//--------------------------------------------------------------
	float chaseSpeed_ = 0.07f;
	std::function<Vector3()> playerGetter_;
	//--------------------------------------------------------------
	//  射撃（後で使う）
	//--------------------------------------------------------------
	bool  canShoot_ = false;
	float shootInterval_ = 120.0f; // フレーム
	float shootTimer_ = 0.0f;
	//--------------------------------------------------------------
	//  死亡演出（BlowAway / Dissolve 等）
	//--------------------------------------------------------------
	bool   isDying_ = false; // 演出中フラグ
	float  deathTimer_ = 0.0f;
	float  deathDuration_ = 1.2f;
	Vector3 deathVelocity_ = { 0,0,0 };
	Vector3 deathRotateSpeed_ = { 0,0,0 };
	float  deathAlpha_ = 1.0f;  // フェード
	EnemyDeathReaction deathReaction_ = EnemyDeathReaction::BlowAway;

	// --- BossFinal 用：ぶっ飛び＆カメラ演出 ---
	bool   bossFinalBigBurstDone_ = false; // 大きいz撃破円を出したか
	bool   bossFinalCameraInited_ = false; // カメラ初期化済みフラグ
	Vector3 bossFinalCameraStartPos_{};    // カメラの開始位置
	Vector3 bossFinalCameraEndPos_{};      // カメラの終了位置
	// ボス最終死亡リアクション用
	bool    bossFinalLaunchStarted_ = false;
	Vector3 bossFinalLaunchStartPos_ = { 0.0f, 0.0f, 0.0f };
	//--------------------------------------------------------------
	//  飛び掛かり（PounceFromAbove / Wave1 敵）
	//--------------------------------------------------------------
	float   pounceTime_ = 0.0f;   // 経過時間
	float   pounceDuration_ = 1.6f;   // 滑空→急降下の長さ
	Vector3 pounceStart_;             // 開始位置
	Vector3 pounceApex_;              // 山の頂点
	Vector3 pounceTarget_;            // 着地点（プレイヤー）
	bool    pounceStarted_ = false;
	bool    pounceDiving_ = false;
	//--------------------------------------------------------------
	//  Wave3 中ボス：FreeRoam（自由移動）
	//--------------------------------------------------------------
	Vector3 roamMin_ = { -18.0f, 4.0f, 40.0f };
	Vector3 roamMax_ = { 18.0f, 10.0f, 62.0f };

	float roamSpeedNormal_ = 0.10f;
	float roamSpeedAngry_ = 0.24f;

	int   roamPattern_ = 0;   // 0:A 1:B 2:C
	float roamPatternTimer_ = 0.0f;
	float roamAngle_ = 0.0f;

	Vector3 roamTarget_ = { 0,0,0 };
	bool    hasRoamTarget_ = false;
	//--------------------------------------------------------------
	//  怒り（Enraged 状態）
	//--------------------------------------------------------------
	bool  isAngry_ = false;
	float angryTimer_ = 0.0f;
	float angryDuration_ = 0.0f;
	bool  freezeMove_ = false;
};