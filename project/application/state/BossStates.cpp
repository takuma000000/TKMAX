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
			c.burstLeft_ = 0;
			c.burstTimer_ = 0.0f;
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
		// ミサイル（1回溜め→3連射）
		c.burstLeft_ = 3;
		c.burstTimer_ = 0.0f;
		c.burstTargetValid_ = false;
		c.burstCharged_ = false;
		// ミサイル攻撃のチャージ開始
		c.missileCharging_ = true;
		c.missileChargeTimer_ = c.missileChargeTime_;
		c.missileChargeFrame_ = 0;

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
	// Missile Burst Execute（Recover中のみ）
	// ============================================================
	if (c.missileCharging_ || c.burstLeft_ > 0 || (c.burstCharged_ && c.burstTimer_ > 0.0f)) { // ミサイル攻撃のチャージ中、または連射が残っている、または連射の待ちが残っている場合
		// ミサイルの発射位置とターゲットを毎フレーム更新
		Vector3 muzzlePos_ = boss.GetWorldPosition();
		muzzlePos_.y += c.missileMuzzleYOffset_; // ミサルの発射位置は、ボスの現在位置から少し上にオフセット
		c.missilePos_ = muzzlePos_; // ミサイルの発射位置を更新
		// ミサイルのターゲットは、チャージ中はスナップ位置、そうでない場合はプレイヤー位置を直接ターゲットにする
		Vector3 target_ = c.burstTargetValid_ ? c.burstTargetSnap_ : c.playerPos_;
		c.missileTarget_ = target_; // ミサイルのターゲット位置を更新

		if (!c.burstCharged_) { // まだチャージ中の場合
			auto* pm_ = TKM::ParticleManager::GetInstance();
			if (pm_) { // パーティクルマネージャーが存在する場合は、チャージ中のエフェクトを出す
				Vector3 p_ = muzzlePos_; // エフェクトの位置はミサイルの発射位置

				float t = 1.0f - (c.missileChargeTimer_ / c.missileChargeTime_); // チャージの進行度（0.0f～1.0f）
				t = std::clamp(t, 0.0f, 1.0f); // チャージの進行度を0.0f～1.0fにクランプ
				// チャージの進行度に応じて、エフェクトの量を増やす
				int inwardCount_ = 2 + (int)(t * 7);
				int crackleCount_ = 1 + (int)(t * 3);
				pm_->Emit("boss_windup_inward", p_, inwardCount_); // ミサイルの発射位置から内側に向かうエフェクト
				pm_->Emit("boss_windup_crackle", p_, crackleCount_); // ミサイルの発射位置でパチパチするエフェクト
				// チャージの進行度に応じて、エフェクトの発生頻度を上げる（後半ほど頻繁に出す）
				int step_ = (t < 0.55f) ? 4 : 2;
				if ((c.missileChargeFrame_ % step_) == 0) { // チャージのフレームカウンターがstep_の倍数のときに、チャージの進行度に応じたエフェクトを出す
					pm_->Emit("boss_windup_shell", p_, 1); // ミサイルの発射位置から外側に向かうエフェクト
				}
			}
			++c.missileChargeFrame_; // チャージのフレームカウンターをインクリメント

			c.missileChargeTimer_ -= dt; // チャージタイマーを減算していく
			if (c.missileChargeTimer_ <= 0.0f) { // チャージタイマーが0以下になったらチャージ完了
				c.burstCharged_ = true; // 連射が可能な状態になったことを示すフラグを立てる
				c.missileCharging_ = false; // チャージ完了 → チャージ中フラグを下ろす

				// 1発目即発射
				c.missileFireReq_ = true;
				c.burstLeft_--;
				c.burstTimer_ = c.burstInterval_;
			}
		} else {
			c.burstTimer_ -= dt; // 連射の待ちタイマーを減算していく

			if (c.burstTimer_ <= 0.0f) { // 連射の待ちタイマーが0以下になったら、次の弾を撃つかどうか判定
				// まだ撃つ弾が残ってる時だけ発射
				if (c.burstLeft_ > 0) {
					c.missileFireReq_ = true;
					c.burstLeft_--;
					c.burstTimer_ = c.burstInterval_;
				} else {
					// 撃ち終わってる：タイマーだけ終わらせる（固まり防止）
					c.burstTimer_ = 0.0f;
				}
			}
		}
	}
	// ミサイルが完全に終わったら状態をクリア（Recover抜け用）
	if (c.burstCharged_ && c.burstLeft_ <= 0 && c.burstTimer_ <= 0.0f) {
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
			float t = 1.0f - (c.slashChargeTimer_ / c.slashChargeTime_); // チャージの進行度（0.0f～1.0f）
			t = std::clamp(t, 0.0f, 1.0f); // チャージの進行度を0.0f～1.0fにクランプ
			// チャージの進行度に応じて、エフェクトの量を増やす
			int line_ = 2 + (int)(t * 8);
			int spark_ = 1 + (int)(t * 4);
			pm_->Emit("boss_slash_windup_line", p_, line_); // スラッシュの発射位置からプレイヤーに向かう線のエフェクト
			pm_->Emit("boss_slash_windup_spark", p_, spark_); // スラッシュの発射位置で火花が散るエフェクト

			if ((c.slashChargeFrame_ % 3) == 0) { // チャージのフレームカウンターが3の倍数のときに、チャージの進行度に応じたエフェクトを出す
				pm_->Emit("boss_slash_windup_arc", p_, 1); // スラッシュの発射位置から周囲に向かうアーク状のエフェクト
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
		(c.burstLeft_ > 0) ||
		(c.burstCharged_ && c.burstTimer_ > 0.0f); // 連射の待ちが残ってる
	// スラッシュはチャージ中だけ忙しいとみなす（発射要求を出したらもう忙しくない＝次の攻撃に移ってもいいとみなす）
	const bool slashBusy_ =
		(c.slashCharging_); // まだ溜め中

	// （安全）ミサイルが終わったら後始末しておく
	if (!missileBusy_) {
		c.burstCharged_ = false; // 連射が可能な状態フラグを下ろす
		c.burstTimer_ = 0.0f; // タイマーをリセットしておく
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