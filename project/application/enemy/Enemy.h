#pragma once
#include <memory>
#include "Object3d.h"
#include "engine/3d/camera/Camera.h"
#include "BaseScene.h"
#include <engine/effect/particle/ParticleManager.h>
#include "engine/effect/line/LineRenderer.h"
#include "application/player/reticle/Reticle.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
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
};
// 敵の役割（通常 / Wave3中ボス / Wave3蘇生核）
enum class EnemyType {
	Normal,      // 通常ザコ
	Wave3MidBoss,// Wave3 中ボス
	Wave3Core,   // Wave3 蘇生用の「核」
};

class Enemy {
public:
	/// <summary>敵を初期化します。</summary>
	/// <param name="common">Object3d共通。</param>
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);

	/// <summary>敵を更新します。</summary>
	void Update();
	/// <summary>敵を描画します。</summary>
	void Draw(DirectXCommon* dxCommon);
	/// <summary>デバッグ用ImGui表示。</summary>
	void ImGuiDebug();

	/// <summary>特殊攻撃でダメージを指定して当たったときの処理。</summary>
	void OnHitWithDamage(int damage); // 特殊攻撃（ダメージ指定）
	/// <summary>敵が死亡したかどうかを取得します。</summary>
	bool IsDead() const { return isDead_; }
	/// <summary>敵がロックオンされているかどうかを取得します。</summary>
	bool IsLocked() const { return isLocked_; }
	/// <summary>敵が死亡演出中かどうかを取得します。</summary>
	bool IsDying() const { return isDying_; }
	/// <summary>敵の死亡リアクションを開始します。</summary>
	void StartDeathReaction(const Vector3& hitDir);
	/// <summary>
	/// Transform情報をObject3dに同期します。
	/// </summary>
	void SyncTransform();
	/// <summary>
	/// 敵が怒っているかどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsAngry() const { return isAngry_; }

	// Getter===================================
	/// <summary>当たり判定用スケールを取得します。</summary>
	Vector3 GetColliderScale() const { return colliderScale_; }
	/// <summary>当たり判定用スケールを取得します。</summary>
	Vector3 GetWorldPosition() const;
	/// <summary>当たり判定用スケールを取得します。</summary>
	Vector3 GetScale() const {
		return object_ ? object_->GetScale() : Vector3{ 1.0f, 1.0f, 1.0f };
	}
	/// <summary>当たり判定用スケールを取得します。</summary>
	int GetHP() const { return hp_; }
	/// <summary>当たり判定用スケールを取得します。</summary>
	int GetMaxHP() const { return maxHP_; }
	/// <summary>当たり判定用スケールを取得します。</summary>
	const std::function<Vector3()>& GetPlayer() const { return playerGetter_; }
	/// <summary>親シーンを取得します。</summary>
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
	/// <summary>HPを設定します（最大HPも更新）。</summary>
	void SetHP(int hp) {
		hp_ = hp;
		maxHP_ = hp;
	}
	/// <summary>モデルを設定します。</summary>
	void SetModel(const std::string& modelName) {
		if (object_) object_->SetModel(modelName);
	}
	/// <summary>スケールを設定します（当たり判定用スケールも更新）。</summary>
	void SetScale(const Vector3& scale) {
		baseScale_ = scale; // 元のスケールを更新
		if (object_) object_->SetScale(scale); // Object3d にも反映
	}
	/// <summary>カメラを設定します。</summary>
	void SetCamera(Camera* camera);
	/// <summary>位置を設定します。</summary>
	void SetPosition(const Vector3& pos);
	/// <summary>親シーンを設定します。</summary>
	void SetParentScene(BaseScene* scene);
	/// <summary>ワールド位置を設定します。</summary>
	void SetLocked(bool v) { isLocked_ = v; if (!v) pulseT_ = 0.0f; }
	/// <summary>当たり判定用スケールを取得します。</summary>
	void SetColliderScale(const Vector3& s) { colliderScale_ = s; }
	/// <summary>挙動タイプを設定します。</summary>
	void SetBehavior(EnemyBehavior b) { behavior_ = b; }
	/// <summary>速度を設定します。</summary>
	void SetVelocity(const Vector3& v) { velocity_ = v; }
	/// <summary>停止Z座標を設定します。</summary>
	void SetStopZ(float z) { stopZ_ = z; }
	/// <summary>SineX用のパラメータを設定します。</summary>
	void SetSineParams(float ampX, float freq) { sineAmpX_ = ampX; sineFreq_ = freq; }
	/// <summary>StrafeLtoR用のパラメータを設定します。</summary>
	void SetStrafeX(float left, float right, float speed) {
		strafeLeft_ = left; strafeRight_ = right; strafeSpeed_ = speed;
		if (strafePosX_ == 0.0f) strafePosX_ = left;
	}
	// 将来の発射フック（今は未使用）
	/// <summary>射撃可能フラグとインターバルを設定します。</summary>
	void SetCanShoot(bool v, float interval) { canShoot_ = v; shootInterval_ = interval; }
	/// <summary>プレイヤー位置取得関数を設定します。</summary>
	void SetPlayer(std::function<Vector3()> getter) { playerGetter_ = std::move(getter); }
	/// <summary>SineX用の位相を設定します。</summary>
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
	// =========================================

private:
	std::unique_ptr<Object3d> object_;
	Camera* camera = nullptr;
	BaseScene* parentScene_ = nullptr;
	Reticle* reticle_ = nullptr;

	int hp_ = 3;
	int maxHP_ = 3;
	bool isDead_ = false;

	// Wave3 用の種別（デフォルトは通常）
	EnemyType type_ = EnemyType::Normal;

	Vector3 velocity_ = { 0.0f, 0.0f, -0.1f }; // 毎フレームの移動量（Z方向に手前）
	float stopZ_ = 30.0f;                      // このZ座標になったら止まる
	bool stopMove_ = false;                    // 到達フラグ

	bool  isLocked_ = false;
	float pulseT_ = 0.0f;   // パルス用の位相
	Vector3 baseScale_ = { 1.0f,1.0f,1.0f }; // 元のスケールを保持

	Vector3 colliderScale_ = { 3.260f,5.5f,4.16f };// 当たり判定用スケール

	EnemyBehavior behavior_ = EnemyBehavior::StraightStop;

	// 共通
	float t_ = 0.0f;

	// Sine 用
	float sineAmpX_ = 0.0f;
	float sineFreq_ = 1.0f;
	float startX_ = 0.0f; // 初期Xを保持

	// Strafe 用
	float strafeLeft_ = -10.0f, strafeRight_ = 10.0f, strafeSpeed_ = 0.2f;
	float strafePosX_ = 0.0f;
	int   strafeDir_ = +1;

	// ChasePlayer 用
	float chaseSpeed_ = 0.07f;

	// 将来の射撃用
	bool  canShoot_ = false;
	float shootInterval_ = 120.0f; // フレーム
	float shootTimer_ = 0.0f;

	std::function<Vector3()> playerGetter_;

	float sinePhase_ = 0.0f;  // SineX用の位相(ラジアン)

	// 死亡リアクション用
	bool isDying_ = false;        // 死亡演出中かどうか
	float deathTimer_ = 0.0f;     // 経過時間
	float deathDuration_ = 1.2f;  // 演出の長さ（秒相当）
	Vector3 deathVelocity_ = { 0.0f, 0.0f, 0.0f }; // 吹っ飛び速度
	Vector3 deathRotateSpeed_ = { 0.0f, 0.0f, 0.0f }; // 撃墜回転用
	float deathAlpha_ = 1.0f;     // フェード用アルファ

	// どのリアクションか
	EnemyDeathReaction deathReaction_ = EnemyDeathReaction::BlowAway;

	// 敵がどんな消え方をしたか
	bool defeated_ = false; // ちゃんと倒された
	bool escaped_ = false; // プレイヤーを通り過ぎて逃げた

	// 飛び掛かり用
	float pounceTime_ = 0.0f;         // 経過時間
	float pounceDuration_ = 1.6f;     // 落下までの時間
	Vector3 pounceStart_;             // 開始位置
	Vector3 pounceApex_;              // 山の頂点
	Vector3 pounceTarget_;            // 落下目標（プレイヤー付近）
	bool   pounceStarted_ = false; // 飛び掛かり動作が開始されたかどうか
	bool   pounceDiving_ = false; // 急降下フェーズに入ったかどうか

	const float dt = 1.0f / 60.0f; // 固定フレームレート想定

	// ==== FreeRoam（Wave3中ボス用） ====
	// 動き回る範囲
	Vector3 roamMin_ = { -18.0f, 4.0f, 40.0f };
	Vector3 roamMax_ = { 18.0f,10.0f, 62.0f };
	// 通常＆怒り時のベース速度
	float   roamSpeedNormal_ = 0.10f;
	float   roamSpeedAngry_ = 0.24f;
	// パターンA/B/C制御用
	int   roamPattern_ = 0;      // 0:A 1:B 2:C
	float roamPatternTimer_ = 0.0f;   // パターン切替用タイマー
	float roamAngle_ = 0.0f;   // 円運動などで使う角度
	// ターゲット追尾用（パターンC用）
	Vector3 roamTarget_ = { 0.0f, 0.0f, 0.0f };
	bool    hasRoamTarget_ = false;
	// ==== 怒り状態 ====
	bool  isAngry_ = false; // 怒り状態フラグ
	float angryTimer_ = 0.0f;  // 怒り経過時間
	float angryDuration_ = 0.0f;  // 怒り持続時間
	bool freezeMove_ = false; // 動きを一時停止するか
};