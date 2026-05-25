#include "PlayerHealth.h"
#include <algorithm>

void PlayerHealth::Initialize(int maxHp) {
	// 最大HPを設定する
	maxHp_ = maxHp;

	// 現在HPを最大HPにする
	hp_ = maxHp_;

	//=========================================================
	// 被弾状態リセット
	//=========================================================
	hitFlashTimer_ = 0.0f;

	//=========================================================
	// 無敵状態リセット
	//=========================================================
	isInvincible_ = false;
	invincibleT_ = 0.0f;
	blinkT_ = 0.0f;
	invincibleVisible_ = true;

	//=========================================================
	// 同一攻撃IDロックリセット
	//=========================================================
	lastHitAttackId_ = -1;
	sameAttackLockT_ = 0.0f;
}

void PlayerHealth::Update(float dt) {
	//=========================================================
	// 無敵時間更新
	//=========================================================
	if (isInvincible_) {
		// 無敵経過時間を進める
		invincibleT_ += dt;

		// 点滅用タイマーを進める
		blinkT_ += dt;

		// 一定間隔ごとに表示/非表示を切り替える
		if (blinkT_ >= kBlinkInterval_) {
			blinkT_ = 0.0f;
			invincibleVisible_ = !invincibleVisible_;
		}

		// 無敵時間が終わったら通常状態へ戻す
		if (invincibleT_ >= kInvincibleSec_) {
			isInvincible_ = false;
			invincibleT_ = 0.0f;
			blinkT_ = 0.0f;
			invincibleVisible_ = true;
		}
	}

	//=========================================================
	// 被弾フラッシュタイマー更新
	//=========================================================
	if (hitFlashTimer_ > 0.0f) {
		// タイマーを減らす
		hitFlashTimer_ -= dt;

		// 0未満にならないようにする
		if (hitFlashTimer_ < 0.0f) {
			hitFlashTimer_ = 0.0f;
		}
	}

	//=========================================================
	// 同一攻撃IDロックタイマー更新
	//=========================================================
	if (sameAttackLockT_ > 0.0f) {
		// ロック時間を減らす
		sameAttackLockT_ -= dt;

		// 0未満にならないようにする
		if (sameAttackLockT_ < 0.0f) {
			sameAttackLockT_ = 0.0f;
		}
	}
}

bool PlayerHealth::Damage(int value) {
	// 無敵中ならダメージを受けない
	if (isInvincible_) {
		return false;
	}

	// HPを減らす
	hp_ -= value;

	// 0未満にならないよう補正する
	if (hp_ < 0) {
		hp_ = 0;
	}

	//=========================================================
	// 被弾フラッシュ開始
	//=========================================================
	hitFlashTimer_ = kHitFlashSec_;

	//=========================================================
	// 無敵開始
	//=========================================================
	isInvincible_ = true;
	invincibleT_ = 0.0f;
	blinkT_ = 0.0f;
	invincibleVisible_ = true;

	return true;
}

bool PlayerHealth::TryDamageFromAttack(int damage, int attackId) {
	// 無敵中なら受けない
	if (isInvincible_) {
		return false;
	}

	// ロック時間中に同じ攻撃IDなら無視する
	if (sameAttackLockT_ > 0.0f && attackId == lastHitAttackId_) {
		return false;
	}

	// ダメージを通す
	if (!Damage(damage)) {
		return false;
	}

	// 今回の攻撃IDを記録する
	lastHitAttackId_ = attackId;

	// 短時間だけ同一攻撃ロックをかける
	sameAttackLockT_ = kSameAttackLockSec_;

	return true;
}

void PlayerHealth::SetHP(int hp) {
	// HPを範囲内に収めて設定する
	hp_ = std::clamp(hp, 0, maxHp_);
}

void PlayerHealth::Reset() {
	// HPを最大値に戻す
	hp_ = maxHp_;

	// 被弾状態をリセットする
	hitFlashTimer_ = 0.0f;

	// 無敵状態をリセットする
	isInvincible_ = false;
	invincibleT_ = 0.0f;
	blinkT_ = 0.0f;
	invincibleVisible_ = true;

	// 同一攻撃IDロックをリセットする
	lastHitAttackId_ = -1;
	sameAttackLockT_ = 0.0f;
}

float PlayerHealth::GetHPRate() const {
	// 最大HPが0以下ならHP割合は0とする
	if (maxHp_ <= 0) {
		return 0.0f;
	}

	// HP割合を計算する
	float rate = static_cast<float>(hp_) / static_cast<float>(maxHp_);
	// 0.0～1.0の範囲に収める
	if (rate < 0.0f) {
		rate = 0.0f;
	}
	// 1.0を超えないようにする
	if (rate > 1.0f) {
		rate = 1.0f;
	}

	return rate; // HP割合を返す
}