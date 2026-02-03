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
	// 怒りモード判定
	if (!rageActive_) { // 怒りモードでなければ発動判定
		if (rageGauge_ >= rageOnThreshold_) { rageActive_ = true; } // 発動
	} else {
		if (rageGauge_ <= rageOffThreshold_) { rageActive_ = false; } // 解除
	}

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
	ImGui::Separator();

	ImGui::Text("ボスHP: %d", boss.GetHP());
	ImGui::Separator();

	if (ImGui::CollapsingHeader("オーラ(Aura)", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("AuraActive: %s", auraActive_ ? "true" : "false");
		ImGui::Text("AuraT: %.2f", auraT_);

		ImGui::Checkbox("足元リング", &auraUseRing_);
		ImGui::DragFloat("強さ(Intensity)", &auraIntensity_, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("広がり倍率(ScaleMul)", &auraScaleMul_, 0.01f, 0.1f, 10.0f);
		ImGui::ColorEdit3("色(Color)", &auraColor_.x);
	}

	if (auraVolume_) {
		auraVolume_->DrawImGui("Aura Volume (3D)");
	}

	ImGui::End();
#endif
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

	// 一定時間で次の行動へ
	if (timer_ >= orbitDuration_) { // 時間到達
		// 怒りモードじゃないならレーザーに行かない
		if (!rageActive_) {
			ChangeState(State::Recover);
			return;
		}

		// ★怒りモード時のみレーザーへ
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