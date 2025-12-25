#include "EnemyManager.h"
#include <limits>
#include "MyMath.h"
#include "manager/BossManager.h"

const EnemyManager::WaveOps EnemyManager::kWaveOps_[4] = {
	/* W1  */ { &EnemyManager::BeginWave1, &EnemyManager::UpdateWave1 },
	/* W2  */ { &EnemyManager::BeginWave2, &EnemyManager::UpdateWave2 },
	/* W3  */ { &EnemyManager::BeginWave3, &EnemyManager::UpdateWave3 },
	/* Done*/ { nullptr, nullptr },
};

void EnemyManager::Initialize(DirectXCommon* dx, Camera* camera, BaseScene* parent, Player* player) {
	dx_ = dx;
	cam_ = camera;
	parent_ = parent;
	player_ = player;

	// CSV読み込み（resources/data に置く運用）
	waveConfigLoaded_ = waveConfig_.Load("./resources/data/enemy_waves.csv");

	// 読めても読めなくても、挙動が壊れないように「既存メンバ」に流し込む
	{
		const auto& w1 = waveConfig_.GetWave1();
		wave1SpawnInterval_ = w1.spawnInterval;
		wave1MaxSimultaneous_ = w1.maxSimultaneous;
	}

	wave2WaitDuration_ = waveConfig_.GetWave2WaitDuration();

	{
		const auto& w3 = waveConfig_.GetWave3();
		wave3LeftPos_ = w3.midBossLeft;
		wave3RightPos_ = w3.midBossRight;
	}
}

void EnemyManager::BindEnemies(std::vector<std::unique_ptr<Enemy>>* enemies,
	int* defeatedEnemyCount,
	int* maxEnemyCount) {
	enemies_ = enemies;
	defeatedEnemyCount_ = defeatedEnemyCount;
	maxEnemyCount_ = maxEnemyCount; // いまはまだ使ってないけど、将来ゲージ表示とかに使える
}

void EnemyManager::Update(float dt) {
	// enemies_ がまだ紐付いてなかったら何もしない
	if (!enemies_) {
		return;
	}

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
	for (auto it = enemies_->begin(); it != enemies_->end();) {
		Enemy* e = it->get();

		// フラグを渡す
		e->SetFreezeMove(freezeEnemies_);
		e->Update(dt);

		if (e->IsDead()) {
			// ちゃんと倒した敵だけ、プレイヤーやカウンタに通知する
			if (e->GetDefeated()) {

				if (player_) {
					player_->OnEnemyDestroyed(e);
				}

				if (defeatedEnemyCount_) {
					++(*defeatedEnemyCount_);

					// 3体倒したら特殊攻撃解禁（既存仕様はそのまま）
					if (*defeatedEnemyCount_ == 3 && player_) {
						player_->EnableSpecialAttack();
					}
				}
			}

			// 逃げた敵（HasEscaped()==true）はここで静かに消えるだけ
			it = enemies_->erase(it);
		} else {
			++it;
		}
	}

	// ───────────────────────────────────────────────
	/// ● Wave進行
	// ───────────────────────────────────────────────
		// Waveごとの更新（分岐しない）
	const auto ops = kWaveOps_[static_cast<int>(wavePhase_)];
	if (ops.update) {
		(this->*ops.update)(dt);
	}
}

void EnemyManager::UpdateClosestEnemy() {
	// プレイヤー or 敵リストが無効なら何もしない
	if (!player_ || !enemies_ || enemies_->empty()) {
		return;
	}

	// ───────────────────────────────────────────────
	/// ● プレイヤーに最も近い「ザコ敵」を検出し、ターゲットとして設定する
	// ───────────────────────────────────────────────
	Enemy* closestEnemy = nullptr;
	float closestDistance = std::numeric_limits<float>::max();
	Vector3 playerPos = player_->GetPosition();

	// ───── 敵リストを走査して、最も近い生存中の敵を探す ─────
	for (auto& enemy : *enemies_) {
		if (!enemy->IsDead() && !enemy->IsDying()) { // 生存中の敵のみ対象
			float dist = MyMath::Length(enemy->GetWorldPosition() - playerPos);

			if (dist < closestDistance) {
				closestDistance = dist;
				closestEnemy = enemy.get();
			}
		}
	}
	// ───── 検出結果をプレイヤーに通知 ─────
	player_->SetEnemy(closestEnemy);
	player_->SetAllEnemies(enemies_);
}

void EnemyManager::InitializeWaves() {
	// enemies_ or player_ が無効なら何もしない
	if (!enemies_ || !player_) {
		return;
	}
	enemies_->clear();

	if (defeatedEnemyCount_) {
		*defeatedEnemyCount_ = 0;
	}
	if (maxEnemyCount_) {
		// Wave1 の目標撃破数をセット（ゲージ用）
		*maxEnemyCount_ = wave1DefeatTarget_;
	}

	wavePhase_ = WavePhase::W1;

	// Wave1用のタイマー初期化 ＆ 最初の1体だけ出しておく
	wave1SpawnTimer_ = 0.0f;
	SpawnWave1Enemy();

	wave2SubWave_ = 0; // Wave2用サブWave初期化

	// 最初のロックオン対象
	if (!enemies_->empty()) {
		player_->SetEnemy(enemies_->front().get());
		player_->SetAllEnemies(enemies_);
	}
}

void EnemyManager::SpawnCurrentWave() {
	if (!enemies_ || !dx_ || !cam_ || !parent_) { return; }

	enemies_->clear();

	const auto ops = kWaveOps_[static_cast<int>(wavePhase_)];
	if (ops.spawn) {
		(this->*ops.spawn)();
	}
}

void EnemyManager::GoToNextWave() {
	int next = static_cast<int>(wavePhase_) + 1;
	if (next > static_cast<int>(WavePhase::Done)) {
		next = static_cast<int>(WavePhase::Done);
	}
	wavePhase_ = static_cast<WavePhase>(next);

	// Done なら spawn=nullptr なので何も起きない（分岐不要）
	SpawnCurrentWave();

	if (defeatedEnemyCount_) {
		*defeatedEnemyCount_ = 0;
	}
}

void EnemyManager::SkipToBossWave() {
	// 敵リストがバインドされていなければ何もしない
	if (!enemies_) {
		return;
	}

	// いま居るザコ敵は全部消す
	enemies_->clear();

	// 撃破数・最大数もリセット（ゲージを空にしておく）
	if (defeatedEnemyCount_) {
		*defeatedEnemyCount_ = 0;
	}
	if (maxEnemyCount_) {
		*maxEnemyCount_ = 0;
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
	if (!enemies_ || !dx_ || !cam_ || !parent_) {
		return;
	}

	// Wave1の目標撃破数に達したら次のWaveへ
	if (defeatedEnemyCount_ && *defeatedEnemyCount_ >= wave1DefeatTarget_) {

		// Wave2に移るときWave1の残敵が邪魔なら消す（混ざるの防止）
		enemies_->clear();

		GoToNextWave();
		return;
	}

	// 現在生存している敵の数（死亡演出中も含めるかどうかは好みだが、ここでは「まだ画面に居るやつ」を数える）
	int aliveCount = 0;
	for (auto& e : *enemies_) {
		if (!e->IsDead()) {
			++aliveCount;
		}
	}

	// 同時出現数が上限ならスポーンしない
	if (aliveCount >= wave1MaxSimultaneous_) {
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
	if (!enemies_ || !dx_ || !cam_ || !parent_) {
		return;
	}

	// 出現位置（Xはちょっとランダム、Zは奥から）
	const auto& w1 = waveConfig_.GetWave1();
	float y = w1.baseY;
	float z = w1.baseZ;
	float rx = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // 0〜1
	float x = w1.randXMin + rx * (w1.randXMax - w1.randXMin);

	DirectXCommon* dxPtr = dx_;
	Camera* camPtr = cam_;
	BaseScene* parentPtr = parent_;

	EnemySpawner::SpawnLine(
		*enemies_,
		1,          // 1体だけ
		y,
		z,
		x,
		0.0f,       // xStep は未使用（1体なので）
		dxPtr,
		camPtr,
		parentPtr,
		[this, x, z](Enemy& e) {

			e.SetBehavior(EnemyBehavior::PounceFromAbove);

			Vector3 start = { x, 20.0f, z }; // 高い位置から降ってくる
			Vector3 playerPos = player_->GetPosition(); // プレイヤー位置取得
			// プレイヤーの少し手前に着地するようにターゲット設定
			Vector3 target = {
				playerPos.x,
				playerPos.y,
				playerPos.z + 3.0f
			};
			// 曲線の頂点（アペックス）を計算
			Vector3 apex = {
				(start.x + target.x) * 0.5f,
				30.0f,   // 曲線の頂点の高さ
				(start.z + target.z) * 0.5f
			};

			e.SetPosition(start); // 開始位置にセット
			e.SetPounceParameters(start, apex, target, 1.6f); // 1.6秒で移動
			e.SetHP(1); // Wave1敵のHP設定
			e.SetScale({ 1.0f,1.0f,1.0f }); // スケールリセット

			// プレイヤー関連セットアップを共通化
			SetupEnemyForPlayer(e);
		}
	);
}

void EnemyManager::UpdateWave2(float dt) {
	if (!enemies_) {
		return;
	}

	// まだ敵が残っている → 何もしない
	if (!enemies_->empty()) {
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

	if (wave2SubWave_ >= 3) { // サブWaveが全部終わったら次のWaveへ
		GoToNextWave();
		return;
	}
	SpawnWave2SubWave(wave2SubWave_); // 次のサブWaveをスポーン
}

namespace {
	using SubWaveFn = void (EnemyManager::*)();

	static const SubWaveFn kWave2SubWaveTable[] = {
		&EnemyManager::SpawnWave2_Triangle,
		&EnemyManager::SpawnWave2_Line,
		&EnemyManager::SpawnWave2_FastColumn,
	};
}

void EnemyManager::SpawnWave2SubWave(int id) {
	if (!enemies_ || !dx_ || !cam_ || !parent_) { return; }
	enemies_->clear(); // 念のためクリア

	const int count = static_cast<int>(std::size(kWave2SubWaveTable));
	if (id < 0 || id >= count) { return; }

	(this->*kWave2SubWaveTable[id])();
}

// ───────────────────────────────────────────────
// ● Wave2 各小Waveスポーン関数群
// ───────────────────────────────────────────────
void EnemyManager::SpawnWave2_Triangle() {
	if (!enemies_ || !dx_ || !cam_ || !parent_) return;

	const auto& s = waveConfig_.GetWave2SubWave(0);

	int idx = 0;

	EnemySpawner::SpawnV(
		*enemies_,
		s.triCountPerSide,
		s.triY, s.triZ,
		s.triXCenter,
		s.triXStep,
		s.triZStep,
		dx_, cam_, parent_,
		[this, &idx](Enemy& e) {
			// ここは挙動。触らない。
			e.SetBehavior(EnemyBehavior::SineX);
			e.SetVelocity({ 0,0,-0.30f });
			e.SetSineParams(4.0f, 1.4f);
			e.SetSinePhase(0.6f * float(idx++));
			e.SetHP(3);

			SetupEnemyForPlayer(e);
		}
	);
}

void EnemyManager::SpawnWave2_Line() {
	if (!enemies_ || !dx_ || !cam_ || !parent_) return;

	const auto& s = waveConfig_.GetWave2SubWave(1);

	EnemySpawner::SpawnLine(
		*enemies_,
		s.lineCount, s.lineY, s.lineZ,
		s.lineXStart, s.lineXStep,
		dx_, cam_, parent_,
		[this](Enemy& e) {
			// 挙動は触らない
			e.SetBehavior(EnemyBehavior::StraightStop);
			e.SetVelocity({ 0,0,-0.32f });
			e.SetStopZ(52.0f);
			e.SetHP(2);

			SetupEnemyForPlayer(e);
		}
	);
}

void EnemyManager::SpawnWave2_FastColumn() {
	if (!enemies_ || !dx_ || !cam_ || !parent_) return;

	const auto& s = waveConfig_.GetWave2SubWave(2);

	EnemySpawner::SpawnColumn(
		*enemies_,
		s.colCount,
		s.colX,
		s.colZStart,
		s.colZStep,
		s.colYStart,
		s.colYStep,
		dx_, cam_, parent_,
		[this](Enemy& e) {
			// 挙動は触らない
			e.SetBehavior(EnemyBehavior::StraightStop);
			e.SetVelocity({ -0.20f, 0.0f, -0.75f });
			e.SetStopZ(-50.0f); // 通過するだけ
			e.SetHP(1);

			SetupEnemyForPlayer(e);
		}
	);
}

void EnemyManager::UpdateWave3(float dt) {
	if (!enemies_) {
		return;
	}

	// ── 中ボスが何体生きているかだけ Enemy から数える ──
	int aliveMidBossCount = 0;
	for (auto& e : *enemies_) {
		if (!e) continue;
		if (e->GetType() == EnemyType::Wave3MidBoss && !e->IsDead() && !e->IsDying()) {
			aliveMidBossCount++; // 生存中の中ボスをカウント
		}
	}

	// ── 核の生存状態は MidBossCore で判定 ──
	bool coreAlive = (midBossCore_ && !midBossCore_->IsDead() && !midBossCore_->IsDying());

	// 「このフレームで中ボスが減ったか？」
	bool midBossJustDied = (aliveMidBossCount < wave3PrevAliveMidBossCount_);

	// ---- 中ボスが 0 体になったら Wave3 終了判定 ----
	if (aliveMidBossCount == 0) {
		// 蘇生中ならコアを強制的に殺してキャンセル
		if (wave3ReviveInProgress_) {
			if (coreAlive) {
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
		if (enemies_->empty() && !midBossCore_) {
			GoToNextWave();
		}

		wave3PrevAliveMidBossCount_ = aliveMidBossCount;
		return;
	}

	// ---- 中ボスが 1 体になった瞬間に核を出す ----
	if (aliveMidBossCount == 1 && !wave3ReviveInProgress_ && midBossJustDied) {
		wave3ReviveInProgress_ = true;
		wave3CoreTimer_ = 0.0f;
		SpawnWave3Core();

		// 残り1体の中ボスを怒りモードにする
		const float angryDuration = 8.0f; // 何秒怒らせるか（あとで調整）
		for (auto& e : *enemies_) {
			if (!e) continue;
			if (e->GetType() != EnemyType::Wave3MidBoss) continue;
			if (e->IsDead() || e->IsDying()) continue;

			e->SetAngry(angryDuration);
		}

		wave3PrevAliveMidBossCount_ = aliveMidBossCount;
		return;
	}

	// まだ蘇生フェーズに入っていないなら何もしない
	if (!wave3ReviveInProgress_) {
		wave3PrevAliveMidBossCount_ = aliveMidBossCount;
		return;
	}

	// ---- ここから「蘇生フェーズ中」 ----

	// 核が既に壊されている → 蘇生キャンセル
	if (!coreAlive) {
		wave3ReviveInProgress_ = false;
		wave3CoreTimer_ = 0.0f;
		if (player_) {
			player_->SetMidBossCore(nullptr);
		}
		midBossCore_.reset();
		wave3PrevAliveMidBossCount_ = aliveMidBossCount;
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

	wave3PrevAliveMidBossCount_ = aliveMidBossCount;
}

void EnemyManager::SpawnWave3MidBossStage() {
	if (!enemies_ || !dx_ || !cam_ || !parent_) {
		return;
	}
	// いったん全消し
	enemies_->clear();
	// 既存の核は捨てる
	midBossCore_.reset();

	// 蘇生状態リセット
	wave3ReviveInProgress_ = false;
	wave3CoreTimer_ = 0.0f;

	auto camPtr = cam_;
	auto dxPtr = dx_;
	auto parentPtr = parent_;

	// 左右 2 体の中ボスを直線で出して、手前で停止させる
	EnemySpawner::SpawnLine(
		*enemies_,
		2,
		wave3LeftPos_.y,
		wave3LeftPos_.z,
		wave3LeftPos_.x,
		(wave3RightPos_.x - wave3LeftPos_.x),
		dxPtr, camPtr, parentPtr,
		[this](Enemy& e) {

			// 中ボスの挙動設定
			e.SetBehavior(EnemyBehavior::FreeRoam);
			// ↓ 速度や範囲はあとでImGui化してもいい
			e.SetFreeRoamArea(
				{ -18.0f, 4.0f, 40.0f },   // min
				{ 18.0f,10.0f, 62.0f },   // max ← z を 70 → 62 に手前寄せ
				0.10f,                     // 通常速度
				0.24f                      // 怒り時速度
			);

			e.SetHP(12);
			e.SetScale({ 1.5f,1.5f,1.5f });

			SetupEnemyForPlayer(e);

			e.SetType(EnemyType::Wave3MidBoss);
		}
	);

	// 倒すべき中ボスは 2 体なので、ゲージ用に +2 だけ足しておく
	if (maxEnemyCount_) {
		*maxEnemyCount_ += 2;
	}

	// Wave3 開始時点では中ボスが 2 体生きている
	wave3PrevAliveMidBossCount_ = 2;
}

void EnemyManager::SpawnWave3Core() {
	if (!dx_ || !cam_ || !parent_) {
		return;
	}

	const auto& w3 = waveConfig_.GetWave3();

	float xRange = w3.coreXRange;
	float zMin = w3.coreZMin;
	float zMax = w3.coreZMax;

	float rx = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	float rz = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);

	float x = -xRange + rx * (xRange * 2.0f);
	float z = zMin + rz * (zMax - zMin);
	float y = w3.coreY;

	// すでにコアが居たら一旦消して作り直し
	midBossCore_ = std::make_unique<MidBossCore>();

	// Object3d 用共通（Enemy でも使ってるやつ）
	auto* common = Object3dCommon::GetInstance();

	midBossCore_->Initialize(common, dx_);
	midBossCore_->SetCamera(cam_);
	midBossCore_->SetParentScene(parent_);

	midBossCore_->SetPosition({ x, y, z });
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
	if (!enemies_ || !dx_ || !cam_ || !parent_) {
		return;
	}

	// どっちサイドの中ボスが生きているか調べる
	bool leftAlive = false;
	bool rightAlive = false;

	for (auto& e : *enemies_) {
		if (!e) continue;
		if (e->GetType() != EnemyType::Wave3MidBoss) continue;
		if (e->IsDead()) continue;

		Vector3 pos = e->GetWorldPosition();
		if (pos.x < 0.0f) {
			leftAlive = true;
		} else {
			rightAlive = true;
		}
	}

	// =========================================================
	// ★PDF方針：値の違いのための if/else をテーブル化
	// state: 0=両方死, 1=左だけ生, 2=右だけ生, 3=両方生(想定外)
	// 空いてる側に出す：左生->右 / 右生->左 / その他->左
	// =========================================================
	const int state = (leftAlive ? 1 : 0) | (rightAlive ? 2 : 0);

	static const int kSpawnSide[4] = {
		0, // 0: 両方死 -> 左
		1, // 1: 左だけ生 -> 右（空いてる側）
		0, // 2: 右だけ生 -> 左（空いてる側）
		0, // 3: 両方生(想定外) -> 左
	};

	const Vector3 kSidePos[2] = { wave3LeftPos_, wave3RightPos_ };
	const Vector3 spawnPos = kSidePos[kSpawnSide[state]];

	auto camPtr = cam_;
	auto dxPtr = dx_;
	auto parentPtr = parent_;

	EnemySpawner::SpawnLine(
		*enemies_,
		1,
		spawnPos.y,
		spawnPos.z,
		spawnPos.x,
		0.0f,
		dxPtr,
		camPtr,
		parentPtr,
		[this](Enemy& e) {
			e.SetBehavior(EnemyBehavior::StraightStop);
			e.SetVelocity({ 0.0f, 0.0f, -0.2f });
			e.SetStopZ(40.0f);
			e.SetHP(12); // 初期中ボスと同じ HP
			e.SetScale({ 1.5f,1.5f,1.5f });

			SetupEnemyForPlayer(e);
			e.SetType(EnemyType::Wave3MidBoss);
		}
	);
}

void EnemyManager::BeginWave1() {
	wave1SpawnTimer_ = 0.0f;
	if (maxEnemyCount_) {
		*maxEnemyCount_ = wave1DefeatTarget_;
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

void EnemyManager::Draw(DirectXCommon* dx) {
	if (!enemies_) { // enemies_ がまだ紐付いてなかったら何もしない
		return;
	}
	for (auto& enemy : *enemies_) { // 敵を全部描画
		enemy->Draw(dx);
	}
	if (midBossCore_) { // 核が居れば描画
		midBossCore_->Draw(dx);
	}
}

void EnemyManager::ImGuiDebug() {
#ifdef USE_IMGUI
	// まだ紐付いてないなら何もしない
	if (!enemies_) {
		return;
	}

	ImGui::Begin("敵ステータス");

	// 敵の動きを止めるトグル
	ImGui::Checkbox("敵の動きを止める", &freezeEnemies_);
	ImGui::Separator();

	// 撃破数／最大数
	int defeated = defeatedEnemyCount_ ? *defeatedEnemyCount_ : 0;
	int maxCount = maxEnemyCount_ ? *maxEnemyCount_ : static_cast<int>(enemies_->size());

	ImGui::Text("撃破数: %d / %d", defeated, maxCount);

	// 撃破進捗バー
	float progress = 0.0f;
	if (maxCount > 0) {
		progress = static_cast<float>(defeated) / static_cast<float>(maxCount);
	}
	ImGui::ProgressBar(progress, ImVec2(200, 20), "撃破進行度");

	// ===== Wave 状態表示 =====
	static const char* kWaveLabel[] = {
	"Wave1",
	"Wave2",
	"Wave3",
	"Bossフェーズ"
	};
	ImGui::Text("現在のWave: %s", kWaveLabel[static_cast<int>(wavePhase_)]);

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

		int aliveMidBoss = 0;
		int dyingMidBoss = 0;
		int deadMidBoss = 0;

		// 中ボスの状態を数える
		for (auto& e : *enemies_) {
			if (!e) continue;
			if (e->GetType() != EnemyType::Wave3MidBoss) continue;

			if (e->IsDead()) {
				deadMidBoss++;
			} else if (e->IsDying()) {
				dyingMidBoss++;
			} else {
				aliveMidBoss++;
			}
		}

		bool coreAlive = (midBossCore_ && !midBossCore_->IsDead() && !midBossCore_->IsDying());

		ImGui::Text("MidBoss Alive:%d  Dying:%d  Dead:%d", aliveMidBoss, dyingMidBoss, deadMidBoss);
		ImGui::Text("prevAliveMidBossCount: %d", wave3PrevAliveMidBossCount_);

		bool midBossJustDiedDbg = (aliveMidBoss < wave3PrevAliveMidBossCount_);
		ImGui::Text("midBossJustDied: %s", midBossJustDiedDbg ? "true" : "false");

		ImGui::Text("ReviveInProgress: %s", wave3ReviveInProgress_ ? "true" : "false");
		ImGui::Text("CoreAlive: %s", coreAlive ? "true" : "false");
		ImGui::Text("CoreTimer: %.2f / %.2f", wave3CoreTimer_, wave3CoreLifetime_);

		ImGui::Separator();
	}

	// 各敵のデバッグ（※二重ループになっていたので 1 回に統一）
	for (size_t i = 0; i < enemies_->size(); ++i) {
		ImGui::PushID(static_cast<int>(i));
		(*enemies_)[i]->ImGuiDebug();
		ImGui::PopID();
	}

	ImGui::End();

	// 核
	if (midBossCore_) {
		midBossCore_->ImGuiDebug();
	}
#endif
}