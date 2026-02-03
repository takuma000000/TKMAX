#pragma once
#include "Enemy.h"

namespace BossParam {
	// ここはとりあえず “動くこと優先” の初期値
	constexpr int   InitHP_ = 1000;

	constexpr float InitScale_ = 5.0f;
	constexpr Vector3 InitColliderScale_ = { 12.180f, 18.210f, 11.560f };

	// ロック中の脈動（見た目だけ）
	constexpr float NormalScale_ = 5.0f;
	constexpr float LockBlinkSpeed_ = 0.2f;
	constexpr float LockBlinkAmount_ = 0.2f;
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
};
