#include "BossController.h"
#include "BossStates.h"
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void BossController::Initialize(const Vector3& arenaMin, const Vector3& arenaMax) {
	// アリーナ範囲設定
	arenaMin_ = arenaMin;
	arenaMax_ = arenaMax;

	// コンフィグから各種パラメータ読み込み
	predictLeadTime_ = config_->predictLeadTime_;
	rageGainPerHp_ = config_->rage_.gainPerHp_;
	rageDecayDelay_ = config_->rage_.decayDelay_;
	rageDecayPerSec_ = config_->rage_.decayPerSec_;
	rageOnThreshold_ = config_->rage_.onThreshold_;
	rageOffThreshold_ = config_->rage_.offThreshold_;
	orbitZ_ = config_->orbit_.z_;
	orbitY_ = config_->orbit_.y_;
	orbitRadiusX_ = config_->orbit_.radiusX_;
	orbitRadiusY_ = config_->orbit_.radiusY_;
	orbitAngularSpeed_ = config_->orbit_.angularSpeed_;
	orbitPlayerInfluence_ = config_->orbit_.playerInfluence_;
	orbitFollow_ = config_->orbit_.follow_;
	orbitDuration_ = config_->orbit_.duration_;
	recoverDuration_ = config_->recover_.duration_;
	missileMuzzleYOffset_ = config_->missile_.muzzleYOffset_;
	missileChargeTime_ = config_->missile_.chargeTime_;
	burstLeft_ = 0;
	burstInterval_ = config_->missile_.burstInterval_;
	missileSpeed_ = config_->missile_.speed_;
	missileCurveHeight_ = config_->missile_.curveHeight_;
	missileDamage_ = config_->missile_.damage_;
	missileLifeFrame_ = config_->missile_.lifeFrame_;
	slashCooldown_ = config_->slash_.cooldown_;
	slashChargeTime_ = config_->slash_.chargeTime_;
	slashSpeed_ = config_->slash_.speed_;
	slashDamage_ = config_->slash_.damage_;
	slashLifeFrame_ = config_->slash_.lifeFrame_;

	// 状態遷移初期化
	sm_.Initialize(this);
	ChangeState(State::Enter);

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
	// 状態遷移初期化
	sm_.Initialize(this);
	ChangeState(State::Enter);

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
	// 怒りモード判定
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

	// ボス位置キャッシュ（状態更新で毎回使うので）と参照保存
	boss_ = &boss;
	posWork_ = boss.GetWorldPosition();

	// 状態別更新
	sm_.Update(dt);

	auraActive_ = false; // オーラ無効化
	auraT_ = 0.0f; // オーラ時間初期化

	// Laser状態以外ではレーザー無効
	if (state_ != State::LaserWindup && state_ != State::LaserFire) { // Laser状態以外
		laserActive_ = false; // レーザー無効化
		laserTelegraph_ = false; // レーザー予告無効化
	}

	ClampToArena(posWork_); // アリーナ内に位置クランプ
	boss.SetPosition(posWork_); // 位置セット
	boss.SyncTransform(); // Transform同期
	boss_ = nullptr; // キャッシュクリア
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

bool BossController::IsAnyCharging() const {
	return missileCharging_ || slashCharging_ || laserTelegraph_ || (state_ == State::LaserWindup);
}

float BossController::GetCharge01() const {
	// -----------------------------------
	// GetCharge01: 攻撃のチャージ状態を0.0～1.0で返す（レーザー予告は強めに）
	// -----------------------------------
	float v = 0.0f; // ミサイルとスラッシュのチャージ状態を計算

	// ミサイル
	if (missileCharging_ && missileChargeTime_ > 0.0001f) { // チャージ中でチャージ時間が正なら
		float t = 1.0f - (missileChargeTimer_ / missileChargeTime_); // 経過割合計算
		v = std::max(v, std::clamp(t, 0.0f, 1.0f)); // 0～1にクランプして最大値を取る
	}
	// スラッシュ
	if (slashCharging_ && slashChargeTime_ > 0.0001f) { // チャージ中でチャージ時間が正なら
		float t = 1.0f - (slashChargeTimer_ / slashChargeTime_); // 経過割合計算
		v = std::max(v, std::clamp(t, 0.0f, 1.0f)); // 0～1にクランプして最大値を取る
	}
	// レーザー予告は強めに
	if (laserTelegraph_ || state_ == State::LaserWindup) { // レーザー予告中またはレーザー予告状態なら
		v = std::max(v, 1.0f); // 予告は1.0f、発射中はレーザー状態で1.0fなので同じ値でOK
	}
	// クランプして返す
	return v;
}

void BossController::SetConfig(const BossControllerConfig* config) {
	config_ = config; // 設定セット
}

void BossController::ChangeState(State s) {
	state_ = s; // 状態更新
	timer_ = 0.0f; // 状態タイマーリセット

	switch (s) { // 状態に応じたステートクラスに遷移
	case State::Enter:        sm_.Change(std::make_unique<BossEnterState>()); break;
	case State::Orbit:        sm_.Change(std::make_unique<BossOrbitState>()); break;
	case State::Recover:      sm_.Change(std::make_unique<BossRecoverState>()); break;
	case State::LaserWindup:  sm_.Change(std::make_unique<BossLaserWindupState>()); break;
	case State::LaserFire:    sm_.Change(std::make_unique<BossLaserFireState>()); break;
	case State::LaserRecover: sm_.Change(std::make_unique<BossLaserRecoverState>()); break;
	}
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