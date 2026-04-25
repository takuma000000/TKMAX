#include "BossStates.h"
#include "BossController.h"
#include <cmath>
#include <algorithm>

/// <summary>
/// 共通のStateContextをBossControllerとして扱えるように変換します。
/// </summary>
/// <param name="ctx">ステートマシンから渡される共通コンテキスト</param>
/// <returns>BossController参照</returns>
static BossController& AsBoss_(TKM::IStateContext& ctx) {
	return static_cast<BossController&>(ctx);
}

//=====================================================
// Enter
//=====================================================
void BossEnterState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);

	// 登場状態の経過時間を初期化する
	c.timer_ = 0.0f;

	// 現在の状態をEnterに設定する
	c.state_ = BossController::State::Enter;
}

void BossEnterState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);
	Enemy& boss = *c.boss_;
	Vector3& pos = c.posWork_;

	// 登場時の目標位置・移動速度・完了判定幅を設定から取得する
	const float targetZ_ = c.config_->orbit_.z_;
	const float speedZ_ = c.config_->enter_.approachSpeedZ_;
	const float speedX_ = c.config_->enter_.approachSpeedX_;
	const float speedY_ = c.config_->enter_.approachSpeedY_;
	const float completeEpsilonZ_ = c.config_->enter_.completeEpsilonZ_;

	// Z方向は奥から目標Zへ近づける
	pos.z = BossController::Approach(pos.z, targetZ_, speedZ_ * dt);

	// X方向は中央へ寄せる
	pos.x = BossController::Approach(pos.x, 0.0f, speedX_ * dt);

	// Y方向は軌道移動用の高さへ寄せる
	pos.y = BossController::Approach(pos.y, c.config_->orbit_.y_, speedY_ * dt);

	// 目標Zに十分近づいたら、通常の軌道移動へ移行する
	if (std::abs(pos.z - targetZ_) < completeEpsilonZ_) {
		c.ChangeState(BossController::State::Orbit);
	}
}

//=====================================================
// Orbit
//=====================================================
void BossOrbitState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);

	// 軌道移動の経過時間を初期化する
	c.timer_ = 0.0f;

	// 現在の状態をOrbitに設定する
	c.state_ = BossController::State::Orbit;
}

void BossOrbitState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);
	Enemy& boss = *c.boss_;
	Vector3& pos = c.posWork_;
	const Vector3& playerPos = c.playerPos_;

	// -----------------------------
	// 軌道移動用の周期オフセットを作る
	// -----------------------------
	float t_ = c.timer_;
	float angle_ = t_ * c.config_->orbit_.angularSpeed_;
	float ox_ = std::cos(angle_) * c.config_->orbit_.radiusX_;
	float oy_ = std::sin(angle_ * 0.9f) * c.config_->orbit_.radiusY_;

	// プレイヤー位置の影響を少し受けつつ、ボス独自の軌道オフセットを加えた目標位置を作る
	Vector3 target_;
	target_.x = playerPos.x * c.config_->orbit_.playerInfluence_ + ox_;
	target_.y = c.config_->orbit_.y_ + playerPos.y * 0.2f + oy_;
	target_.z = c.config_->orbit_.z_;

	// 現在位置から目標位置へ滑らかに追従させる
	pos = BossController::SmoothDamp(pos, target_, c.config_->orbit_.follow_, dt);

	// 一定時間は軌道移動だけを行い、攻撃選択へ進まない
	if (c.timer_ < c.config_->orbit_.duration_) { return; }

	// -----------------------------
	// 通常時：ミサイル or スラッシュ
	// 怒り中：現在はレーザー処理を抑制してRecoverへ
	// -----------------------------
	if (!c.rageActive_) {

		// スラッシュがクールダウン中でなければ選択候補に入れる
		const bool canSlash_ = (c.slashCooldownT_ <= 0.0f);

		// 0.0f～1.0fの乱数で攻撃を選ぶ
		std::uniform_real_distribution<float> u01(0.0f, 1.0f);

		// クールダウン中はスラッシュ選択率を0にする
		const float slashRate_ = canSlash_ ? c.config_->slash_.selectRate_ : 0.0f;

		// スラッシュを行うか判定する
		const bool doSlash_ = (u01(c.rng_) < slashRate_);

		if (doSlash_) {
			// スラッシュ攻撃のチャージを開始する
			c.slashCharging_ = true;
			c.slashChargeTimer_ = c.config_->slash_.chargeTime_;
			c.slashChargeFrame_ = 0;

			// プレイヤーが取れる場合は現在位置をスナップする
			if (boss.GetPlayer()) {
				c.slashTargetSnap_ = boss.GetPlayer()();
				c.slashTargetValid_ = true;
			}
			// プレイヤー参照がない場合は、保持しているプレイヤー位置を使う
			else {
				c.slashTargetSnap_ = playerPos;
				c.slashTargetValid_ = true;
			}

			// スラッシュのクールダウンを開始する
			c.slashCooldownT_ = c.config_->slash_.cooldown_;

			// スラッシュ選択時はミサイル系の状態を完全に止め、攻撃混在を防ぐ
			c.burstTargetValid_ = false;
			c.burstCharged_ = false;
			c.missileCharging_ = false;
			c.missileChargeTimer_ = 0.0f;
			c.missileChargeFrame_ = 0;

			// Recover中でスラッシュチャージ処理を進める
			c.ChangeState(BossController::State::Recover);
			return;
		}

		// ミサイル選択時はスラッシュ系の状態を完全に止め、攻撃混在を防ぐ
		c.slashCharging_ = false;
		c.slashChargeTimer_ = 0.0f;
		c.slashChargeFrame_ = 0;
		c.slashFireReq_ = false;

		// ミサイルチャージを開始する
		c.burstTargetValid_ = false;
		c.burstCharged_ = false;
		c.missileCharging_ = true;
		c.missileChargeTimer_ = c.config_->missile_.chargeTime_;
		c.missileChargeFrame_ = 0;
		c.missileRequestCount_ = 0;
		c.missileRequestConsumeIndex_ = 0;

		// プレイヤーが取れる場合は現在位置をスナップする
		if (boss.GetPlayer()) {
			c.burstTargetSnap_ = boss.GetPlayer()();
			c.burstTargetValid_ = true;
		}
		// プレイヤー参照がない場合は、保持しているプレイヤー位置を使う
		else {
			c.burstTargetSnap_ = playerPos;
			c.burstTargetValid_ = true;
		}

		// Recover中でミサイルチャージ処理を進める
		c.ChangeState(BossController::State::Recover);
		return;
	}

	// 怒り中のレーザー処理は現在コメントアウト中のため、Recoverへ戻す
	c.ChangeState(BossController::State::Recover);
}

//=====================================================
// Recover（ここに Burst / Slash charge を移植）
//=====================================================
void BossRecoverState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);

	// Recover状態の経過時間を初期化する
	c.timer_ = 0.0f;

	// 現在の状態をRecoverに設定する
	c.state_ = BossController::State::Recover;
}

void BossRecoverState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);
	Enemy& boss = *c.boss_;
	Vector3& pos = c.posWork_;

	// 軌道移動時の高さとZへ戻るための目標位置を作る
	Vector3 target_{ pos.x, c.config_->orbit_.y_, c.config_->orbit_.z_ };

	// Xは現在値を保ちつつ、Y/Zを滑らかに戻す
	pos = BossController::SmoothDamp(pos, target_, c.config_->recover_.follow_, dt);

	// ============================================================
	// Missile Charge / 6-way Fire Execute（Recover中のみ）
	// ============================================================
	if (c.missileCharging_ || c.burstCharged_) {
		// ボスの現在位置を基準にミサイル発射口の中心を作る
		Vector3 muzzleCenter_ = boss.GetWorldPosition();
		muzzleCenter_.y += c.config_->missile_.muzzleYOffset_;

		if (c.missileCharging_) {
			auto* pm_ = TKM::ParticleManager::GetInstance();

			if (pm_) {
				Vector3 p_ = muzzleCenter_;

				// チャージ進行率を0.0f～1.0fで求める
				float t = 1.0f - (c.missileChargeTimer_ / c.config_->missile_.chargeTime_);
				t = std::clamp(t, 0.0f, 1.0f);

				// ミサイルは「収束」ではなく「発射システム起動」として見せる。
				// 時間経過で、発射口点灯 → レーン展開 → 骨組み表示 → 最終点火へ進める。
				if (t < 0.20f) {
					// 序盤：発射口が点き始める
					pm_->Emit("bossMissile_node", p_, 2);
				} else if (t < 0.45f) {
					// 中盤前：点火ノード増加 + 細い前方レーン
					pm_->Emit("bossMissile_node", p_, 3);
					pm_->Emit("bossMissile_lane", p_, 2);

					// 数フレームおきにジェットを混ぜる
					if ((c.missileChargeFrame_ % 4) == 0) {
						pm_->Emit("bossMissile_jet", p_, 2);
					}
				} else if (t < 0.75f) {
					// 中盤後：レーンが増え、兵器UIの骨組みが前方空間に出る
					pm_->Emit("bossMissile_node", p_, 4);
					pm_->Emit("bossMissile_lane", p_, 4);
					pm_->Emit("bossMissile_jet", p_, 3);

					// グリッド演出は少し間引いて出す
					if ((c.missileChargeFrame_ % 3) == 0) {
						pm_->Emit("bossMissile_grid", p_, 1);
					}
				} else {
					// 終盤：全レーン点灯 + フラッシュ
					pm_->Emit("bossMissile_node", p_, 7);
					pm_->Emit("bossMissile_lane", p_, 9);
					pm_->Emit("bossMissile_jet", p_, 7);

					// 終盤はグリッドの密度も上げる
					if ((c.missileChargeFrame_ % 2) == 0) {
						pm_->Emit("bossMissile_grid", p_, 2);
					}

					pm_->Emit("bossMissile_flash", p_, 3);
				}
			}

			// チャージ演出のフレーム数を進める
			++c.missileChargeFrame_;

			// チャージ残り時間を減らす
			c.missileChargeTimer_ -= dt;

			if (c.missileChargeTimer_ <= 0.0f) {
				// 発射直前のプレイヤー座標を固定する
				if (boss.GetPlayer()) {
					c.burstTargetSnap_ = boss.GetPlayer()();
				} else {
					c.burstTargetSnap_ = c.playerPos_;
				}

				c.burstTargetValid_ = true;

				// 左3発・右3発で固定配置する
				static constexpr int kMissileCount_ = BossController::kMissileSimultaneousCount_;

				const float kSideX_ = 7.5f;
				const float kUpperY_ = 3.5f;
				const float kMiddleY_ = 1.2f;
				const float kLowerY_ = -1.5f;
				const float kFrontZ_ = 2.5f;

				const Vector3 kOffsets_[kMissileCount_] = {
					{-kSideX_,  kUpperY_,  -kFrontZ_},
					{-kSideX_,  kMiddleY_,  0.0f},
					{-kSideX_,  kLowerY_,   kFrontZ_},
					{ kSideX_,  kUpperY_,  -kFrontZ_},
					{ kSideX_,  kMiddleY_,  0.0f},
					{ kSideX_,  kLowerY_,   kFrontZ_},
				};

				const Vector3 kControlOffsets_[kMissileCount_] = {
					{ -10.0f,  14.0f,  0.0f }, // 左上 → 左上へ大きくふくらむ
					{ -16.0f,  0.0f,  0.0f }, // 左中 → 左へ大きくふくらむ
					{ -10.0f, -14.0f,  0.0f }, // 左下 → 左下へ大きくふくらむ
					{  10.0f,  14.0f,  0.0f }, // 右上 → 右上へ大きくふくらむ
					{  16.0f,  0.0f,  0.0f }, // 右中 → 右へ大きくふくらむ
					{  10.0f, -14.0f,  0.0f }, // 右下 → 右下へ大きくふくらむ
				};

				// 発射リクエストの登録数と消費位置を初期化する
				c.missileRequestCount_ = 0;
				c.missileRequestConsumeIndex_ = 0;

				for (int i = 0; i < kMissileCount_; ++i) {
					// ボスの発射口中心から左右上下にずらして6発分の発射位置を作る
					Vector3 spawnPos_ = muzzleCenter_;
					spawnPos_.x += kOffsets_[i].x;
					spawnPos_.y += kOffsets_[i].y;
					spawnPos_.z += kOffsets_[i].z;

					// ミサイル生成側に渡す発射リクエストを積む
					auto& req = c.missileRequests_[c.missileRequestCount_++];
					req.pos_ = spawnPos_;
					req.target_ = c.burstTargetSnap_;
					req.controlOffset_ = kControlOffsets_[i];
				}

				// チャージ完了状態にして、以降はリクエスト消費側に任せる
				c.burstCharged_ = true;
				c.missileCharging_ = false;
			}
		}
	}

	// 6発の発射要求を全部吐き終えたら、ミサイル攻撃を完了扱いにする
	if (c.burstCharged_ && c.missileRequestCount_ == 0) {
		c.burstCharged_ = false;
		c.burstTargetValid_ = false;
		c.missileCharging_ = false;
	}

	// ============================================================
	// Slash Charge / Fire Execute（Recover中のみ）
	// ============================================================
	if (c.slashCooldownT_ > 0.0f) {
		// スラッシュのクールダウンを0未満にならないように減らす
		c.slashCooldownT_ = std::max(0.0f, c.slashCooldownT_ - dt);
	}

	if (c.slashCharging_) {
		// スラッシュの発射位置をボスの現在位置から作る
		Vector3 p_ = boss.GetWorldPosition();
		p_.y += 10.0f;
		c.slashPos_ = p_;

		// チャージ中もプレイヤー位置を追い、発射方向の基準にする
		if (boss.GetPlayer()) {
			c.slashTarget_ = boss.GetPlayer()();
		} else {
			c.slashTarget_ = c.playerPos_;
		}

		if (auto* pm_ = TKM::ParticleManager::GetInstance()) {
			// チャージ進行率を0.0f～1.0fで求める
			float t = 1.0f - (c.slashChargeTimer_ / c.config_->slash_.chargeTime_);
			t = std::clamp(t, 0.0f, 1.0f);

			// プレイヤー方向へ少し前に出した位置を、予兆の中心にする
			Vector3 omenCenter_ = p_;
			Vector3 toTarget_ = c.slashTarget_ - p_;

			float lenSq_ =
				(toTarget_.x * toTarget_.x) +
				(toTarget_.y * toTarget_.y) +
				(toTarget_.z * toTarget_.z);

			// ターゲット方向が取れる場合だけ正規化して、予兆中心を前方へずらす
			if (lenSq_ > 0.0001f) {
				float invLen_ = 1.0f / std::sqrt(lenSq_);
				toTarget_.x *= invLen_;
				toTarget_.y *= invLen_;
				toTarget_.z *= invLen_;
				omenCenter_ += toTarget_ * 6.5f;
			}

			// 中心核は常に出し、後半ほど少し密度を上げる
			int coreCount_ = 2 + static_cast<int>(t * 4.0f);
			pm_->Emit("boss_slash_omen_core", omenCenter_, coreCount_);

			// 周囲一帯から大きく吸い込む演出
			int inwardCount_ = 18 + static_cast<int>(t * 22.0f);
			pm_->Emit("boss_slash_omen_inward", omenCenter_, inwardCount_);

			// 巨大な殻。序盤から出すが、後半で頻度を上げる
			if ((c.slashChargeFrame_ % 5) == 0) {
				pm_->Emit("boss_slash_omen_ring", omenCenter_, 2);
			}
			if (t > 0.45f && (c.slashChargeFrame_ % 3) == 0) {
				pm_->Emit("boss_slash_omen_ring", omenCenter_, 2);
			}

			// 殻に亀裂が走る。中盤以降かなり増やす
			if (t > 0.20f) {
				int crackCount_ = 3 + static_cast<int>((t - 0.20f) * 14.0f);
				pm_->Emit("boss_slash_omen_crack", omenCenter_, crackCount_);
			}

			// 終盤は内側から殻が膨れて破れそうになる
			if (t > 0.55f) {
				int pulseCount_ = 3 + static_cast<int>((t - 0.55f) * 18.0f);
				pm_->Emit("boss_slash_omen_pulse", omenCenter_, pulseCount_);
			}
		}

		// チャージ演出のフレーム数を進める
		++c.slashChargeFrame_;

		// チャージ残り時間を減らす
		c.slashChargeTimer_ -= dt;

		if (c.slashChargeTimer_ <= 0.0f) {
			// チャージ完了後、発射リクエストを立てる
			c.slashCharging_ = false;
			c.slashFireReq_ = true;

			// スラッシュのクールダウンを再設定する
			c.slashCooldownT_ = c.config_->slash_.cooldown_;
		}
	}

	// ------------------------------
	// 攻撃が終わるまでOrbitへ戻さない
	//   - ミサイル：6発の発射要求を吐き終えるまで
	//   - スラッシュ：溜め完了して発射要求を出すまで
	// ------------------------------
	const bool missileBusy_ =
		(c.missileCharging_) ||
		(c.burstCharged_) ||
		(c.missileRequestCount_ > 0);

	const bool slashBusy_ =
		(c.slashCharging_);

	// ミサイル処理が完全に終わっている場合は、残っている状態を整理する
	if (!missileBusy_) {
		c.burstCharged_ = false;
		c.missileRequestCount_ = 0;
		c.missileRequestConsumeIndex_ = 0;
	}

	// Recover時間が過ぎ、攻撃処理も終わっていればOrbitへ戻る
	if (c.timer_ >= c.config_->recover_.duration_ && !missileBusy_ && !slashBusy_) {
		c.ChangeState(BossController::State::Orbit);
	}
}

//=====================================================
// LaserWindup
//=====================================================
void BossLaserWindupState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);

	// レーザー溜め状態の経過時間を初期化する
	c.timer_ = 0.0f;

	// 現在の状態をLaserWindupに設定する
	c.state_ = BossController::State::LaserWindup;

	// レーザー予兆を有効にする
	c.laserActive_ = true;
	c.laserTelegraph_ = true;

	// 溜め開始時のボス位置をレーザー基準位置として固定する
	c.laserBasePos_ = c.posWork_;

	// レーザーの開始位置と終了位置を初期化する
	c.laserStartWS_ = c.laserBasePos_ + Vector3{ 0.0f, c.config_->laser_.muzzleYOffset_, 0.0f };
	c.laserEndWS_ = c.laserAimFixed_;
}

void BossLaserWindupState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);

	// 怒り状態が解除されていたら、レーザーを中止してRecoverへ戻る
	if (!c.rageActive_) {
		c.laserActive_ = false;
		c.laserTelegraph_ = false;
		c.ChangeState(BossController::State::Recover);
		return;
	}

	Vector3& pos = c.posWork_;

	// 溜め中はボス位置をレーザー基準位置に固定する
	pos = c.laserBasePos_;

	// 溜め進行率を計算する
	float t_ = (c.config_->laser_.windup_ > 0.0001f)
		? (c.timer_ / c.config_->laser_.windup_)
		: 1.0f;

	t_ = std::clamp(t_, 0.0f, 1.0f);

	// 終盤ほど揺れを強くする
	float ramp_ = t_ * t_;
	float amp_ = 0.18f * (0.2f + 0.8f * ramp_);

	// 溜め中の震えをX/Y方向に加える
	pos.x += std::sin(c.timer_ * 60.0f) * amp_;
	pos.y += std::sin(c.timer_ * 87.0f + 1.7f) * (amp_ * 0.55f);

	// レーザーの開始位置と終了位置を更新する
	c.laserStartWS_ = c.laserBasePos_ + Vector3{ 0.0f, c.config_->laser_.muzzleYOffset_, 0.0f };
	c.laserEndWS_ = c.laserAimFixed_;

	// 溜め時間が終わったら、レーザー発射へ移行する
	if (c.timer_ >= c.config_->laser_.windup_) {
		c.ChangeState(BossController::State::LaserFire);
	}
}

//=====================================================
// LaserFire
//=====================================================
void BossLaserFireState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);

	// レーザー発射状態の経過時間を初期化する
	c.timer_ = 0.0f;

	// 現在の状態をLaserFireに設定する
	c.state_ = BossController::State::LaserFire;

	// レーザー本体を有効化し、予兆表示は消す
	c.laserActive_ = true;
	c.laserTelegraph_ = false;

	// 発射中のボス位置はWindupで固定した基準位置を使う
	c.posWork_ = c.laserBasePos_;
}

void BossLaserFireState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);
	Enemy& boss = *c.boss_;

	// 怒り状態が解除されていたら、レーザーを中止してRecoverへ戻る
	if (!c.rageActive_) {
		c.laserActive_ = false;
		c.laserTelegraph_ = false;
		c.ChangeState(BossController::State::Recover);
		return;
	}

	// 発射中はレーザー本体のみ表示する
	c.laserActive_ = true;
	c.laserTelegraph_ = false;

	// ボス位置は基準位置に固定する
	c.posWork_ = c.laserBasePos_;

	// 基本の照準はWindup時点で決めた固定照準
	Vector3 aim_ = c.laserAimFixed_;

	if (boss.GetPlayer()) {
		// プレイヤーの現在位置を取得する
		Vector3 p_ = boss.GetPlayer()();

		// プレイヤー速度から少し先の位置を予測する
		Vector3 v_ = c.playerVel_;
		Vector3 pred_ = p_ + v_ * c.config_->predictLeadTime_;

		// 予測照準がアリーナ外に出ないように制限する
		pred_.x = std::clamp(pred_.x, c.arenaMin_.x, c.arenaMax_.x);
		pred_.y = std::clamp(pred_.y, c.arenaMin_.y, c.arenaMax_.y);
		pred_.z = std::clamp(pred_.z, c.arenaMin_.z, c.arenaMax_.z);

		// 固定照準から予測照準へ少しだけ寄せる
		aim_ = MyMath::Vector3Lerp(aim_, pred_, c.config_->laser_.trackStrength_);
	}

	// レーザーの開始位置と終了位置を更新する
	c.laserStartWS_ = c.laserBasePos_ + Vector3{ 0.0f, c.config_->laser_.muzzleYOffset_, 0.0f };
	c.laserEndWS_ = aim_;

	// 発射時間が終わったら、レーザー回復状態へ移行する
	if (c.timer_ >= c.config_->laser_.fire_) {
		c.laserCooldownT_ = 0.0f;
		c.ChangeState(BossController::State::LaserRecover);
	}
}

//=====================================================
// LaserRecover
//=====================================================
void BossLaserRecoverState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);

	// レーザー回復状態の経過時間を初期化する
	c.timer_ = 0.0f;

	// 現在の状態をLaserRecoverに設定する
	c.state_ = BossController::State::LaserRecover;

	// レーザー表示を完全に無効化する
	c.laserActive_ = false;
	c.laserTelegraph_ = false;
}

void BossLaserRecoverState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);
	Vector3& pos = c.posWork_;

	// 回復中はレーザー表示を出さない
	c.laserActive_ = false;
	c.laserTelegraph_ = false;

	// レーザー回復中は、ボスの位置を軌道の高さとZへスムーズに戻す
	pos = BossController::SmoothDamp(
		pos,
		Vector3{ pos.x, c.config_->orbit_.y_, c.config_->orbit_.z_ },
		0.18f,
		dt
	);

	// 回復時間が終わったら通常Recoverへ戻る
	if (c.timer_ >= c.config_->laser_.recover_) {
		c.ChangeState(BossController::State::Recover);
	}
}