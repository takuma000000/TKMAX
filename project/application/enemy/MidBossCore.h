#pragma once
#include <memory>
#include <functional>
#include "Object3d.h"
#include "camera/Camera.h"
#include "BaseScene.h"
#include "ParticleManager.h"
#include "LineRenderer.h"
#include "AABB.h"
#include "MyMath.h"
#include "reticle/Reticle.h"

class MidBossCore {
public:
	MidBossCore() = default;
	~MidBossCore() = default;

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
	/// デバッグ用ImGui表示。
	/// </summary>
	void ImGuiDebug();
	/// <summary>
	/// 敵が死亡したかどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsDead() const { return isDead_; }
	/// <summary>
	/// 敵が死亡演出中かどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsDying() const { return isDying_; }

	/// <summary>
	/// 特殊攻撃によってダメージを受けたときの処理を行います。
	/// </summary>
	/// <param name="damage">受けるダメージ量</param>
	void OnHitWithDamage(int damage);
	/// <summary>
	/// 敵の死亡リアクションを開始します。
	/// </summary>
	/// <param name="hitDir">被弾方向（正規化ベクトル）</param>
	void StartDeathReaction(const Vector3& hitDir);
	/// <summary>
	/// モデルの変換情報を同期します。
	/// </summary>
	void SyncTransform();

	// Getter==================================
	/// <summary>
	/// 現在のHPを取得します。
	/// </summary>
	/// <returns></returns>
	int  GetHP() const { return hp_; }
	/// <summary>
	/// 最大HPを取得します。
	/// </summary>
	/// <returns></returns>
	Vector3 GetWorldPosition() const;
	/// <summary>
	/// スケールを取得します。
	/// </summary>
	/// <returns></returns>
	Vector3 GetScale() const { return baseScale_; }
	/// <summary>
	/// 当たり判定用スケールを取得します。
	/// </summary>
	/// <returns></returns>
	Vector3 GetColliderScale() const { return colliderScale_; }
	// ========================================
	// Setter==================================
	/// <summary>
	/// HPを設定します（最大HPも更新）。
	/// </summary>
	/// <param name="hp"></param>
	void SetHP(int hp) { hp_ = hp; maxHP_ = hp; }
	/// <summary>
	/// モデルを設定します。
	/// </summary>
	/// <param name="pos"></param>
	void SetPosition(const Vector3& pos);
	/// <summary>
	/// スケールを設定します（当たり判定用スケールも更新）。
	/// </summary>
	/// <param name="s"></param>
	void SetScale(const Vector3& s);
	/// <summary>
	/// カメラ設定
	/// </summary>
	/// <param name="cam"></param>
	void SetCamera(TKM::Camera* cam);
	/// <summary>
	/// 親シーンを設定します。
	/// </summary>
	/// <param name="scene"></param>
	void SetParentScene(TKM::BaseScene* scene) { parent_ = scene; }
	/// <summary>
	/// 当たり判定用スケールを設定します。
	/// </summary>
	/// <param name="s"></param>
	void SetColliderScale(const Vector3& s) { colliderScale_ = s; }
	/// <summary>
	/// レティクルを設定します。
	/// </summary>
	/// <param name="r"></param>
	void SetReticle(Reticle* r) { reticle_ = r; }
	/// <summary>
	/// プレイヤー位置取得関数を設定します。
	/// </summary>
	/// <param name="getter"></param>
	void SetPlayer(std::function<Vector3()> getter) { playerGetter_ = std::move(getter); }
	// ========================================
private:
	std::unique_ptr<TKM::Object3d> object_;
	TKM::Camera* camera_ = nullptr;
	TKM::BaseScene* parent_ = nullptr;
	Reticle* reticle_ = nullptr;
	std::function<Vector3()> playerGetter_;

	int hp_ = 3; // 現在のHP
	int maxHP_ = 3; // 最大HP
	bool isDead_ = false; // 完全に死亡したかどうか
	bool isDying_ = false; // 死亡演出中かどうか

	Vector3 baseScale_{ 0.8f, 0.8f, 0.8f }; // 基本スケール
	Vector3 colliderScale_{ 1.71f, 1.71f, 1.71f }; // 当たり判定用スケール

	// 死亡演出用
	float deathTimer_ = 0.0f; // 経過時間
	float deathDuration_ = 1.0f; // 演出の長さ（秒相当）
	Vector3 deathVelocity_{ 0.0f, 0.0f, 0.0f };
	Vector3 deathRotateSpeed_{ 0.0f, 0.0f, 0.0f }; // 回転速度
	float deathAlpha_ = 1.0f;

	const float fixedDt_ = 1.0f / 60.0f;
};