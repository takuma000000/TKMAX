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
	//=============================================================
	// 敵弾の種類
	//=============================================================
	enum class Type {
		Normal,              // 通常弾
		SpecialCoreCharging, // 特殊攻撃コアのチャージ中
		SpecialCoreLaunched, // 特殊攻撃コアの発射後
	};

	EnemyBullet() = default;
	~EnemyBullet() = default;

	/// <summary>
	/// 敵弾を初期化します。
	/// </summary>
	/// <param name="common">Object3d の共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	/// <param name="camera">描画および判定に使用するカメラ</param>
	/// <param name="position">初期位置</param>
	/// <param name="velocity">初速度</param>
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
	/// <param name="dt">経過時間（秒）</param>
	void Update(float dt);

	/// <summary>
	/// 敵弾を描画します。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dx);

	/// <summary>
	/// SP攻撃用のチャージ玉を初期化します。
	/// 初期状態ではチャージ中で、スケールが徐々に大きくなります。
	/// 一定時間後に発射状態へ切り替える前提で使用します。
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
	void InitializeSpecialCore(
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
	/// <returns>true なら死亡済み、false なら生存中</returns>
	bool IsDead() const { return isDead_; }

	/// <summary>
	/// 敵弾を死亡状態にします。
	/// </summary>
	void Kill() { isDead_ = true; }

	/// <summary>
	/// SP用のチャージ玉を発射状態に切り替えます。
	/// これ以降は通常弾と同様に移動し、一定時間後に消滅します。
	/// </summary>
	/// <param name="velocity">発射時の速度ベクトル</param>
	void LaunchSpecialCore(const Vector3& velocity);

	/// <summary>
	/// この敵弾がSP用のチャージ玉（充電中または発射後）かどうかを返します。
	/// </summary>
	/// <returns>true ならチャージ玉、false なら通常弾</returns>
	bool IsSpecialCore() const {
		return type_ == Type::SpecialCoreCharging || type_ == Type::SpecialCoreLaunched;
	}

	// Getter===================================
	/// <summary>
	/// 敵弾のワールド座標を取得します。
	/// </summary>
	/// <returns>敵弾のワールド座標</returns>
	Vector3 GetWorldPosition() const;

	/// <summary>
	/// 当たり判定半径を取得します。
	/// </summary>
	/// <returns>当たり判定半径</returns>
	float GetRadius() const { return radius_; }

	/// <summary>
	/// 敵弾の種類を取得します。
	/// </summary>
	/// <returns>敵弾の種類</returns>
	Type GetType() const { return type_; }

	/// <summary>
	/// 敵弾のダメージ量を取得します。
	/// </summary>
	/// <returns>ダメージ量</returns>
	int GetDamage() const { return damage_; }
	// =========================================

	// Setter===================================
	/// <summary>
	/// 敵弾の位置を設定します。
	/// </summary>
	/// <param name="position">新しい位置</param>
	void SetPosition(const Vector3& position);

	/// <summary>
	/// 敵弾のスケールを一様に設定します。
	/// </summary>
	/// <param name="uniformScale">新しい一様スケール</param>
	void SetScale(float uniformScale);

	/// <summary>
	/// 敵弾の色を設定します。
	/// </summary>
	/// <param name="color">新しい色（RGBA）</param>
	void SetColor(const Vector4& color);

	/// <summary>
	/// 敵弾の可視状態を設定します。
	/// </summary>
	/// <param name="visible">true で表示、false で非表示</param>
	void SetVisible(bool visible);
	// =========================================

private:

	//======================================================================
	// 描画 / 外部参照
	//======================================================================
	std::unique_ptr<TKM::Object3d> object_ = nullptr; // 敵弾の描画オブジェクト
	TKM::Camera* camera_ = nullptr;                   // 描画および判定に使用するカメラ

	//======================================================================
	// 移動 / 状態
	//======================================================================
	Vector3 velocity_ = { 0.0f, 0.0f, 0.0f }; // 敵弾の速度
	Type type_ = Type::Normal;                // 敵弾の種類

	//======================================================================
	// SPコア用チャージ状態
	//======================================================================
	float scaleNow_ = 0.6f;       // 現在のスケール
	float scaleEnd_ = 0.6f;       // チャージ完了時のスケール
	float chargeTimer_ = 0.0f;    // チャージ経過時間
	float chargeDuration_ = 0.0f; // チャージ完了までの時間

	//======================================================================
	// 基本パラメータ
	//======================================================================
	int damage_ = 1;         // 敵弾のダメージ量
	bool visible_ = true;    // 敵弾の表示状態
	float radius_ = 0.8f;    // 当たり判定半径

	//======================================================================
	// 生存管理
	//======================================================================
	float lifeTimer_ = 0.0f; // 生存経過時間
	float lifeTime_ = 4.0f;  // 生存時間上限
	bool isDead_ = false;    // 死亡フラグ
};