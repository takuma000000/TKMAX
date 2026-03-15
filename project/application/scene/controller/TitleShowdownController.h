#pragma once
#include <memory>
#include <random>

#include "Player.h"
#include "BossEnemy.h"
#include "DirectXCommon.h"
#include "srvManager.h"
#include "Camera.h"

class TitleScene;

//=============================================================
// TitleShowdownControllerクラス
// タイトル中の見つめ合い演出とビーム打ち合い演出を管理するクラス。
//=============================================================
class TitleShowdownController {
public:
	/// <summary>
	/// 見つめ合い演出を初期化します。
	/// </summary>
	/// <param name="ownerScene">所有シーン</param>
	/// <param name="dxCommon">DirectX共通</param>
	/// <param name="srvManager">SrvManager</param>
	/// <param name="camera">使用カメラ</param>
	void Initialize(
		TitleScene* ownerScene,
		TKM::DirectXCommon* dxCommon,
		TKM::SrvManager* srvManager,
		TKM::Camera* camera
	);
	/// <summary>
	/// 見つめ合い演出を更新します。
	/// </summary>
	/// <param name="dt">デルタタイム</param>
	/// <param name="enableBeam">ビーム演出を有効にするか</param>
	void Update(float dt, bool enableBeam);
	/// <summary>
	/// 見つめ合い演出を描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX共通</param>
	void Draw(TKM::DirectXCommon* dxCommon);

	// Setter==================================
	/// <summary>
	/// ビーム演出の有効状態を設定します。
	/// </summary>
	/// <param name="active">有効状態</param>
	void SetBeamActive(bool active);
	/// <summary>
	/// プレイヤー位置を設定します。
	/// </summary>
	void SetPlayerPos(const Vector3& pos) { playerPos_ = pos; }
	/// <summary>
	/// ボス位置を設定します。
	/// </summary>
	void SetBossPos(const Vector3& pos) { bossPos_ = pos; }
	/// <summary>
	/// 自動見つめ合いの有効状態を設定します。
	/// </summary>
	void SetAutoLookAt(bool enable) { autoLookAt_ = enable; }
	// ========================================
	// Getter==================================
	/// <summary>
	/// プレイヤー位置を取得します。
	/// </summary>
	const Vector3& GetPlayerPos() const { return playerPos_; }
	/// <summary>
	/// ボス位置を取得します。
	/// </summary>
	const Vector3& GetBossPos() const { return bossPos_; }
	/// <summary>
	/// 自動見つめ合いの有効状態を取得します。
	/// </summary>
	bool GetAutoLookAt() const { return autoLookAt_; }
	/// <summary>
	/// プレイヤー回転オフセット（度）を取得します。
	/// </summary>
	Vector3& GetPlayerRotDeg() { return playerRotDeg_; }
	/// <summary>
	/// ボス回転オフセット（度）を取得します。
	/// </summary>
	Vector3& GetBossRotDeg() { return bossRotDeg_; }
	// ========================================

private:
	/// <summary>
	/// 見つめ合い用アクターを生成します。
	/// </summary>
	void CreateActors_();
	/// <summary>
	/// ビーム押し合い位置を更新します。
	/// </summary>
	/// <param name="dt">デルタタイム</param>
	void UpdateBeamPush_(float dt);
	/// <summary>
	/// ビームクラッシュ演出を更新します。
	/// </summary>
	/// <param name="dt">デルタタイム</param>
	void UpdateBeamClash_(float dt);
	/// <summary>
	/// 次のビーム目標位置を再設定します。
	/// </summary>
	void ResetBeamTarget_();
	/// <summary>
	/// from から to へのYawを求めます。
	/// </summary>
	float LookAtYaw_(const Vector3& from, const Vector3& to) const;

	//======================================================================
	// 定数
	//======================================================================
	static constexpr float kTitleBeamCenterT_ = 0.30f;            // ビーム衝突位置の基準
	static constexpr float kTitleBeamMinT_ = 0.25f;               // 衝突位置の最小（プレイヤー側）
	static constexpr float kTitleBeamMaxT_ = 0.58f;               // 衝突位置の最大（ボス側）
	static constexpr float kTitleBeamTargetChangeMinSec_ = 1.10f; // 押し合い方向変更の最短時間
	static constexpr float kTitleBeamTargetChangeMaxSec_ = 1.85f; // 押し合い方向変更の最長時間
	static constexpr float kTitleBeamApproachSpeed_ = 1.90f;      // 衝突位置の補間速度
	static constexpr float kTitleBeamMicroOscAmp_ = 0.012f;       // 衝突位置の細かい揺れ振幅
	static constexpr float kTitleBeamMicroOscSpeed_ = 4.00f;      // 揺れのスピード
	static constexpr float kTitleBeamNeutralReturnSpeed_ = 1.60f; // ビーム停止時の中央復帰速度
	static constexpr float kTitleBeamPlayerStartOffsetY_ = 1.20f; // プレイヤービーム発射Yオフセット
	static constexpr float kTitleBeamBossStartOffsetY_ = 6.20f;   // ボスビーム発射Yオフセット
	static constexpr float kTitleBeamClashHz_ = 30.0f;            // クラッシュパーティクル発生頻度
	static constexpr int kTitleBeamSegments_ = 18;                // ビームの分割数
	static constexpr int kTitleBeamPerSegment_ = 1;               // 各分割のパーティクル数
	static constexpr int kTitleClashCoreCount_ = 8;               // 衝突中心の火花数
	static constexpr int kTitleClashRaysCount_ = 10;              // 放射スパーク数
	static constexpr int kTitleClashRingCount_ = 1;               // 衝突リング数
	//======================================================================
	// 参照
	//======================================================================
	TitleScene* ownerScene_ = nullptr;       // 所有シーン
	TKM::DirectXCommon* dxCommon_ = nullptr; // DirectX共通
	TKM::SrvManager* srvManager_ = nullptr;  // SRV管理
	TKM::Camera* camera_ = nullptr;          // 使用カメラ
	//======================================================================
	// アクター
	//======================================================================
	std::unique_ptr<Player> player_ = nullptr;  // タイトル用プレイヤー
	std::unique_ptr<BossEnemy> boss_ = nullptr; // タイトル用ボス
	//======================================================================
	// 状態
	//======================================================================
	Vector3 playerPos_ = { -12.0f, -3.8f, 13.3f }; // プレイヤー位置
	Vector3 bossPos_ = { 24.7f, 6.7f, 53.3f };     // ボス位置
	Vector3 playerRot_ = { 0.0f, 0.0f, 0.0f };     // プレイヤー回転(rad)
	Vector3 bossRot_ = { 0.0f, 0.0f, 0.0f };       // ボス回転(rad)
	Vector3 playerRotDeg_ = { 0.0f, 0.0f, 0.0f };  // プレイヤー追加回転(deg)
	Vector3 bossRotDeg_ = { 0.0f, 0.0f, 0.0f };    // ボス追加回転(deg)
	bool autoLookAt_ = true;                       // 自動で互いを見る
	bool beamActive_ = true;                       // ビーム演出有効
	float clashEmitAcc_ = 0.0f;                    // クラッシュ発生タイマー
	float beamT_ = kTitleBeamCenterT_;             // 現在の衝突位置
	float beamTargetT_ = kTitleBeamCenterT_;       // 目標衝突位置
	float beamTargetTimer_ = 0.0f;                 // 目標変更タイマー
	float beamMicroOscTime_ = 0.0f;                // 微振動用時間
	std::mt19937 rng_{ std::random_device{}() };   // 乱数生成器
};