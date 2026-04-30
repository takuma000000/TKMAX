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

	// 雑魚敵フェーズ本隊として扱う
	enemy->SetType(EnemyType::MainSquad);

	// クラゲモデルの場合は触手も設定する
	if (desc.model_ == "jerryfish.obj") {
		enemy->SetTentacleModel("tentacle.obj");
		// 触手のローカル座標は、モデルの原点を基準に適切な位置・回転・スケールを設定します。
		enemy->SetTentacleLocal(
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f },
			{ 1.0f, 1.0f, 1.0f }
		);
	}

	// 本隊は円形隊列の目標位置へ移動する
	enemy->SetBehavior(EnemyBehavior::MoveToTarget);
	enemy->SetFormationMoveSpeed(desc.formationMoveSpeed_);

	return enemy;
}