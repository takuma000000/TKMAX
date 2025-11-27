#include "EnemyManager.h"
#include <limits>
#include "MyMath.h"

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
		if (!enemy->IsDead()) {
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

	ImGui::End();
#endif
}