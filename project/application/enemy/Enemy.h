#pragma once
#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "BaseScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

//=============================================================
// Enemyクラス
// 通常敵の挙動と当たり判定を管理するクラス。
//=============================================================
enum class EnemyBehavior {
	StraightStop,   // いまの「Z手前に進んでstopZで止まる」
	SineX,          // Xをサイン波で揺らしながら前進
	StrafeLtoR,     // Xを左右往復（矩形波）しながら前進
	ChasePlayer,    // プレイヤー方向にじわっと追尾
	/// ここに将来：ShootOnly / Kamikaze なども追加可
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

	// HP設定
	/// <summary>HPを設定します（最大HPも更新）。</summary>
	void SetHP(int hp) {
		hp_ = hp;
		maxHP_ = hp;
	}
	// モデル差し替え
	/// <summary>モデルを設定します。</summary>
	void SetModel(const std::string& modelName) {
		if (object_) object_->SetModel(modelName);
	}
	// スケール変更
	/// <summary>スケールを設定します（当たり判定用スケールも更新）。</summary>
	void SetScale(const Vector3& scale) {
		baseScale_ = scale; // 元のスケールを更新
		colliderScale_ = scale; // 当たり判定用スケールも更新
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
	Vector3 GetColliderScale() const { return colliderScale_; }
	/// <summary>当たり判定用スケールを取得します。</summary>
	void SetColliderScale(const Vector3& s) { colliderScale_ = s; }
	/// <summary>当たり判定用スケールを取得します。</summary>
	Vector3 GetWorldPosition() const;
	/// <summary>当たり判定用スケールを取得します。</summary>
	Vector3 GetScale() const {
		return object_ ? object_->GetScale() : Vector3{ 1.0f, 1.0f, 1.0f };
	}
	/// <summary>当たり判定用スケールを取得します。</summary>
	const std::function<Vector3()>& GetPlayer() const { return playerGetter_; }
	/// <summary>当たり判定用スケールを取得します。</summary>
	int GetHP() const { return hp_; }
	/// <summary>当たり判定用スケールを取得します。</summary>
	int GetMaxHP() const { return maxHP_; }

	// --- 設定系を追加 ---
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
	/// <summary>追尾用プレイヤー位置参照を設定します。</summary>
	void SetPlayerRef(const Vector3* playerPos) { playerPos_ = playerPos; } // 追尾用
	// 将来の発射フック（今は未使用）
	/// <summary>射撃可能フラグとインターバルを設定します。</summary>
	void SetCanShoot(bool v, float interval) { canShoot_ = v; shootInterval_ = interval; }
	/// <summary>プレイヤー位置取得関数を設定します。</summary>
	void SetPlayer(std::function<Vector3()> getter) { playerGetter_ = std::move(getter); }
	/// <summary>SineX用の位相を設定します。</summary>
	void SetSinePhase(float rad) { sinePhase_ = rad; }
	/// <summary>親シーンを取得します。</summary>
	BaseScene* GetParentScene() const { return parentScene_; }

private:
	std::unique_ptr<Object3d> object_;
	Camera* camera = nullptr;
	BaseScene* parentScene_ = nullptr;

	int hp_ = 3;
	int maxHP_ = 3;
	bool isDead_ = false;

	Vector3 velocity_ = { 0.0f, 0.0f, -0.1f }; // 毎フレームの移動量（Z方向に手前）
	float stopZ_ = 30.0f;                      // このZ座標になったら止まる
	bool stopMove_ = false;                    // 到達フラグ

	bool  isLocked_ = false;
	float pulseT_ = 0.0f;   // パルス用の位相
	Vector3 baseScale_ = { 1.0f,1.0f,1.0f }; // 元のスケールを保持

	Vector3 colliderScale_ = { 1.0f, 1.0f, 1.0f }; // 当たり判定用スケール

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

	// 追尾用
	const Vector3* playerPos_ = nullptr;
	float chaseSpeed_ = 0.07f;

	// 将来の射撃用
	bool  canShoot_ = false;
	float shootInterval_ = 120.0f; // フレーム
	float shootTimer_ = 0.0f;

	std::function<Vector3()> playerGetter_;

	float sinePhase_ = 0.0f;  // SineX用の位相(ラジアン)
};
