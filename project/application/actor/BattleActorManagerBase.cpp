#include "BattleActorManagerBase.h"
#include "Player.h"

void BattleActorManagerBase::InitializeCommon(TKM::DirectXCommon* dx, TKM::Camera* camera, TKM::BaseScene* parent, Player* player) {
	// DirectX共通情報を保持する
	dx_ = dx;

	// カメラ参照を保持する
	camera_ = camera;

	// 親シーン参照を保持する
	parent_ = parent;

	// プレイヤー参照を保持する
	player_ = player;
}

void BattleActorManagerBase::SetCamera(TKM::Camera* camera) {
	// 新しいカメラ参照を保持する
	camera_ = camera;

	// カメラ変更後の反映処理を派生クラスへ任せる
	OnCameraChanged();
}