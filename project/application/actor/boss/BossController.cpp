#include "BossController.h"
#include "BossStates.h"
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

//=============================================================
// 初期化
//=============================================================
void BossController::Initialize(const Vector3& arenaMin, const Vector3& arenaMax) {
	//=========================================================
	// 設定チェック
	//=========================================================
	assert(config_ && "BossControllerConfig が未設定です");

	//=========================================================
	// アリーナ範囲設定
	//=========================================================
	arenaMin_ = arenaMin;
	arenaMax_ = arenaMax;

	//=========================================================
	// 状態遷移初期化
	//=========================================================
	sm_.Initialize(this);
	ChangeState(State::Enter);

	//=========================================================
	// 乱数初期化
	//=========================================================
	rng_.seed(std::random_device{}());

	//=========================================================
	// Rage Gauge 初期化
	//=========================================================
	rageGauge_ = 0.0f;        // 怒りゲージ初期化
	rageActive_ = false;      // 怒りモード初期化
	lastHpForRage_ = -1;      // 前回HP初期化
	noDamageTime_ = 0.0f;     // 無被ダメ時間初期化

	//=========================================================
	// プレイヤー予測用情報初期化
	//=========================================================
	hasPrevPlayerPos_ = false;                 // 前フレーム位置なし
	prevPlayerPos_ = { 0.0f, 0.0f, 0.0f };    // 前フレーム位置初期化
	playerVel_ = { 0.0f, 0.0f, 0.0f };        // プレイヤー速度初期化
}

//=============================================================
// リセット
//=============================================================
void BossController::Reset() {
	//=========================================================
	// 状態遷移初期化
	//=========================================================
	sm_.Initialize(this);
	ChangeState(State::Enter);

	//=========================================================
	// Rage Gauge リセット
	//=========================================================
	rageGauge_ = 0.0f;        // 怒りゲージ初期化
	rageActive_ = false;      // 怒りモード初期化
	lastHpForRage_ = -1;      // 前回HP初期化
	noDamageTime_ = 0.0f;     // 無被ダメ時間初期化

	//=========================================================
	// プレイヤー予測用情報リセット
	//=========================================================
	hasPrevPlayerPos_ = false;                 // 前フレーム位置なし
	prevPlayerPos_ = { 0.0f, 0.0f, 0.0f };    // 前フレーム位置初期化
	playerVel_ = { 0.0f, 0.0f, 0.0f };        // プレイヤー速度初期化

	//=========================================================
	// ミサイル関連リセット
	//=========================================================
	missileRequestCount_ = 0;
	missileRequestConsumeIndex_ = 0;
	burstTargetValid_ = false;
	burstCharged_ = false;
	missileCharging_ = false;
	missileChargeTimer_ = 0.0f;
	missileChargeFrame_ = 0;
}

//=============================================================
// 更新
//=============================================================
void BossController::Update(float dt, Enemy& boss) {
	//=========================================================
	// 状態タイマー更新
	//=========================================================
	timer_ += dt;

	//=========================================================
	// Rage Gauge 更新
	// 毎フレーム、被ダメ量や無被ダメ時間を元にゲージを更新する
	//=========================================================
	if (lastHpForRage_ < 0) {
		// 初回だけ現在HPを基準値としてセット
		lastHpForRage_ = boss.GetHP();
		noDamageTime_ = 0.0f;
	}

	int hpNow_ = boss.GetHP();                            // 現在HP取得
	int dmg_ = std::max(0, lastHpForRage_ - hpNow_);     // 前回からの被ダメ量計算

	// 次回比較用に現在HPを保存
	lastHpForRage_ = hpNow_;

	if (dmg_ > 0) {
		// ダメージを受けていたら怒りゲージ増加
		rageGauge_ += static_cast<float>(dmg_) * config_->rage_.gainPerHp_;
		noDamageTime_ = 0.0f; // 無被ダメ時間リセット
	} else {
		// ダメージを受けていなければ無被ダメ時間を加算
		noDamageTime_ += dt;

		// 一定時間以上被弾していなければゲージ減衰
		if (noDamageTime_ >= config_->rage_.decayDelay_) {
			rageGauge_ -= config_->rage_.decayPerSec_ * dt;
		}
	}

	// ゲージを有効範囲に収める
	rageGauge_ = std::clamp(rageGauge_, 0.0f, config_->rage_.maxGauge_);

	//=========================================================
	// 怒りモード判定
	// ヒステリシスあり
	//=========================================================
	if (!rageActive_) {
		if (rageGauge_ >= config_->rage_.onThreshold_) {
			rageActive_ = true;
		}
	} else {
		if (rageGauge_ <= config_->rage_.offThreshold_) {
			rageActive_ = false;
		}
	}

	//=========================================================
	// プレイヤー位置・速度更新
	//=========================================================
	if (boss.GetPlayer()) {
		playerPos_ = boss.GetPlayer()(); // プレイヤー位置取得
	}

	// 前フレーム位置があれば速度を計算
	if (hasPrevPlayerPos_) {
		playerVel_ = (playerPos_ - prevPlayerPos_) * (1.0f / std::max(0.0001f, dt));
	}

	// Y成分は不要なので無効化
	playerVel_.y = 0.0f;

	// 次フレーム用に位置保存
	prevPlayerPos_ = playerPos_;
	hasPrevPlayerPos_ = true;

	//=========================================================
	// ボス位置キャッシュ
	// 状態更新で毎回使うため、参照と現在位置を保持しておく
	//=========================================================
	boss_ = &boss;
	posWork_ = boss.GetWorldPosition();

	//=========================================================
	// 状態別更新
	//=========================================================
	sm_.Update(dt);

	//=========================================================
	// オーラ状態更新
	// 現状は毎フレーム無効化している
	//=========================================================
	auraActive_ = false;
	auraT_ = 0.0f;

	//=========================================================
	// 位置反映
	//=========================================================
	ClampToArena(posWork_); // アリーナ範囲に収める
	boss.SetPosition(posWork_);
	boss.SyncTransform();

	// キャッシュクリア
	boss_ = nullptr;
}

//=============================================================
// ImGuiデバッグ表示
//=============================================================
void BossController::ImGuiDebug(Enemy& boss) {
#ifdef USE_IMGUI
	ImGui::Begin("ボスコントローラ");

	static const char* kStateName_[] = {
	"登場",
	"旋回",
	"復帰",
	"審判溜め",
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

//=============================================================
// ミサイル発射リクエスト消費
//=============================================================
bool BossController::ConsumeMissileFireRequest(
	Vector3& outPos,
	Vector3& outTarget,
	float& outSpeed,
	Vector3& outControlOffset,
	int& outDamage,
	int& outLifeFrame
) {
	// ミサイル発射リクエストが存在するかチェック
	if (missileRequestConsumeIndex_ >= missileRequestCount_) {
		return false;
	}

	//=========================================================
	// リクエスト消費
	//=========================================================
	const MissileFireRequest& req = missileRequests_[missileRequestConsumeIndex_];
	++missileRequestConsumeIndex_;

	outPos = req.pos_;
	outTarget = req.target_;
	outSpeed = config_->missile_.speed_;
	outControlOffset = req.controlOffset_;
	outDamage = config_->missile_.damage_;
	outLifeFrame = config_->missile_.lifeFrame_;

	// 全て消費し終えたらリクエスト状態をリセット
	if (missileRequestConsumeIndex_ >= missileRequestCount_) {
		missileRequestCount_ = 0;
		missileRequestConsumeIndex_ = 0;
	}

	return true;
}

//=============================================================
// スラッシュ発射リクエスト消費
//=============================================================
bool BossController::ConsumeSlashFireRequest(Vector3& outPos, Vector3& outTarget, float& outSpeed, int& outDamage, int& outLifeFrame) {
	// リクエストが無ければ失敗
	if (!slashFireReq_) {
		return false;
	}

	//=========================================================
	// リクエスト消費
	//=========================================================
	slashFireReq_ = false;

	// 出力設定
	outPos = slashPos_;
	outTarget = slashTarget_;
	outSpeed = config_->slash_.speed_;
	outDamage = config_->slash_.damage_;
	outLifeFrame = config_->slash_.lifeFrame_;

	return true;
}

//=============================================================
// いずれかの攻撃をチャージ中か
//=============================================================
bool BossController::IsAnyCharging() const {
	return missileCharging_ || slashCharging_;
}

//=============================================================
// チャージ率取得
//=============================================================
float BossController::GetCharge01() const {
	//=========================================================
	// 攻撃のチャージ状態を 0.0 ～ 1.0 で返す
	// レーザー予告は強めに 1.0 扱いにする
	//=========================================================
	float v = 0.0f;

	//=========================================================
	// ミサイルチャージ率
	//=========================================================
	if (missileCharging_ && config_->missile_.chargeTime_ > 0.0001f) {
		float t = 1.0f - (missileChargeTimer_ / config_->missile_.chargeTime_);
		v = std::max(v, std::clamp(t, 0.0f, 1.0f));
	}

	//=========================================================
	// スラッシュチャージ率
	//=========================================================
	if (slashCharging_ && config_->slash_.chargeTime_ > 0.0001f) {
		float t = 1.0f - (slashChargeTimer_ / config_->slash_.chargeTime_);
		v = std::max(v, std::clamp(t, 0.0f, 1.0f));
	}

	return v;
}

//=============================================================
// 設定セット
//=============================================================
void BossController::SetConfig(const BossControllerConfig* config) {
	config_ = config;
	assert(config_ && "BossControllerConfig が未設定です");
}

//=============================================================
// 状態変更
//=============================================================
void BossController::ChangeState(State s) {
	state_ = s;     // 状態更新
	timer_ = 0.0f; // 状態タイマーリセット

	//=========================================================
	// 状態に応じたステートクラスへ遷移
	//=========================================================
	switch (s) {
	case State::Enter: sm_.Change(std::make_unique<BossEnterState>()); break;
	case State::Orbit: sm_.Change(std::make_unique<BossOrbitState>()); break;
	case State::Recover: sm_.Change(std::make_unique<BossRecoverState>()); break;
	case State::JudgementWindup: sm_.Change(std::make_unique<BossJudgementWindupState>()); break;
	}
}

//=============================================================
// アリーナ内クランプ
//=============================================================
void BossController::ClampToArena(Vector3& p) {
	p.x = std::clamp(p.x, arenaMin_.x, arenaMax_.x);
	p.y = std::clamp(p.y, arenaMin_.y, arenaMax_.y);
	p.z = std::clamp(p.z, arenaMin_.z, arenaMax_.z);
}

//=============================================================
// 値を目標値へ近づける
//=============================================================
float BossController::Approach(float v, float target, float delta) {
	if (v < target) {
		return std::min(v + delta, target); // 上方向へ近づける
	}
	return std::max(v - delta, target);     // 下方向へ近づける
}

//=============================================================
// ベクトルのスムージング補間
//=============================================================
Vector3 BossController::SmoothDamp(const Vector3& from, const Vector3& to, float factor, float dt) {
	float k = 1.0f - std::pow(1.0f - factor, dt * 60.0f); // フレームレート補正込み補間率
	return MyMath::Vector3Lerp(from, to, k);
}