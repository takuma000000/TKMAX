#include "BossStates.h"
#include "BossController.h"
#include <cmath>
#include <algorithm>

static BossController& AsBoss_(TKM::IStateContext& ctx) {
	return static_cast<BossController&>(ctx); // 状態コンテキストをボスコントローラーにキャスト
}

//=====================================================
// Enter
//=====================================================
void BossEnterState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト
	c.timer_ = 0.0f; // 予備動作の経過時間初期化
	c.state_ = BossController::State::Enter; // 状態を Enter に設定
}

void BossEnterState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト
	Enemy& boss = *c.boss_; // ボス敵オブジェクトへの参照
	Vector3& pos = c.posWork_; // ボスの位置ワーク（実際の位置は boss.GetWorldPosition() で取得）

	const float targetZ_ = c.orbitZ_; // 予備動作中の目標Z座標
	const float speed_ = 18.0f; // 予備動作中の移動速度（Z方向）

	// 予備動作：Z方向に近づきつつ、X/Yはゆっくり中央へ
	pos.z = BossController::Approach(pos.z, targetZ_, speed_ * dt);
	pos.x = BossController::Approach(pos.x, 0.0f, 10.0f * dt);
	pos.y = BossController::Approach(pos.y, c.orbitY_, 10.0f * dt);

	if (std::abs(pos.z - targetZ_) < 0.05f) { // 予備動作完了判定（Zが目標に十分近づいたら）
		c.ChangeState(BossController::State::Orbit); // 予備動作完了 → 軌道移動へ遷移
	}
}

//=====================================================
// Orbit
//=====================================================
void BossOrbitState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト
	c.timer_ = 0.0f; // 軌道移動開始の経過時間初期化
	c.state_ = BossController::State::Orbit; // 状態を Orbit に設定
}

void BossOrbitState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト
	Enemy& boss = *c.boss_; // ボス敵オブジェクトへの参照
	Vector3& pos = c.posWork_; // ボスの位置ワーク（実際の位置は boss.GetWorldPosition() で取得）
	const Vector3& playerPos = c.playerPos_; // プレイヤー位置（毎フレーム更新される）

	// -----------------------------
	float t_ = c.timer_; // 軌道移動の経過時間
	float angle_ = t_ * c.orbitAngularSpeed_; // 軌道移動の角度（時間経過で増加）
	float ox_ = std::cos(angle_) * c.orbitRadiusX_; // 円軌道のXオフセット
	float oy_ = std::sin(angle_ * 0.9f) * c.orbitRadiusY_; // 円軌道のYオフセット（角速度を少し変えて楕円っぽく）
	// -----------------------------

	// 目標位置：プレイヤーに少し引き寄せつつ、軌道オフセットも加える
	Vector3 target_;
	target_.x = playerPos.x * c.orbitPlayerInfluence_ + ox_; // プレイヤー位置を少し反映 + 軌道オフセット
	target_.y = c.orbitY_ + playerPos.y * 0.2f + oy_; // 基準高さ + プレイヤー位置を少し反映 + 軌道オフセット
	target_.z = c.orbitZ_; // Zは常に一定（軌道の半径）
	// 現在位置から目標位置へスムーズに移動
	pos = BossController::SmoothDamp(pos, target_, c.orbitFollow_, dt);

	if (c.timer_ < c.orbitDuration_) { return; } // 軌道移動の経過時間が一定に満たない場合は攻撃せず、引き続き軌道移動を続ける

	// -----------------------------
	// 通常時：ミサイル or スラッシュ
	// 怒り中：レーザー
	// -----------------------------
	if (!c.rageActive_) { // 通常時

		const bool canSlash_ = (c.slashCooldownT_ <= 0.0f); // スラッシュ攻撃がクールダウン中でないか
		std::uniform_real_distribution<float> u01(0.0f, 1.0f); // スラッシュ攻撃の選択率（クールダウン中は0%、そうでない場合は45%）

		const float slashRate_ = canSlash_ ? 0.45f : 0.0f; // スラッシュ攻撃の選択率（クールダウン中は0%、そうでない場合は45%）
		const bool doSlash_ = (u01(c.rng_) < slashRate_); // スラッシュ攻撃を行うかどうかの判定

		if (doSlash_) { // スラッシュ攻撃を選択した場合
			// スラッシュ（溜め→発射）
			c.slashCharging_ = true; // スラッシュ攻撃のチャージ開始
			c.slashChargeTimer_ = c.slashChargeTime_; // スラッシュ攻撃のチャージタイマーを初期化
			c.slashChargeFrame_ = 0; // スラッシュ攻撃のチャージフレームカウンターを初期化

			if (boss.GetPlayer()) { // プレイヤー位置のスナップを取得（プレイヤーが存在する場合）
				c.slashTargetSnap_ = boss.GetPlayer()(); // プレイヤー位置をスナップ
				c.slashTargetValid_ = true; // スラッシュ攻撃のターゲットが有効であることを示すフラグを立てる
			} else { // プレイヤー位置のスナップを取得（プレイヤーが存在しない場合は、現在のプレイヤー位置をスナップ）
				c.slashTargetSnap_ = playerPos; // プレイヤー位置をスナップ
				c.slashTargetValid_ = true; // スラッシュ攻撃のターゲットが有効であることを示すフラグを立てる
			}
			// スラッシュ攻撃のクールダウンタイマーを初期化
			c.slashCooldownT_ = c.slashCooldown_;

			// --- スラッシュ選択時：ミサイル系は完全に止める（混在防止）---
			c.burstTargetValid_ = false;
			c.burstCharged_ = false;
			c.missileCharging_ = false;
			c.missileChargeTimer_ = 0.0f;
			c.missileChargeFrame_ = 0;
			// ミサイル発射リクエストも消費しておく
			c.ChangeState(BossController::State::Recover);
			return;
		}

		// --- ミサイル選択時：スラッシュ系は完全に止める（混在防止）---
		c.slashCharging_ = false;
		c.slashChargeTimer_ = 0.0f;
		c.slashChargeFrame_ = 0;
		c.slashFireReq_ = false;
		// ミサイル（1回溜め→6方向同時発射）
		c.burstTargetValid_ = false;
		c.burstCharged_ = false;
		c.missileCharging_ = true;
		c.missileChargeTimer_ = c.missileChargeTime_;
		c.missileChargeFrame_ = 0;
		c.missileRequestCount_ = 0;
		c.missileRequestConsumeIndex_ = 0;

		if (boss.GetPlayer()) { // プレイヤー位置のスナップを取得（プレイヤーが存在する場合）
			c.burstTargetSnap_ = boss.GetPlayer()(); // プレイヤー位置をスナップ
			c.burstTargetValid_ = true; // ミサイル攻撃のターゲットが有効であることを示すフラグを立てる
		} else {
			c.burstTargetSnap_ = playerPos; // プレイヤー位置をスナップ
			c.burstTargetValid_ = true; // ミサイル攻撃のターゲットが有効であることを示すフラグを立てる
		}

		c.ChangeState(BossController::State::Recover);
		return;
	}

	// 怒り中のみレーザーへ
	c.laserAimFixed_ = playerPos + c.playerVel_ * c.predictLeadTime_;
	// レーザーの照準はアリーナ内にクランプしておく（当たり判定がアリーナ外に出ないように）
	c.laserAimFixed_.x = std::clamp(c.laserAimFixed_.x, c.arenaMin_.x, c.arenaMax_.x);
	c.laserAimFixed_.y = std::clamp(c.laserAimFixed_.y, c.arenaMin_.y, c.arenaMax_.y);
	c.laserAimFixed_.z = std::clamp(c.laserAimFixed_.z, c.arenaMin_.z, c.arenaMax_.z);
	// レーザーの基準位置は、ボスの現在位置から少し前方（プレイヤー側）に出す
	c.laserBasePos_ = pos;
	// レーザーの基準位置をプレイヤー側に少しオフセット（Z方向に前方）する
	c.ChangeState(BossController::State::LaserWindup);
}

//=====================================================
// Recover（ここに Burst / Slash charge を移植）
//=====================================================
void BossRecoverState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト
	c.timer_ = 0.0f; // 回復状態開始の経過時間初期化
	c.state_ = BossController::State::Recover; // 状態を Recover に設定
}

void BossRecoverState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト
	Enemy& boss = *c.boss_; // ボス敵オブジェクトへの参照
	Vector3& pos = c.posWork_; // ボスの位置ワーク（実際の位置は boss.GetWorldPosition() で取得）

	// 目標：Orbitの高さとZへ戻す
	Vector3 target_{ pos.x, c.orbitY_, c.orbitZ_ };
	pos = BossController::SmoothDamp(pos, target_, 0.18f, dt); // スムーズに軌道の高さとZへ戻す

	// ============================================================
	// Missile Charge / 6-way Fire Execute（Recover中のみ）
	// ============================================================
	if (c.missileCharging_ || c.burstCharged_) {
		Vector3 muzzleCenter_ = boss.GetWorldPosition();
		muzzleCenter_.y += c.missileMuzzleYOffset_;

		if (c.missileCharging_) {
			auto* pm_ = TKM::ParticleManager::GetInstance();
			if (pm_) {
				Vector3 p_ = muzzleCenter_;

				float t = 1.0f - (c.missileChargeTimer_ / c.missileChargeTime_);
				t = std::clamp(t, 0.0f, 1.0f);

				int inwardCount_ = 2 + (int)(t * 7);
				int crackleCount_ = 1 + (int)(t * 3);
				pm_->Emit("boss_windup_inward", p_, inwardCount_);
				pm_->Emit("boss_windup_crackle", p_, crackleCount_);

				int step_ = (t < 0.55f) ? 4 : 2;
				if ((c.missileChargeFrame_ % step_) == 0) {
					pm_->Emit("boss_windup_shell", p_, 1);
				}
			}
			++c.missileChargeFrame_;

			c.missileChargeTimer_ -= dt;
			if (c.missileChargeTimer_ <= 0.0f) {
				// 発射瞬間のplayer座標を固定
				if (boss.GetPlayer()) {
					c.burstTargetSnap_ = boss.GetPlayer()();
				} else {
					c.burstTargetSnap_ = c.playerPos_;
				}
				c.burstTargetValid_ = true;

				// 左3発・右3発で固定配置
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

				c.missileRequestCount_ = 0;
				c.missileRequestConsumeIndex_ = 0;

				for (int i = 0; i < kMissileCount_; ++i) {
					Vector3 spawnPos_ = muzzleCenter_;
					spawnPos_.x += kOffsets_[i].x;
					spawnPos_.y += kOffsets_[i].y;
					spawnPos_.z += kOffsets_[i].z;

					auto& req = c.missileRequests_[c.missileRequestCount_++];
					req.pos_ = spawnPos_;
					req.target_ = c.burstTargetSnap_;
					req.controlOffset_ = kControlOffsets_[i];
				}

				c.burstCharged_ = true;
				c.missileCharging_ = false;
			}
		}
	}

	// 6発の発射要求を全部吐き終えたら完了
	if (c.burstCharged_ && c.missileRequestCount_ == 0) {
		c.burstCharged_ = false;
		c.burstTargetValid_ = false;
		c.missileCharging_ = false;
	}

	// ============================================================
	// Slash Charge / Fire Execute（Recover中のみ）
	// ============================================================
	if (c.slashCooldownT_ > 0.0f) { // スラッシュ攻撃のクールダウンタイマーが残っている場合は、タイマーを減算していく
		c.slashCooldownT_ = std::max(0.0f, c.slashCooldownT_ - dt); // クールダウンタイマーを0以下にならないように減算
	}

	if (c.slashCharging_) { // スラッシュ攻撃のチャージ中の場合
		Vector3 p_ = boss.GetWorldPosition(); // スラッシュの発射位置は、ボスの現在位置から少し前方（プレイヤー側）に出す
		p_.y += c.missileMuzzleYOffset_; // スラッシュの発射位置は、ボスの現在位置から少し上にオフセット（ミサイルと同じ高さ）
		c.slashPos_ = p_; // スラッシュの発射位置を更新

		if (boss.GetPlayer()) { // スラッシュのターゲットは、チャージ中はスナップ位置、そうでない場合はプレイヤー位置を直接ターゲットにする（プレイヤーが存在する場合）
			c.slashTarget_ = boss.GetPlayer()(); // プレイヤー位置を直接ターゲットにする
		} else {
			c.slashTarget_ = c.playerPos_; // プレイヤー位置を直接ターゲットにする（プレイヤーが存在しない場合は、現在のプレイヤー位置をターゲットにする）
		}

		if (auto* pm_ = TKM::ParticleManager::GetInstance()) { // パーティクルマネージャーが存在する場合は、チャージ中のエフェクトを出す
			float t = 1.0f - (c.slashChargeTimer_ / c.slashChargeTime_);
			t = std::clamp(t, 0.0f, 1.0f);

			// プレイヤー方向へ少し前に出した位置を、予兆の中心にする
			Vector3 omenCenter_ = p_;
			Vector3 toTarget_ = c.slashTarget_ - p_;
			float lenSq_ =
				(toTarget_.x * toTarget_.x) +
				(toTarget_.y * toTarget_.y) +
				(toTarget_.z * toTarget_.z);

			if (lenSq_ > 0.0001f) {
				float invLen_ = 1.0f / std::sqrt(lenSq_);
				toTarget_.x *= invLen_;
				toTarget_.y *= invLen_;
				toTarget_.z *= invLen_;
				omenCenter_ += toTarget_ * 6.5f;
			}

			// 中心核は常に出す。後半ほど少し密度を上げる
			int coreCount_ = 2 + static_cast<int>(t * 4.0f);
			pm_->Emit("boss_slash_omen_core", omenCenter_, coreCount_);

			// 周囲一帯から大きく吸い込む
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

		++c.slashChargeFrame_; // チャージのフレームカウンターをインクリメント
		c.slashChargeTimer_ -= dt; // チャージタイマーを減算していく

		if (c.slashChargeTimer_ <= 0.0f) { // チャージタイマーが0以下になったらチャージ完了
			c.slashCharging_ = false; // チャージ完了 → チャージ中フラグを下ろす
			c.slashFireReq_ = true; // スラッシュ発射リクエストを立てる
			c.slashCooldownT_ = c.slashCooldown_; // スラッシュ攻撃のクールダウンタイマーを初期化
		}
	}

	// ------------------------------
	// 攻撃が終わるまでOrbitへ戻さない
	//   - ミサイル：3連射が終わるまで
	//   - スラッシュ：溜め完了（発射要求発行）まで
	// ------------------------------
	// どちらもチャージ中はもちろん忙しいし、ミサイルは連射の待ちも残ってると忙しいとみなす
	const bool missileBusy_ =
		(c.missileCharging_) ||
		(c.burstCharged_) ||
		(c.missileRequestCount_ > 0); // まだチャージ中、または連射の待ちが残っている
	// スラッシュはチャージ中だけ忙しいとみなす（発射要求を出したらもう忙しくない＝次の攻撃に移ってもいいとみなす）
	const bool slashBusy_ =
		(c.slashCharging_); // まだ溜め中

	// 攻撃が完全に終わっている場合は、次の攻撃に移るための準備をする
	if (!missileBusy_) {
		c.burstCharged_ = false;
		c.missileRequestCount_ = 0;
		c.missileRequestConsumeIndex_ = 0;
	}

	if (c.timer_ >= c.recoverDuration_ && !missileBusy_ && !slashBusy_) { // 回復状態の経過時間が一定を超えていて、かつミサイルもスラッシュも忙しくない（攻撃が完全に終わっている）場合
		c.ChangeState(BossController::State::Orbit); // 回復状態終了 → 軌道移動へ遷移
	}
}

//=====================================================
// LaserWindup
//=====================================================
void BossLaserWindupState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト
	c.timer_ = 0.0f; // レーザー溜め開始の経過時間初期化
	c.state_ = BossController::State::LaserWindup; // 状態を LaserWindup に設定

	// 初回処理をEnterに寄せる（元の timer_<=dt 相当）
	c.laserActive_ = true;
	c.laserTelegraph_ = true;
	c.laserBasePos_ = c.posWork_;
	// レーザーの開始位置と終了位置を初期化
	c.laserStartWS_ = c.laserBasePos_ + Vector3{ 0.0f, c.laserMuzzleYOffset_, 0.0f };
	c.laserEndWS_ = c.laserAimFixed_;
}

void BossLaserWindupState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト

	if (!c.rageActive_) { // 怒りが解除されていたら、レーザー攻撃を中止してRecoverへ
		c.laserActive_ = false;
		c.laserTelegraph_ = false;
		c.ChangeState(BossController::State::Recover); // レーザー攻撃中止 → 回復状態へ遷移
		return;
	}
	// レーザーの照準はアリーナ内にクランプしておく（当たり判定がアリーナ外に出ないように）
	Vector3& pos = c.posWork_;
	// レーザーの基準位置は、ボスの現在位置から少し前方（プレイヤー側）に出す
	pos = c.laserBasePos_;
	// レーザーの基準位置をプレイヤー側に少しオフセット（Z方向に前方）する
	float t_ = (c.laserWindup_ > 0.0001f) ? (c.timer_ / c.laserWindup_) : 1.0f;
	t_ = std::clamp(t_, 0.0f, 1.0f);
	float ramp_ = t_ * t_;
	float amp_ = 0.18f * (0.2f + 0.8f * ramp_);
	// レーザーの基準位置をプレイヤー側に少しオフセット（Z方向に前方）する
	pos.x += std::sin(c.timer_ * 60.0f) * amp_;
	pos.y += std::sin(c.timer_ * 87.0f + 1.7f) * (amp_ * 0.55f);
	// レーザーの開始位置と終了位置を更新
	c.laserStartWS_ = c.laserBasePos_ + Vector3{ 0.0f, c.laserMuzzleYOffset_, 0.0f };
	c.laserEndWS_ = c.laserAimFixed_;

	if (c.timer_ >= c.laserWindup_) { // レーザーの溜め時間が経過したら、レーザー発射へ遷移
		c.ChangeState(BossController::State::LaserFire); // レーザー溜め完了 → レーザー発射へ遷移
	}
}

//=====================================================
// LaserFire
//=====================================================
void BossLaserFireState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト
	c.timer_ = 0.0f; // レーザー発射開始の経過時間初期化
	c.state_ = BossController::State::LaserFire; // 状態を LaserFire に設定

	c.laserActive_ = true; // レーザー攻撃をアクティブにする
	c.laserTelegraph_ = false; // レーザーのテレグラフは消す（溜めと同時には出さない）

	// pos固定基準は windup で作った laserBasePos_ を使う
	c.posWork_ = c.laserBasePos_;
}

void BossLaserFireState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト
	Enemy& boss = *c.boss_; // ボス敵オブジェクトへの参照

	if (!c.rageActive_) { // 怒りが解除されていたら、レーザー攻撃を中止してRecoverへ
		c.laserActive_ = false;
		c.laserTelegraph_ = false;
		c.ChangeState(BossController::State::Recover); // レーザー攻撃中止 → 回復状態へ遷移
		return;
	}

	c.laserActive_ = true;
	c.laserTelegraph_ = false;

	c.posWork_ = c.laserBasePos_;

	Vector3 aim_ = c.laserAimFixed_; // レーザーの照準は、基本的には溜め段階で決めた位置を固定するが、プレイヤーが動いている場合は少し追尾するようにする
	if (boss.GetPlayer()) { // プレイヤーが存在する場合は、プレイヤーの位置と速度を考慮してレーザーの照準を少し追尾する
		// レーザーの照準は、プレイヤーの現在位置 + プレイヤーの速度 * 予測時間 で計算する
		Vector3 p_ = boss.GetPlayer()();
		Vector3 v_ = c.playerVel_;
		Vector3 pred_ = p_ + v_ * c.predictLeadTime_;
		// レーザーの照準はアリーナ内にクランプしておく（当たり判定がアリーナ外に出ないように）
		pred_.x = std::clamp(pred_.x, c.arenaMin_.x, c.arenaMax_.x);
		pred_.y = std::clamp(pred_.y, c.arenaMin_.y, c.arenaMax_.y);
		pred_.z = std::clamp(pred_.z, c.arenaMin_.z, c.arenaMax_.z);
		// レーザーの照準を、固定照準と予測照準の間で線形補間する（追尾の強さは c.laserTrackStrength_ で調整）
		aim_ = MyMath::Vector3Lerp(aim_, pred_, c.laserTrackStrength_);
	}

	c.laserStartWS_ = c.laserBasePos_ + Vector3{ 0.0f, c.laserMuzzleYOffset_, 0.0f };
	c.laserEndWS_ = aim_;

	if (c.timer_ >= c.laserFire_) { // レーザーの発射時間が経過したら、レーザー回復へ遷移
		c.laserCooldownT_ = 0.0f; // レーザー攻撃のクールダウンタイマーを初期化（次のレーザー攻撃までの待ち時間に使う）
		c.ChangeState(BossController::State::LaserRecover); // レーザー発射完了 → レーザー回復へ遷移
	}
}

//=====================================================
// LaserRecover
//=====================================================
void BossLaserRecoverState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト
	c.timer_ = 0.0f; // レーザー回復開始の経過時間初期化
	c.state_ = BossController::State::LaserRecover; // 状態を LaserRecover に設定

	c.laserActive_ = false;
	c.laserTelegraph_ = false;
}

void BossLaserRecoverState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx); // 状態コンテキストをボスコントローラーにキャスト
	Vector3& pos = c.posWork_; // ボスの位置ワーク（実際の位置は boss.GetWorldPosition() で取得）

	c.laserActive_ = false;
	c.laserTelegraph_ = false;
	// レーザー回復中は、ボスの位置を軌道の高さとZへスムーズに戻す
	pos = BossController::SmoothDamp(pos, Vector3{ pos.x, c.orbitY_, c.orbitZ_ }, 0.18f, dt);

	if (c.timer_ >= c.laserRecover_) { // レーザー回復の経過時間が一定を超えたら、次の行動へ遷移
		c.ChangeState(BossController::State::Recover); // レーザー回復完了 → 回復状態へ遷移
	}
}