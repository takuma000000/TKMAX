#include "BossManager.h"
#include "application/scene/GameScene.h"

void BossManager::Initialize(DirectXCommon* dxCommon, Camera* camera, BaseScene* parent, Player* player) {
	dxCommon_ = dxCommon;
	camera_ = camera;
	parentScene_ = parent;
	player_ = player;

	bossBattle_ = false;
	bossP2BgmPlayed_ = false;
	boss_.reset();
	bossBullets_.clear();
}

void BossManager::StartBattle() {
	// すでにボス戦中なら何もしない
	if (bossBattle_) {
		return;
	}

	if (!dxCommon_ || !camera_ || !parentScene_) {
		return;
	}

	bossBattle_ = true;
	bossP2BgmPlayed_ = false;

	boss_ = std::make_unique<BossEnemy>();
	boss_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	boss_->SetCamera(camera_);
	boss_->SetParentScene(parentScene_);

	// プレイヤー位置取得ラムダ
	if (player_) {
		boss_->SetPlayer([this]() {
			return player_->GetPosition();
			});
	}

	// 既存と同じ初期位置（奥から登場）
	boss_->SetPosition({ 0, 0, 200 });
}

void BossManager::Update(float dt) {
	if (!bossBattle_ || !boss_) {
		// ボス戦中でないなら弾だけ掃除しておく
		for (auto it = bossBullets_.begin(); it != bossBullets_.end();) {
			(*it)->Update();
			if ((*it)->IsDead()) {
				it = bossBullets_.erase(it);
			} else {
				++it;
			}
		}
		return;
	}

	// ボス本体更新
	boss_->Update();

	// P2突入時にBGMを1回だけ再生（GameScene にあった処理を移植）
	if (!bossP2BgmPlayed_) {
		int phase = boss_->GetPhase(); // P1=0, P2=1, P3=2
		if (phase == 1) { // P2
			//AudioManager::GetInstance()->PlaySound("bossP2");
			bossP2BgmPlayed_ = true;
		}
	}

	// ボス弾更新
	for (auto it = bossBullets_.begin(); it != bossBullets_.end();) {
		(*it)->Update();
		if ((*it)->IsDead()) {
			it = bossBullets_.erase(it);
		} else {
			++it;
		}
	}
}

void BossManager::Draw(DirectXCommon* dxCommon) {
	if (bossBattle_ && boss_) {
		boss_->Draw(dxCommon);
	}

	for (auto& b : bossBullets_) {
		b->Draw(dxCommon);
	}
}

void BossManager::SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame) {
	if (!dxCommon_ || !camera_) {
		return;
	}

	auto bullet = std::make_unique<BossBullet>();
	bullet->Initialize(Object3dCommon::GetInstance(), dxCommon_, camera_, pos, dir, speed, damage, lifeFrame);
	bossBullets_.push_back(std::move(bullet));
}

bool BossManager::IsBattleActive() const {
	return bossBattle_ && boss_ != nullptr && !boss_->IsDead();
}

bool BossManager::IsBossAlive() const {
	return boss_ && !boss_->IsDead();
}

bool BossManager::IsBossDead() const {
	return boss_ && boss_->IsDead();
}

void BossManager::OnClearSequenceStart() {
	// クリア演出に入るタイミングでボス関連を全部破棄
	bossBattle_ = false;
	bossBullets_.clear();
	boss_.reset();
	bossP2BgmPlayed_ = false;
}