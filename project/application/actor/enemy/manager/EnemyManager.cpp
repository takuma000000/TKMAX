#include "EnemyManager.h"
#include <limits>
#include "MyMath.h"
#include "manager/BossManager.h"
#include "BarrierCommon.h"
#include "AudioManager.h"
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void EnemyManager::Initialize(TKM::DirectXCommon* dx, TKM::Camera* camera, TKM::BaseScene* parent, Player* player) {
	// 共通初期化
	InitializeCommon(dx, camera, parent, player);
	// BarrierCommon初期化
	TKM::BarrierCommon::GetInstance()->Initialize(dx);

	// CSV読み込み（resources/data に置く運用）
	waveConfig_.Load("./resources/data/enemy_waves.json");

	// Wave1の設定
	{
		const auto& w1_ = waveConfig_.GetWave1();
		wave1DefeatTarget_ = w1_.defeatTarget_;
	}
}

void EnemyManager::Update(float dt) {
	if (!initializedWaves_) { return; }

	if (wave1Barrier_) {
		if (player_) {
			Vector3 flashPos;
			if (player_->ConsumeWave1BarrierFlashRequest(flashPos)) {
				wave1Barrier_->OnHit(flashPos);
			}
		}

		UpdateWave1Barrier_();
		wave1Barrier_->Update(dt);
	}
	SyncWave1BarrierInfoToPlayer_();

	if (barrierCoreManager_) {
		barrierCoreManager_->Update(dt);
	}

	for (auto it = enemies_.begin(); it != enemies_.end();) {
		Enemy* e_ = it->get();
		e_->SetFreezeMove(freezeEnemies_);
		e_->Update(dt);

		if (e_->IsDead()) {
			if (player_) {
				player_->OnEnemyDestroyed(e_);
			}

			if (e_->GetDefeated()) {
				if (e_->GetType() == EnemyType::Wave1Main) {
					++wave1MainDefeatedCount_;
				}
				++defeatedEnemyCount_;

				if (defeatedEnemyCount_ == 3 && player_) {
					player_->EnableSpecialAttack();
				}
			}

			it = enemies_.erase(it);
		} else {
			++it;
		}
	}

	if (wavePhase_ == WavePhase::W1) {
		UpdateWave1(dt);
	}

	UpdateEnemyBullets_(dt);
	CheckEnemyBulletPlayerCollision_(dt);
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

	NotifyPlayerBeforeClearEnemies_(); // プレイヤーに敵全削除を通知（ロックオン解除などのため）
	enemies_.clear(); // 敵リストクリア
	enemyBullets_.clear(); // 敵弾リストもクリア
	playerHitCooldown_ = 0.0f; // プレイヤー被弾クールダウンリセット
	wave1SpecialCharging_ = false;
	wave1SpecialCoreBullet_ = nullptr;
	wave1NormalShotTimer_ = 0.0f;

	defeatedEnemyCount_ = 0; // 撃破数リセット
	maxEnemyCount_ = wave1DefeatTarget_; // 最大敵数は最初のWaveの撃破目標数に合わせておく（必要なら後で更新）

	wavePhase_ = WavePhase::W1;
	BeginWave1();

	initializedWaves_ = true; // Wave初期化完了フラグセット

	// 最初のロックオン対象
	if (!enemies_.empty()) {
		player_->SetEnemy(enemies_.front().get()); // 最初の敵をロックオン対象にセット
		player_->SetAllEnemies(&enemies_); // 敵リストもセット
	}
}

void EnemyManager::SpawnCurrentWave() {
	if (!dx_ || !camera_ || !parent_) { return; } // 必要な参照が揃ってないなら何もしない

	NotifyPlayerBeforeClearEnemies_(); // プレイヤーに敵全削除を通知（ロックオン解除などのため）
	enemies_.clear(); // 敵リストクリア（前のWaveの敵を消す）
	enemyBullets_.clear(); // 敵弾リストもクリア
	playerHitCooldown_ = 0.0f; // プレイヤー被弾クールダウンリセット
	wave1SpecialCharging_ = false;
	wave1SpecialCoreBullet_ = nullptr;
	wave1NormalShotTimer_ = 0.0f;
	SetWave1BarrierActive_(false);
	SyncWave1BarrierInfoToPlayer_();

	if (wavePhase_ == WavePhase::W1) {
		BeginWave1();
	}
}

void EnemyManager::GoToNextWave() {
	wavePhase_ = WavePhase::Done;
	SpawnCurrentWave();

	if (defeatedEnemyCount_) {
		defeatedEnemyCount_ = 0;
	}
}

void EnemyManager::SkipToBossWave() {
	// いま居るザコ敵は全部消す
	NotifyPlayerBeforeClearEnemies_();
	enemies_.clear(); // 敵を全部消す
	enemyBullets_.clear(); // 敵弾も全部消す
	playerHitCooldown_ = 0.0f; // プレイヤー被弾クールダウンリセット
	wave1SpecialCharging_ = false;
	wave1SpecialCoreBullet_ = nullptr;
	wave1NormalShotTimer_ = 0.0f;
	SetWave1BarrierActive_(false);
	SyncWave1BarrierInfoToPlayer_();

	// 撃破数・最大数もリセット（ゲージを空にしておく）
	if (defeatedEnemyCount_) {
		defeatedEnemyCount_ = 0;
	}
	if (maxEnemyCount_) {
		maxEnemyCount_ = 0;
	}

	// Wave を Done（＝ボスフェーズ）にする
	wavePhase_ = WavePhase::Done;
	// 雑魚戦が終わったらplayBGMを止める
	TKM::AudioManager::GetInstance()->StopSound("playBGM");
}

void EnemyManager::SetupEnemyForPlayer(Enemy& e) {
	if (!player_) { // プレイヤー参照がないなら何もしない
		return;
	}
	e.SetReticle(player_->GetReticle()); // 敵の照準にプレイヤーの照準をセット
	e.SetPlayer([this]() { return player_->GetPosition(); }); // 敵のプレイヤー位置取得関数に、プレイヤーの位置を返すラムダをセット
}

void EnemyManager::NotifyPlayerBeforeClearEnemies_() {
	if (!player_) { // プレイヤー参照がないなら何もしない
		return;
	}

	// これから敵が全滅することをプレイヤーに通知して、ロックオン解除などの処理をさせる
	for (auto& e : enemies_) {
		if (!e) { continue; } // 念のためヌルチェック
		player_->OnEnemyDestroyed(e.get()); // プレイヤーに敵が消えることを通知（ロックオン解除などのため）
	}
}

void EnemyManager::UpdateWave1(float dt) {
	UpdateEnemyBullets_(dt);

	if (playerHitCooldown_ > 0.0f) {
		playerHitCooldown_ -= dt;
		if (playerHitCooldown_ < 0.0f) {
			playerHitCooldown_ = 0.0f;
		}
	}

	// 本隊10体を全部倒したらそのままボス戦へ
	if (CountAliveWave1Main_() <= 0) {
		NotifyPlayerBeforeClearEnemies_();
		enemies_.clear();

		ClearBarrierCores_();

		SkipToBossWave();
		return;
	}

	switch (wave1Phase_) {
	case Wave1Phase::BarrierBattle:
		UpdateWave1CircleFormation_(dt);
		UpdateWave1ScatterAttack_(dt);
		UpdateWave1SpecialAttackCycle_(dt);

		// バリア中は本隊を常に無敵維持
		SetWave1AllInvincible_(true);

		if (AreAllBarrierCoresDestroyed_()) {
			BreakWave1Barrier_();
			return;
		}
		break;

	case Wave1Phase::ExposedBattle:
		UpdateWave1CircleFormation_(dt);
		UpdateWave1ScatterAttack_(dt);
		UpdateWave1SpecialAttackCycle_(dt);
		break;
	}
}

void EnemyManager::UpdateWave1ScatterAttack_(float dt) {


	/// ====================================
	/// 一時的に通常攻撃を止める
	return;
	/// ====================================


	wave1NormalShotTimer_ += dt;
	if (wave1NormalShotTimer_ < wave1NormalShotInterval_) {
		return;
	}
	wave1NormalShotTimer_ = 0.0f;

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
		bullet_->Initialize(common_, dx_, camera_, start_, dir_ * wave1NormalBulletSpeed_);
		enemyBullets_.push_back(std::move(bullet_));
	}
}

void EnemyManager::BeginWave1SpecialCharge_() {
	auto* common_ = TKM::Object3dCommon::GetInstance();
	if (!common_) {
		return;
	}

	wave1SpecialCharging_ = true;
	wave1SpecialCoreBullet_ = nullptr;

	const Vector3 corePos_ = GetWave1SpecialCorePosition_();

	auto core_ = std::make_unique<EnemyBullet>();
	core_->InitializeFormationCore(
		common_,
		dx_,
		camera_,
		corePos_,
		wave1SpecialCoreStartScale_,
		wave1SpecialCoreEndScale_,
		wave1SpecialCoreRadius_,
		wave1SpecialChargeDuration_,
		wave1SpecialCoreDamage_
	);

	wave1SpecialCoreBullet_ = core_.get();
	enemyBullets_.push_back(std::move(core_));
}

void EnemyManager::UpdateWave1SpecialCharge_(float dt) {
	if (!wave1SpecialCharging_ || !wave1SpecialCoreBullet_) {
		return;
	}

	EmitWave1SpecialChargeParticles_();

	if (wave1SpecialCoreBullet_->IsDead()) {
		wave1SpecialCharging_ = false;
		wave1SpecialCoreBullet_ = nullptr;
		return;
	}

	if (wave1PhaseTimer_ >= wave1SpecialChargeDuration_) {
		FireWave1SpecialCore_();
		wave1SpecialCharging_ = false;
	}
}

void EnemyManager::FireWave1SpecialCore_() {
	if (!wave1SpecialCoreBullet_) {
		return;
	}

	Vector3 start_ = wave1SpecialCoreBullet_->GetWorldPosition();
	Vector3 target_ = player_ ? player_->GetPosition() : (start_ + Vector3{ 0.0f, 0.0f, -30.0f });

	Vector3 dir_ = target_ - start_;
	float len_ = MyMath::Length(dir_);
	if (len_ <= 0.0001f) {
		return;
	}
	dir_ = dir_ / len_;

	wave1SpecialCoreBullet_->LaunchFormationCore(dir_ * wave1SpecialCoreShotSpeed_);

	// 発射時の主役演出
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

	wave1SpecialCoreBullet_ = nullptr;
}

void EnemyManager::EmitWave1SpecialChargeParticles_() {
	TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
	if (!pm_) {
		return;
	}

	const Vector3 corePos_ = GetWave1SpecialCorePosition_();
	const auto loadLevel_ = pm_->GetLoadLevel();

	// 継続演出なので、重い時は線の分割数自体を落とす
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

	// 生存している敵全員がコアへ送る
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

		// 軽く横ブレを入れて点列感を減らす
		Vector3 side_ = { -dir_.z, 0.0f, dir_.x };
		if (MyMath::Length(side_) <= 0.0001f) {
			side_ = { 1.0f, 0.0f, 0.0f };
		} else {
			side_ = MyMath::Normalize(side_);
		}

		for (int seg_ = 1; seg_ <= segmentCount_; ++seg_) {
			const float u_ = static_cast<float>(seg_) / static_cast<float>(segmentCount_ + 1);

			// コアに近いほど密になるように後半へ寄せる
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

	// コア本体の見た目
	pm_->Emit("w1sp_core_body", corePos_, pm_->GetEmitCountScaled(2, true));
	pm_->Emit("w1sp_core_inner", corePos_, pm_->GetEmitCountScaled(2, true));
	//pm_->Emit("w1sp_core_ring", corePos_, pm_->GetEmitCountScaled(1, true));
	//pm_->Emit("w1sp_core_shell", corePos_, pm_->GetEmitCountScaled(1, true));
	//pm_->Emit("w1sp_core_smoke", corePos_, pm_->GetEmitCountScaled(0, false));
	pm_->Emit("w1sp_core_arc", corePos_, pm_->GetEmitCountScaled(1, false));

	// チャージ終盤の加速演出
	if (wave1PhaseTimer_ >= wave1SpecialChargeDuration_ * 0.55f) {
		//pm_->Emit("w1sp_core_flash", corePos_, pm_->GetEmitCountScaled(1, true));
		//pm_->Emit("w1sp_core_spark", corePos_, pm_->GetEmitCountScaled(3, false));
		//pm_->Emit("w1sp_core_arc", corePos_, pm_->GetEmitCountScaled(1, false));
	}
}

void EnemyManager::SpawnWave1Group() {
	if (!dx_ || !camera_ || !parent_) {
		return;
	}

	NotifyPlayerBeforeClearEnemies_();
	enemies_.clear();

	const auto& p1_ = waveConfig_.GetWave1EnemyParams();

	for (int i = 0; i < kWave1EnemyCount_; ++i) {
		auto e_ = std::make_unique<Enemy>();
		e_->Initialize(TKM::Object3dCommon::GetInstance(), dx_);
		e_->SetCamera(camera_);
		e_->SetParentScene(parent_);

		e_->SetModel(p1_.model_);
		e_->SetHP(p1_.hp_);
		e_->SetScale({ 1.0f, 1.0f, 1.0f });
		e_->SetType(EnemyType::Wave1Main);

		if (p1_.model_ == "jerryfish.obj") {
			e_->SetTentacleModel("tentacle.obj");
			e_->SetTentacleLocal(
				{ 0.0f, 0.0f, 0.0f },
				{ 0.0f, 0.0f, 0.0f },
				{ 1.0f, 1.0f, 1.0f }
			);
		}

		e_->SetBehavior(EnemyBehavior::FormationMove);
		e_->SetFormationMoveSpeed(wave1FormationMoveSpeed_);

		const float step_ = 6.28318530718f / static_cast<float>(kWave1EnemyCount_);
		const float ang_ = wave1CircleAngle_ + step_ * static_cast<float>(i);

		Vector3 pos_ = wave1CircleCenter_;
		pos_.x += std::cos(ang_) * wave1CircleRadius_;
		pos_.y += std::sin(ang_) * wave1CircleRadius_;

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

void EnemyManager::UpdateEnemyBullets_(float dt) {
	for (auto it = enemyBullets_.begin(); it != enemyBullets_.end();) {
		if (!(*it)) {
			it = enemyBullets_.erase(it);
			continue;
		}

		(*it)->Update(dt);

		// 発射済みの共有SPコア弾だけ、飛翔中パーティクルを出す
		if ((*it)->GetType() == EnemyBullet::Type::FormationCoreLaunched) {
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

void EnemyManager::SetWave1MainFreeze_(bool enable) {
	for (auto& e : enemies_) {
		if (!e || e->IsDead() || e->IsDying()) {
			continue;
		}
		if (e->GetType() != EnemyType::Wave1Main) {
			continue;
		}
		e->SetFreezeMove(enable);
	}
}

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

	barrierCoreManager_->Spawn(GetWave1BarrierCenter());
}

void EnemyManager::ClearBarrierCores_() {
	if (!barrierCoreManager_) {
		if (player_) {
			player_->SetMidBossCore(nullptr);
		}
		return;
	}

	barrierCoreManager_->Clear();

	if (player_) {
		player_->SetBarrierCoreManager(nullptr);
	}
}

bool EnemyManager::AreAllBarrierCoresDestroyed_() const {
	if (!barrierCoreManager_) {
		return false;
	}
	return barrierCoreManager_->IsAllDestroyed();
}

int EnemyManager::CountAliveWave1Main_() const {
	int count_ = 0;
	for (const auto& e : enemies_) {
		if (!e || e->IsDead() || e->IsDying()) {
			continue;
		}
		if (e->GetType() == EnemyType::Wave1Main) {
			++count_;
		}
	}
	return count_;
}

void EnemyManager::BreakWave1Barrier_() {
	wave1BarrierBroken_ = true;
	wave1MainStopped_ = false;

	SetWave1AllInvincible_(false);
	SetWave1MainFreeze_(false);

	if (wave1Barrier_) {
		wave1Barrier_->StartBreak();
	}

	ClearBarrierCores_();

	wave1Phase_ = Wave1Phase::ExposedBattle;
	wave1PhaseTimer_ = 0.0f;
}

void EnemyManager::UpdateWave1SpecialAttackCycle_(float dt) {
	wave1PhaseTimer_ += dt;

	if (!wave1SpecialCharging_) {
		if (wave1PhaseTimer_ >= wave1SpecialChargeDuration_ + 1.0f) {
			wave1PhaseTimer_ = 0.0f;
			BeginWave1SpecialCharge_();
		}
	}

	UpdateWave1SpecialCharge_(dt);
}

void EnemyManager::DrawEnemyBullets_(TKM::DirectXCommon* dx) {
	for (auto& bullet : enemyBullets_) {
		if (!bullet) {
			continue;
		}
		bullet->Draw(dx);
	}
}

void EnemyManager::CheckEnemyBulletPlayerCollision_(float dt) {
	if (!player_) {
		return;
	}

	// 被弾クールタイム更新
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

Vector3 EnemyManager::GetWave1SpecialCorePosition_() const {
	return wave1CircleCenter_ + wave1SpecialCoreOffset_;
}

Vector3 EnemyManager::GetWave1BarrierCenter() const {
	return wave1Barrier_ ? wave1Barrier_->GetCenter() : Vector3{ 0.0f, 0.0f, 0.0f };
}

Vector3 EnemyManager::GetWave1BarrierSize() const {
	return wave1Barrier_ ? wave1Barrier_->GetAABBSize() : Vector3{ 0.0f, 0.0f, 0.0f };
}

void EnemyManager::SetCamera(TKM::Camera* camera) {
	BattleActorManagerBase::SetCamera(camera);
}

void EnemyManager::OnCameraChanged() {
	for (auto& e : enemies_) {
		if (e) { e->SetCamera(camera_); }
	}

	if (wave1Barrier_) {
		wave1Barrier_->SetCamera(camera_);
	}

	if (barrierCoreManager_) {
		barrierCoreManager_->SetCamera(camera_);
	}
}

void EnemyManager::BeginWave1() {

	ClearBarrierCores_();

	if (maxEnemyCount_) {
		maxEnemyCount_ = kWave1EnemyCount_;
	}

	SpawnWave1Group();
	SetWave1AllInvincible_(true);
	SetWave1MainFreeze_(false);

	InitializeWave1Barrier_();
	SetWave1BarrierActive_(true);
	UpdateWave1Barrier_();
	if (wave1Barrier_) {
		wave1Barrier_->SetVisible(true);
	}
	SyncWave1BarrierInfoToPlayer_();

	InitializeBarrierCoreManager_();
	SpawnBarrierCores_();

	wave1BarrierBroken_ = false;
	wave1MainStopped_ = false;
	wave1SpecialCharging_ = false;
	wave1SpecialCoreBullet_ = nullptr;

	wave1Phase_ = Wave1Phase::BarrierBattle;
	wave1PhaseTimer_ = 0.0f;
	wave1NormalShotTimer_ = 0.0f;
}

void EnemyManager::ApplyWave1CircleTargets_() {
	const float step_ = 6.28318530718f / static_cast<float>(kWave1EnemyCount_);

	int aliveIndex_ = 0;
	for (auto& e : enemies_) {
		if (!e || e->IsDead() || e->IsDying()) {
			continue;
		}

		// Wave1本隊だけを円運動の対象にする
		if (e->GetType() != EnemyType::Wave1Main) {
			continue;
		}

		const float ang_ = wave1CircleAngle_ + step_ * static_cast<float>(aliveIndex_);

		Vector3 pos_ = wave1CircleCenter_;
		pos_.x += std::cos(ang_) * wave1CircleRadius_;
		pos_.y += std::sin(ang_) * wave1CircleRadius_;

		e->SetBehavior(EnemyBehavior::FormationMove);
		e->SetFormationMoveSpeed(wave1FormationMoveSpeed_);
		e->SetFormationTarget(pos_);

		++aliveIndex_;
	}
}
void EnemyManager::UpdateWave1CircleFormation_(float dt) {
	wave1CircleAngle_ -= wave1CircleAngularSpeed_ * dt; // 右回転
	ApplyWave1CircleTargets_();
}
void EnemyManager::SetWave1AllInvincible_(bool enable) {
	for (auto& e : enemies_) {
		if (!e || e->IsDead() || e->IsDying()) {
			continue;
		}

		// Wave1本隊だけ無敵化する
		if (e->GetType() != EnemyType::Wave1Main) {
			continue;
		}

		e->SetDamageInvincible(enable);
	}
}

void EnemyManager::InitializeWave1Barrier_() {
	if (wave1Barrier_) {
		return;
	}

	auto* common_ = TKM::Object3dCommon::GetInstance();
	if (!common_ || !dx_) {
		return;
	}

	wave1Barrier_ = std::make_unique<EnemyBarrier>();
	wave1Barrier_->SetCamera(camera_);
	wave1Barrier_->SetPlayer(player_);
	wave1Barrier_->Initialize(common_, dx_);
	wave1Barrier_->SetCenter(GetWave1SpecialCorePosition_() + wave1BarrierOffset_);
	wave1Barrier_->SetRadius(1.0f);
	wave1Barrier_->SetShapeScale(wave1BarrierSize_);
	wave1Barrier_->SetVisible(false);
	wave1Barrier_->SetActive(false);
}
void EnemyManager::UpdateWave1Barrier_() {
	if (!wave1Barrier_) {
		return;
	}

	if (wave1BarrierFollowCore_) {
		wave1Barrier_->SetCenter(GetWave1SpecialCorePosition_() + wave1BarrierOffset_);
	}

	wave1Barrier_->SetRadius(1.0f);
	wave1Barrier_->SetShapeScale(wave1BarrierSize_);
}
void EnemyManager::SetWave1BarrierActive_(bool active) {
	if (!wave1Barrier_) {
		InitializeWave1Barrier_();
	}
	if (!wave1Barrier_) {
		return;
	}

	wave1Barrier_->SetActive(active);
}
void EnemyManager::SyncWave1BarrierInfoToPlayer_() {
	if (!wave1Barrier_) {
		if (player_) {
			player_->SetWave1BarrierInfo(false, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
		}
		return;
	}

	wave1Barrier_->SyncToPlayer();
}

void EnemyManager::Draw(TKM::DirectXCommon* dx) {

	if (wave1Barrier_) {
		wave1Barrier_->Draw(dx);
	}

	TKM::Object3dCommon::GetInstance()->DrawSetCommon();

	if (barrierCoreManager_) {
		barrierCoreManager_->Draw(dx);
	}

	if (!&enemies_) { // enemies_ がまだ紐付いてなかったら何もしない
		return;
	}
	for (auto& enemy : enemies_) {
		enemy->Draw(dx);
	}
	// 敵の弾は敵が描画された後に描く（手前に来るように）
	DrawEnemyBullets_(dx);
}

void EnemyManager::ImGuiDebug() {
#ifdef USE_IMGUI
	// まだ紐付いてないなら何もしない
	if (!&enemies_) {
		return;
	}

	ImGui::Begin("敵ステータス");
	// ===== Wave 状態表示 =====
	static const char* kWaveLabel_[] = {
	"雑魚フェーズ",
	"Bossフェーズ"
	};
	ImGui::Text("現在のWave: %s", kWaveLabel_[static_cast<int>(wavePhase_)]);

	// 「次のフェーズへ」ボタン
	if (ImGui::Button("次のフェーズへ")) {
		GoToNextWave();
	}
	ImGui::SameLine(); // 横並びに
	// 「ボスWaveへ」ボタン
	if (ImGui::Button("ボスWaveへ")) {
		SkipToBossWave();
	}

	if (ImGui::CollapsingHeader("Wave1バリア")) {
		ImGui::Checkbox("中心追従", &wave1BarrierFollowCore_);
		ImGui::DragFloat3("バリアオフセット", &wave1BarrierOffset_.x, 0.1f);
		ImGui::DragFloat3("バリアサイズXYZ", &wave1BarrierSize_.x, 0.1f, 0.1f, 200.0f);

		if (wave1Barrier_) {

			ImGui::Separator();
			ImGui::Text("バリアシェーダ");

			ImGui::DragFloat("フレネル強さ", &wave1BarrierShaderFresnelPower_, 0.01f, 0.1f, 10.0f);
			ImGui::DragFloat("ベース明るさ", &wave1BarrierShaderBaseStrength_, 0.01f, 0.0f, 5.0f);
			ImGui::DragFloat("縁の強さ", &wave1BarrierShaderRimStrength_, 0.01f, 0.0f, 5.0f);
			ImGui::DragFloat("中央の濃さ", &wave1BarrierShaderAlphaBase_, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("縁の濃さ", &wave1BarrierShaderAlphaRim_, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat3("色補正RGB", &wave1BarrierShaderTint_.x, 0.01f, 0.0f, 2.0f);

			bool active = wave1Barrier_->IsActive();
			bool visible = wave1Barrier_->IsVisible();

			if (ImGui::Checkbox("バリア有効", &active)) {
				wave1Barrier_->SetActive(active);
			}
			if (ImGui::Checkbox("バリア表示", &visible)) {
				wave1Barrier_->SetVisible(visible);
			}

			ImGui::Text("現在Center : %.2f, %.2f, %.2f",
				wave1Barrier_->GetCenter().x,
				wave1Barrier_->GetCenter().y,
				wave1Barrier_->GetCenter().z);

			ImGui::Text("現在Radius : %.2f", wave1Barrier_->GetRadius());
		}
	}

	ImGui::End();
#endif
}