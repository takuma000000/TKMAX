#include "PlayerDeath.h"
#include <algorithm>
#include <cstdlib>

void PlayerDeath::Initialize() {
	// 撃墜状態を解除する
	isDead_ = false;
	// 撃墜開始要求を消す
	startRequested_ = false;
	// 速度を初期化する
	deathVelocity_ = { 0.0f, 0.0f, 0.0f };
	// 角速度を初期化する
	deathAngularVelocity_ = { 0.0f, 0.0f, 0.0f };
}

void PlayerDeath::Start() {
	// すでに撃墜中なら二重開始しない
	if (isDead_) {
		return;
	}

	// 撃墜状態にする
	isDead_ = true;
	// 撃墜開始時の一度きり処理を要求する
	startRequested_ = true;
	// 後方へ弾かれつつ落下する初速を与える
	deathVelocity_ = { 0.0f, -kDeathFallStartSpeed_, -kDeathBackwardSpeed_ * 0.8f };

	// 左右どちらに崩れるかをランダムで決める
	float rollSign = (rand() % 2 == 0) ? -1.0f : 1.0f;
	// 初期角速度を設定する
	deathAngularVelocity_.x = 0.012f;
	deathAngularVelocity_.y = 0.0f;
	deathAngularVelocity_.z = 0.020f * rollSign;
}

void PlayerDeath::Update(TKM::Object3d* ownerObject) {
	// 撃墜中でなければ更新しない
	if (!isDead_) {
		return;
	}
	// 本体がなければ更新しない
	if (!ownerObject) {
		return;
	}

	//=========================================================
	// 落下・吹き飛び更新
	//=========================================================

	// 下方向へ重力加速させる
	deathVelocity_.y -= kDeathGravity_;

	// 落下速度の下限を設ける
	if (deathVelocity_.y < -kDeathFallMaxSpeed_) {
		deathVelocity_.y = -kDeathFallMaxSpeed_;
	}

	// 後方への勢いは少しずつ減衰させる
	deathVelocity_.z *= kDeathBackwardDamping_;

	//=========================================================
	// 位置更新
	//=========================================================

	// 現在位置を取得する
	Vector3 pos = ownerObject->GetTranslate();
	// 撃墜速度を加算する
	pos += deathVelocity_;
	// 位置を反映する
	ownerObject->SetTranslate(pos);

	//=========================================================
	// 姿勢更新
	//=========================================================

	// 現在回転を取得する
	Vector3 newRot = ownerObject->GetRotate();

	// 角速度を加算する
	newRot.x += deathAngularVelocity_.x;
	newRot.z += deathAngularVelocity_.z;

	// 前後回転の最大値を制限する
	if (newRot.x > kDeathMaxPitch_) {
		newRot.x = kDeathMaxPitch_;
	}

	// 左右ロールの最大値を制限する
	newRot.z = std::clamp(newRot.z, -kDeathMaxRoll_, kDeathMaxRoll_);
	// 回転を反映する
	ownerObject->SetRotate(newRot);
	// 角速度は徐々に減衰させる
	deathAngularVelocity_ *= kDeathRotateDamping_;
}

void PlayerDeath::ConsumeStartRequest() {
	// 撃墜開始要求を消費する
	startRequested_ = false;
}