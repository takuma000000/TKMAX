#include "EnemySpawner.h"
#include "Enemy.h"
#include "Object3dCommon.h"
#include "camera/Camera.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include <ParticleManager.h>

namespace EnemySpawner {

	// ------------------------------------------------------------
	// 共通：1 体分の敵を生成して push_back するヘルパー
	// ------------------------------------------------------------
	static void CreateAndPushEnemy(
		std::vector<std::unique_ptr<Enemy>>& enemies,
		const Vector3& spawnPos,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent,
		const EnemyConfig& config,
		bool useSpawnEffect
	) {
		auto e = std::make_unique<Enemy>();
		e->Initialize(TKM::Object3dCommon::GetInstance(), dx);
		e->SetPosition(spawnPos);
		if (parent) { e->SetParentScene(parent); }
		if (cam) { e->SetCamera(cam); }
		if (config) { config(*e); }
		e->SyncTransform(); // Transform情報をObject3dに同期

		if (useSpawnEffect) {
			Vector3 emitPos = spawnPos;
			TKM::ParticleManager::GetInstance()->Emit("enemySpawn", emitPos, kSpawnParticleCount);
		}

		enemies.push_back(std::move(e));
	}

	// "1直線状" に敵をスポーン
	void SpawnLine(std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float y, float z,
		float xStart, float xStep,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent, EnemyConfig config) {

		for (int i = 0; i < count; ++i) {
			Vector3 spawnPos = { xStart + xStep * i, y, z };
			CreateAndPushEnemy(enemies, spawnPos, dx, cam, parent, config, true);
		}
	}

	// "V字型" に敵をスポーン
	void SpawnV(std::vector<std::unique_ptr<Enemy>>& enemies,
		int countPerSide, float y, float z,
		float xCenter, float xStep, float zStep,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent, EnemyConfig config) {

		// 中央
			{
				Vector3 spawnPos = { xCenter, y, z };
				CreateAndPushEnemy(enemies, spawnPos, dx, cam, parent, config, true);
			}

			// 左右展開
			for (int i = 1; i <= countPerSide; ++i) {
				for (int side = -1; side <= 1; side += 2) {
					Vector3 spawnPos = {
						xCenter + side * xStep * i,
						y,
						z + zStep * i
					};
					CreateAndPushEnemy(enemies, spawnPos, dx, cam, parent, config, true);
				}
			}
	}

	// "1列柱状" に敵をスポーン
	void SpawnColumn(std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float x, float zStart, float zStep,
		float yStart, float yStep,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent, EnemyConfig config) {

		for (int i = 0; i < count; ++i) {
			Vector3 spawnPos = { x, yStart + yStep * i, zStart + zStep * i };
			CreateAndPushEnemy(enemies, spawnPos, dx, cam, parent, config, true);
		}
	}

	// "正三角形配置" に敵をスポーン
	void SpawnTriangle3(
		std::vector<std::unique_ptr<Enemy>>& enemies,
		float centerX, float centerY, float z,
		float size,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent,
		EnemyConfig config) {

		// 上（先頭）
			{
				Vector3 spawnPos = { centerX, centerY + size, z };
				// もともとここは Spawn エフェクトを出してなかったので false
				CreateAndPushEnemy(enemies, spawnPos, dx, cam, parent, config, false);
			}

			// 下左右
			for (int side = -1; side <= 1; side += 2) {
				Vector3 spawnPos = { centerX + side * size, centerY - size, z };
				// こちらも元コードに合わせてエフェクト無し
				CreateAndPushEnemy(enemies, spawnPos, dx, cam, parent, config, false);
			}
	}
}