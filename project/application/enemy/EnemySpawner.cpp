#include "application/enemy/EnemySpawner.h"
#include "Enemy.h"
#include "Object3dCommon.h"
#include "engine/3d/camera/Camera.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include <engine/effect/particle/ParticleManager.h>

namespace EnemySpawner {

	// "1直線状" に敵をスポーン
	void SpawnLine(std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float y, float z,
		float xStart, float xStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent, EnemyConfig config) {
		for (int i = 0; i < count; ++i) { // 敵の生成と初期化
			auto e = std::make_unique<Enemy>();
			e->Initialize(Object3dCommon::GetInstance(), dx);
			Vector3 spawnPos = { xStart + xStep * i, y, z };
			e->SetPosition(spawnPos);
			if (parent) e->SetParentScene(parent);
			if (cam)    e->SetCamera(cam);
			if (config) config(*e);
			e->SyncTransform(); // Transform情報をObject3dに同期

			Vector3 emitPos = spawnPos;
			ParticleManager::GetInstance()->Emit("enemySpawn", emitPos, kSpawnParticleCount);

			enemies.push_back(std::move(e));
		}
	}

	// "V字型" に敵をスポーン
	void SpawnV(std::vector<std::unique_ptr<Enemy>>& enemies,
		int countPerSide, float y, float z,
		float xCenter, float xStep, float zStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent, EnemyConfig config) {
		// 中央
			{ // 敵の生成と初期化
				auto e = std::make_unique<Enemy>();
				e->Initialize(Object3dCommon::GetInstance(), dx);
				Vector3 spawnPos = { xCenter, y, z };
				e->SetPosition(spawnPos);
				if (parent) e->SetParentScene(parent);
				if (cam)    e->SetCamera(cam);
				if (config) config(*e);
				e->SyncTransform(); // Transform情報をObject3dに同期

				Vector3 emitPos = spawnPos;
				ParticleManager::GetInstance()->Emit("enemySpawn", emitPos, kSpawnParticleCount);

				enemies.push_back(std::move(e));
			}
			// 左右展開
			for (int i = 1; i <= countPerSide; ++i) { // 敵の生成と初期化
				for (int side = -1; side <= 1; side += 2) {
					auto e = std::make_unique<Enemy>();
					e->Initialize(Object3dCommon::GetInstance(), dx);
					Vector3 spawnPos = { xCenter + side * xStep * i, y, z + zStep * i };
					e->SetPosition(spawnPos);
					if (parent) e->SetParentScene(parent);
					if (cam)    e->SetCamera(cam);
					if (config) config(*e);
					e->SyncTransform();

					Vector3 emitPos = spawnPos;
					ParticleManager::GetInstance()->Emit("enemySpawn", emitPos, kSpawnParticleCount);

					enemies.push_back(std::move(e));
				}
			}
	}

	// "1列柱状" に敵をスポーン
	void SpawnColumn(std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float x, float zStart, float zStep,
		float yStart, float yStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent, EnemyConfig config) {
		for (int i = 0; i < count; ++i) { // 敵の生成と初期化
			auto e = std::make_unique<Enemy>();
			e->Initialize(Object3dCommon::GetInstance(), dx);
			Vector3 spawnPos = { x, yStart + yStep * i, zStart + zStep * i };
			e->SetPosition(spawnPos);
			if (parent) e->SetParentScene(parent);
			if (cam)    e->SetCamera(cam);
			if (config) config(*e);
			e->SyncTransform();

			Vector3 emitPos = spawnPos;
			ParticleManager::GetInstance()->Emit("enemySpawn", emitPos, kSpawnParticleCount);

			enemies.push_back(std::move(e));
		}
	}

	// "正三角形配置" に敵をスポーン
	void SpawnTriangle3(
		std::vector<std::unique_ptr<Enemy>>& enemies,
		float centerX, float centerY, float z,
		float size,
		DirectXCommon* dx, Camera* cam, BaseScene* parent,
		EnemyConfig config) {
		// 上（先頭）
			{
				auto e = std::make_unique<Enemy>();
				e->Initialize(Object3dCommon::GetInstance(), dx);
				e->SetPosition({ centerX, centerY + size, z });
				if (parent) e->SetParentScene(parent);
				if (cam)    e->SetCamera(cam);
				if (config) config(*e);
				e->SyncTransform();
				enemies.push_back(std::move(e));
			}
			// 下左右
			for (int side = -1; side <= 1; side += 2) {
				auto e = std::make_unique<Enemy>();
				e->Initialize(Object3dCommon::GetInstance(), dx);
				e->SetPosition({ centerX + side * size, centerY - size, z });
				if (parent) e->SetParentScene(parent);
				if (cam)    e->SetCamera(cam);
				if (config) config(*e);
				e->SyncTransform();
				enemies.push_back(std::move(e));
			}
	}

}