#include "BattleActorManagerBase.h"
#include "Player.h"

void BattleActorManagerBase::InitializeCommon(TKM::DirectXCommon* dx, TKM::Camera* camera, TKM::BaseScene* parent, Player* player) {
	// 共通で参照を保持するための初期化関数
	dx_ = dx;
	camera_ = camera;
	parent_ = parent;
	player_ = player;
}

void BattleActorManagerBase::SetCamera(TKM::Camera* camera) {
	camera_ = camera; // カメラを更新
	OnCameraChanged(); // カメラが変わったことを派生クラスに通知して、必要な処理をさせる
}
