#include "EnemyManager.h"
#include <limits>
#include "MyMath.h"
#include "BossManager.h"

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
		Enemy* e = it->get(); // erase 前に生存中の生ポインタを保持
		e->Update();

		if (e->IsDead()) {
			// 死亡していたら
			if (player_) {
				player_->OnEnemyDestroyed(e); // プレイヤーに通知
			}

			if (defeatedEnemyCount_) {
				++(*defeatedEnemyCount_);      // 倒した数をカウント
				if (*defeatedEnemyCount_ == 3 && player_) {
					player_->EnableSpecialAttack();
				}
			}

			it = enemies_->erase(it);          // erase でイテレータが無効化されるので注意
		} else {
			++it;
		}
	}

	// 「敵が全滅」かつ「まだ最後のWaveじゃない」なら次のWaveへ
	if (enemies_->empty() && wavePhase_ != WavePhase::Done) {
		GoToNextWave(); // Wave 進行
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
		*defeatedEnemyCount_ = 0;   // ついでに進捗をリセット
	}
	if (maxEnemyCount_) {
		*maxEnemyCount_ = 0;        // 全Wave合計で加算していく
	}

	wavePhase_ = WavePhase::W1; // Wave1から
	SpawnCurrentWave();         // 最初のWaveだけ出す（ここでmaxEnemyCount_も加算）

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
		// W1: 直進停止（密度で圧）＋HP控えめ
		EnemySpawner::SpawnLine(
			*enemies_,                   // ★ ポインタではなく参照にして渡す
			5,                           // count
			/*y*/ 5.0f,
			/*z*/ 60.0f,
			-20.0f,                      // xStart
			10.0f,                       // xStep
			dxPtr,
			camPtr,
			parentPtr,
			[&](Enemy& e) {
				e.SetBehavior(EnemyBehavior::StraightStop);
				e.SetVelocity({ 0,0,-0.25f });
				e.SetStopZ(60.0f);
				e.SetHP(2);
				e.SetScale({ 1.1f,1.1f,1.1f });
			}
		);
		if (maxEnemyCount_) {
			*maxEnemyCount_ += 5;
		}
		break;
	}
	case WavePhase::W2: {
		// W2: サイン蛇行で避けにくく（重なり防止で位相＆停止Zを個体別にオフセット）
		int idx = 0;                  // 個体インデックス（ラムダ内でインクリメント）
		const float phaseStep = 0.7f; // 位相刻み（ラジアン）
		const float stopStep = 0.6f;  // 停止Zのズラし量

		EnemySpawner::SpawnV(
			*enemies_,
			3,                          // V字の列数（中央＋左右）
			/*y*/ 6.0f,
			/*z*/ 80.0f,
			0.0f,                       // centerX
			8.0f,                       // xStep
			6.0f,                       // zStep
			dxPtr,
			camPtr,
			parentPtr,
			[&](Enemy& e) {
				e.SetBehavior(EnemyBehavior::SineX);
				e.SetVelocity({ 0,0,-0.22f });
				e.SetSineParams(/*ampX*/ 6.0f, /*freq*/ 1.6f);

				// 個体ごとに位相と停止Zを少しずつズラす
				e.SetSinePhase(phaseStep * float(idx));
				e.SetStopZ(60.0f + stopStep * float(idx % 3));

				e.SetHP(3);
				++idx;
			}
		);
		if (maxEnemyCount_) {
			// 中央1 + 左右3*2 = 7体
			*maxEnemyCount_ += 7;
		}
		break;
	}
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