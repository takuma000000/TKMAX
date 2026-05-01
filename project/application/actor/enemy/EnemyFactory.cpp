#include "EnemyFactory.h"

std::unique_ptr<Enemy> EnemyFactory::CreateMainSquadEnemy(
	TKM::Object3dCommon* common,
	TKM::DirectXCommon* dxCommon,
	TKM::Camera* camera,
	TKM::BaseScene* parent,
	const MainSquadDesc& desc
) {
	auto enemy = std::make_unique<Enemy>();

	// Enemy本体を初期化する
	enemy->Initialize(common, dxCommon);

	// 描画・シーン参照を設定する
	enemy->SetCamera(camera);
	enemy->SetParentScene(parent);

	// JSON由来の基本パラメータを反映する
	enemy->SetModel(desc.model_);
	enemy->SetHP(desc.hp_);
	enemy->SetScale(desc.scale_);
	enemy->SetType(desc.type_);

	// 触手の設定
	if (desc.useTentacle_) {
		enemy->SetTentacleModel(desc.tentacleModel_);
		enemy->SetTentacleLocal(
			desc.tentacleLocalPosition_,
			desc.tentacleLocalRotation_,
			desc.tentacleLocalScale_
		);
	}

	// 本隊は円形隊列の目標位置へ移動する
	enemy->SetBehavior(EnemyBehavior::MoveToTarget);
	enemy->SetFormationMoveSpeed(desc.formationFollowSpeed_);

	return enemy;
}