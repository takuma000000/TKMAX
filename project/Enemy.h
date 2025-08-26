#pragma once
#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "BaseScene.h"
#include "externals/imgui/imgui.h"

class Enemy {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	void Update();
	void Draw(DirectXCommon* dxCommon);
	void ImGuiDebug();

	void OnHit(); // 弾が当たったとき呼ぶ
	void OnHitWithDamage(int damage); // 特殊攻撃（ダメージ指定）
	bool IsDead() const { return isDead_; }

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
		baseScale_ = scale;
		if (object_) object_->SetScale(scale);
	}

	void SetCamera(Camera* camera);
	void SetPosition(const Vector3& pos);
	void SetParentScene(BaseScene* scene);
	void SetLocked(bool v) { isLocked_ = v; if (!v) pulseT_ = 0.0f; }
	Vector3 GetWorldPosition() const;
	Vector3 GetScale() const {
		return object_ ? object_->GetScale() : Vector3{ 1.0f, 1.0f, 1.0f };
	}

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
};
