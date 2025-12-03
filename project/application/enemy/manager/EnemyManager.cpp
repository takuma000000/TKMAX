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
	if (wavePhase_ == WavePhase::W1) {
		// Wave1: 「5体倒すまで無限湧き」

		// 5体倒したら次のWaveへ
		if (defeatedEnemyCount_ && *defeatedEnemyCount_ >= wave1DefeatTarget_) {
			GoToNextWave();
			return; // このフレームはここまで
		}

		// Wave1 用のスポーン制御
		UpdateWave1(dt);
	} 

	// --- Wave2 新仕様 ---
	if (wavePhase_ == WavePhase::W2) {
		UpdateWave2();
		return;
	}

	// --- Wave3 ---
	if (wavePhase_ == WavePhase::W3) {
		if (enemies_->empty()) {
			GoToNextWave();
		}
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
		// W3: 追尾＋左右ストレーフ混在で圧を上げる
		EnemySpawner::SpawnColumn(
			*enemies_,
			6,                          // count
			/*x*/ 25.0f,
			/*zStart*/ 100.0f,
			/*zStep*/ 10.0f,
			/*y*/ 4.0f,
			/*intervalSec*/ 0.5f,       // （EnemySpawner 実装に合わせて）
			dxPtr,
			camPtr,
			parentPtr,
			[&](Enemy& e) {
				// 交互にパターン変える例
				static int idx = 0;
				if ((idx++ % 2) == 0) {
					e.SetBehavior(EnemyBehavior::ChasePlayer);
					e.SetVelocity({ 0,0,-0.20f });
					e.SetStopZ(34.0f);
					// 追尾用にプレイヤー位置の参照を渡す
					e.SetPlayer([this]() { return player_->GetPosition(); });
					e.SetHP(3);
				} else {
					e.SetBehavior(EnemyBehavior::StrafeLtoR);
					e.SetVelocity({ 0,0,-0.25f });
					e.SetStopZ(60.0f);
					e.SetStrafeX(-18.0f, 18.0f, 0.45f);
					e.SetHP(4);
				}
				if (player_) { // レティクルをセット
					e.SetReticle(player_->GetReticle());
				}
			}
		);
		if (maxEnemyCount_) {
			*maxEnemyCount_ += 6;
		}
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

void EnemyManager::Draw(DirectXCommon* dx) {
	if (!enemies_) {
		return;
	}
	for (auto& enemy : *enemies_) {
		enemy->Draw(dx);
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
#endif
}