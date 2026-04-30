#pragma once
#include <memory>
#include <string>

#include "Enemy.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "BaseScene.h"

//=============================================================
// EnemyFactoryクラス
// 敵オブジェクトの生成と初期設定を担当するFactoryクラス。
//=============================================================
class EnemyFactory {
public:
	// MainSquadDesc構造体
	// MainSquadEnemyを生成するための設定をまとめた構造体
	struct MainSquadDesc {
		std::string model_;                    // モデルファイル名
		int hp_ = 1;                           // HP
		Vector3 scale_ = { 1.0f, 1.0f, 1.0f }; // スケール
		float formationMoveSpeed_ = 0.0f;      // 編隊移動速度
	};

	/// <summary>
	/// MainSquadEnemyを生成します。
	/// </summary>
	/// <param name="common">Object3dCommonのポインタ</param>
	/// <param name="dxCommon">DirectXCommonのポインタ</param>
	/// <param name="camera">Cameraのポインタ</param>
	/// <param name="parent">生成したEnemyの親シーン</param>
	/// <param name="desc">MainSquadEnemyの生成に必要な設定</param>
	/// <returns>生成されたMainSquadEnemyのユニークポインタ</returns>
	static std::unique_ptr<Enemy> CreateMainSquadEnemy(
		TKM::Object3dCommon* common,
		TKM::DirectXCommon* dxCommon,
		TKM::Camera* camera,
		TKM::BaseScene* parent,
		const MainSquadDesc& desc
	);
};