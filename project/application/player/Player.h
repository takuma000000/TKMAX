#pragma once

#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "ModelManager.h"
#include "PlayerBullet.h"
#include "Input.h"
#include "application/enemy/Enemy.h"
#include "externals/imgui/imgui.h"
#include <algorithm>
#include <list>
#include <engine/effect/particle/ParticlerEmitter.h>

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

	/// <summary>弾のリストを取得します。</summary>
	const std::list<std::unique_ptr<PlayerBullet>>& GetBullets() const {
		return bullets_;
	}

	/// <summary>プレイヤーの位置を取得します。</summary>
	Vector3 GetPosition() const {
		return object_ ? object_->GetTranslate() : Vector3();
	}

	/// <summary>ジェット噴射の有効/無効を設定します。</summary>
	/// <param name="enable">有効にする場合はtrue、無効にする場合はfalse。</param>
	void SetEnableJetSmoke(bool enable) { enableJetSmoke_ = enable; }

	/// <summary>カメラを設定します。</summary>
	void SetCamera(Camera* camera) 
	{
		this->camera = camera;
		if (object_) {
			object_->SetCamera(camera);
		}
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

private:

	/// <summary>ゲームパッドの入力に基づいてプレイヤーを移動させます。</summary>
	void HandleGamePadMove();
	/// <summary>カメラ制御を処理します。</summary>
	void HandleCameraControl();
	/// <summary>追従カメラを処理します。</summary>
	void HandleFollowCamera();
	/// <summary>射撃処理を行います。</summary>
	void HandleShooting();

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

	Vector3 cameraShakeOffset_ = { 0, 0, 0 };
	int cameraShakeFrame_ = 0;

	bool canUseSpecial_ = false; // 一撃必殺が使用可能かどうか

	float bankAngle_ = 0.0f;      // 現在の傾き（ロール）
	float bankVel_ = 0.0f;      // 補間用
	Vector3 moveMin_ = { -20.0f, -3.0f, 0.0f }; // 移動範囲（Zは固定）
	Vector3 moveMax_ = { 20.0f,  8.0f, 0.0f };

	bool ltHeld_ = false; // LTの押下状態ラッチ

	ParticleEmitter jetEmitter_;

	bool debugUnlimitedSpecial_ = false; // ImGuiでONならRTを無制限発射

	bool enableJetSmoke_ = true; // デフォルトON
};
