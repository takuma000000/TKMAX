#include "EnemySpawner.h"
#include "Enemy.h"
#include "Object3dCommon.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "BaseScene.h"

namespace EnemySpawner {

	void SpawnLine(std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float y, float z,
		float xStart, float xStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent, EnemyConfig config) {
		for (int i = 0; i < count; ++i) {
			auto e = std::make_unique<Enemy>();
			e->Initialize(Object3dCommon::GetInstance(), dx);
			e->SetPosition({ xStart + xStep * i, y, z });
			if (parent) e->SetParentScene(parent);
			if (cam)    e->SetCamera(cam);
			if (config) config(*e);
			enemies.push_back(std::move(e));
		}
	}

	void SpawnV(std::vector<std::unique_ptr<Enemy>>& enemies,
		int countPerSide, float y, float z,
		float xCenter, float xStep, float zStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent, EnemyConfig config) {
		// 中央
			{
				auto e = std::make_unique<Enemy>();
				e->Initialize(Object3dCommon::GetInstance(), dx);
				e->SetPosition({ xCenter, y, z });
				if (parent) e->SetParentScene(parent);
				if (cam)    e->SetCamera(cam);
				if (config) config(*e);
				enemies.push_back(std::move(e));
			}
			// 左右展開
			for (int i = 1; i <= countPerSide; ++i) {
				for (int side = -1; side <= 1; side += 2) {
					auto e = std::make_unique<Enemy>();
					e->Initialize(Object3dCommon::GetInstance(), dx);
					e->SetPosition({ xCenter + side * xStep * i, y, z + zStep * i });
					if (parent) e->SetParentScene(parent);
					if (cam)    e->SetCamera(cam);
					if (config) config(*e);
					enemies.push_back(std::move(e));
				}
			}
	}

	void SpawnColumn(std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float x, float zStart, float zStep,
		float yStart, float yStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent, EnemyConfig config) {
		for (int i = 0; i < count; ++i) {
			auto e = std::make_unique<Enemy>();
			e->Initialize(Object3dCommon::GetInstance(), dx);
			e->SetPosition({ x, yStart + yStep * i, zStart + zStep * i });
			if (parent) e->SetParentScene(parent);
			if (cam)    e->SetCamera(cam);
			if (config) config(*e);
			enemies.push_back(std::move(e));
		}
	}
}
