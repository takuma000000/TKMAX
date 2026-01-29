#include "EnemyManager.h"
#include <limits>
#include "MyMath.h"
#include "manager/BossManager.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

const EnemyManager::WaveOps EnemyManager::kWaveOps_[4] = {
	/* W1  */ { &EnemyManager::BeginWave1, &EnemyManager::UpdateWave1 },
	/* W2  */ { &EnemyManager::BeginWave2, &EnemyManager::UpdateWave2 },
	/* W3  */ { &EnemyManager::BeginWave3, &EnemyManager::UpdateWave3 },
	/* Done*/ { nullptr, nullptr },
};

void EnemyManager::Initialize(TKM::DirectXCommon* dx, TKM::Camera* camera, TKM::BaseScene* parent, Player* player) {
	dx_ = dx;
	cam_ = camera;
	parent_ = parent;
	player_ = player;

	// CSV読み込み（resources/data に置く運用）
	waveConfigLoaded_ = waveConfig_.Load("./resources/data/enemy_waves.csv");

	// 読めても読めなくても、挙動が壊れないように「既存メンバ」に流し込む
	{
		const auto& w1_ = waveConfig_.GetWave1();
		wave1SpawnInterval_ = w1_.spawnInterval_;
		wave1MaxSimultaneous_ = w1_.maxSimultaneous_;
		wave1DefeatTarget_ = w1_.defeatTarget_;
	}

	wave2WaitDuration_ = waveConfig_.GetWave2WaitDuration(); // Wave2開始前の待機時間
	wave2SubWaveCount_ = waveConfig_.GetWave2SubWaveCount(); // Wave2のサブWave数

	{
		const auto& w3_ = waveConfig_.GetWave3();
		wave3LeftPos_ = w3_.midBossLeft_;
		wave3RightPos_ = w3_.midBossRight_;
		wave3CoreLifetime_ = w3_.coreLifetime_;
		wave3CoreHP_ = w3_.coreHP_;
		wave3AngryDuration_ = w3_.angryDuration_;
	}
}

void EnemyManager::Update(float dt) {
	if (!initializedWaves_) { return; } // Wave未初期化なら何もしない

	// ───────────────────────────────
	/// ● Wave3 の核が居れば更新
	// ───────────────────────────────
	if (midBossCore_) {
		midBossCore_->Update(dt);

		// 死亡しきったらポインタ破棄
		if (midBossCore_->IsDead()) {
			if (player_) {
				player_->SetMidBossCore(nullptr);
			}
			midBossCore_.reset();
		}
	}

	// ───────────────────────────────────────────────
	/// ● 敵の状態を更新し、死亡したものは削除＆カウント
	// ───────────────────────────────────────────────
	for (auto it = enemies_.begin(); it != enemies_.end();) { // 敵リスト走査
		Enemy* e_ = it->get(); // 敵ポインタ取得
		e_->SetFreezeMove(freezeEnemies_); // 敵移動停止フラグセット
		e_->Update(dt); // 敵更新

		if (e_->IsDead()) {
			// ちゃんと倒した敵だけ、プレイヤーやカウンタに通知する
			if (e_->GetDefeated()) {
				if (player_) { // プレイヤーに通知
					player_->OnEnemyDestroyed(e_); // 敵撃破時の処理
				}
				// 撃破カウント増加
				++defeatedEnemyCount_;
				// 3体倒したら特殊攻撃解禁（既存仕様はそのまま）
				if (defeatedEnemyCount_ == 3 && player_) {
					player_->EnableSpecialAttack();
				}
			}

			// 逃げた敵（HasEscaped()==true）はここで静かに消えるだけ
			it = enemies_.erase(it);
		} else {
			++it;
		}
	}

	// ───────────────────────────────────────────────
	/// ● Wave進行
	// ───────────────────────────────────────────────
		// Waveごとの更新（分岐しない）
	const auto ops_ = kWaveOps_[static_cast<int>(wavePhase_)];
	if (ops_.update_) {
		(this->*ops_.update_)(dt);
	}
}

void EnemyManager::UpdateClosestEnemy() {
	if (!player_) { return; } // Player無効なら何もしない
	if (enemies_.empty()) { return; } // 敵リスト空なら何もしない

	Enemy* closestEnemy_ = nullptr; // 最も近い敵
	float closestDistance_ = std::numeric_limits<float>::max(); // 最も近い敵までの距離
	Vector3 playerPos_ = player_->GetPosition(); // プレイヤー位置

	for (auto& enemy : enemies_) { // 敵リスト走査
		if (!enemy) { continue; } // 念のためヌルチェック
		if (!enemy->IsDead() && !enemy->IsDying()) { // 生存中の敵のみ対象
			float dist_ = MyMath::Length(enemy->GetWorldPosition() - playerPos_); // プレイヤーからの距離計算
			if (dist_ < closestDistance_) { // 最短距離更新
				closestDistance_ = dist_; // 最短距離更新
				closestEnemy_ = enemy.get(); // 最も近い敵更新
			}
		}
	}

	player_->SetEnemy(closestEnemy_); // 最も近い敵をPlayerにセット
	player_->SetAllEnemies(&enemies_); // 敵リストもセット
}

void EnemyManager::InitializeWaves() {
	if (!player_) { return; }

	enemies_.clear();

	defeatedEnemyCount_ = 0;
	maxEnemyCount_ = wave1DefeatTarget_;

	wavePhase_ = WavePhase::W1;

	// Wave1用のタイマー初期化 ＆ 最初の1体だけ出しておく
	wave1SpawnTimer_ = 0.0f;
	SpawnWave1Enemy();

	wave2SubWave_ = 0;

	initializedWaves_ = true;

	// 最初のロックオン対象
	if (!enemies_.empty()) {
		player_->SetEnemy(enemies_.front().get());
		player_->SetAllEnemies(&enemies_);
	}
}

void EnemyManager::SpawnCurrentWave() {
	if (!dx_ || !cam_ || !parent_) { return; }

	enemies_.clear();

	const auto ops_ = kWaveOps_[static_cast<int>(wavePhase_)];
	if (ops_.spawn_) {
		(this->*ops_.spawn_)();
	}
}

void EnemyManager::GoToNextWave() {
	int next_ = static_cast<int>(wavePhase_) + 1;
	if (next_ > static_cast<int>(WavePhase::Done)) {
		next_ = static_cast<int>(WavePhase::Done);
	}
	wavePhase_ = static_cast<WavePhase>(next_);

	// Done なら spawn=nullptr なので何も起きない（分岐不要）
	SpawnCurrentWave();

	if (defeatedEnemyCount_) {
		defeatedEnemyCount_ = 0;
	}
}

void EnemyManager::SkipToBossWave() {
	// 敵リストがバインドされていなければ何もしない
	if (!&enemies_) {
		return;
	}

	// いま居るザコ敵は全部消す
	enemies_.clear();

	// 撃破数・最大数もリセット（ゲージを空にしておく）
	if (defeatedEnemyCount_) {
		defeatedEnemyCount_ = 0;
	}
	if (maxEnemyCount_) {
		maxEnemyCount_ = 0;
	}

	// Wave を Done（＝ボスフェーズ）にする
	wavePhase_ = WavePhase::Done;
}

void EnemyManager::SetupEnemyForPlayer(Enemy& e) {
	if (!player_) {
		return;
	}
	e.SetReticle(player_->GetReticle());
	e.SetPlayer([this]() { return player_->GetPosition(); });
}

void EnemyManager::UpdateWave1(float dt) {
	if (!&enemies_ || !dx_ || !cam_ || !parent_) {
		return;
	}

	// Wave1の目標撃破数に達したら次のWaveへ
	if (defeatedEnemyCount_ && defeatedEnemyCount_ >= wave1DefeatTarget_) {
		// Wave2に移るときWave1の残敵が邪魔なら消す（混ざるの防止）
		enemies_.clear();
		// 次のWaveへ
		GoToNextWave();
		return;
	}

	// 現在生存している敵の数（死亡演出中も含めるかどうかは好みだが、ここでは「まだ画面に居るやつ」を数える）
	int aliveCount_ = 0;
	for (auto& e : enemies_) {
		if (!e->IsDead()) {
			++aliveCount_;
		}
	}

	// 同時出現数が上限ならスポーンしない
	if (aliveCount_ >= wave1MaxSimultaneous_) {
		return;
	}

	// タイマーを進めて、一定間隔で敵を出す
	wave1SpawnTimer_ += dt;
	if (wave1SpawnTimer_ >= wave1SpawnInterval_) {
		wave1SpawnTimer_ = 0.0f;
		SpawnWave1Enemy();
	}
}

void EnemyManager::SpawnWave1Enemy() {
	if (!&enemies_ || !dx_ || !cam_ || !parent_) {
		return;
	}

	const auto& w1_ = waveConfig_.GetWave1();
	float y_ = w1_.baseY_;
	float z_ = w1_.baseZ_;

	float rx_ = MyMath::Rand01();
	float x_ = w1_.randXMin_ + rx_ * (w1_.randXMax_ - w1_.randXMin_);

	TKM::DirectXCommon* dxPtr_ = dx_;
	TKM::Camera* camPtr_ = cam_;
	TKM::BaseScene* parentPtr_ = parent_;

	EnemySpawner::SpawnLine(
		enemies_,
		1,
		y_,
		z_,
		x_,
		0.0f,
		dxPtr_,
		camPtr_,
		parentPtr_,
		[this, x_, z_](Enemy& e) {

			const auto& p1_ = waveConfig_.GetWave1EnemyParams();

			e.SetModel(p1_.model_);
			e.SetBehavior(p1_.behavior_);

			Vector3 start_ = { x_, p1_.startY_, z_ };
			Vector3 playerPos_ = player_->GetPosition();
			Vector3 target_ = { playerPos_.x, playerPos_.y, playerPos_.z + p1_.targetForwardZ_ };
			Vector3 apex = { (start_.x + target_.x) * 0.5f, p1_.apexY_, (start_.z + target_.z) * 0.5f };

			e.SetPosition(start_);
			e.SetPounceParameters(start_, apex, target_, p1_.pounceTime_);
			e.SetHP(p1_.hp_);
			e.SetScale({ 1.0f,1.0f,1.0f });

			SetupEnemyForPlayer(e);
		}
	);
}

void EnemyManager::UpdateWave2(float dt) {
	if (!&enemies_) {
		return;
	}

	// まだ敵が残っている → 何もしない
	if (!enemies_.empty()) {
		return;
	}

	// 全滅した後、まだ待ち始めていないなら待ち開始
	if (!wave2Waiting_) {
		wave2Waiting_ = true;
		wave2WaitTimer_ = 0.0f;
		return;
	}

	// 待っている間はタイマー進行
	wave2WaitTimer_ += dt;
	if (wave2WaitTimer_ < wave2WaitDuration_) {
		return; // まだ待ち時間中
	}

	// 待ち時間が終わった！次の隊列へ
	wave2Waiting_ = false;
	wave2SubWave_++; // 次のサブWaveへ

	if (wave2SubWave_ >= wave2SubWaveCount_) {
		GoToNextWave();
		return;
	}
	SpawnWave2SubWave(wave2SubWave_); // 次のサブWaveをスポーン
}

namespace {
	using SubWaveFn = void (EnemyManager::*)();

	static const SubWaveFn kWave2SubWaveTable_[] = {
		&EnemyManager::SpawnWave2_Triangle,
		&EnemyManager::SpawnWave2_Line,
		&EnemyManager::SpawnWave2_FastColumn,
	};
}

void EnemyManager::SpawnWave2SubWave(int id) {
	if (!&enemies_ || !dx_ || !cam_ || !parent_) { return; }
	enemies_.clear(); // 念のためクリア

	const int count_ = static_cast<int>(std::size(kWave2SubWaveTable_));
	if (id < 0 || id >= count_) { return; }

	(this->*kWave2SubWaveTable_[id])();
}

// ───────────────────────────────────────────────
// ● Wave2 各小Waveスポーン関数群
// ───────────────────────────────────────────────
void EnemyManager::SpawnWave2_Triangle() {
	if (!&enemies_ || !dx_ || !cam_ || !parent_) return;

	const auto& s_ = waveConfig_.GetWave2SubWave(0);
	int idx_ = 0;

	EnemySpawner::SpawnV(
		enemies_,
		s_.triCountPerSide_,
		s_.triY_, s_.triZ_,
		s_.triXCenter_,
		s_.triXStep_,
		s_.triZStep_,
		dx_, cam_, parent_,
		[this, &idx_](Enemy& e) {
			const auto& pt_ = waveConfig_.GetWave2TriEnemyParams();

			e.SetModel(pt_.model_);
			e.SetBehavior(pt_.behavior_);
			e.SetVelocity(pt_.vel_);
			e.SetSineParams(pt_.sineAmp_, pt_.sineFreq_);
			e.SetSinePhase(pt_.phaseStep_ * float(idx_++));
			e.SetHP(pt_.hp_);

			SetupEnemyForPlayer(e);
		}
	);
}

void EnemyManager::SpawnWave2_Line() {
	if (!&enemies_ || !dx_ || !cam_ || !parent_) return;

	const auto& s_ = waveConfig_.GetWave2SubWave(1);

	EnemySpawner::SpawnLine(
		enemies_,
		s_.lineCount_, s_.lineY_, s_.lineZ_,
		s_.lineXStart_, s_.lineXStep_,
		dx_, cam_, parent_,
		[this](Enemy& e) {
			const auto& pl_ = waveConfig_.GetWave2LineEnemyParams();

			e.SetModel(pl_.model_);
			e.SetBehavior(pl_.behavior_);
			e.SetVelocity(pl_.vel_);
			e.SetStopZ(pl_.stopZ_);
			e.SetHP(pl_.hp_);

			SetupEnemyForPlayer(e);
		}
	);
}

void EnemyManager::SpawnWave2_FastColumn() {
	if (!&enemies_ || !dx_ || !cam_ || !parent_) return;

	const auto& s_ = waveConfig_.GetWave2SubWave(2);

	EnemySpawner::SpawnColumn(
		enemies_,
		s_.colCount_,
		s_.colX_,
		s_.colZStart_,
		s_.colZStep_,
		s_.colYStart_,
		s_.colYStep_,
		dx_, cam_, parent_,
		[this](Enemy& e) {
			const auto& pc_ = waveConfig_.GetWave2ColEnemyParams();

			e.SetModel(pc_.model_);
			e.SetBehavior(pc_.behavior_);
			e.SetVelocity(pc_.vel_);
			e.SetStopZ(pc_.stopZ_);
			e.SetHP(pc_.hp_);

			SetupEnemyForPlayer(e);
		}
	);
}

void EnemyManager::UpdateWave3(float dt) {
	if (!&enemies_) {
		return;
	}

	// ── 中ボスが何体生きているかだけ Enemy から数える ──
	int aliveMidBossCount_ = 0;
	for (auto& e : enemies_) {
		if (!e) continue;
		if (e->GetType() == EnemyType::Wave3MidBoss && !e->IsDead() && !e->IsDying()) {
			aliveMidBossCount_++; // 生存中の中ボスをカウント
		}
	}

	// ── 核の生存状態は MidBossCore で判定 ──
	bool coreAlive_ = (midBossCore_ && !midBossCore_->IsDead() && !midBossCore_->IsDying());

	// 「このフレームで中ボスが減ったか？」
	bool midBossJustDied_ = (aliveMidBossCount_ < wave3PrevAliveMidBossCount_);

	// ---- 中ボスが 0 体になったら Wave3 終了判定 ----
	if (aliveMidBossCount_ == 0) {
		// 蘇生中ならコアを強制的に殺してキャンセル
		if (wave3ReviveInProgress_) {
			if (coreAlive_) {
				midBossCore_->StartDeathReaction({ 0.0f, 0.0f, 1.0f });
			}
			wave3ReviveInProgress_ = false;
			wave3CoreTimer_ = 0.0f;
			if (player_) {
				player_->SetMidBossCore(nullptr);
			}
			midBossCore_.reset();
		}

		// 中ボスも核も居なければ Wave3 終了 → 次のWave（ボス）へ
		if (enemies_.empty() && !midBossCore_) {
			GoToNextWave();
		}

		wave3PrevAliveMidBossCount_ = aliveMidBossCount_;
		return;
	}

	// ---- 中ボスが 1 体になった瞬間に核を出す ----
	if (aliveMidBossCount_ == 1 && !wave3ReviveInProgress_ && midBossJustDied_) {
		wave3ReviveInProgress_ = true;
		wave3CoreTimer_ = 0.0f;
		SpawnWave3Core();

		for (auto& e : enemies_) {
			if (!e) continue;
			if (e->GetType() != EnemyType::Wave3MidBoss) continue;
			if (e->IsDead() || e->IsDying()) continue;

			e->SetAngry(wave3AngryDuration_);
		}

		wave3PrevAliveMidBossCount_ = aliveMidBossCount_;
		return;
	}

	// まだ蘇生フェーズに入っていないなら何もしない
	if (!wave3ReviveInProgress_) {
		wave3PrevAliveMidBossCount_ = aliveMidBossCount_;
		return;
	}

	// ---- ここから「蘇生フェーズ中」 ----

	// 核が既に壊されている → 蘇生キャンセル
	if (!coreAlive_) {
		wave3ReviveInProgress_ = false;
		wave3CoreTimer_ = 0.0f;
		if (player_) {
			player_->SetMidBossCore(nullptr);
		}
		midBossCore_.reset();
		wave3PrevAliveMidBossCount_ = aliveMidBossCount_;
		return;
	}

	// 核がまだ生きているならタイマーを進める
	wave3CoreTimer_ += dt;

	// 規定時間生き残った → 蘇生成功（中ボス再スポーン）
	if (wave3CoreTimer_ >= wave3CoreLifetime_) {
		// 中ボスを 1 体復活
		SpawnWave3ExtraMidBoss();

		// 核は役目を終えたので消す
		if (midBossCore_) {
			midBossCore_->StartDeathReaction({ 0.0f, 0.0f, 1.0f });
		}

		wave3ReviveInProgress_ = false;
		wave3CoreTimer_ = 0.0f;
	}

	wave3PrevAliveMidBossCount_ = aliveMidBossCount_;
}

void EnemyManager::SpawnWave3MidBossStage() {
	if (!&enemies_ || !dx_ || !cam_ || !parent_) {
		return;
	}
	enemies_.clear();
	midBossCore_.reset();

	wave3ReviveInProgress_ = false;
	wave3CoreTimer_ = 0.0f;

	auto camPtr_ = cam_;
	auto dxPtr_ = dx_;
	auto parentPtr_ = parent_;

	EnemySpawner::SpawnLine(
		enemies_,
		2,
		wave3LeftPos_.y,
		wave3LeftPos_.z,
		wave3LeftPos_.x,
		(wave3RightPos_.x - wave3LeftPos_.x),
		dxPtr_, camPtr_, parentPtr_,
		[this](Enemy& e) {

			const auto& pm_ = waveConfig_.GetWave3MidBossParams();

			e.SetModel(pm_.model_);
			e.SetBehavior(pm_.behavior_);

			e.SetFreeRoamArea(pm_.areaMin_, pm_.areaMax_, pm_.normalSpeed_, pm_.rageSpeed_);
			e.SetHP(pm_.hp_);
			e.SetScale({ pm_.scale_, pm_.scale_, pm_.scale_ });

			SetupEnemyForPlayer(e);
			e.SetType(EnemyType::Wave3MidBoss);
		}
	);

	if (maxEnemyCount_) {
		maxEnemyCount_ += 2;
	}

	wave3PrevAliveMidBossCount_ = 2;
}

void EnemyManager::SpawnWave3Core() {
	if (!dx_ || !cam_ || !parent_) {
		return;
	}

	const auto& w3_ = waveConfig_.GetWave3();

	float xRange_ = w3_.coreXRange_;
	float zMin_ = w3_.coreZMin_;
	float zMax_ = w3_.coreZMax_;

	float rx_ = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	float rz_ = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);

	float x_ = -xRange_ + rx_ * (xRange_ * 2.0f);
	float z_ = zMin_ + rz_ * (zMax_ - zMin_);
	float y_ = w3_.coreY_;

	// すでにコアが居たら一旦消して作り直し
	midBossCore_ = std::make_unique<MidBossCore>();

	// Object3d 用共通（Enemy でも使ってるやつ）
	auto* common_ = TKM::Object3dCommon::GetInstance();

	midBossCore_->Initialize(common_, dx_);
	midBossCore_->SetCamera(cam_);
	midBossCore_->SetParentScene(parent_);

	midBossCore_->SetPosition({ x_, y_, z_ });
	midBossCore_->SetScale({ 0.8f, 0.8f, 0.8f });
	midBossCore_->SetHP(wave3CoreHP_);

	midBossCore_->SyncTransform();

	if (player_) {
		// コアにもロック・プレイヤー情報を渡す
		midBossCore_->SetReticle(player_->GetReticle());
		midBossCore_->SetPlayer([this]() { return player_->GetPosition(); });

		// プレイヤーにもコアを教える
		player_->SetMidBossCore(midBossCore_.get());
	}
}

void EnemyManager::SpawnWave3ExtraMidBoss() {
	if (!&enemies_ || !dx_ || !cam_ || !parent_) {
		return;
	}

	bool leftAlive_ = false;
	bool rightAlive_ = false;

	for (auto& e : enemies_) {
		if (!e) continue;
		if (e->GetType() != EnemyType::Wave3MidBoss) continue;
		if (e->IsDead()) continue;

		Vector3 pos = e->GetWorldPosition();
		if (pos.x < 0.0f) {
			leftAlive_ = true;
		} else {
			rightAlive_ = true;
		}
	}

	const int state_ = (leftAlive_ ? 1 : 0) | (rightAlive_ ? 2 : 0);

	static const int kSpawnSide_[4] = {
		0,
		1,
		0,
		0,
	};

	const Vector3 kSidePos_[2] = { wave3LeftPos_, wave3RightPos_ };
	const Vector3 spawnPos_ = kSidePos_[kSpawnSide_[state_]];

	auto camPtr_ = cam_;
	auto dxPtr_ = dx_;
	auto parentPtr_ = parent_;

	EnemySpawner::SpawnLine(
		enemies_,
		1,
		spawnPos_.y,
		spawnPos_.z,
		spawnPos_.x,
		0.0f,
		dxPtr_,
		camPtr_,
		parentPtr_,
		[this](Enemy& e) {

			const auto& px_ = waveConfig_.GetWave3ExtraMidBossParams();

			e.SetModel(px_.model_);
			e.SetBehavior(px_.behavior_);
			e.SetVelocity(px_.vel_);
			e.SetStopZ(px_.stopZ_);
			e.SetHP(px_.hp_);
			e.SetScale({ px_.scale_, px_.scale_, px_.scale_ });

			SetupEnemyForPlayer(e);
			e.SetType(EnemyType::Wave3MidBoss);
		}
	);
}

void EnemyManager::BeginWave1() {
	wave1SpawnTimer_ = 0.0f;
	if (maxEnemyCount_) {
		maxEnemyCount_ = wave1DefeatTarget_;
	}
	SpawnWave1Enemy(); // 最初の1体だけ出す（元のまま）
}

void EnemyManager::BeginWave2() {
	// Wave2の初期化（InitializeWaves と同等にする）
	wave2SubWave_ = 0;
	wave2Waiting_ = false;
	wave2WaitTimer_ = 0.0f;

	// Wave2開始：最初のサブWaveを出す
	SpawnWave2SubWave(wave2SubWave_);
}

void EnemyManager::BeginWave3() {
	// Wave3開始：中ボスステージ生成関数の中で必要なリセットは全部やってる
	SpawnWave3MidBossStage();
}

void EnemyManager::Draw(TKM::DirectXCommon* dx) {
	if (!&enemies_) { // enemies_ がまだ紐付いてなかったら何もしない
		return;
	}
	for (auto& enemy : enemies_) { // 敵を全部描画
		enemy->Draw(dx);
	}
	if (midBossCore_) { // 核が居れば描画
		midBossCore_->Draw(dx);
	}
}

void EnemyManager::ImGuiDebug() {
#ifdef USE_IMGUI
	// まだ紐付いてないなら何もしない
	if (!&enemies_) {
		return;
	}

	ImGui::Begin("敵ステータス");

	// 敵の動きを止めるトグル
	ImGui::Checkbox("敵の動きを止める", &freezeEnemies_);
	ImGui::Separator();

	// 撃破数／最大数
	int defeated_ = defeatedEnemyCount_ ? defeatedEnemyCount_ : 0;
	int maxCount_ = maxEnemyCount_ ? maxEnemyCount_ : static_cast<int>(enemies_.size());

	ImGui::Text("撃破数: %d / %d", defeated_, maxCount_);

	// 撃破進捗バー
	float progress_ = 0.0f;
	if (maxCount_ > 0) {
		progress_ = static_cast<float>(defeated_) / static_cast<float>(maxCount_);
	}
	ImGui::ProgressBar(progress_, ImVec2(200, 20), "撃破進行度");

	// ===== Wave 状態表示 =====
	static const char* kWaveLabel_[] = {
	"Wave1",
	"Wave2",
	"Wave3",
	"Bossフェーズ"
	};
	ImGui::Text("現在のWave: %s", kWaveLabel_[static_cast<int>(wavePhase_)]);

	// 「次のWaveへ」ボタン
	if (ImGui::Button("次のWaveへ")) {
		GoToNextWave();
	}
	ImGui::SameLine(); // 横並びに
	// 「ボスWaveへ」ボタン
	if (ImGui::Button("ボスWaveへ")) {
		SkipToBossWave();
	}

	ImGui::Separator();

	// ===== Wave3 中ボス＆核 デバッグ =====
	if (wavePhase_ == WavePhase::W3) {
		ImGui::Text("=== Wave3 MidBoss / Core Debug ===");

		int aliveMidBoss_ = 0;
		int dyingMidBoss_ = 0;
		int deadMidBoss_ = 0;

		// 中ボスの状態を数える
		for (auto& e : enemies_) {
			if (!e) continue;
			if (e->GetType() != EnemyType::Wave3MidBoss) continue;

			if (e->IsDead()) {
				deadMidBoss_++;
			} else if (e->IsDying()) {
				dyingMidBoss_++;
			} else {
				aliveMidBoss_++;
			}
		}

		bool coreAlive_ = (midBossCore_ && !midBossCore_->IsDead() && !midBossCore_->IsDying());

		ImGui::Text("MidBoss Alive:%d  Dying:%d  Dead:%d", aliveMidBoss_, dyingMidBoss_, deadMidBoss_);
		ImGui::Text("prevAliveMidBossCount: %d", wave3PrevAliveMidBossCount_);

		bool midBossJustDiedDbg_ = (aliveMidBoss_ < wave3PrevAliveMidBossCount_);
		ImGui::Text("midBossJustDied: %s", midBossJustDiedDbg_ ? "true" : "false");

		ImGui::Text("ReviveInProgress: %s", wave3ReviveInProgress_ ? "true" : "false");
		ImGui::Text("CoreAlive: %s", coreAlive_ ? "true" : "false");
		ImGui::Text("CoreTimer: %.2f / %.2f", wave3CoreTimer_, wave3CoreLifetime_);

		ImGui::Separator();
	}

	// 各敵のデバッグ（※二重ループになっていたので 1 回に統一）
	for (size_t i = 0; i < enemies_.size(); ++i) {
		ImGui::PushID(static_cast<int>(i));
		(enemies_)[i]->ImGuiDebug();
		ImGui::PopID();
	}

	ImGui::End();

	// 核
	if (midBossCore_) {
		midBossCore_->ImGuiDebug();
	}
#endif
}