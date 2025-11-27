#include "EnemyManager.h"

void EnemyManager::Initialize(DirectXCommon* dx, Camera* camera, BaseScene* parent, Player* player){
	dx_ = dx;
	cam_ = camera;
	parent_ = parent;
	player_ = player;
}

void EnemyManager::Update(float dt){
	(void)dt;

	// まだゲーム開始前 or バインドされてないときは何もしない
	if (!enemies_ || !player_) {
		return;
	}

	// ===== ここが元 GameScene::UpdateEnemies の中身 =====
	for (auto it = enemies_->begin(); it != enemies_->end(); ) {
		Enemy* e = it->get();      // erase 前に生存中の生ポインタを保持
		e->Update();

		if (e->IsDead()) {
			// 死亡していたら
			player_->OnEnemyDestroyed(e); // プレイヤーに通知

			// 撃破カウント（ポインタが有効なときだけ）
			if (defeatedEnemyCount_) {
				++(*defeatedEnemyCount_);
				if (*defeatedEnemyCount_ == 3) {
					player_->EnableSpecialAttack();
				}
			}

			it = enemies_->erase(it); // erase でイテレータが無効化されるので注意
		} else {
			++it;
		}
	}
}

void EnemyManager::Draw(DirectXCommon* dx){
	if (!enemies_) {
		return;
	}

	for (auto& e : *enemies_) {
		if (e) {
			e->Draw(dx);
		}
	}
}

void EnemyManager::BindEnemyData(std::vector<std::unique_ptr<Enemy>>* enemies,
	int* defeatedCount,
	int* maxCount){
	enemies_ = enemies;
	defeatedEnemyCount_ = defeatedCount;
	maxEnemyCount_ = maxCount;
}

void EnemyManager::ImGuiDebug(){
#ifdef USE_IMGUI
	ImGui::Begin("EnemyManager");

	int enemyCount = enemies_ ? static_cast<int>(enemies_->size()) : 0;
	ImGui::Text("Managed enemies: %d", enemyCount);

	if (defeatedEnemyCount_ && maxEnemyCount_) {
		ImGui::Text("Defeated: %d / %d", *defeatedEnemyCount_, *maxEnemyCount_);
	}

	ImGui::End();
#endif
}
