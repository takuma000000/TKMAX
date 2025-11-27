#pragma once

#include <memory>
#include <vector>

#include "application/enemy/Enemy.h"          // 敵そのもの
#include "application/enemy/EnemySpawner.h"   // 敵スポーンユーティリティ
#include "application/player/Player.h"        // プレイヤー
#include "Camera.h"
#include "DirectXCommon.h"
#include "BaseScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

/// <summary>
/// ザコ敵全体を管理するクラス。
/// まずは入れ物。あとから機能を移植していく。
/// </summary>
class EnemyManager {
public:
	EnemyManager() = default;
	~EnemyManager() = default;

	/// <summary>
	/// 敵マネージャを初期化します
	/// </summary>
	/// <param name="dx"></param>
	/// <param name="camera"></param>
	/// <param name="parent"></param>
	/// <param name="player"></param>
	void Initialize(DirectXCommon* dx, Camera* camera, BaseScene* parent, Player* player);

	/// <summary>
	/// 敵全体の更新
	/// </summary>
	/// <param name="dt"></param>
	void Update(float dt);

	/// <summary>
	/// 敵全体の描画
	/// </summary>
	/// <param name="dx"></param>
	void Draw(DirectXCommon* dx);

	/// <summary>
	/// デバッグ用ImGui表示
	/// </summary>
	void ImGuiDebug();

	/// <summary>
	/// 敵リストをバインドします
	/// </summary>
	/// <param name="enemies"></param>
	/// <param name="defeatedEnemyCount"></param>
	/// <param name="maxEnemyCount"></param>
	void BindEnemies(std::vector<std::unique_ptr<Enemy>>* enemies,
		int* defeatedEnemyCount,
		int* maxEnemyCount);

	/// <summary>
	/// プレイヤーに最も近い敵を更新します
	/// </summary>
	void UpdateClosestEnemy();

	/// Getter=====================================================================================
	// 管理している敵リスト（ロックオン用など）
	const std::vector<std::unique_ptr<Enemy>>& GetEnemies() const { return *enemies_; }
	/// ===========================================================================================

private:
	DirectXCommon* dx_ = nullptr;
	Camera* cam_ = nullptr;
	BaseScene* parent_ = nullptr;
	Player* player_ = nullptr;

	// GameScene 側の実体を「参照」するだけ
	std::vector<std::unique_ptr<Enemy>>* enemies_ = nullptr;
	int* defeatedEnemyCount_ = nullptr;
	int* maxEnemyCount_ = nullptr;
};