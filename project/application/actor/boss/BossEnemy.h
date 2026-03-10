#pragma once
#include "Enemy.h"

namespace BossParam {
	constexpr int   InitHP_ = 1000; // 初期HP
	constexpr float InitScale_ = 5.0f; // 初期スケール
	constexpr Vector3 InitColliderScale_ = { 12.180f, 26.970f, 11.560f }; // 当たり判定スケール
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
	void SetTentacleCharge(bool active, float charge01);
	/// <summary>
	/// イントロ用の慌て触手演出を設定します。
	/// </summary>
	/// <param name="active">有効かどうか</param>
	/// <param name="panic01">強度（0..1）</param>
	void SetIntroPanic(bool active, float panic01);
	// =========================================
private:
	//=============================
	// 触手チャージ
	//=============================
	bool  tentacleChargeActive_ = false; // チャージ中かどうか
	float tentacleCharge01_ = 0.0f; // チャージ量（0..1）
	float tentacleWiggleT_ = 0.0f; // 触手のうねり時間（チャージ中だけ増える）
	// ベース（チャージしてない時のローカル）
	Vector3 tentacleBasePos_{ 0.0f, 0.0f, 0.0f }; // 触手のローカル位置
	Vector3 tentacleBaseRot_{ 0.0f, 0.0f, 0.0f }; // 触手のローカル回転
	Vector3 tentacleBaseScale_{ 1.0f, 1.0f, 1.0f }; // 触手のローカルスケール
	//=============================
	// イントロ用パニック触手
	//=============================
	bool  introPanicActive_ = false; // イントロ中の慌て演出
	float introPanic01_ = 0.0f;      // 慌て強度（0..1）
};
