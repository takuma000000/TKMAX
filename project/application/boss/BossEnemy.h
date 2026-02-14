#pragma once
#include "Enemy.h"

namespace BossParam {
	constexpr int   InitHP_ = 1000; // 初期HP
	constexpr float InitScale_ = 5.0f; // 初期スケール
	constexpr Vector3 InitColliderScale_ = { 12.180f, 26.210f, 11.560f }; // 当たり判定スケール
	constexpr float NormalScale_ = 5.0f; // 通常スケール
	constexpr float LockBlinkSpeed_ = 0.2f; // 点滅速度
	constexpr float LockBlinkAmount_ = 0.2f; // 点滅幅
}

//=============================================================
// BossEnemy
// ・座標（移動）は BossController が決める
// ・BossEnemy は Enemy としての共通更新だけ（死亡/描画/当たり判定など）
//=============================================================
class BossEnemy : public Enemy {
public:
	BossEnemy() = default;
	~BossEnemy() = default;

	/// <summary>
	/// 敵オブジェクトの初期化を行います。
	/// </summary>
	/// <param name="common">Object3d の共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// 毎フレームの更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void Update(float dt);
	/// <summary>
	/// ImGuiデバッグ表示。
	/// </summary>
	void ImGuiDebug();

	// Setter===================================
	/// <summary>
	/// 触手のモデルとローカル座標を設定します。
	/// </summary>
	/// <param name="model">モデルファイル名</param>
	/// <param name="localPos">ローカル位置</param>
	void SetTentacleCharge(bool active, float charge01) {
		tentacleChargeActive_ = active;
		tentacleCharge01_ = charge01;
	}
	// =========================================

private:
	// 触手関連
	bool  tentacleChargeActive_ = false;
	float tentacleCharge01_ = 0.0f;
	float tentacleWiggleT_ = 0.0f;
	// ベース（チャージしてない時のローカル）
	Vector3 tentacleBasePos_{ 0.0f, 0.0f, 0.0f };
	Vector3 tentacleBaseRot_{ 0.0f, 0.0f, 0.0f };
	Vector3 tentacleBaseScale_{ 1.0f, 1.0f, 1.0f };
};
