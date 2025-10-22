#pragma once
#include <memory>
#include <vector>
#include <functional>

class Enemy;
class Camera;
class DirectXCommon;
class BaseScene;

//=============================================================
// EnemySpawner名前空間
// 敵の生成処理をまとめたユーティリティ。
//=============================================================
namespace EnemySpawner {

	using EnemyConfig = std::function<void(Enemy&)>;

	/// <summary>一直線に敵をスポーンします。</summary>
	/// <param name="count">数。</param><param name="y">高さ。</param><param name="z">基準Z。</param>
	/// <param name="xStart">開始X。</param><param name="xStep">X間隔。</param>
	/// <param name="dx">DX共通。</param><param name="cam">カメラ。</param><param name="parent">親シーン。</param>
	/// <param name="config">生成直後に適用する設定関数。</param>
	void SpawnLine(std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float y, float z,
		float xStart, float xStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent,
		EnemyConfig config); // 追加オーバーロード

	/// <summary>V字隊列で敵をスポーンします。</summary>
	/// <param name="countPerSide">片側の数。</param><param name="y">高さ。</param>
	/// <param name="z">先頭Z。</param><param name="xCenter">中心X。</param>
	/// <param name="xStep">左右のX間隔。</param><param name="zStep">後列とのZ間隔。</param>
	void SpawnV(std::vector<std::unique_ptr<Enemy>>& enemies,
		int countPerSide, float y, float z,
		float xCenter, float xStep, float zStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent,
		EnemyConfig config);

	/// <summary>縦一列で敵をスポーンします。</summary>
	/// <param name="count">数。</param><param name="x">X位置。</param>
	/// <param name="zStart">開始Z。</param><param name="zStep">Z間隔。</param>
	/// <param name="yStart">開始Y。</param><param name="yStep">Y間隔。</param>
	void SpawnColumn(std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float x, float zStart, float zStep,
		float yStart, float yStep,
		DirectXCommon* dx, Camera* cam, BaseScene* parent,
		EnemyConfig config);
}
