#pragma once
#include <memory>
#include "Object3d.h"
#include "camera/Camera.h"
#include "BaseScene.h"
#include <ParticleManager.h>
#include "LineRenderer.h"
#include "reticle/Reticle.h"
#include "Easing.h"

//=============================================================
// Enemyクラス
// 敵キャラクターの基本クラス
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
	/// 敵オブジェクトを初期化します。
	/// </summary>
	/// <param name="common">Object3d の共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// 敵の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void Update(float dt);

	/// <summary>
	/// 敵を描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// ImGui によるデバッグ情報を表示します。
	/// </summary>
	void ImGuiDebug();

	/// <summary>
	/// 敵がダメージを受けたときの処理を行います。
	/// </summary>
	/// <param name="damage">受けるダメージ量</param>
	void OnHitWithDamage(int damage); // 特殊攻撃（ダメージ指定）
	/// <summary>
	/// 敵が死亡しているかどうかを取得します。
	/// </summary>
	/// <returns>死亡している場合 true、それ以外は false</returns>
	bool IsDead() const { return isDead_; }
	/// <summary>
	/// 敵が位置ロック中かどうかを取得します。
	/// </summary>
	/// <returns>位置ロック中の場合 true、それ以外は false</returns>
	bool IsLocked() const { return isLocked_; }
	/// <summary>
	/// 敵が死亡リアクション中かどうかを取得します。
	/// </summary>
	/// <returns>死亡リアクション中の場合 true、それ以外は false</returns>
	bool IsDying() const { return isDying_; }
	/// <summary>
	/// 敵の死亡リアクションを開始します。
	/// </summary>
	/// <param name="hitDir">被弾方向（正規化ベクトル）</param>
	void StartDeathReaction(const Vector3& hitDir);
	/// <summary>
	/// 敵の Transform を内部状態と同期します。
	/// </summary>
	void SyncTransform();
	/// <summary>
	/// 敵が怒り状態かどうかを取得します。
	/// </summary>
	/// <returns>怒り状態の場合 true、それ以外は false</returns>
	bool IsAngry() const { return isAngry_; }
	/// <summary>
	/// ボスの最終死亡リアクションを開始します。
	/// </summary>
	/// <param name="hitDir">被弾方向（正規化ベクトル）</param>
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
	TKM::BaseScene* GetParentScene() { return parentScene_; }
	/// <summary>
	/// 親シーンを取得します。(const版)
	/// </summary>
	/// <returns></returns>
	const TKM::BaseScene* GetParentScene() const { return parentScene_; }
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
	/// HP（最大HPも同値）を設定します。
	/// </summary>
	/// <param name="hp">設定する HP</param>
	void SetHP(int hp) {
		hp_ = hp;
		maxHP_ = hp;
	}
	/// <summary>
	/// モデルを設定します。
	/// </summary>
	/// <param name="modelName">モデル名（例: "sphere.obj"）</param>
	void SetModel(const std::string& modelName) {
		if (object_) object_->SetModel(modelName);
	}
	/// <summary>
	/// スケールを設定します。
	/// </summary>
	/// <param name="scale">設定するスケール</param>
	void SetScale(const Vector3& scale) {
		baseScale_ = scale; // 元のスケールを更新
		if (object_) object_->SetScale(scale); // Object3d にも反映
	}
	/// <summary>
	/// 使用するカメラを設定します。
	/// </summary>
	/// <param name="camera">描画および判定に使用するカメラ</param>
	void SetCamera(TKM::Camera* camera);
	/// <summary>
	/// 位置を設定します。
	/// </summary>
	/// <param name="pos">設定する位置（ワールド座標）</param>
	void SetPosition(const Vector3& pos);
	/// <summary>
	/// 親シーンを設定します。
	/// </summary>
	/// <param name="scene">親シーン</param>
	void SetParentScene(TKM::BaseScene* scene);
	/// <summary>
	/// 位置ロックフラグを設定します。
	/// </summary>
	/// <param name="v">ロックする場合 true、それ以外は false</param>
	void SetLocked(bool v) { isLocked_ = v; if (!v) pulseT_ = 0.0f; }
	/// <summary>
	/// 当たり判定用スケールを設定します。
	/// </summary>
	/// <param name="s">当たり判定用スケール</param>
	void SetColliderScale(const Vector3& s) { colliderScale_ = s; }
	/// <summary>
	/// 挙動パターンを設定します。
	/// </summary>
	/// <param name="b">挙動パターン</param>
	void SetBehavior(EnemyBehavior b) { behavior_ = b; }
	/// <summary>
	/// 毎フレームの移動量（速度ベクトル）を設定します。
	/// </summary>
	/// <param name="v">移動ベクトル</param>
	void SetVelocity(const Vector3& v) { velocity_ = v; }
	/// <summary>
	/// 停止 Z 座標を設定します。
	/// </summary>
	/// <param name="z">停止 Z 座標（ワールド座標）</param>
	void SetStopZ(float z) { stopZ_ = z; }
	/// <summary>
	/// SineX 用のパラメータを設定します。
	/// </summary>
	/// <param name="ampX">振幅</param>
	/// <param name="freq">周波数</param>
	void SetSineParams(float ampX, float freq) { sineAmpX_ = ampX; sineFreq_ = freq; }
	/// <summary>
	/// StrafeX 用のパラメータを設定します。
	/// </summary>
	/// <param name="left">左端 X 座標</param>
	/// <param name="right">右端 X 座標</param>
	/// <param name="speed">移動速度</param>
	void SetStrafeX(float left, float right, float speed) {
		strafeLeft_ = left; strafeRight_ = right; strafeSpeed_ = speed;
		if (strafePosX_ == 0.0f) strafePosX_ = left;
	}
	/// <summary>
	/// 射撃可否と射撃間隔を設定します。
	/// </summary>
	/// <param name="v">射撃可能にする場合 true、それ以外は false</param>
	/// <param name="interval">射撃間隔（秒）</param>
	void SetCanShoot(bool v, float interval) { canShoot_ = v; shootInterval_ = interval; }
	/// <summary>
	/// プレイヤー位置取得関数を設定します。
	/// </summary>
	/// <param name="getter">プレイヤー位置を返す関数オブジェクト</param>
	void SetPlayer(std::function<Vector3()> getter) { playerGetter_ = std::move(getter); }
	/// <summary>
	/// Sine 波の位相を設定します。
	/// </summary>
	/// <param name="rad">位相（ラジアン）</param>
	void SetSinePhase(float rad) { sinePhase_ = rad; }
	/// <summary>
	/// レティクル参照を設定します。
	/// </summary>
	/// <param name="r">レティクル</param>
	void SetReticle(class Reticle* r) { reticle_ = r; }
	/// <summary>
	/// 飛び掛かり用のパラメータを設定します。
	/// </summary>
	/// <param name="start">開始位置（ワールド座標）</param>
	/// <param name="apex">頂点位置（ワールド座標）</param>
	/// <param name="target">目標位置（ワールド座標）</param>
	/// <param name="duration">演出時間（秒）</param>
	void SetPounceParameters(const Vector3& start, const Vector3& apex, const Vector3& target, float duration = 1.6f) {
		pounceStart_ = start;
		pounceApex_ = apex;
		pounceTarget_ = target;
		pounceDuration_ = duration;
		pounceTime_ = 0.0f;
		pounceStarted_ = true;
		pounceDiving_ = false;
	}
	/// <summary>
	/// 敵のタイプを設定します。
	/// </summary>
	/// <param name="t">敵タイプ</param>
	void SetType(EnemyType t) {
		type_ = t;
		lockPulseEnabled_ = (type_ != EnemyType::Boss);
		if (!lockPulseEnabled_) { pulseT_ = 0.0f; }
	}
	/// <summary>
	/// FreeRoam 用の行動範囲と速度を設定します。
	/// </summary>
	/// <param name="min">行動範囲の最小座標（ワールド座標）</param>
	/// <param name="max">行動範囲の最大座標（ワールド座標）</param>
	/// <param name="normalSpeed">通常時の移動速度</param>
	/// <param name="angrySpeed">怒り時の移動速度</param>
	void SetFreeRoamArea(const Vector3& min, const Vector3& max, float normalSpeed, float angrySpeed) {
		// 中身そのまま
	}
	/// <summary>
	/// 敵を怒り状態にします。
	/// </summary>
	/// <param name="duration">怒り状態の継続時間（秒）</param>
	void SetAngry(float duration) {
		isAngry_ = true;
		angryDuration_ = duration;
		angryTimer_ = 0.0f;
	}
	/// <summary>
	/// 移動凍結フラグを設定します。
	/// </summary>
	/// <param name="v">凍結する場合 true、それ以外は false</param>
	void SetFreezeMove(bool v) { freezeMove_ = v; }
	/// <summary>
	/// 現在の HP を設定します。
	/// </summary>
	/// <param name="hp">設定する HP（0〜maxHP_ にクランプされます）</param>
	void SetCurrentHP(int hp) {
		if (hp < 0) { hp = 0; }
		if (hp > maxHP_) { hp = maxHP_; }
		hp_ = hp;
	}
	/// <summary>
	/// 触手モデルを設定します。
	/// </summary>
	/// <param name="modelName">モデル名（例: "tentacle.obj"）</param>
	void SetTentacleModel(const std::string& modelName);
	/// <summary>
	/// 触手のローカル変換を設定します。
	/// </summary>
	/// <param name="pos">位置（ローカル座標）</param>
	/// <param name="rot">回転（ローカル座標、オイラー角）</param>
	/// <param name="scale">スケール（ローカル座標）</param>
	void SetTentacleLocal(const Vector3& pos, const Vector3& rot, const Vector3& scale);
	// =========================================
private:
	//--------------------------------------------------------------
	//  Enemy 内部データ（基本）
	//--------------------------------------------------------------
	std::unique_ptr<TKM::Object3d> object_; // 敵の3Dオブジェクト(傘)
	std::unique_ptr<TKM::Object3d> tentacle_ = nullptr; // 触手オブジェクト
	Vector3 tentacleLocalPos_{ 0.0f, 0.0f, 0.0f };        /// 触手の取り付け位置（調整用）
	Vector3 tentacleLocalRot_{ 0.0f, 0.0f, 0.0f };
	Vector3 tentacleLocalScale_{ 1.0f, 1.0f, 1.0f };
	TKM::Camera* camera_ = nullptr;
	TKM::BaseScene* parentScene_ = nullptr;
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

	// Enemy.h の private: に追加
	bool lockPulseEnabled_ = true; // ロック中の脈動を有効にするか（ボスはOFFにする）
};