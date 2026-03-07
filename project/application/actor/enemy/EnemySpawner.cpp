#include "EnemySpawner.h"
#include "Enemy.h"
#include "Object3dCommon.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include <ParticleManager.h>

namespace EnemySpawner {

	// ------------------------------------------------------------
	// CreateAndPushEnemy
	// ------------------------------------------------------------
	// 【目的】
	//   1体分の Enemy を生成→初期化→設定→（任意でスポーン演出）→enemies に追加する共通処理。
	//
	// 【ここにまとめる理由】
	//   SpawnLine / SpawnV / SpawnColumn / SpawnTriangle3 が
	//   “座標だけ違って、やることは同じ”なので重複を消して保守しやすくするため。
	// ------------------------------------------------------------
	static void CreateAndPushEnemy(
		std::vector<std::unique_ptr<Enemy>>& enemies, // 生成した敵を格納する配列
		const Vector3& spawnPos,                     // ワールドのスポーン座標
		TKM::DirectXCommon* dx,                      // Enemy内部の描画/初期化に必要
		TKM::Camera* cam,                            // 追従や演出などで参照するカメラ（任意）
		TKM::BaseScene* parent,                      // シーン参照（任意：UI/演出/参照用）
		const EnemyConfig& config,                   // 追加設定を差し込むコールバック（任意）
		bool useSpawnEffect                          // スポーンパーティクルを出すか
	) {
		// 1) Enemy を生成（unique_ptrで所有）
		auto e_ = std::make_unique<Enemy>();

		// 2) Enemy の初期化（Object3dCommon は singleton から取得）
		//    ※Object3dCommonはObject3dの共通リソース/パイプライン等を持つ想定
		e_->Initialize(TKM::Object3dCommon::GetInstance(), dx);

		// 3) 座標を設定
		e_->SetPosition(spawnPos);

		// 4) シーン/カメラ参照を設定（必要な場合のみ）
		//    ※Enemyがシーン参照やカメラ参照を使う設計なら、ここで渡しておく
		if (parent) { e_->SetParentScene(parent); }
		if (cam) { e_->SetCamera(cam); }

		// 5) 追加の個体設定（移動速度、HP、種類など）を外から注入
		//    ※EnemyConfig は「設定したい処理がある時だけ渡せる」ようにしている
		if (config) { config(*e_); }

		// 6) Enemy 側の Transform 情報を Object3d に同期
		//    ※SetPosition などで内部値が変わっても、描画側Object3dへ反映が必要ならここで同期
		e_->SyncTransform();

		// 7) スポーン演出（パーティクル）を出す場合
		if (useSpawnEffect) {
			// Emit位置は敵のスポーン座標に合わせる
			Vector3 emitPos_ = spawnPos;

			// "enemySpawn" グループから指定数発生
			TKM::ParticleManager::GetInstance()->Emit("enemySpawn", emitPos_, kSpawnParticleCount_);
		}

		// 8) 配列に追加（所有権を移動）
		enemies.push_back(std::move(e_));
	}

	// ------------------------------------------------------------
	// SpawnLine
	// ------------------------------------------------------------
	// 【目的】
	//   敵を “横方向に等間隔” で 1直線に並べてスポーンする。
	//
	// 【配置】
	//   x = xStart + xStep * i
	//   y = 固定
	//   z = 固定
	// ------------------------------------------------------------
	void SpawnLine(
		std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float y, float z,
		float xStart, float xStep,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent,
		EnemyConfig config
	) {
		// i 体分、X方向に並べて生成
		for (int i = 0; i < count; ++i) {

			// それぞれのスポーン座標を計算
			Vector3 spawnPos_ = { xStart + xStep * i, y, z };

			// 共通生成処理（ここはスポーン演出あり）
			CreateAndPushEnemy(enemies, spawnPos_, dx, cam, parent, config, true);
		}
	}

	// ------------------------------------------------------------
	// SpawnV
	// ------------------------------------------------------------
	// 【目的】
	//   敵を “V字型” に配置してスポーンする。
	//
	// 【配置】
	//   まず中央に1体
	//   その後、左右に i 段ずつ展開（z方向にも段数に応じて奥へ）
	//
	// 【ポイント】
	//   side = -1（左） / +1（右）
	// ------------------------------------------------------------
	void SpawnV(
		std::vector<std::unique_ptr<Enemy>>& enemies,
		int countPerSide, float y, float z,
		float xCenter, float xStep, float zStep,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent,
		EnemyConfig config
	) {
		// 1) 中央の1体（先頭）
		{
			Vector3 spawnPos_ = { xCenter, y, z };
			CreateAndPushEnemy(enemies, spawnPos_, dx, cam, parent, config, true);
		}

		// 2) 左右に段数分展開
		//    i = 1..countPerSide で、中心からの距離が広がる
		for (int i = 1; i <= countPerSide; ++i) {

			// 左右それぞれに1体ずつ配置
			for (int side = -1; side <= 1; side += 2) {

				// x は左右に広げ、z は段数に応じて奥へ進める
				Vector3 spawnPos_ = {
					xCenter + side * xStep * i, // 左右オフセット
					y,
					z + zStep * i              // V字の“開き”をzでも表現
				};

				CreateAndPushEnemy(enemies, spawnPos_, dx, cam, parent, config, true);
			}
		}
	}

	// ------------------------------------------------------------
	// SpawnColumn
	// ------------------------------------------------------------
	// 【目的】
	//   敵を “柱（縦方向/斜め方向）” に等間隔で並べてスポーンする。
	//
	// 【配置】
	//   x は固定
	//   y = yStart + yStep * i
	//   z = zStart + zStep * i
	//
	// 【使いどころ】
	//   高低差のある列、または奥に流れる列、などの表現に使える。
	// ------------------------------------------------------------
	void SpawnColumn(
		std::vector<std::unique_ptr<Enemy>>& enemies,
		int count, float x, float zStart, float zStep,
		float yStart, float yStep,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent,
		EnemyConfig config
	) {
		// i 体分、y と z を段数に応じて進めながら生成
		for (int i = 0; i < count; ++i) {

			// i に応じて y と z を進める（柱状/斜め列状）
			Vector3 spawnPos_ = {
				x,
				yStart + yStep * i,
				zStart + zStep * i
			};
			// こちらもスポーン演出ありで生成
			CreateAndPushEnemy(enemies, spawnPos_, dx, cam, parent, config, true);
		}
	}

	// ------------------------------------------------------------
	// SpawnTriangle3
	// ------------------------------------------------------------
	// 【目的】
	//   “正三角形（3体）” の配置でスポーンする。
	//
	// 【配置（簡易）】
	//   上に1体： (centerX, centerY + size)
	//   下左右： (centerX ± size, centerY - size)
	//
	// 【注意】
	//   ここは元コードの意図に合わせてスポーン演出を出さない（useSpawnEffect=false）
	// ------------------------------------------------------------
	void SpawnTriangle3(
		std::vector<std::unique_ptr<Enemy>>& enemies,
		float centerX, float centerY, float z,
		float size,
		TKM::DirectXCommon* dx, TKM::Camera* cam, TKM::BaseScene* parent,
		EnemyConfig config
	) {
		// 1) 上（頂点）に1体
		{
			Vector3 spawnPos_ = { centerX, centerY + size, z };

			// 元の仕様に合わせて演出なし
			CreateAndPushEnemy(enemies, spawnPos_, dx, cam, parent, config, false);
		}

		// 2) 下左右に2体
		for (int side = -1; side <= 1; side += 2) {

			// side=-1 で左、side=+1 で右
			Vector3 spawnPos_ = { centerX + side * size, centerY - size, z };

			// こちらも演出なし
			CreateAndPushEnemy(enemies, spawnPos_, dx, cam, parent, config, false);
		}
	}
}