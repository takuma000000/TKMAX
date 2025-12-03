#pragma once

#include <memory>
#include <vector>

#include "application/enemy/Enemy.h"          // 敵そのもの
#include "application/enemy/EnemySpawner.h"   // 敵スポーンユーティリティ
#include "application/player/Player.h"        // プレイヤー
#include "engine/3d/camera/Camera.h"
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

	// --- Wave 管理周りを追加 ---
	enum class WavePhase { W1, W2, W3, Done };

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

	/// <summary>
	/// Wave 初期化（GameScene::InitializeWaves 相当）
	/// </summary>
	void InitializeWaves();

	/// <summary>
	/// 現在の wavePhase_ に応じて敵をスポーンします（GameScene::SpawnCurrentWave 相当）
	/// </summary>
	void SpawnCurrentWave();

	/// <summary>
	/// wavePhase_ を進めます（GameScene::GoToNextWave 相当）
	/// </summary>
	void GoToNextWave();

	/// <summary>
	/// 生存している敵が存在するかどうかを判定して返します。
	/// </summary>
	/// <returns>生存している敵が1体以上いる場合は true、そうでない場合は false を返します。</returns>
	bool HasAliveEnemies() const { return enemies_ && !enemies_->empty(); }

	/// <summary>
	/// 全Waveクリア済みかどうかを取得します
	/// </summary>
	/// <returns></returns>
	bool IsAllWavesCleared() const { return (wavePhase_ == WavePhase::Done) && (!enemies_ || enemies_->empty()); }

	/// <summary>
	/// 全Waveクリア済みかどうかを取得します
	/// </summary>
	/// <returns></returns>
	bool IsWaveDone() const { return wavePhase_ == WavePhase::Done; }

	/// <summary>
	/// デバッグ用：即座にボスWave（Done）へスキップします
	/// </summary>
	void SkipToBossWave();

	// Getter==========================================================================
	/// <summary>
	/// 敵リストを取得します
	/// </summary>
	/// <returns></returns>
	const std::vector<std::unique_ptr<Enemy>>& GetEnemies() const { return *enemies_; }
	/// <summary>
	/// 現在の WavePhase を取得します
	/// </summary>
	/// <returns></returns>
	WavePhase GetWavePhase() const { return wavePhase_; }
	// ================================================================================
	// Setter==========================================================================
	/// <summary>
	/// カメラを設定します
	/// </summary>
	/// <param name="camera"></param>
	void SetCamera(Camera* camera) {
		cam_ = camera;
		for (auto& e : *enemies_) { // 敵全員にカメラをセット
			if (e) e->SetCamera(cam_); // 敵にもカメラをセット
		}
	}
	// ================================================================================

private:
	DirectXCommon* dx_ = nullptr;
	Camera* cam_ = nullptr;
	BaseScene* parent_ = nullptr;
	Player* player_ = nullptr;

	// GameScene 側の実体を「参照」するだけ
	std::vector<std::unique_ptr<Enemy>>* enemies_ = nullptr;
	int* defeatedEnemyCount_ = nullptr;
	int* maxEnemyCount_ = nullptr;

	// Wave 状態は EnemyManager が持つようにする
	WavePhase wavePhase_ = WavePhase::W1;
};