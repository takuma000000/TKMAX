#pragma once
#include <memory>
#include <array>

#include "Object3d.h"
#include "Input.h"

namespace TKM {
	class Camera;
	class DirectXCommon;
	class Object3dCommon;
}

//=============================================================
// PlayerDodgeクラス
// プレイヤーの回避行動を管理するクラス。
// 回避移動、残像、クールタイムなどを管理する。
//=============================================================
class PlayerDodge {
public:

	//=============================================================
	// 回避残像構造体
	//=============================================================
	struct Ghost {
		std::unique_ptr<TKM::Object3d> body_;    // 残像本体
		std::unique_ptr<TKM::Object3d> flipper_; // 残像ヒレ

		Vector3 pos_ = { 0.0f, 0.0f, 0.0f };     // 残像位置
		Vector3 rot_ = { 0.0f, 0.0f, 0.0f };     // 残像回転
		Vector3 scale_ = { 1.0f, 1.0f, 1.0f };   // 残像拡縮

		float age_ = 0.0f;                       // 経過時間
		float life_ = 0.0f;                      // 寿命
							                     
		bool active_ = false;                    // 使用中か
	};

	/// <summary>
	/// 回避システムを初期化します。
	/// </summary>
	/// <param name="common">Object3d共通</param>
	/// <param name="dxCommon">DirectX共通</param>
	/// <param name="camera">描画用カメラ</param>
	void Initialize(
		TKM::Object3dCommon* common,
		TKM::DirectXCommon* dxCommon,
		TKM::Camera* camera
	);
	/// <summary>
	/// 回避システムを更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間</param>
	/// <param name="ownerObject">プレイヤー本体</param>
	/// <param name="controlEnabled">操作可能か</param>
	/// <param name="moveMin">移動可能範囲の最小値</param>
	/// <param name="moveMax">移動可能範囲の最大値</param>
	/// <param name="bankAngle">現在のバンク角</param>
	void Update(
		float dt,
		TKM::Object3d* ownerObject,
		bool controlEnabled,
		const Vector3& moveMin,
		const Vector3& moveMax,
		float bankAngle
	);
	/// <summary>
	/// 回避残像を描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX共通</param>
	void Draw(TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// 使用するカメラを設定します。
	/// </summary>
	/// <param name="camera">描画用カメラ</param>
	void SetCamera(TKM::Camera* camera);
	/// <summary>
	/// 回避中かどうか。
	/// </summary>
	/// <returns></returns>
	bool IsDodging() const { return isDodging_; }
	/// <summary>
	/// 回避クールタイム中かどうか。
	/// </summary>
	bool IsCooldown() const { return dodgeCooldownTimer_ > 0.0f; }
	/// <summary>
	/// 回避ゲージを表示するかどうか。
	/// 回避中、またはクールタイム中なら true。
	/// </summary>
	bool IsCooldownGaugeVisible() const { return isDodging_ || dodgeCooldownTimer_ > 0.0f; }

	// Getter===========================================
	/// <summary>
	/// 回避再使用までの進行率を取得します。
	/// 0.0f が空、1.0f が満タン。
	/// </summary>
	float GetCooldownGaugeRate() const;
	// =================================================

private:

	/// <summary>
	/// 回避入力・回避移動を更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間</param>
	/// <param name="ownerObject">プレイヤー本体</param>
	/// <param name="moveMin">移動可能範囲の最小値</param>
	/// <param name="moveMax">移動可能範囲の最大値</param>
	/// <param name="bankAngle">現在のバンク角</param>
	void UpdateDodge_(
		float dt,
		TKM::Object3d* ownerObject,
		const Vector3& moveMin,
		const Vector3& moveMax,
		float bankAngle
	);
	/// <summary>
	/// 回避を開始します。
	/// </summary>
	/// <param name="ownerObject">プレイヤー本体</param>
	void StartDodge_(TKM::Object3d* ownerObject);
	/// <summary>
	/// 回避残像を更新します。
	/// </summary>
	/// <param name="dt">デルタタイム</param>
	/// <param name="ownerObject">プレイヤー本体</param>
	void UpdateGhost_(
		float dt,
		TKM::Object3d* ownerObject
	);
	/// <summary>
	/// 現在位置に回避残像を追加します。
	/// </summary>
	/// <param name="ownerObject">プレイヤー本体</param>
	void AddGhost_(TKM::Object3d* ownerObject);


	//=============================================================
	// 外部参照
	//=============================================================

	TKM::Object3dCommon* common_ = nullptr;
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::Camera* camera_ = nullptr;

	//=============================================================
	// 回避調整値
	//=============================================================

	static constexpr int kGhostMax_ = 4; // 最大残像数

	static constexpr float kGhostLife_ = 0.22f; // 残像寿命
	static constexpr float kGhostInterval_ = 0.035f; // 残像生成間隔

	static constexpr float kDodgeDuration_ = 0.35f;     // 回避移動時間
	static constexpr float kDodgeDistance_ = 14.0f;     // 回避距離
	static constexpr float kDodgeSpinDuration_ = 0.42f; // 回避回転時間
	static constexpr float kDodgeSpinTurns_ = 1.0f;     // 回転数

	float dodgeSpinRollSign_ = 1.0f;  // Z回転の向き
	float dodgeSpinPitchSign_ = 1.0f; // X回転の向き
	float dodgeSpinWRoll_ = 0.0f;     // ロール比率
	float dodgeSpinWPitch_ = 0.0f;    // ピッチ比率

	static constexpr float kDodgeTime_ = 0.18f;    // 回避時間
	static constexpr float kDodgeCooldown_ = 1.0f; // 回避クールタイム

	//=============================================================
	// 回避状態
	//=============================================================

	bool isDodging_ = false; // 回避中か

	float dodgeTimer_ = 0.0f; // 回避経過時間
	float dodgeCooldownTimer_ = 0.0f; // クールタイマー

	Vector3 dodgeDirection_ = { 0.0f, 0.0f, 0.0f }; // 回避方向
	Vector3 dodgeStartPos_ = { 0.0f, 0.0f, 0.0f }; // 回避開始位置
	Vector3 dodgeBaseRot_ = { 0.0f, 0.0f, 0.0f };  // 回避開始時の回転

	//=============================================================
	// 回避残像
	//=============================================================

	std::array<Ghost, kGhostMax_> ghosts_; // 残像配列

	float ghostSpawnTimer_ = 0.0f; // 残像生成タイマー

	int ghostWriteIndex_ = 0; // 次回書き込み位置
};