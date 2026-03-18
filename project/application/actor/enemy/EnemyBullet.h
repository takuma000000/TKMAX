#pragma once
#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "MyMath.h"

//=============================================================
// EnemyBulletクラス
// 敵が発射する弾を管理するクラス。
//=============================================================
class EnemyBullet {
public:
	// 敵弾の種類
	enum class Type {
		Normal,                // 通常弾
		FormationCoreCharging, // 隊列SP用のチャージ玉
		FormationCoreLaunched, // 発射後のSP玉
	};

	EnemyBullet() = default;
	~EnemyBullet() = default;

	/// <summary>
	/// 敵弾を初期化します。
	/// </summary>
	void Initialize(
		TKM::Object3dCommon* common,
		TKM::DirectXCommon* dxCommon,
		TKM::Camera* camera,
		const Vector3& position,
		const Vector3& velocity
	);
	/// <summary>
	/// 敵弾を更新します。
	/// </summary>
	void Update(float dt);
	/// <summary>
	/// 敵弾を描画します。
	/// </summary>
	void Draw(TKM::DirectXCommon* dx);

	/// <summary>
	/// 隊列SP用のチャージ玉として敵弾を初期化します。
	/// </summary>
	/// <param name="common">Object3d の共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	/// <param name="camera">描画および判定に使用するカメラ</param>
	/// <param name="position">初期位置</param>
	/// <param name="startScale">チャージ開始時のスケール</param>
	/// <param name="endScale">チャージ完了時のスケール</param>
	/// <param name="radius">当たり判定半径</param>
	/// <param name="chargeDuration">チャージに必要な時間（秒）</param>
	/// <param name="damage">この弾のダメージ量</param>
	void InitializeFormationCore(
		TKM::Object3dCommon* common,
		TKM::DirectXCommon* dxCommon,
		TKM::Camera* camera,
		const Vector3& position,
		float startScale,
		float endScale,
		float radius,
		float chargeDuration,
		int damage
	);
	/// <summary>
	/// 敵弾が死亡しているかを返します。
	/// </summary>
	bool IsDead() const { return isDead_; }
	/// <summary>
	/// 敵弾を死亡状態にします。
	/// </summary>
	void Kill() { isDead_ = true; }
	/// <summary>
	/// 隊列SP用のチャージ玉を発射状態にします。
	/// </summary>
	/// <param name="velocity">発射時の速度ベクトル</param>
	void LaunchFormationCore(const Vector3& velocity);

	/// <summary>
	/// この敵弾が隊列SP用のチャージ玉（充電中または発射後）かどうかを返します。
	/// </summary>
	/// <returns>true ならチャージ玉、false なら通常弾</returns>
	bool IsFormationCore() const {
		return type_ == Type::FormationCoreCharging || type_ == Type::FormationCoreLaunched;
	}

	// Getter===================================
	/// <summary>
	/// 敵弾のワールド座標を返します。
	/// </summary>
	Vector3 GetWorldPosition() const;
	/// <summary>
	/// 当たり判定半径を返します。
	/// </summary>
	float GetRadius() const { return radius_; }
	/// <summary>
	/// 敵弾の種類を返します。
	/// </summary>
	Type GetType() const { return type_; }
	/// <summary>
	/// 敵弾のダメージ量を返します（将来的に種類ごとに変えることも想定）。
	/// </summary>
	/// <returns>ダメージ量</returns>
	int GetDamage() const { return damage_; }
	// =========================================
	// Setter===================================
	/// <summary>
	/// 敵弾の位置を設定します。
	/// </summary>
	/// <param name="position">新しい位置ベクトル</param>
	void SetPosition(const Vector3& position);
	/// <summary>
	/// 敵弾のスケールを一様に設定します。
	/// </summary>
	/// <param name="uniformScale">新しいスケール値（例: 1.0f で等倍）</param>
	void SetScale(float uniformScale);
	/// <summary>
	/// 敵弾の色を設定します。
	/// </summary>
	/// <param name="color">新しい色ベクトル（RGBA）</param>
	void SetColor(const Vector4& color);
	/// <summary>
	/// 敵弾の可視状態を設定します。
	/// </summary>
	/// <param name="visible">true で表示、false で非表示</param>
	void SetVisible(bool visible);
	// =========================================

private:
	std::unique_ptr<TKM::Object3d> object_ = nullptr;
	TKM::Camera* camera_ = nullptr;

	Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };

	Type type_ = Type::Normal;

	float scaleNow_ = 0.6f;
	float scaleEnd_ = 0.6f;
	float chargeTimer_ = 0.0f;
	float chargeDuration_ = 0.0f;

	int damage_ = 1;
	bool visible_ = true;

	float radius_ = 0.8f;
	float lifeTimer_ = 0.0f;
	float lifeTime_ = 4.0f;

	bool isDead_ = false;
};