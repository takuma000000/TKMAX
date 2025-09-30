#pragma once
#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "BaseScene.h"
#include "externals/imgui/imgui.h"

enum class EnemyBehavior {
	StraightStop,   // いまの「Z手前に進んでstopZで止まる」
	SineX,          // Xをサイン波で揺らしながら前進
	StrafeLtoR,     // Xを左右往復（矩形波）しながら前進
	ChasePlayer,    // プレイヤー方向にじわっと追尾
	/// ここに将来：ShootOnly / Kamikaze なども追加可
};

class Enemy {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	void Update();
	void Draw(DirectXCommon* dxCommon);
	void ImGuiDebug();

	void OnHit(); // 弾が当たったとき呼ぶ
	void OnHitWithDamage(int damage); // 特殊攻撃（ダメージ指定）
	bool IsDead() const { return isDead_; }

	// 既存 Enemy クラスの public: に追記
	bool IsLocked() const { return isLocked_; }

	// HP設定
	void SetHP(int hp) {
		hp_ = hp;
		maxHP_ = hp;
	}
	// モデル差し替え
	void SetModel(const std::string& modelName) {
		if (object_) object_->SetModel(modelName);
	}
	// スケール変更
	void SetScale(const Vector3& scale) {
		baseScale_ = scale; // 元のスケールを更新
		colliderScale_ = scale; // 当たり判定用スケールも更新
		if (object_) object_->SetScale(scale); // Object3d にも反映
	}

	void SetCamera(Camera* camera);
	void SetPosition(const Vector3& pos);
	void SetParentScene(BaseScene* scene);
	void SetLocked(bool v) { isLocked_ = v; if (!v) pulseT_ = 0.0f; }
	Vector3 GetColliderScale() const { return colliderScale_; }
	void SetColliderScale(const Vector3& s) { colliderScale_ = s; }
	Vector3 GetWorldPosition() const;
	Vector3 GetScale() const {
		return object_ ? object_->GetScale() : Vector3{ 1.0f, 1.0f, 1.0f };
	}
	const std::function<Vector3()>& GetPlayer() const { return playerGetter_; }
	int GetHP() const { return hp_; }
	int GetMaxHP() const { return maxHP_; }

	// --- 設定系を追加 ---
	void SetBehavior(EnemyBehavior b) { behavior_ = b; }
	void SetVelocity(const Vector3& v) { velocity_ = v; }
	void SetStopZ(float z) { stopZ_ = z; }
	void SetSineParams(float ampX, float freq) { sineAmpX_ = ampX; sineFreq_ = freq; }
	void SetStrafeX(float left, float right, float speed) {
		strafeLeft_ = left; strafeRight_ = right; strafeSpeed_ = speed;
		if (strafePosX_ == 0.0f) strafePosX_ = left;
	}
	void SetPlayerRef(const Vector3* playerPos) { playerPos_ = playerPos; } // 追尾用（参照だけ）

	// 将来の発射フック（今は未使用）
	void SetCanShoot(bool v, float interval) { canShoot_ = v; shootInterval_ = interval; }

	void SetPlayer(std::function<Vector3()> getter) { playerGetter_ = std::move(getter); }


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
};
