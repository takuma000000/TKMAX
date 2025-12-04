#include "EnemyManager.h"
#include <limits>
#include "MyMath.h"
#include "application/boss/manager/BossManager.h"

void EnemyManager::Initialize(DirectXCommon* dx, Camera* camera, BaseScene* parent, Player* player) {
	dx_ = dx;
	cam_ = camera;
	parent_ = parent;
	player_ = player;
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
	// ● Wave3 の核が居れば更新
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

	/// ───────────────────────────────────────────────
	/// ● 敵の状態を更新し、死亡したものは削除＆カウント
	/// ───────────────────────────────────────────────
	for (auto it = enemies_->begin(); it != enemies_->end(); ) {
		Enemy* e = it->get();
		e->Update();

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
	// ● Wave進行
	// ───────────────────────────────────────────────
	switch (wavePhase_) {
	case WavePhase::W1: {
		// Wave1: 「5体倒すまで無限湧き」
		// 5体倒したら次のWaveへ
		if (defeatedEnemyCount_ && *defeatedEnemyCount_ >= wave1DefeatTarget_) {
			GoToNextWave();
			return; // このフレームはここまで
		}

		// Wave1 用のスポーン制御
		UpdateWave1(dt);
		break;
	}
	case WavePhase::W2: {
		// --- Wave2 新仕様 ---
		UpdateWave2();
		break;
	}
	case WavePhase::W3: {
		// --- Wave3: 中ボスステージ（蘇生核含む） ---
		// 中ボスの生存数・核の状態などは UpdateWave3 側で管理する
		UpdateWave3(dt);
		break;
	}
	case WavePhase::Done:
	default:
		// 何もしない（全Wave終了・ボス管理などに任せる）
		break;
	}
}

void EnemyManager::UpdateClosestEnemy() {
	// プレイヤー or 敵リストが無効なら何もしない
	if (!player_ || !enemies_ || enemies_->empty()) {
		return;
	}

	/// ───────────────────────────────────────────────
	/// ● プレイヤーに最も近い「ザコ敵」を検出し、ターゲットとして設定する
	/// ───────────────────────────────────────────────

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
	if (!enemies_ || !dx_ || !cam_ || !parent_) {
		return;
	}

	// いったん全消し
	enemies_->clear();

	// 呼び出しで毎回書くのダルいのでローカルに詰める
	DirectXCommon* dxPtr = dx_;
	Camera* camPtr = cam_;
	BaseScene* parentPtr = parent_;

	switch (wavePhase_) {
	case WavePhase::W1: {
		// タイマー初期化と最初の1体スポーンだけを行う。
		wave1SpawnTimer_ = 0.0f;
		if (maxEnemyCount_) {
			*maxEnemyCount_ = wave1DefeatTarget_; // ゲージ用にリセット
		}
		SpawnWave1Enemy(); // 最初の1体だけ出す
		break;
	}
	case WavePhase::W2: {
		wave2SubWave_ = 0; // サブWave初期化
		SpawnWave2SubWave(0); // 最初のサブWaveをスポーン
		break;
	case WavePhase::W3: {
		// ここから中ボスステージ
		SpawnWave3MidBossStage();
		break;
	}
	case WavePhase::Done:
		// 何もしない
		break;
	}
	}
}

void EnemyManager::GoToNextWave() {
	if (wavePhase_ == WavePhase::W1) {
		wavePhase_ = WavePhase::W2;
		SpawnCurrentWave();
	} else if (wavePhase_ == WavePhase::W2) {
		wavePhase_ = WavePhase::W3;
		SpawnCurrentWave();
	} else if (wavePhase_ == WavePhase::W3) {
		wavePhase_ = WavePhase::Done; // 最終Waveまで終了
		// Done のときは Spawnしない（終わり）
	}

	// Waveごとに撃破カウントリセットする仕様ならここも移植
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

void EnemyManager::UpdateWave1(float dt) {
	if (!enemies_ || !dx_ || !cam_ || !parent_) {
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

	DirectXCommon* dxPtr = dx_;
	Camera* camPtr = cam_;
	BaseScene* parentPtr = parent_;

	// 出現位置（Xはちょっとランダム、Zは奥から）
	float y = 5.0f;
	float z = 100.0f;
	float xRange = 20.0f;
	float rx = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // 0〜1
	float x = -xRange + rx * (xRange * 2.0f); // -xRange〜+xRange

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
		[&](Enemy& e) {

			e.SetBehavior(EnemyBehavior::PounceFromAbove);

			Vector3 start = { x, 20.0f, z }; // 高い位置から降ってくる
			Vector3 playerPos = player_->GetPosition(); // プレイヤー位置取得
			// プレイヤーの少し手前に着地するようにターゲット設定
			Vector3 target = { playerPos.x,
							   playerPos.y,
							   playerPos.z + 3.0f };
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

			if (player_) {
				e.SetReticle(player_->GetReticle());
				e.SetPlayer([this]() { return player_->GetPosition(); }); // プレイヤー位置参照セット
			}
		}
	);
}

void EnemyManager::UpdateWave2() {
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

void EnemyManager::SpawnWave2SubWave(int id) {
	if (!enemies_ || !dx_ || !cam_ || !parent_) return;
	enemies_->clear(); // 念のためクリア

	switch (id) { // サブWaveごとにパターン分け
	case 0: SpawnWave2_Triangle();  break; // 下2 上1 の三角隊列
	case 1: SpawnWave2_Line();      break; // 横一列
	case 2: SpawnWave2_FastColumn(); break; // 右側高速通過
	}
}

// ───────────────────────────────────────────────
// ● Wave2 各小Waveスポーン関数群
// ───────────────────────────────────────────────
void EnemyManager::SpawnWave2_Triangle() {
	int idx = 0;

	EnemySpawner::SpawnV(
		*enemies_,
		1,  // 1段
		6.0f, 80.0f,
		0.0f,
		7.0f,
		5.0f,
		dx_, cam_, parent_,
		[&](Enemy& e) {
			e.SetBehavior(EnemyBehavior::SineX);
			e.SetVelocity({ 0,0,-0.30f });
			e.SetSineParams(4.0f, 1.4f);
			e.SetSinePhase(0.6f * float(idx++));
			e.SetHP(3);
			e.SetReticle(player_->GetReticle());
		}
	);
}
void EnemyManager::SpawnWave2_Line() {
	EnemySpawner::SpawnLine(
		*enemies_,
		4, 4.5f, 90.0f,
		-12.0f, 8.0f,
		dx_, cam_, parent_,
		[&](Enemy& e) {
			e.SetBehavior(EnemyBehavior::StraightStop);
			e.SetVelocity({ 0,0,-0.32f });
			e.SetStopZ(52.0f);
			e.SetHP(2);
			e.SetReticle(player_->GetReticle());
		}
	);
}
void EnemyManager::SpawnWave2_FastColumn() {
	EnemySpawner::SpawnColumn(
		*enemies_,
		3,
		18.0f,
		100.0f,
		10.0f,
		5.0f,
		0.0f,
		dx_, cam_, parent_,
		[&](Enemy& e) {
			e.SetBehavior(EnemyBehavior::StraightStop);
			e.SetVelocity({ -0.20f, 0.0f, -0.75f });
			e.SetStopZ(-50.0f); // 通過するだけ
			e.SetHP(1);
			e.SetReticle(player_->GetReticle());
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
		if (e->GetType() == EnemyType::Wave3MidBoss && !e->IsDead()) {
			aliveMidBossCount++;
		}
	}

	// ── 核の生存状態は MidBossCore で判定 ──
	bool coreAlive = (midBossCore_ && !midBossCore_->IsDead() && !midBossCore_->IsDying());

	// 「このフレームで中ボスが減ったか？」
	bool midBossJustDied = (aliveMidBossCount < wave3PrevAliveMidBossCount_);

	// === ここから下のロジックも MidBossCore ベースに置き換え ===

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
		2,                             // 敵の数
		wave3LeftPos_.y,               // Y は左右同じ
		wave3LeftPos_.z,               // Z 開始位置
		wave3LeftPos_.x,               // X 開始（左）
		(wave3RightPos_.x - wave3LeftPos_.x), // X間隔（右まで）
		dxPtr,
		camPtr,
		parentPtr,
		[&](Enemy& e) {
			e.SetBehavior(EnemyBehavior::StraightStop);
			e.SetVelocity({ 0.0f, 0.0f, -0.2f });
			e.SetStopZ(40.0f); // ある程度手前で止まる
			e.SetHP(12);       // 仮の中ボスHP（あとで調整）
			e.SetScale({ 1.5f,1.5f,1.5f }); // ちょっと大きめにしてボス感

			if (player_) {
				e.SetReticle(player_->GetReticle());
				e.SetPlayer([this]() { return player_->GetPosition(); });
			}

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

	// 画面内っぽい範囲でランダムに配置（ざっくり）
	float xRange = 18.0f;
	float zMin = 35.0f;
	float zMax = 75.0f;

	float rx = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	float rz = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);

	float x = -xRange + rx * (xRange * 2.0f);
	float z = zMin + rz * (zMax - zMin);
	float y = 6.0f;

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

	// 片方だけ生きている想定なので、空いている側に復活させる
	Vector3 spawnPos = wave3LeftPos_;
	if (leftAlive && !rightAlive) {
		spawnPos = wave3RightPos_;
	} else if (!leftAlive && rightAlive) {
		spawnPos = wave3LeftPos_;
	} else {
		// 想定外だけど、両方死んでいたら左側に出しておく
		spawnPos = wave3LeftPos_;
	}

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
		[&](Enemy& e) {
			e.SetBehavior(EnemyBehavior::StraightStop);
			e.SetVelocity({ 0.0f, 0.0f, -0.2f });
			e.SetStopZ(40.0f);
			e.SetHP(12); // 初期中ボスと同じ HP
			e.SetScale({ 1.5f,1.5f,1.5f });

			if (player_) {
				e.SetReticle(player_->GetReticle());
				e.SetPlayer([this]() { return player_->GetPosition(); });
			}
			e.SetType(EnemyType::Wave3MidBoss);
		}
	);
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

	// 撃破数／最大数
	int defeated = defeatedEnemyCount_ ? *defeatedEnemyCount_ : 0;
	int maxCount = maxEnemyCount_ ? *maxEnemyCount_ : static_cast<int>(enemies_->size());

	ImGui::Text("撃破数: %d / %d", defeated, maxCount);

	// 各敵のデバッグ
	for (size_t i = 0; i < enemies_->size(); ++i) {
		ImGui::PushID(static_cast<int>(i));
		(*enemies_)[i]->ImGuiDebug();
		ImGui::PopID();
	}

	// 撃破進捗バー
	float progress = 0.0f;
	if (maxCount > 0) {
		progress = static_cast<float>(defeated) / static_cast<float>(maxCount);
	}
	ImGui::ProgressBar(progress, ImVec2(200, 20), "撃破進行度");

	// ===== Wave 状態表示 =====
	const char* waveLabel = "";
	switch (wavePhase_) {
	case WavePhase::W1:   waveLabel = "Wave1";        break;
	case WavePhase::W2:   waveLabel = "Wave2";        break;
	case WavePhase::W3:   waveLabel = "Wave3";        break;
	case WavePhase::Done: waveLabel = "Bossフェーズ"; break;
	}
	ImGui::Text("現在のWave: %s", waveLabel);

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

	// 各敵のデバッグ
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