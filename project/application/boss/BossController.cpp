#include "BossController.h"
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void BossController::Initialize(const Vector3& arenaMin, const Vector3& arenaMax) {
	// アリーナ範囲設定
	arenaMin_ = arenaMin;
	arenaMax_ = arenaMax;

	state_ = State::Enter; // 初期状態
	timer_ = 0.0f; // タイマー初期化
	// 乱数初期化
	rng_.seed(std::random_device{}());
	// --- Rage Gauge init ---
	rageGauge_ = 0.0f; // 怒りゲージ初期化
	rageActive_ = false; // 怒りモード初期化s
	lastHpForRage_ = -1; // 前回HP初期化
	noDamageTime_ = 0.0f; // 無被ダメ時間初期化
	// laser
	laserCooldownT_ = 0.0f; // クールタイム初期化
	laserActive_ = false; // レーザー状態初期化
	laserTelegraph_ = false; // レーザー予告状態初期化
	// 予測系
	hasPrevPlayerPos_ = false; // 前フレーム位置無し
	prevPlayerPos_ = { 0.0f, 0.0f, 0.0f }; // 前フレーム位置初期化
	playerVel_ = { 0.0f, 0.0f, 0.0f }; // 速度初期化
}

void BossController::Reset() {
	state_ = State::Enter; // 初期状態
	timer_ = 0.0f; // タイマー初期化

	// --- Rage Gauge reset ---
	rageGauge_ = 0.0f; // 怒りゲージ初期化
	rageActive_ = false; // 怒りモード初期化
	lastHpForRage_ = -1; // 前回HP初期化
	noDamageTime_ = 0.0f; // 無被ダメ時間初期化
	// laser
	laserCooldownT_ = 0.0f; // クールタイム初期化
	laserActive_ = false; // レーザー状態初期化
	laserTelegraph_ = false; // レーザー予告状態初期化
	// 予測系もリセットしておくと安全
	hasPrevPlayerPos_ = false; // 前フレーム位置無し
	prevPlayerPos_ = { 0.0f, 0.0f, 0.0f }; // 前フレーム位置初期化
	playerVel_ = { 0.0f, 0.0f, 0.0f }; // 速度初期化
}

void BossController::Update(float dt, Enemy& boss) {
	timer_ += dt; // 状態タイマー更新

	// レーザークールタイム
	if (laserCooldownT_ > 0.0f) { // 残っているなら減少
		laserCooldownT_ = std::max(0.0f, laserCooldownT_ - dt);
	}

	// ============================================================
	// Rage Gauge Update（毎フレーム）
	// ============================================================
	if (lastHpForRage_ < 0) { // 初回セット
		lastHpForRage_ = boss.GetHP(); // 現在HP取得
		noDamageTime_ = 0.0f; // 無被ダメ時間初期化
	}
	int hpNow_ = boss.GetHP(); // 現在HP取得
	int dmg_ = std::max(0, lastHpForRage_ - hpNow_); // ダメージ量計算
	// 次回用に現在HP保存
	lastHpForRage_ = hpNow_;

	if (dmg_ > 0) { // ダメージを受けていたらゲージ増加
		rageGauge_ += static_cast<float>(dmg_) * rageGainPerHp_; // ゲージ増加
		noDamageTime_ = 0.0f; // 無被ダメ時間リセット
	} else {
		noDamageTime_ += dt; // 無被ダメ時間加算
		if (noDamageTime_ >= rageDecayDelay_) { // 減衰開始
			rageGauge_ -= rageDecayPerSec_ * dt; // ゲージ減少
		}
	}
	rageGauge_ = std::clamp(rageGauge_, 0.0f, 1.5f); // ゲージクランプ
	//// 怒りモード判定
	//if (!rageActive_) { // 怒りモードでなければ発動判定
	//	if (rageGauge_ >= rageOnThreshold_) { rageActive_ = true; } // 発動
	//} else {
	//	if (rageGauge_ <= rageOffThreshold_) { rageActive_ = false; } // 解除
	//}
	rageActive_ = false;

	// プレイヤー位置・速度更新
	if (boss.GetPlayer()) { // プレイヤー位置取得関数があるなら
		playerPos_ = boss.GetPlayer()(); // プレイヤー位置取得
	}
	// 速度計算
	if (hasPrevPlayerPos_) { // 前フレーム位置があるなら速度計算
		playerVel_ = (playerPos_ - prevPlayerPos_) * (1.0f / std::max(0.0001f, dt)); // 速度計算
	}
	// Y成分は不要なので0にする
	playerVel_.y = 0.0f;
	// 前フレーム位置保存
	prevPlayerPos_ = playerPos_;
	// フラグON
	hasPrevPlayerPos_ = true;
	Vector3 pos_ = boss.GetWorldPosition(); // 現在位置取得

	// 状態別更新
	switch (state_) { // 状態別更新
	case State::Enter:       UpdateEnter(dt, boss, pos_); break; // 侵入
	case State::Orbit:       UpdateOrbit(dt, boss, pos_, playerPos_); break; // 旋回
	case State::LaserWindup: UpdateLaserWindup(dt, boss, pos_, playerPos_); break; // レーザー予告
	case State::LaserFire:   UpdateLaserFire(dt, boss, pos_, playerPos_); break; // レーザー発射
	case State::LaserRecover:UpdateLaserRecover(dt, boss, pos_); break; // レーザー復帰
	case State::Recover:     UpdateRecover(dt, boss, pos_); break; // 復帰
	}

	auraActive_ = false; // オーラ無効化
	auraT_ = 0.0f; // オーラ時間初期化

	// Laser状態以外ではレーザー無効
	if (state_ != State::LaserWindup && state_ != State::LaserFire) { // Laser状態以外
		laserActive_ = false; // レーザー無効化
		laserTelegraph_ = false; // レーザー予告無効化
	}

	// ============================================================
	// Missile Burst Execute（毎フレーム）
	// 仕様：バースト開始時に1回だけ溜め → その後3発を一定間隔で撃つ
	// ============================================================
	if (burstLeft_ > 0) { // バースト中

		// 発射口位置は毎回更新（ボスが動いても自然）
		Vector3 muzzlePos_ = boss.GetWorldPosition();
		muzzlePos_.y += missileMuzzleYOffset_;
		missilePos_ = muzzlePos_;

		// ターゲットはバースト開始時点で固定されたものを使う
		Vector3 target_ = burstTargetValid_ ? burstTargetSnap_ : playerPos_;
		missileTarget_ = target_;

		// 1) まだチャージが終わってないなら、溜め演出だけやる（毎弾じゃない！）
		if (!burstCharged_) {
			auto* pm_ = TKM::ParticleManager::GetInstance();
			if (pm_) {
				Vector3 p_ = muzzlePos_;

				float t = 1.0f - (missileChargeTimer_ / missileChargeTime_); // 0→1
				t = std::clamp(t, 0.0f, 1.0f);

				int inwardCount_ = 2 + (int)(t * 7); // 2→9
				int crackleCount_ = 1 + (int)(t * 3); // 1→4
				pm_->Emit("boss_windup_inward", p_, inwardCount_);
				pm_->Emit("boss_windup_crackle", p_, crackleCount_);

				int step_ = (t < 0.55f) ? 4 : 2;
				if ((missileChargeFrame_ % step_) == 0) {
					pm_->Emit("boss_windup_shell", p_, 1);
				}
			}
			++missileChargeFrame_;

			missileChargeTimer_ -= dt;
			if (missileChargeTimer_ <= 0.0f) {
				// チャージ完了
				burstCharged_ = true;
				missileCharging_ = false;

				// ここで1発目を即発射（チャージ1回→3発の1発目）
				missileFireReq_ = true;
				burstLeft_--;
				burstTimer_ = burstInterval_;
			}
		}
		// 2) チャージ済みなら、これまで通り一定間隔で撃つ
		else {
			burstTimer_ -= dt;
			if (burstTimer_ <= 0.0f) {
				missileFireReq_ = true;
				burstLeft_--;
				burstTimer_ = burstInterval_;
			}
		}
	}
	// ============================================
	// Slash Charge / Fire Execute（毎フレーム）
	// ============================================
	if (slashCooldownT_ > 0.0f) { // クールタイム
		slashCooldownT_ = std::max(0.0f, slashCooldownT_ - dt); // 減少
	}
	// チャージ中
	if (slashCharging_) {
		// 溜め位置（ボス位置＋少し前）
		Vector3 p_ = boss.GetWorldPosition(); // ボス位置取得
		p_.y += missileMuzzleYOffset_; // 既存のYオフセットを流用
		slashPos_ = p_; // 発射位置設定

		// 狙い：発射時点のプレイヤー座標（ミサイルと同じ思想）
		if (boss.GetPlayer()) {
			slashTarget_ = boss.GetPlayer()();
		} else {
			slashTarget_ = playerPos_;
		}

		// スラッシュ用の溜めパーティクル（別グループ）
		if (auto* pm_ = TKM::ParticleManager::GetInstance()) {
			float t = 1.0f - (slashChargeTimer_ / slashChargeTime_); // 0→1
			t = std::clamp(t, 0.0f, 1.0f); // クランプ
			// “刃が形成される”感じ：線状 + 火花
			int line_ = 2 + (int)(t * 8);    // 2→10
			int spark_ = 1 + (int)(t * 4);   // 1→5
			pm_->Emit("boss_slash_windup_line", p_, line_);
			pm_->Emit("boss_slash_windup_spark", p_, spark_);
			// 輪郭（弧）をたまに出す
			if ((slashChargeFrame_ % 3) == 0) {
				pm_->Emit("boss_slash_windup_arc", p_, 1);
			}
		}
		++slashChargeFrame_; // フレームカウンタ増加
		slashChargeTimer_ -= dt; // 溜め時間減少
		if (slashChargeTimer_ <= 0.0f) { // slashChargeTimer_が0以下になったら
			// 溜め完了→発射
			slashCharging_ = false; // 溜め終了
			slashFireReq_ = true; // 発射リクエスト
			slashCooldownT_ = slashCooldown_; // クールタイムセット
		}
	}

	ClampToArena(pos_); // アリーナ内に位置制限
	boss.SetPosition(pos_); // 位置設定
	boss.SyncTransform(); // Transform同期
}

void BossController::ImGuiDebug(Enemy& boss) {
#ifdef USE_IMGUI
	ImGui::Begin("ボスコントローラ");
	static const char* kStateName_[] = {
		"登場",
		"旋回",
		"レーザー予告",
		"レーザー発射",
		"レーザー復帰",
		"復帰",
	};

	int si = static_cast<int>(state_);
	si = std::clamp(si, 0, (int)(sizeof(kStateName_) / sizeof(kStateName_[0])) - 1);
	ImGui::Text("状態: %s", kStateName_[si]);
	ImGui::Text("怒り: %s", rageActive_ ? "怒りモード" : "怒ってない");

	ImGui::Text("怒りゲージ: %.2f", rageGauge_);
	ImGui::ProgressBar(std::clamp(rageGauge_, 0.0f, 1.0f), ImVec2(0.0f, 0.0f));
	ImGui::Separator();
	ImGui::Text("状態タイマー: %.2f", timer_);
	ImGui::End();
#endif
}

bool BossController::ConsumeMissileFireRequest(Vector3& outPos, Vector3& outTarget, float& outSpeed, float& outCurveHeight, int& outDamage, int& outLifeFrame) {
	if (!missileFireReq_) { return false; } // リクエスト無し
	missileFireReq_ = false; // リクエスト消費
	// 出力セット
	outPos = missilePos_; // 発射位置
	outTarget = missileTarget_; // 目標位置
	outSpeed = missileSpeed_; // 速度
	outCurveHeight = missileCurveHeight_; // 曲線高さ
	outDamage = missileDamage_; // ダメージ
	outLifeFrame = missileLifeFrame_; // 寿命フレーム
	return true;
}

bool BossController::ConsumeSlashFireRequest(Vector3& outPos, Vector3& outTarget, float& outSpeed, int& outDamage, int& outLifeFrame) {
	if (!slashFireReq_) { return false; } // リクエスト無し
	slashFireReq_ = false; // リクエスト消費
	// 出力セット
	outPos = slashPos_; // 発射位置
	outTarget = slashTarget_; // 目標位置
	outSpeed = slashSpeed_; // 速度
	outDamage = slashDamage_; // ダメージ
	outLifeFrame = slashLifeFrame_; // 寿命フレーム
	return true;
}

void BossController::UpdateEnter(float dt, Enemy& boss, Vector3& pos) {
	const float targetZ_ = orbitZ_; // 目標Z座標
	const float speed_ = 18.0f; // 侵入速度
	// 目標位置へ移動
	pos.z = Approach(pos.z, targetZ_, speed_ * dt);
	pos.x = Approach(pos.x, 0.0f, 10.0f * dt);
	pos.y = Approach(pos.y, orbitY_, 10.0f * dt);

	if (std::abs(pos.z - targetZ_) < 0.05f) { // 目標Z到達で旋回へ
		ChangeState(State::Orbit); // 旋回へ
	}
}

void BossController::UpdateOrbit(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos) {
	// 目標：プレイヤー中心に旋回しつつY,Zは固定
	float t_ = timer_;
	float angle_ = t_ * orbitAngularSpeed_;
	float ox_ = std::cos(angle_) * orbitRadiusX_;
	float oy_ = std::sin(angle_ * 0.9f) * orbitRadiusY_;

	// 目標位置計算
	Vector3 target_;
	target_.x = playerPos.x * orbitPlayerInfluence_ + ox_;
	target_.y = orbitY_ + playerPos.y * 0.2f + oy_;
	target_.z = orbitZ_;

	// ふわっと移動
	pos = SmoothDamp(pos, target_, orbitFollow_, dt);

	// 攻撃移行判定
	if (timer_ >= orbitDuration_) {
		// -----------------------------
		// 通常時：ミサイル or スラッシュ
		// 怒り中：レーザー
		// -----------------------------
		if (!rageActive_) {

			// ===== 攻撃抽選（通常時）=====
			// スラッシュはクールタイムがある想定（無いなら canSlash_ は常に true でOK）
			const bool canSlash_ = (slashCooldownT_ <= 0.0f);

			// 0.0〜1.0
			std::uniform_real_distribution<float> u01(0.0f, 1.0f);

			// スラッシュ割合（好みで調整）
			const float slashRate_ = canSlash_ ? 0.45f : 0.0f;
			const bool doSlash_ = (u01(rng_) < slashRate_);

			Vector3 muzzlePos_ = pos;
			muzzlePos_.y += missileMuzzleYOffset_;

			if (doSlash_) {
				// -----------------------------
				// 通常時：スラッシュ（溜め→発射）
				// -----------------------------
				slashCharging_ = true;
				slashChargeTimer_ = slashChargeTime_;
				slashChargeFrame_ = 0;

				// 発射時点のplayer座標を固定（ミサイルと同じ思想）
				if (boss.GetPlayer()) {
					slashTargetSnap_ = boss.GetPlayer()();
					slashTargetValid_ = true;
				} else {
					slashTargetSnap_ = playerPos;
					slashTargetValid_ = true;
				}

				// クールタイム開始（溜め開始時点でも、発射後でもどっちでもOK）
				slashCooldownT_ = slashCooldown_;

				ChangeState(State::Recover);
				return;
			}

			// -----------------------------
			// 通常時：ミサイル（1回溜め→3連射）
			// -----------------------------
			// --- 3連射開始（1回だけ溜めてから撃つ）---
			burstLeft_ = 3;              // 3発セット
			burstTimer_ = 0.0f;          // チャージ完了後、すぐ1発目に使う
			burstTargetValid_ = false;   // ターゲット未固定
			burstCharged_ = false;       // このバーストはまだチャージしてない
			missileCharging_ = true;     // バースト開始時にだけチャージ開始
			missileChargeTimer_ = missileChargeTime_;
			missileChargeFrame_ = 0;

			if (boss.GetPlayer()) {
				burstTargetSnap_ = boss.GetPlayer()(); // 発射開始時点のplayer座標を固定
				burstTargetValid_ = true;
			} else {
				burstTargetSnap_ = playerPos;
				burstTargetValid_ = true;
			}

			ChangeState(State::Recover);
			return;
		}

		// -----------------------------
		// 怒りモード時のみレーザーへ（ここはそのまま）
		// -----------------------------
		laserAimFixed_ = playerPos + playerVel_ * predictLeadTime_;
		laserAimFixed_.x = std::clamp(laserAimFixed_.x, arenaMin_.x, arenaMax_.x);
		laserAimFixed_.y = std::clamp(laserAimFixed_.y, arenaMin_.y, arenaMax_.y);
		laserAimFixed_.z = std::clamp(laserAimFixed_.z, arenaMin_.z, arenaMax_.z);

		laserBasePos_ = pos;
		ChangeState(State::LaserWindup);
		return;
	}
}

void BossController::UpdateRecover(float dt, Enemy& boss, Vector3& pos) {
	// 目標：軌道(Orbit)の高さとZへ戻す（Xは今のままでもOK）
	Vector3 target_{ pos.x, orbitY_, orbitZ_ };

	// ふわっと戻す（Orbitより少し強めでもいい）
	pos = SmoothDamp(pos, target_, 0.18f, dt);

	// 一定時間でOrbitへ
	if (timer_ >= recoverDuration_) { // 時間到達
		ChangeState(State::Orbit); // 旋回へ
	}
}

void BossController::UpdateLaserRecover(float dt, Enemy& boss, Vector3& pos) {
	laserActive_ = false; // レーザー無効化
	laserTelegraph_ = false; // レーザー無効化
	// 目標：軌道(Orbit)の高さとZへ戻す（Xは今のままでもOK）
	pos = SmoothDamp(pos, Vector3{ pos.x, orbitY_, orbitZ_ }, 0.18f, dt);

	if (timer_ >= laserRecover_) { // 時間到達
		ChangeState(State::Recover); // 復帰へ
	}
}

// ============================
// Laser（怒り中のみ）
// ============================
void BossController::UpdateLaserWindup(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos) {
	if (!rageActive_) { // 怒りモード解除されたら中断
		laserActive_ = false;
		laserTelegraph_ = false;
		ChangeState(State::Recover);
		return;
	}

	if (timer_ <= dt) { // 初回のみ
		laserActive_ = true; // レーザー有効化
		laserTelegraph_ = true; // レーザー予告有効化
		laserBasePos_ = pos; // 基準位置保存
		// 狙い位置計算
		laserStartWS_ = laserBasePos_ + Vector3{ 0.0f, laserMuzzleYOffset_, 0.0f };
		// 予測込みで狙い位置計算
		laserEndWS_ = laserAimFixed_;
	}

	pos = laserBasePos_; // 基準位置に固定
	float t_ = (laserWindup_ > 0.0001f) ? (timer_ / laserWindup_) : 1.0f; // 0～1
	t_ = std::clamp(t_, 0.0f, 1.0f); // クランプ
	float ramp_ = t_ * t_; // イーズイン風
	float amp_ = 0.18f * (0.2f + 0.8f * ramp_); // 揺れ振幅計算
	pos.x += std::sin(timer_ * 60.0f) * amp_; // 横揺れ
	pos.y += std::sin(timer_ * 87.0f + 1.7f) * (amp_ * 0.55f); // 縦揺れ

	laserStartWS_ = laserBasePos_ + Vector3{ 0.0f, laserMuzzleYOffset_, 0.0f }; // レーザー開始位置
	laserEndWS_ = laserAimFixed_; // レーザー終了位置

	if (timer_ >= laserWindup_) { // 時間到達
		ChangeState(State::LaserFire); // 発射へ
	}
}

void BossController::UpdateLaserFire(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos) {
	if (!rageActive_) {
		laserActive_ = false;
		laserTelegraph_ = false;
		ChangeState(State::Recover);
		return;
	}

	laserActive_ = true; // レーザー有効化
	laserTelegraph_ = false; // レーザー予告無効化
	pos = laserBasePos_; // 基準位置に固定
	// 狙い位置計算（追尾）
	Vector3 aim_ = laserAimFixed_;
	if (boss.GetPlayer()) { // プレイヤー位置取得関数があるなら
		Vector3 p_ = boss.GetPlayer()(); // プレイヤー位置取得
		Vector3 v_ = playerVel_; // プレイヤー速度取得
		Vector3 pred_ = p_ + v_ * predictLeadTime_; // 予測位置計算
		// アリーナ内にクランプ
		pred_.x = std::clamp(pred_.x, arenaMin_.x, arenaMax_.x);
		pred_.y = std::clamp(pred_.y, arenaMin_.y, arenaMax_.y);
		pred_.z = std::clamp(pred_.z, arenaMin_.z, arenaMax_.z);
		// 追尾
		aim_ = MyMath::Vector3Lerp(aim_, pred_, laserTrackStrength_);
	}
	laserStartWS_ = laserBasePos_ + Vector3{ 0.0f, laserMuzzleYOffset_, 0.0f }; // レーザー開始位置
	laserEndWS_ = aim_; // レーザー終了位置

	if (timer_ >= laserFire_) { // 時間到達
		// 100%出すためクールタイム無し
		laserCooldownT_ = 0.0f;
		ChangeState(State::LaserRecover); // 復帰へ
	}
}

void BossController::ChangeState(State s) {
	state_ = s; // 状態変更
	timer_ = 0.0f; // タイマーリセット
}

void BossController::ClampToArena(Vector3& p) {
	// アリーナ内にクランプ
	p.x = std::clamp(p.x, arenaMin_.x, arenaMax_.x);
	p.y = std::clamp(p.y, arenaMin_.y, arenaMax_.y);
	p.z = std::clamp(p.z, arenaMin_.z, arenaMax_.z);
}

float BossController::Approach(float v, float target, float delta) {
	if (v < target) { return std::min(v + delta, target); } // 上方向
	return std::max(v - delta, target); // 下方向
}

Vector3 BossController::SmoothDamp(const Vector3& from, const Vector3& to, float factor, float dt) {
	float k = 1.0f - std::pow(1.0f - factor, dt * 60.0f); // フレームレート補正
	return MyMath::Vector3Lerp(from, to, k); // 補間計算
}