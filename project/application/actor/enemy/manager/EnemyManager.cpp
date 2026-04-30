#include "EnemyManager.h"
#include <limits>
#include "MyMath.h"
#include "manager/BossManager.h"
#include "BarrierCommon.h"
#include "AudioManager.h"
#include "EnemyFactory.h"
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

//=============================================================
// 初期化
//=============================================================
void EnemyManager::Initialize(TKM::DirectXCommon* dx, TKM::Camera* camera, TKM::BaseScene* parent, Player* player) {
	//=========================================================
	// 共通初期化
	//=========================================================
	InitializeCommon(dx, camera, parent, player);

	//=========================================================
	// BarrierCommon初期化
	//=========================================================
	TKM::BarrierCommon::GetInstance()->Initialize(dx);

	//=========================================================
	// 敵遭遇設定読み込み
	//=========================================================
	const bool loaded_ = encounterConfig_.Load("./resources/data/enemy_encounter.json");
	assert(loaded_ && "enemy_encounter.json の読込に失敗しました");
}

//=============================================================
// 更新
//=============================================================
void EnemyManager::Update(float dt) {
	// Wave初期化前なら何もしない
	if (!initializedWaves_) {
		return;
	}

	//=========================================================
	// バリア更新
	//=========================================================
	if (barrier_) {
		if (player_) {
			Vector3 flashPos;
			if (player_->ConsumeWave1BarrierFlashRequest(flashPos)) {
				barrier_->OnHit(flashPos);
			}
		}

		UpdateBarrier_();
		barrier_->Update(dt);
	}

	// プレイヤーへバリア情報同期
	SyncBarrierInfoToPlayer_();

	//=========================================================
	// バリアコア更新
	//=========================================================
	if (barrierCoreManager_) {
		barrierCoreManager_->Update(dt);
	}

	//=========================================================
	// 敵更新
	//=========================================================
	for (auto it = enemies_.begin(); it != enemies_.end();) {
		Enemy* e_ = it->get();

		e_->SetFreezeMove(freezeEnemies_);
		e_->Update(dt);

		//=====================================================
		// 敵死亡処理
		//=====================================================
		if (e_->IsDead()) {
			if (player_) {
				player_->OnEnemyDestroyed(e_);
			}

			if (e_->GetDefeated()) {
				if (e_->GetType() == EnemyType::MainSquad) {
					++mainSquadDefeatedCount_;
				}
				++defeatedEnemyCount_;

				// 一定数撃破で特殊攻撃解禁
				if (defeatedEnemyCount_ == 3 && player_) {
					player_->EnableSpecialAttack();
				}
			}

			it = enemies_.erase(it);
		} else {
			++it;
		}
	}

	//=========================================================
	// 雑魚敵フェーズ更新
	//=========================================================
	if (enemyPhase_ == EnemyPhase::SmallEnemyBattle) {
		UpdateMainSquadBattle_(dt);
	}

	//=========================================================
	// 敵弾更新・当たり判定
	//=========================================================
	UpdateEnemyBullets_(dt);
	CheckEnemyBulletPlayerCollision_(dt);
}

//=============================================================
// 最も近い敵を更新
//=============================================================
void EnemyManager::UpdateClosestEnemy() {
	if (!player_) { return; }       // Player無効なら何もしない
	if (enemies_.empty()) { return; } // 敵リストが空なら何もしない

	Enemy* closestEnemy_ = nullptr;                              // 最も近い敵
	float closestDistance_ = std::numeric_limits<float>::max(); // 最短距離
	Vector3 playerPos_ = player_->GetPosition();                // プレイヤー位置

	for (auto& enemy : enemies_) {
		if (!enemy) { continue; } // 念のためヌルチェック

		if (!enemy->IsDead() && !enemy->IsDying()) {
			float dist_ = MyMath::Length(enemy->GetWorldPosition() - playerPos_);
			if (dist_ < closestDistance_) {
				closestDistance_ = dist_;
				closestEnemy_ = enemy.get();
			}
		}
	}

	player_->SetEnemy(closestEnemy_);
	player_->SetAllEnemies(&enemies_);
}

//=============================================================
// 雑魚敵フェーズ開始
//=============================================================
void EnemyManager::StartSmallEnemyPhase() {
	if (!player_) {
		return;
	}

	//=========================================================
	// 前状態クリア
	//=========================================================
	NotifyPlayerBeforeClearEnemies_(); // ロックオン解除などのため通知
	enemies_.clear();
	enemyBullets_.clear();
	playerHitCooldown_ = 0.0f;
	specialCoreCharging_ = false;
	specialCoreBullet_ = nullptr;
	scatterShotTimer_ = 0.0f;

	//=========================================================
	// 撃破数初期化
	//=========================================================
	defeatedEnemyCount_ = 0;
	maxEnemyCount_ = std::max(0, encounterConfig_.GetSmallEnemyPhase().defeatTarget_);

	//=========================================================
	// フェーズ開始
	//=========================================================
	enemyPhase_ = EnemyPhase::SmallEnemyBattle;
	BeginMainSquadBattle_();

	initializedWaves_ = true;

	//=========================================================
	// 最初のロックオン対象設定
	//=========================================================
	if (!enemies_.empty()) {
		player_->SetEnemy(enemies_.front().get());
		player_->SetAllEnemies(&enemies_);
	}
}

//=============================================================
// 雑魚敵フェーズリセット
//=============================================================
void EnemyManager::ResetSmallEnemyPhase() {
	if (!dx_ || !camera_ || !parent_) {
		return;
	}

	//=========================================================
	// 戦闘状態クリア
	//=========================================================
	NotifyPlayerBeforeClearEnemies_();
	enemies_.clear();
	enemyBullets_.clear();
	playerHitCooldown_ = 0.0f;
	specialCoreCharging_ = false;
	specialCoreBullet_ = nullptr;
	scatterShotTimer_ = 0.0f;

	SetBarrierActive_(false);
	SyncBarrierInfoToPlayer_();

	//=========================================================
	// 雑魚敵フェーズ中なら再開
	//=========================================================
	if (enemyPhase_ == EnemyPhase::SmallEnemyBattle) {
		BeginMainSquadBattle_();
	}
}

//=============================================================
// 雑魚敵フェーズ終了
//=============================================================
void EnemyManager::FinishSmallEnemyPhase() {
	//=========================================================
	// 現在いるザコ敵を全消去
	//=========================================================
	NotifyPlayerBeforeClearEnemies_();
	enemies_.clear();
	enemyBullets_.clear();
	playerHitCooldown_ = 0.0f;
	specialCoreCharging_ = false;
	specialCoreBullet_ = nullptr;
	scatterShotTimer_ = 0.0f;

	SetBarrierActive_(false);
	SyncBarrierInfoToPlayer_();

	//=========================================================
	// 撃破数・最大数リセット
	//=========================================================
	if (defeatedEnemyCount_) {
		defeatedEnemyCount_ = 0;
	}
	if (maxEnemyCount_) {
		maxEnemyCount_ = 0;
	}

	//=========================================================
	// ボスフェーズへ移行
	//=========================================================
	enemyPhase_ = EnemyPhase::BossReady;

	// 雑魚戦BGM停止
	TKM::AudioManager::GetInstance()->StopSound("playBGM");
}

//=============================================================
// 雑魚敵フェーズ終了判定
//=============================================================
bool EnemyManager::IsSmallEnemyPhaseFinished() const {
	return (enemyPhase_ == EnemyPhase::BossReady) && enemies_.empty();
}

//=============================================================
// 敵へプレイヤー参照を設定
//=============================================================
void EnemyManager::SetupEnemyForPlayer(Enemy& e) {
	if (!player_) {
		return;
	}

	e.SetReticle(player_->GetReticle());
	e.SetPlayer([this]() { return player_->GetPosition(); });
}

//=============================================================
// 敵全削除前のプレイヤー通知
//=============================================================
void EnemyManager::NotifyPlayerBeforeClearEnemies_() {
	if (!player_) {
		return;
	}

	// これから敵が全滅することを通知し、ロックオン解除などを行わせる
	for (auto& e : enemies_) {
		if (!e) { continue; }
		player_->OnEnemyDestroyed(e.get());
	}
}

//=============================================================
// 雑魚本隊戦更新
//=============================================================
void EnemyManager::UpdateMainSquadBattle_(float dt) {
	UpdateEnemyBullets_(dt);

	//=========================================================
	// プレイヤー被弾クールタイム更新
	//=========================================================
	if (playerHitCooldown_ > 0.0f) {
		playerHitCooldown_ -= dt;
		if (playerHitCooldown_ < 0.0f) {
			playerHitCooldown_ = 0.0f;
		}
	}

	//=========================================================
	// 本隊全滅でボス戦へ移行
	//=========================================================
	if (CountAliveMainSquad_() <= 0) {
		NotifyPlayerBeforeClearEnemies_();
		enemies_.clear();

		ClearBarrierCores_();

		FinishSmallEnemyPhase();
		return;
	}

	//=========================================================
	// 本隊状態ごとの更新
	//=========================================================
	switch (mainSquadPhase_) {
	case MainSquadPhase::BarrierBattle:
		UpdateMainSquadOrbit_(dt);
		UpdateScatterAttack_(dt);
		UpdateSpecialAttackCycle_(dt);
		SetMainSquadInvincible_(true);

		if (AreAllBarrierCoresDestroyed_()) {
			BreakBarrier_();
			return;
		}
		break;

	case MainSquadPhase::ExposedBattle:
		UpdateMainSquadOrbit_(dt);
		UpdateScatterAttack_(dt);
		UpdateSpecialAttackCycle_(dt);
		break;
	}
}

//=============================================================
// 通常散弾攻撃更新
//=============================================================
void EnemyManager::UpdateScatterAttack_(float dt) {

	///=============================
	return; // 散弾攻撃は一旦封印
	///=============================

	scatterShotTimer_ += dt;
	if (scatterShotTimer_ < scatterShotInterval_) {
		return;
	}
	scatterShotTimer_ = 0.0f;

	auto* common_ = TKM::Object3dCommon::GetInstance();
	if (!common_) {
		return;
	}

	for (auto& e : enemies_) {
		if (!e || e->IsDead() || e->IsDying()) {
			continue;
		}

		Vector3 start_ = e->GetWorldPosition();
		Vector3 target_ = player_ ? player_->GetPosition() : (start_ + Vector3{ 0.0f, 0.0f, -30.0f });
		Vector3 dir_ = target_ - start_;

		float len_ = MyMath::Length(dir_);
		if (len_ <= 0.0001f) {
			continue;
		}
		dir_ = dir_ / len_;

		auto bullet_ = std::make_unique<EnemyBullet>();
		bullet_->Initialize(common_, dx_, camera_, start_, dir_ * scatterShotBulletSpeed_);
		enemyBullets_.push_back(std::move(bullet_));
	}
}

//=============================================================
// 特殊コア攻撃チャージ開始
//=============================================================
void EnemyManager::BeginSpecialCoreCharge_() {
	auto* common_ = TKM::Object3dCommon::GetInstance();
	if (!common_) {
		return;
	}

	specialCoreCharging_ = true;
	specialCoreBullet_ = nullptr;

	const Vector3 corePos_ = GetSpecialCorePosition_();
	auto core_ = std::make_unique<EnemyBullet>();
	core_->InitializeSpecialCore(
		common_,
		dx_,
		camera_,
		corePos_,
		specialCoreStartScale_,
		specialCoreEndScale_,
		specialCoreRadius_,
		specialCoreChargeDuration_,
		specialCoreDamage_
	);

	specialCoreBullet_ = core_.get();
	enemyBullets_.push_back(std::move(core_));
}

//=============================================================
// 特殊コア攻撃チャージ更新
//=============================================================
void EnemyManager::UpdateSpecialCoreCharge_(float dt) {
	if (!specialCoreCharging_ || !specialCoreBullet_) {
		return;
	}

	EmitSpecialCoreChargeParticles_();

	if (specialCoreBullet_->IsDead()) {
		specialCoreCharging_ = false;
		specialCoreBullet_ = nullptr;
		return;
	}

	if (mainSquadPhaseTimer_ >= specialCoreChargeDuration_) {
		FireSpecialCore_();
		specialCoreCharging_ = false;
	}
}

//=============================================================
// 特殊コア発射
//=============================================================
void EnemyManager::FireSpecialCore_() {
	if (!specialCoreBullet_) {
		return;
	}

	Vector3 start_ = specialCoreBullet_->GetWorldPosition();
	Vector3 target_ = player_ ? player_->GetPosition() : (start_ + Vector3{ 0.0f, 0.0f, -30.0f });

	Vector3 dir_ = target_ - start_;
	float len_ = MyMath::Length(dir_);
	if (len_ <= 0.0001f) {
		return;
	}
	dir_ = dir_ / len_;

	specialCoreBullet_->LaunchSpecialCore(dir_ * specialCoreShotSpeed_);

	//=========================================================
	// 発射時主役演出
	//=========================================================
	auto* pm_ = TKM::ParticleManager::GetInstance();
	if (pm_) {
		const bool priority_ = true;

		pm_->Emit("w1sp_core_flash", start_, pm_->GetEmitCountScaled(1, priority_));
		pm_->Emit("w1sp_core_spark", start_, pm_->GetEmitCountScaled(3, priority_));
		pm_->Emit("w1sp_core_body", start_, pm_->GetEmitCountScaled(2, priority_));

		pm_->Emit("w1sp_core_flash", start_, pm_->GetEmitCountScaled(2, priority_));
		pm_->Emit("w1sp_core_shell", start_, pm_->GetEmitCountScaled(1, priority_));
		pm_->Emit("w1sp_core_burst", start_, pm_->GetEmitCountScaled(6, priority_));
		pm_->Emit("w1sp_core_arc", start_, pm_->GetEmitCountScaled(3, priority_));
	}

	specialCoreBullet_ = nullptr;
}

//=============================================================
// 特殊コアチャージ中パーティクル
//=============================================================
void EnemyManager::EmitSpecialCoreChargeParticles_() {
	TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
	if (!pm_) {
		return;
	}

	const Vector3 corePos_ = GetSpecialCorePosition_();
	const auto loadLevel_ = pm_->GetLoadLevel();

	//=========================================================
	// 負荷に応じた線分数調整
	//=========================================================
	int segmentCount_ = 6;
	switch (loadLevel_) {
	case TKM::ParticleManager::LoadLevel::Low:
		segmentCount_ = 6;
		break;

	case TKM::ParticleManager::LoadLevel::Medium:
		segmentCount_ = 5;
		break;

	case TKM::ParticleManager::LoadLevel::High:
		segmentCount_ = 4;
		break;

	case TKM::ParticleManager::LoadLevel::Critical:
		segmentCount_ = 3;
		break;
	}

	//=========================================================
	// 生存している敵全員からコアへ流す
	//=========================================================
	for (auto& e : enemies_) {
		if (!e || e->IsDead() || e->IsDying()) {
			continue;
		}

		Enemy* e_ = e.get();
		const Vector3 src_ = e_->GetWorldPosition();

		Vector3 dir_ = corePos_ - src_;
		float len_ = MyMath::Length(dir_);
		if (len_ <= 0.0001f) {
			continue;
		}
		dir_ = dir_ / len_;

		// 軽い横ブレで点列感を減らす
		Vector3 side_ = { -dir_.z, 0.0f, dir_.x };
		if (MyMath::Length(side_) <= 0.0001f) {
			side_ = { 1.0f, 0.0f, 0.0f };
		} else {
			side_ = MyMath::Normalize(side_);
		}

		for (int seg_ = 1; seg_ <= segmentCount_; ++seg_) {
			const float u_ = static_cast<float>(seg_) / static_cast<float>(segmentCount_ + 1);

			// コアに近いほど密になるよう後半へ寄せる
			const float t_ = 1.0f - (1.0f - u_) * (1.0f - u_);

			Vector3 p_ = src_ + dir_ * (len_ * t_);

			const float sideJitter_ = (0.16f - 0.10f * t_);
			if ((seg_ % 2) == 0) {
				p_ += side_ * sideJitter_;
			} else {
				p_ -= side_ * sideJitter_;
			}

			// 芯はなるべく残す
			pm_->Emit("w1sp_stream_core", p_, pm_->GetEmitCountScaled(1, true));

			// 外側グローは補助
			//pm_->Emit("w1sp_stream_glow", p_, pm_->GetEmitCountScaled(1, false));

			// 細線は2個に1回
			//if ((seg_ % 2) == 0) {
			//	pm_->Emit("w1sp_stream_streak", p_, pm_->GetEmitCountScaled(1, false));
			//}

			//// 丸粒はかなり補助
			//if ((seg_ % 4) == 0) {
			//	pm_->Emit("w1sp_stream", p_, pm_->GetEmitCountScaled(1, false));
			//}
		}

		// 発射元の火花
		//pm_->Emit("w1sp_sender_glow", src_, pm_->GetEmitCountScaled(1, false));
	}

	//=========================================================
	// コア本体の見た目
	//=========================================================
	pm_->Emit("w1sp_core_body", corePos_, pm_->GetEmitCountScaled(2, true));
	pm_->Emit("w1sp_core_inner", corePos_, pm_->GetEmitCountScaled(2, true));
	//pm_->Emit("w1sp_core_ring", corePos_, pm_->GetEmitCountScaled(1, true));
	//pm_->Emit("w1sp_core_shell", corePos_, pm_->GetEmitCountScaled(1, true));
	//pm_->Emit("w1sp_core_smoke", corePos_, pm_->GetEmitCountScaled(0, false));
	pm_->Emit("w1sp_core_arc", corePos_, pm_->GetEmitCountScaled(1, false));

	//=========================================================
	// チャージ終盤の加速演出
	//=========================================================
	if (mainSquadPhaseTimer_ >= specialCoreChargeDuration_ * 0.55f) {
		//pm_->Emit("w1sp_core_flash", corePos_, pm_->GetEmitCountScaled(1, true));
		//pm_->Emit("w1sp_core_spark", corePos_, pm_->GetEmitCountScaled(3, false));
		//pm_->Emit("w1sp_core_arc", corePos_, pm_->GetEmitCountScaled(1, false));
	}
}

//=============================================================
// 雑魚本隊生成
//=============================================================
void EnemyManager::SpawnMainSquad_() {
	if (!dx_ || !camera_ || !parent_) {
		return;
	}

	NotifyPlayerBeforeClearEnemies_();
	enemies_.clear();

	const auto& params_ = encounterConfig_.GetMainEnemyParams();

	for (int i = 0; i < kMainSquadEnemyCount_; ++i) {

		EnemyFactory::MainSquadDesc desc_;
		desc_.model_ = params_.model_;
		desc_.hp_ = params_.hp_;
		desc_.scale_ = { 1.0f, 1.0f, 1.0f };
		desc_.formationMoveSpeed_ = mainSquadMoveSpeed_;

		auto e_ = EnemyFactory::CreateMainSquadEnemy(
			TKM::Object3dCommon::GetInstance(),
			dx_,
			camera_,
			parent_,
			desc_
		);

		const float step_ = 6.28318530718f / static_cast<float>(kMainSquadEnemyCount_);
		const float ang_ = mainSquadOrbitAngle_ + step_ * static_cast<float>(i);

		Vector3 pos_ = mainSquadCenter_;
		pos_.x += std::cos(ang_) * mainSquadOrbitRadius_;
		pos_.y += std::sin(ang_) * mainSquadOrbitRadius_;

		e_->SetPosition(pos_);
		e_->SetFormationTarget(pos_);

		SetupEnemyForPlayer(*e_);
		e_->SyncTransform();

		enemies_.push_back(std::move(e_));
	}

	if (!enemies_.empty() && player_) {
		player_->SetEnemy(enemies_.front().get());
		player_->SetAllEnemies(&enemies_);
	}
}

//=============================================================
// 敵弾更新
//=============================================================
void EnemyManager::UpdateEnemyBullets_(float dt) {
	for (auto it = enemyBullets_.begin(); it != enemyBullets_.end();) {
		if (!(*it)) {
			it = enemyBullets_.erase(it);
			continue;
		}

		(*it)->Update(dt);

		//=====================================================
		// 発射済み特殊コア弾の飛翔パーティクル
		//=====================================================
		if ((*it)->GetType() == EnemyBullet::Type::SpecialCoreLaunched) {
			TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
			if (pm_) {
				const Vector3 p_ = (*it)->GetWorldPosition();

				pm_->Emit("w1sp_fly_body", p_, 2);
				//pm_->Emit("w1sp_fly_shell", p_, 1);
				//pm_->Emit("w1sp_fly_corona", p_, 1);
				pm_->Emit("w1sp_fly_arc", p_, 1);
				pm_->Emit("w1sp_fly_tail", p_, 2);
				pm_->Emit("w1sp_fly_spark", p_, 2);
			}
		}

		if ((*it)->IsDead()) {
			it = enemyBullets_.erase(it);
		} else {
			++it;
		}
	}
}

//=============================================================
// 本隊凍結切り替え
//=============================================================
void EnemyManager::SetMainSquadFreeze_(bool enable) {
	for (auto& e : enemies_) {
		if (!e || e->IsDead() || e->IsDying()) {
			continue;
		}
		if (e->GetType() != EnemyType::MainSquad) {
			continue;
		}
		e->SetFreezeMove(enable);
	}
}

//=============================================================
// バリアコアマネージャ初期化
//=============================================================
void EnemyManager::InitializeBarrierCoreManager_() {
	if (barrierCoreManager_) {
		return;
	}

	auto* common_ = TKM::Object3dCommon::GetInstance();
	if (!common_ || !dx_) {
		return;
	}

	barrierCoreManager_ = std::make_unique<BarrierCoreManager>();
	barrierCoreManager_->Initialize(common_, dx_, camera_, parent_, player_);
}

//=============================================================
// バリアコア生成
//=============================================================
void EnemyManager::SpawnBarrierCores_() {
	InitializeBarrierCoreManager_();

	if (!barrierCoreManager_) {
		return;
	}

	barrierCoreManager_->SetCamera(camera_);
	barrierCoreManager_->SetParentScene(parent_);
	barrierCoreManager_->SetPlayer(player_);

	if (player_) {
		player_->SetBarrierCoreManager(barrierCoreManager_.get());
	}

	barrierCoreManager_->Spawn(GetBarrierCenter());
}

//=============================================================
// バリアコア全削除
//=============================================================
void EnemyManager::ClearBarrierCores_() {
	if (!barrierCoreManager_) {
		if (player_) {
			player_->SetBarrierCore(nullptr);
		}
		return;
	}

	barrierCoreManager_->Clear();

	if (player_) {
		player_->SetBarrierCoreManager(nullptr);
	}
}

//=============================================================
// バリアコア全破壊判定
//=============================================================
bool EnemyManager::AreAllBarrierCoresDestroyed_() const {
	if (!barrierCoreManager_) {
		return false;
	}
	return barrierCoreManager_->IsAllDestroyed();
}

//=============================================================
// 生存中の本隊数カウント
//=============================================================
int EnemyManager::CountAliveMainSquad_() const {
	int count_ = 0;
	for (const auto& e : enemies_) {
		if (!e || e->IsDead() || e->IsDying()) {
			continue;
		}
		if (e->GetType() == EnemyType::MainSquad) {
			++count_;
		}
	}
	return count_;
}

//=============================================================
// バリア破壊処理
//=============================================================
void EnemyManager::BreakBarrier_() {
	barrierBroken_ = true;
	mainSquadStopped_ = false;

	SetMainSquadInvincible_(false);
	SetMainSquadFreeze_(false);

	if (barrier_) {
		barrier_->StartBreak();
	}

	ClearBarrierCores_();

	mainSquadPhase_ = MainSquadPhase::ExposedBattle;
	mainSquadPhaseTimer_ = 0.0f;
}

//=============================================================
// 特殊攻撃サイクル更新
//=============================================================
void EnemyManager::UpdateSpecialAttackCycle_(float dt) {
	// 攻撃OFF中は特殊コア攻撃も進行させない
	if (!enableEnemyAttack_) {
		mainSquadPhaseTimer_ = 0.0f;

		if (specialCoreBullet_) {
			specialCoreBullet_->Kill();
		}

		specialCoreCharging_ = false;
		specialCoreBullet_ = nullptr;
		return;
	}

	mainSquadPhaseTimer_ += dt;

	if (!specialCoreCharging_) {
		if (mainSquadPhaseTimer_ >= specialCoreChargeDuration_ + 1.0f) {
			mainSquadPhaseTimer_ = 0.0f;
			BeginSpecialCoreCharge_();
		}
	}

	UpdateSpecialCoreCharge_(dt);
}

//=============================================================
// 敵弾描画
//=============================================================
void EnemyManager::DrawEnemyBullets_(TKM::DirectXCommon* dx) {
	for (auto& bullet : enemyBullets_) {
		if (!bullet) {
			continue;
		}
		bullet->Draw(dx);
	}
}

//=============================================================
// 敵弾とプレイヤーの当たり判定
//=============================================================
void EnemyManager::CheckEnemyBulletPlayerCollision_(float dt) {
	if (!player_) {
		return;
	}

	//=========================================================
	// 被弾クールタイム更新
	//=========================================================
	if (playerHitCooldown_ > 0.0f) {
		playerHitCooldown_ -= dt;
		if (playerHitCooldown_ < 0.0f) {
			playerHitCooldown_ = 0.0f;
		}
	}

	// クールタイム中は当たり判定しない
	if (playerHitCooldown_ > 0.0f) {
		return;
	}

	const Vector3 playerPos_ = player_->GetPosition();

	for (auto& bullet : enemyBullets_) {
		if (!bullet || bullet->IsDead()) {
			continue;
		}

		const Vector3 bulletPos_ = bullet->GetWorldPosition();
		const float hitDist_ = playerHitRadius_ + bullet->GetRadius();
		const float dist_ = MyMath::Length(bulletPos_ - playerPos_);

		if (dist_ <= hitDist_) {
			// 弾を消す
			bullet->Kill();

			// プレイヤーへダメージ
			player_->Damage(enemyBulletDamage_);

			// 連続ヒット防止
			playerHitCooldown_ = playerHitCooldownDuration_;

			// 1フレーム1ヒットだけ
			break;
		}
	}
}

//=============================================================
// 特殊コア位置取得
//=============================================================
Vector3 EnemyManager::GetSpecialCorePosition_() const {
	return mainSquadCenter_ + specialCoreOffset_;
}

//=============================================================
// バリア中心取得
//=============================================================
Vector3 EnemyManager::GetBarrierCenter() const {
	return barrier_ ? barrier_->GetCenter() : Vector3{ 0.0f, 0.0f, 0.0f };
}

//=============================================================
// バリアサイズ取得
//=============================================================
Vector3 EnemyManager::GetBarrierSize() const {
	return barrier_ ? barrier_->GetAABBSize() : Vector3{ 0.0f, 0.0f, 0.0f };
}

//=============================================================
// カメラ設定
//=============================================================
void EnemyManager::SetCamera(TKM::Camera* camera) {
	BattleActorManagerBase::SetCamera(camera);
}

//=============================================================
// カメラ変更時処理
//=============================================================
void EnemyManager::OnCameraChanged() {
	for (auto& e : enemies_) {
		if (e) {
			e->SetCamera(camera_);
		}
	}

	if (barrier_) {
		barrier_->SetCamera(camera_);
	}

	if (barrierCoreManager_) {
		barrierCoreManager_->SetCamera(camera_);
	}
}

//=============================================================
// 本隊戦開始
//=============================================================
void EnemyManager::BeginMainSquadBattle_() {
	ClearBarrierCores_();

	// 撃破数リセット（最大数は目標撃破数）
	maxEnemyCount_ = std::max(0, encounterConfig_.GetSmallEnemyPhase().defeatTarget_);

	SpawnMainSquad_();
	SetMainSquadInvincible_(true);
	SetMainSquadFreeze_(false);

	InitializeBarrier_();
	SetBarrierActive_(true);
	UpdateBarrier_();

	if (barrier_) {
		barrier_->SetVisible(true);
	}
	SyncBarrierInfoToPlayer_();

	InitializeBarrierCoreManager_();
	SpawnBarrierCores_();

	barrierBroken_ = false;
	mainSquadStopped_ = false;
	specialCoreCharging_ = false;
	specialCoreBullet_ = nullptr;

	mainSquadPhase_ = MainSquadPhase::BarrierBattle;
	mainSquadPhaseTimer_ = 0.0f;
	scatterShotTimer_ = 0.0f;
}

//=============================================================
// 本隊円運動ターゲット反映
//=============================================================
void EnemyManager::ApplyMainSquadOrbitTargets_() {
	const float step_ = 6.28318530718f / static_cast<float>(kMainSquadEnemyCount_);

	int aliveIndex_ = 0;
	for (auto& e : enemies_) {
		if (!e || e->IsDead() || e->IsDying()) {
			continue;
		}

		// Wave1本隊だけを円運動対象にする
		if (e->GetType() != EnemyType::MainSquad) {
			continue;
		}

		const float ang_ = mainSquadOrbitAngle_ + step_ * static_cast<float>(aliveIndex_);

		Vector3 pos_ = mainSquadCenter_;
		pos_.x += std::cos(ang_) * mainSquadOrbitRadius_;
		pos_.y += std::sin(ang_) * mainSquadOrbitRadius_;

		e->SetBehavior(EnemyBehavior::MoveToTarget);
		e->SetFormationMoveSpeed(mainSquadMoveSpeed_);
		e->SetFormationTarget(pos_);

		++aliveIndex_;
	}
}

//=============================================================
// 本隊円運動更新
//=============================================================
void EnemyManager::UpdateMainSquadOrbit_(float dt) {
	mainSquadOrbitAngle_ -= mainSquadOrbitAngularSpeed_ * dt;
	ApplyMainSquadOrbitTargets_();
}

//=============================================================
// 本隊無敵切り替え
//=============================================================
void EnemyManager::SetMainSquadInvincible_(bool enable) {
	for (auto& e : enemies_) {
		if (!e || e->IsDead() || e->IsDying()) {
			continue;
		}

		// 本隊だけ無敵化する
		if (e->GetType() != EnemyType::MainSquad) {
			continue;
		}

		e->SetDamageInvincible(enable);
	}
}

//=============================================================
// バリア初期化
//=============================================================
void EnemyManager::InitializeBarrier_() {
	if (barrier_) {
		return;
	}

	auto* common_ = TKM::Object3dCommon::GetInstance();
	if (!common_ || !dx_) {
		return;
	}

	barrier_ = std::make_unique<EnemyBarrier>();
	barrier_->SetCamera(camera_);
	barrier_->SetPlayer(player_);
	barrier_->Initialize(common_, dx_);
	barrier_->SetCenter(GetSpecialCorePosition_() + barrierOffset_);
	barrier_->SetRadius(1.0f);
	barrier_->SetShapeScale(barrierSize_);
	barrier_->SetVisible(false);
	barrier_->SetActive(false);
}

//=============================================================
// バリア更新
//=============================================================
void EnemyManager::UpdateBarrier_() {
	if (!barrier_) {
		return;
	}

	if (barrierFollowCore_) {
		barrier_->SetCenter(GetSpecialCorePosition_() + barrierOffset_);
	}

	barrier_->SetRadius(1.0f);
	barrier_->SetShapeScale(barrierSize_);
}

//=============================================================
// バリア有効切り替え
//=============================================================
void EnemyManager::SetBarrierActive_(bool active) {
	if (!barrier_) {
		InitializeBarrier_();
	}
	if (!barrier_) {
		return;
	}

	barrier_->SetActive(active);
}

//=============================================================
// バリア情報をプレイヤーへ同期
//=============================================================
void EnemyManager::SyncBarrierInfoToPlayer_() {
	if (!barrier_) {
		if (player_) {
			player_->SetWave1BarrierInfo(false, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
		}
		return;
	}

	barrier_->SyncToPlayer();
}

//=============================================================
// 描画
//=============================================================
void EnemyManager::Draw(TKM::DirectXCommon* dx) {
	//=========================================================
	// バリア描画
	//=========================================================
	if (barrier_) {
		barrier_->Draw(dx);
	}

	TKM::Object3dCommon::GetInstance()->DrawSetCommon();

	//=========================================================
	// バリアコア描画
	//=========================================================
	if (barrierCoreManager_) {
		barrierCoreManager_->Draw(dx);
	}

	//=========================================================
	// 敵描画
	//=========================================================
	if (!&enemies_) { // enemies_ がまだ紐付いてなかったら何もしない
		return;
	}

	for (auto& enemy : enemies_) {
		enemy->Draw(dx);
	}

	// 敵弾は敵の後に描画する
	DrawEnemyBullets_(dx);
}

//=============================================================
// ImGuiデバッグ表示
//=============================================================
void EnemyManager::ImGuiDebug() {
#ifdef USE_IMGUI
	// まだ紐付いてないなら何もしない
	if (!&enemies_) {
		return;
	}

	ImGui::Begin("敵ステータス");

	//=========================================================
	// フェーズ状態表示
	//=========================================================
	static const char* kWaveLabel_[] = {
		"雑魚フェーズ",
		"Bossフェーズ"
	};
	ImGui::Text("現在のフェーズ: %s", kWaveLabel_[static_cast<int>(enemyPhase_)]);

	//=========================================================
	// 強制ボス移行
	//=========================================================
	if (ImGui::Button("ボスWaveへ")) {
		FinishSmallEnemyPhase();
	}

	//=========================================================
	// バリア調整
	//=========================================================
	if (ImGui::CollapsingHeader("バリア")) {
		ImGui::Checkbox("中心追従", &barrierFollowCore_);
		ImGui::DragFloat3("バリアオフセット", &barrierOffset_.x, 0.1f);
		ImGui::DragFloat3("バリアサイズXYZ", &barrierSize_.x, 0.1f, 0.1f, 200.0f);

		if (barrier_) {
			ImGui::Separator();
			ImGui::Text("バリアシェーダ");

			bool active = barrier_->IsActive();
			bool visible = barrier_->IsVisible();

			if (ImGui::Checkbox("バリア有効", &active)) {
				barrier_->SetActive(active);
			}
			if (ImGui::Checkbox("バリア表示", &visible)) {
				barrier_->SetVisible(visible);
			}

			ImGui::Text(
				"現在Center : %.2f, %.2f, %.2f",
				barrier_->GetCenter().x,
				barrier_->GetCenter().y,
				barrier_->GetCenter().z
			);

			ImGui::Text("現在Radius : %.2f", barrier_->GetRadius());
		}
	}

	//=========================================================
	// 雑魚敵攻撃ON/OFF
	//=========================================================
	if (ImGui::Button(enableEnemyAttack_ ? "雑魚敵攻撃 OFF" : "雑魚敵攻撃 ON")) {
		enableEnemyAttack_ = !enableEnemyAttack_;

		if (!enableEnemyAttack_) {
			enemyBullets_.clear();
			playerHitCooldown_ = 0.0f;
			specialCoreCharging_ = false;
			specialCoreBullet_ = nullptr;
			scatterShotTimer_ = 0.0f;
			mainSquadPhaseTimer_ = 0.0f;
		}
	}

	ImGui::Text("雑魚敵攻撃: %s", enableEnemyAttack_ ? "ON" : "OFF");

	ImGui::End();
#endif
}