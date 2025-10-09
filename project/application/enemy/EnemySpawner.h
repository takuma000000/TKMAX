#pragma once
#include <memory>
#include <vector>
#include <functional>

class Enemy;
class Camera;
class DirectXCommon;
class BaseScene;

namespace EnemySpawner {

	using EnemyConfig = std::function<void(Enemy&)>;

	void SpawnLine(std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float y, float z,
		float xStart, float xStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent,
		EnemyConfig config); // 追加オーバーロード

	void SpawnV(std::vector<std::unique_ptr<Enemy>>& enemies,
		int countPerSide, float y, float z,
		float xCenter, float xStep, float zStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent,
		EnemyConfig config);

	void SpawnColumn(std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float x, float zStart, float zStep,
		float yStart, float yStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent,
		EnemyConfig config);
}
