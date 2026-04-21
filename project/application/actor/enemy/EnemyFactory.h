#pragma once
#include <memory>
#include <vector>
#include <functional>

class Enemy;
namespace TKM { 
	class Camera;
	class DirectXCommon;
	class BaseScene;
}

//=============================================================
// EnemyFactory名前空間
// 旧隊列生成用を含む敵生成ユーティリティ。現行の本隊生成では未使用。
//=============================================================
namespace EnemyFactory {
	// 敵出現エフェクトの粒子数（マジックナンバー 32 の定数化）
	constexpr int kSpawnParticleCount_ = 32;
	using EnemyConfig = std::function<void(Enemy&)>;

	/// <summary>
	/// 直線状に敵をスポーンします。
	/// </summary>
	/// <param name="enemies">敵を追加するコンテナ</param>
	/// <param name="count">生成する敵の数</param>
	/// <param name="y">生成高さ（ワールド座標）</param>
	/// <param name="z">基準となる Z 座標（ワールド座標）</param>
	/// <param name="xStart">開始 X 座標（ワールド座標）</param>
	/// <param name="xStep">X 方向の間隔</param>
	/// <param name="dx">DirectX 共通管理クラス</param>
	/// <param name="cam">描画および判定に使用するカメラ</param>
	/// <param name="parent">所属する親シーン</param>
	/// <param name="config">生成直後に適用する敵設定関数</param>
	void SpawnLine(
		std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float y, float z,
		float xStart, float xStep,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent,
		EnemyConfig config
	);
	/// <summary>
	/// V 字隊列で敵をスポーンします。
	/// </summary>
	/// <param name="enemies">敵を追加するコンテナ</param>
	/// <param name="countPerSide">片側あたりの敵数</param>
	/// <param name="y">生成高さ（ワールド座標）</param>
	/// <param name="z">先頭の Z 座標（ワールド座標）</param>
	/// <param name="xCenter">中心となる X 座標</param>
	/// <param name="xStep">左右方向の X 間隔</param>
	/// <param name="zStep">後列との Z 間隔</param>
	/// <param name="dx">DirectX 共通管理クラス</param>
	/// <param name="cam">描画および判定に使用するカメラ</param>
	/// <param name="parent">所属する親シーン</param>
	/// <param name="config">生成直後に適用する敵設定関数</param>
	void SpawnV(
		std::vector<std::unique_ptr<Enemy>>& enemies,
		int countPerSide, float y, float z,
		float xCenter, float xStep, float zStep,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent,
		EnemyConfig config
	);
	/// <summary>
	/// 縦一列に敵をスポーンします。
	/// </summary>
	/// <param name="enemies">敵を追加するコンテナ</param>
	/// <param name="count">生成する敵の数</param>
	/// <param name="x">X 座標（ワールド座標）</param>
	/// <param name="zStart">開始 Z 座標（ワールド座標）</param>
	/// <param name="zStep">Z 方向の間隔</param>
	/// <param name="yStart">開始 Y 座標（ワールド座標）</param>
	/// <param name="yStep">Y 方向の間隔</param>
	/// <param name="dx">DirectX 共通管理クラス</param>
	/// <param name="cam">描画および判定に使用するカメラ</param>
	/// <param name="parent">所属する親シーン</param>
	/// <param name="config">生成直後に適用する敵設定関数</param>
	void SpawnColumn(
		std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float x, float zStart, float zStep,
		float yStart, float yStep,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent,
		EnemyConfig config
	);
	/// <summary>
	/// 三角形配置で敵をスポーンします。
	/// </summary>
	/// <param name="enemies">敵を追加するコンテナ</param>
	/// <param name="centerX">配置の中心 X 座標</param>
	/// <param name="centerY">配置の中心 Y 座標</param>
	/// <param name="z">配置の Z 座標（ワールド座標）</param>
	/// <param name="size">三角形の大きさ（間隔スケール）</param>
	/// <param name="dx">DirectX 共通管理クラス</param>
	/// <param name="cam">描画および判定に使用するカメラ</param>
	/// <param name="parent">所属する親シーン</param>
	/// <param name="config">生成直後に適用する敵設定関数</param>
	void SpawnTriangle3(
		std::vector<std::unique_ptr<Enemy>>& enemies,
		float centerX, float centerY, float z,
		float size,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent,
		EnemyConfig config
	);

}