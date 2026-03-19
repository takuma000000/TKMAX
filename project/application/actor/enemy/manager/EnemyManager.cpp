#include "EnemyManager.h"
#include <limits>
#include "MyMath.h"
#include "manager/BossManager.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

const EnemyManager::WaveOps EnemyManager::kWaveOps_[4] = {
	// WavePhase 列挙値に対応させて、スポーン関数と更新関数をセットで定義
	/* W1  */ { &EnemyManager::BeginWave1, &EnemyManager::UpdateWave1 },
	/* W2  */ { &EnemyManager::BeginWave2, &EnemyManager::UpdateWave2 },
	/* W3  */ { &EnemyManager::BeginWave3, &EnemyManager::UpdateWave3 },
	/* Done*/ { nullptr, nullptr },
};

void EnemyManager::Initialize(TKM::DirectXCommon* dx, TKM::Camera* camera, TKM::BaseScene* parent, Player* player) {
	// 共通初期化
	InitializeCommon(dx, camera, parent, player);

	// CSV読み込み（resources/data に置く運用）
	waveConfig_.Load("./resources/data/enemy_waves.json");

	// Wave1の設定
	{
		const auto& w1_ = waveConfig_.GetWave1();
		wave1DefeatTarget_ = w1_.defeatTarget_;
	}

	wave2WaitDuration_ = waveConfig_.GetWave2WaitDuration(); // Wave2開始前の待機時間
	wave2SubWaveCount_ = waveConfig_.GetWave2SubWaveCount(); // Wave2のサブWave数

	// Wave3の設定
	{
		const auto& w3_ = waveConfig_.GetWave3(); // Wave3の設定をCSVから取得
		wave3LeftPos_ = w3_.midBossLeft_; // MidBossの左側の位置
		wave3RightPos_ = w3_.midBossRight_; // MidBossの右側の位置
		wave3CoreLifetime_ = w3_.coreLifetime_; // MidBossの核の寿命
		wave3CoreHP_ = w3_.coreHP_; // MidBossの核のHP
		wave3AngryDuration_ = w3_.angryDuration_; // MidBossの怒り状態の持続時間
	}
}

void EnemyManager::Update(float dt) {
	if (!initializedWaves_) { return; } // Wave未初期化なら何もしない

	// ───────────────────────────────
	/// ● Wave3 の核が居れば更新
	// ───────────────────────────────
	if (midBossCore_) { // MidBossの核が存在すれば
		midBossCore_->Update(dt); // 核の更新

		// 死亡しきったらポインタ破棄
		if (midBossCore_->IsDead()) {
			if (player_) {
				player_->SetMidBossCore(nullptr); // Playerの参照も切る（nullptrセット）
			}
			midBossCore_.reset(); // 核オブジェクト破棄
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

			// 倒した/逃げた/消えた どれでも「参照してる側」を先に切る
			if (player_) {
				player_->OnEnemyDestroyed(e_);
			}

			// 撃破として数えるのは「倒した時だけ」
			if (e_->GetDefeated()) {
				++defeatedEnemyCount_; // 撃破数カウントアップ

				if (defeatedEnemyCount_ == 3 && player_) { // 3体撃破でスペシャル攻撃解禁（例）
					player_->EnableSpecialAttack(); // プレイヤーのスペシャル攻撃を有効化
				}
			}
			// 敵リストから削除
			it = enemies_.erase(it);
		} else {
			++it; // 次の敵へ
		}
	}

	// ───────────────────────────────────────────────
	/// ● Wave進行
	// ───────────────────────────────────────────────
		// Waveごとの更新（分岐しない）
	const auto ops_ = kWaveOps_[static_cast<int>(wavePhase_)];

	// 更新関数があれば呼び出す
	if (ops_.update_) {
		(this->*ops_.update_)(dt); // メンバ関数ポインタ呼び出し
	}
	// 敵弾の更新はWaveに関係なく行う
	UpdateEnemyBullets_(dt);
	// 敵弾とプレイヤーの当たり判定もWaveに関係なく行う
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

	wavePhase_ = WavePhase::W1; // 最初のWaveはW1から

	wave1Phase_ = Wave1Phase::Scatter;
	wave1PhaseTimer_ = 0.0f;
	BuildWave1ScatterPositions_();
	BuildWave1FormationPositions_();
	SpawnWave1Group();

	wave2SubWave_ = 0; // Wave2のサブWaveカウンタリセット

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

	// WavePhase に対応したスポーン関数があれば呼び出す（Done ならスポーン関数は nullptr なので何もしない）
	const auto ops_ = kWaveOps_[static_cast<int>(wavePhase_)];

	// スポーン関数があれば呼び出す
	if (ops_.spawn_) {
		(this->*ops_.spawn_)(); // メンバ関数ポインタ呼び出し
	}
}

void EnemyManager::GoToNextWave() {
	int next_ = static_cast<int>(wavePhase_) + 1; // 次のWaveに進める（WavePhaseの列挙値を整数として扱って次へ）

	// 次のWaveが WavePhase::Done を超えることはないようにする（念のため）
	if (next_ > static_cast<int>(WavePhase::Done)) {
		next_ = static_cast<int>(WavePhase::Done); // Done を超えないようにする
	}
	wavePhase_ = static_cast<WavePhase>(next_); // WavePhase を次に進める

	// Done なら spawn=nullptr なので何も起きない（分岐不要）
	SpawnCurrentWave();

	// 撃破数・最大数もリセット（ゲージを空にしておく）
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
	if (!dx_ || !camera_ || !parent_) {
		return;
	}

	// 目標撃破数に達したら次のWaveへ
	if (defeatedEnemyCount_ >= wave1DefeatTarget_) {
		NotifyPlayerBeforeClearEnemies_();
		enemies_.clear();
		GoToNextWave();
		return;
	}

	wave1PhaseTimer_ += dt;

	switch (wave1Phase_) {
	case Wave1Phase::Scatter:
		UpdateWave1ScatterAttack_(dt);
		if (wave1PhaseTimer_ >= wave1ScatterDuration_) {
			wave1Phase_ = Wave1Phase::FormUp;
			wave1PhaseTimer_ = 0.0f;
			ApplyWave1FormationTargets_();
		}
		break;

	case Wave1Phase::FormUp:
		if (AreAllWave1EnemiesInFormation_()) {
			wave1Phase_ = Wave1Phase::Hold;
			wave1PhaseTimer_ = 0.0f;
		}
		break;

	case Wave1Phase::Hold:
		if (wave1PhaseTimer_ >= wave1HoldDuration_) {
			wave1Phase_ = Wave1Phase::Attack;
			wave1PhaseTimer_ = 0.0f;
			BeginWave1SpecialCharge_();
		}
		break;

	case Wave1Phase::Attack:
		UpdateWave1SpecialCharge_(dt);

		// 発射後に少し余韻を見せてから解散
		if (!wave1SpecialCharging_ &&
			wave1SpecialCoreBullet_ == nullptr &&
			wave1PhaseTimer_ >= wave1SpecialChargeDuration_ + 0.35f) {
			wave1Phase_ = Wave1Phase::Break;
			wave1PhaseTimer_ = 0.0f;

			BuildWave1ScatterPositions_();
			ApplyWave1ScatterTargets_();
		}
		break;

	case Wave1Phase::Break:
		UpdateWave1ScatterAttack_(dt);
		if (AreAllWave1EnemiesInFormation_() || wave1PhaseTimer_ >= wave1BreakDuration_) {
			wave1Phase_ = Wave1Phase::Scatter;
			wave1PhaseTimer_ = 0.0f;
		}
		break;
	}
}

void EnemyManager::UpdateWave1ScatterAttack_(float dt) {
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

		pm_->Emit("w1sp_core_flash", start_, pm_->GetEmitCountScaled(2, priority_));
		pm_->Emit("w1sp_core_ring", start_, pm_->GetEmitCountScaled(2, priority_));
		pm_->Emit("w1sp_core_spark", start_, pm_->GetEmitCountScaled(12, priority_));
		pm_->Emit("w1sp_core_body", start_, pm_->GetEmitCountScaled(3, priority_));

		pm_->Emit("w1sp_core_flash", start_, pm_->GetEmitCountScaled(4, priority_));
		pm_->Emit("w1sp_core_ring", start_, pm_->GetEmitCountScaled(3, priority_));
		pm_->Emit("w1sp_core_shell", start_, pm_->GetEmitCountScaled(2, priority_));
		pm_->Emit("w1sp_core_spark", start_, pm_->GetEmitCountScaled(18, priority_));
		pm_->Emit("w1sp_core_burst", start_, pm_->GetEmitCountScaled(14, priority_));
		pm_->Emit("w1sp_core_arc", start_, pm_->GetEmitCountScaled(6, priority_));
		pm_->Emit("w1sp_core_body", start_, pm_->GetEmitCountScaled(4, priority_));
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
	int segmentCount_ = 10;
	switch (loadLevel_) {
	case TKM::ParticleManager::LoadLevel::Low:
		segmentCount_ = 10;
		break;

	case TKM::ParticleManager::LoadLevel::Medium:
		segmentCount_ = 8;
		break;

	case TKM::ParticleManager::LoadLevel::High:
		segmentCount_ = 6;
		break;

	case TKM::ParticleManager::LoadLevel::Critical:
		segmentCount_ = 4;
		break;
	}

	// 各敵からコアへ送る
	for (auto& e : enemies_) {
		if (!e || e->IsDead() || e->IsDying()) {
			continue;
		}

		const Vector3 src_ = e->GetWorldPosition();
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

		for (int i = 1; i <= segmentCount_; ++i) {
			const float u_ = static_cast<float>(i) / static_cast<float>(segmentCount_ + 1);

			// コアに近いほど密になるように後半へ寄せる
			const float t_ = 1.0f - (1.0f - u_) * (1.0f - u_);

			Vector3 p_ = src_ + dir_ * (len_ * t_);

			const float sideJitter_ = (0.16f - 0.10f * t_);
			if ((i % 2) == 0) {
				p_ += side_ * sideJitter_;
			} else {
				p_ -= side_ * sideJitter_;
			}

			// 芯はなるべく残す
			pm_->Emit("w1sp_stream_core", p_, pm_->GetEmitCountScaled(1, true));

			// 外側グローは補助なので負荷時は減らす
			pm_->Emit("w1sp_stream_glow", p_, pm_->GetEmitCountScaled(1, false));

			// 細線は2個に1回
			if ((i % 2) == 0) {
				pm_->Emit("w1sp_stream_streak", p_, pm_->GetEmitCountScaled(1, false));
			}

			// 丸粒はかなり補助なので少なめ
			if ((i % 4) == 0) {
				pm_->Emit("w1sp_stream", p_, pm_->GetEmitCountScaled(1, false));
			}
		}

		// 発射元の火花は補助
		pm_->Emit("w1sp_sender_glow", src_, pm_->GetEmitCountScaled(2, false));
	}

	// コア本体は見せ場側なので残し気味
	pm_->Emit("w1sp_core_body", corePos_, pm_->GetEmitCountScaled(3, true));
	pm_->Emit("w1sp_core_inner", corePos_, pm_->GetEmitCountScaled(5, true));
	pm_->Emit("w1sp_core_ring", corePos_, pm_->GetEmitCountScaled(2, true));
	pm_->Emit("w1sp_core_shell", corePos_, pm_->GetEmitCountScaled(1, true));
	pm_->Emit("w1sp_core_smoke", corePos_, pm_->GetEmitCountScaled(1, false));
	pm_->Emit("w1sp_core_arc", corePos_, pm_->GetEmitCountScaled(3, false));

	// チャージ終盤の加速演出
	if (wave1PhaseTimer_ >= wave1SpecialChargeDuration_ * 0.55f) {
		pm_->Emit("w1sp_core_flash", corePos_, pm_->GetEmitCountScaled(2, true));
		pm_->Emit("w1sp_core_spark", corePos_, pm_->GetEmitCountScaled(8, false));
		pm_->Emit("w1sp_core_arc", corePos_, pm_->GetEmitCountScaled(4, false));
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
		e_->SetType(EnemyType::Normal);

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
		e_->SetPosition(wave1ScatterPositions_[i]);
		e_->SetFormationTarget(wave1ScatterPositions_[i]);

		SetupEnemyForPlayer(*e_);
		e_->SyncTransform();

		enemies_.push_back(std::move(e_));
	}

	if (!enemies_.empty() && player_) {
		player_->SetEnemy(enemies_.front().get());
		player_->SetAllEnemies(&enemies_);
	}
}

void EnemyManager::BuildWave1ScatterPositions_() {
	wave1ScatterPositions_[0] = { -16.0f, 8.0f, 78.0f };
	wave1ScatterPositions_[1] = { -7.0f,  4.5f, 70.0f };
	wave1ScatterPositions_[2] = { 0.0f,  9.0f, 86.0f };
	wave1ScatterPositions_[3] = { 9.0f,  5.0f, 73.0f };
	wave1ScatterPositions_[4] = { 17.0f,  7.5f, 80.0f };
}
void EnemyManager::BuildWave1FormationPositions_() {
	const Vector3 c_ = wave1FormationCenter_;

	wave1FormationPositions_[0] = c_ + Vector3{ 0.0f,  2.0f,  0.0f };
	wave1FormationPositions_[1] = c_ + Vector3{ -6.0f,  0.0f,  2.5f };
	wave1FormationPositions_[2] = c_ + Vector3{ 6.0f,  0.0f,  2.5f };
	wave1FormationPositions_[3] = c_ + Vector3{ -11.0f, -2.0f, 5.0f };
	wave1FormationPositions_[4] = c_ + Vector3{ 11.0f, -2.0f, 5.0f };
}
void EnemyManager::ApplyWave1FormationTargets_() {
	for (size_t i = 0; i < enemies_.size() && i < wave1FormationPositions_.size(); ++i) {
		if (!enemies_[i] || enemies_[i]->IsDead()) {
			continue;
		}

		enemies_[i]->SetBehavior(EnemyBehavior::FormationMove);
		enemies_[i]->SetFormationMoveSpeed(wave1FormationMoveSpeed_);
		enemies_[i]->SetFormationTarget(wave1FormationPositions_[i]);
	}
}
bool EnemyManager::AreAllWave1EnemiesInFormation_() const {
	int aliveCount_ = 0;
	int formedCount_ = 0;

	for (const auto& e : enemies_) {
		if (!e || e->IsDead()) {
			continue;
		}

		++aliveCount_;
		if (e->IsInFormation()) {
			++formedCount_;
		}
	}

	if (aliveCount_ == 0) {
		return false;
	}

	return aliveCount_ == formedCount_;
}
void EnemyManager::ApplyWave1ScatterTargets_() {
	for (size_t i = 0; i < enemies_.size() && i < wave1ScatterPositions_.size(); ++i) {
		if (!enemies_[i] || enemies_[i]->IsDead()) {
			continue;
		}

		enemies_[i]->SetBehavior(EnemyBehavior::FormationMove);
		enemies_[i]->SetFormationMoveSpeed(wave1BreakMoveSpeed_);
		enemies_[i]->SetFormationTarget(wave1ScatterPositions_[i]);
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

				pm_->Emit("w1sp_fly_body", p_, 4);
				pm_->Emit("w1sp_fly_shell", p_, 2);
				pm_->Emit("w1sp_fly_corona", p_, 3);
				pm_->Emit("w1sp_fly_arc", p_, 3);
				pm_->Emit("w1sp_fly_tail", p_, 3);
				pm_->Emit("w1sp_fly_spark", p_, 5);
			}
		}

		if ((*it)->IsDead()) {
			it = enemyBullets_.erase(it);
		} else {
			++it;
		}
	}
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

	// サブWaveの数を超えたら次のWaveへ
	if (wave2SubWave_ >= wave2SubWaveCount_) {
		GoToNextWave(); // 次のWaveへ
		return;
	}
	SpawnWave2SubWave(wave2SubWave_); // 次のサブWaveをスポーン
}

namespace {
	using SubWaveFn = void (EnemyManager::*)();
	// Wave2のサブWaveスポーン関数テーブル。CSVのサブWaveの順番に対応させて定義する。
	static const SubWaveFn kWave2SubWaveTable_[] = {
		&EnemyManager::SpawnWave2_Triangle,
		&EnemyManager::SpawnWave2_Line,
		&EnemyManager::SpawnWave2_FastColumn,
	};
}

void EnemyManager::SpawnWave2SubWave(int id) {
	if (!&enemies_ || !dx_ || !camera_ || !parent_) { return; }

	// サブWaveをスポーンする前に、前のサブWaveの敵が残っているなら消す（混ざるの防止）
	NotifyPlayerBeforeClearEnemies_();
	enemies_.clear(); // 念のためクリア
	// サブWaveのIDが範囲外なら何もしない
	const int count_ = static_cast<int>(std::size(kWave2SubWaveTable_));
	if (id < 0 || id >= count_) { return; } // 範囲外なら何もしない
	// サブWaveのスポーン関数を呼び出す
	(this->*kWave2SubWaveTable_[id])();
}

Vector3 EnemyManager::GetWave1SpecialCorePosition_() const {
	return wave1FormationCenter_ + wave1SpecialCoreOffset_;
}

void EnemyManager::SetCamera(TKM::Camera* camera) {
	BattleActorManagerBase::SetCamera(camera);
}

void EnemyManager::OnCameraChanged() {
	// カメラが変わったことを、全ての敵と中ボスの核に通知して、必要ならカメラ参照を更新させる
	for (auto& e : enemies_) {
		if (e) { e->SetCamera(camera_); }
	}
	// 中ボスの核もカメラ参照を更新しておく（存在すれば）
	if (midBossCore_) {
		midBossCore_->SetCamera(camera_);
	}
}

// ───────────────────────────────────────────────
// ● Wave2 各小Waveスポーン関数群
// ───────────────────────────────────────────────
void EnemyManager::SpawnWave2_Triangle() {
	if (!&enemies_ || !dx_ || !camera_ || !parent_) return;
	// Wave2のサブWave1のスポーン設定をCSVから取得
	const auto& s_ = waveConfig_.GetWave2SubWave(0);
	int idx_ = 0; // 敵ごとに位相をずらすためのインデックス（0から始まる連番）

	EnemyFactory::SpawnV( // 画面中央を頂点とするV字型の隊列で敵をスポーンさせる
		enemies_,
		s_.triCountPerSide_,
		s_.triY_, s_.triZ_,
		s_.triXCenter_,
		s_.triXStep_,
		s_.triZStep_,
		dx_, camera_, parent_,
		[this, &idx_](Enemy& e) {
			// Wave2のサブWave1の敵のパラメータをCSVから取得
			const auto& pt_ = waveConfig_.GetWave2TriEnemyParams();

			e.SetModel(pt_.model_); // モデルセット

			// モデルによっては触手もセットする（例：jerryfish.objならtentacle.objもセットして触手表示）
			if (pt_.model_ == "jerryfish.obj") {
				e.SetTentacleModel("tentacle.obj"); // 触手モデルセット
				e.SetTentacleLocal({ 0.0f, 0.0f, 0.0f }, { 0.0f,0.0f,0.0f }, { 1.0f,1.0f,1.0f }); // 触手のローカル位置・回転・スケールセット（例ではモデル原点に配置して等倍スケール）
			}
			e.SetBehavior(pt_.behavior_); // 振る舞いセット
			e.SetVelocity(pt_.vel_); // ベロシティセット
			e.SetSineParams(pt_.sineAmp_, pt_.sineFreq_); // サイン波移動の振幅と周波数セット
			e.SetSinePhase(pt_.phaseStep_ * float(idx_++)); // 敵ごとに位相をずらすためのパラメータセット（インデックスに応じて位相をずらす）
			e.SetHP(pt_.hp_); // HPセット
			// その他のパラメータもセット
			SetupEnemyForPlayer(e);
		}
	);
}

void EnemyManager::SpawnWave2_Line() {
	if (!&enemies_ || !dx_ || !camera_ || !parent_) return;
	// Wave2のサブWave2のスポーン設定をCSVから取得
	const auto& s_ = waveConfig_.GetWave2SubWave(1);

	EnemyFactory::SpawnLine( // 画面奥から手前に向かって、等間隔で敵を並べてスポーンさせる
		enemies_,
		s_.lineCount_, s_.lineY_, s_.lineZ_,
		s_.lineXStart_, s_.lineXStep_,
		dx_, camera_, parent_,
		[this](Enemy& e) {
			// Wave2のサブWave2の敵のパラメータをCSVから取得
			const auto& pl_ = waveConfig_.GetWave2LineEnemyParams();

			e.SetModel(pl_.model_); // モデルセット

			// モデルによっては触手もセットする
			if (pl_.model_ == "jerryfish.obj") {
				e.SetTentacleModel("tentacle.obj"); // 触手モデルセット
				e.SetTentacleLocal({ 0.0f, 0.0f, 0.0f }, { 0.0f,0.0f,0.0f }, { 1.0f,1.0f,1.0f }); // 触手のローカル位置・回転・スケールセット（例ではモデル原点に配置して等倍スケール）
			}
			e.SetBehavior(pl_.behavior_); // 振る舞いセット
			e.SetVelocity(pl_.vel_); // ベロシティセット
			e.SetStopZ(pl_.stopZ_); // 停止するZ座標セット（これに達したら止まる）
			e.SetHP(pl_.hp_); // HPセット
			// その他のパラメータもセット
			SetupEnemyForPlayer(e);
		}
	);
}

void EnemyManager::SpawnWave2_FastColumn() {
	if (!&enemies_ || !dx_ || !camera_ || !parent_) return;
	// Wave2のサブWave3のスポーン設定をCSVから取得
	const auto& s_ = waveConfig_.GetWave2SubWave(2);

	EnemyFactory::SpawnColumn( // 画面奥から手前に向かって、等間隔で敵を並べてスポーンさせる（列）
		enemies_,
		s_.colCount_,
		s_.colX_,
		s_.colZStart_,
		s_.colZStep_,
		s_.colYStart_,
		s_.colYStep_,
		dx_, camera_, parent_,
		[this](Enemy& e) {
			// Wave2のサブWave3の敵のパラメータをCSVから取得
			const auto& pc_ = waveConfig_.GetWave2ColEnemyParams();

			e.SetModel(pc_.model_); // モデルセット

			// モデルによっては触手もセットする
			if (pc_.model_ == "jerryfish.obj") {
				e.SetTentacleModel("tentacle.obj"); // 触手モデルセット
				e.SetTentacleLocal({ 0.0f, 0.0f, 0.0f }, { 0.0f,0.0f,0.0f }, { 1.0f,1.0f,1.0f }); // 触手のローカル位置・回転・スケールセット（例ではモデル原点に配置して等倍スケール）
			}
			e.SetBehavior(pc_.behavior_); // 振る舞いセット
			e.SetVelocity(pc_.vel_); // ベロシティセット
			e.SetStopZ(pc_.stopZ_); // 停止するZ座標セット（これに達したら止まる）
			e.SetHP(pc_.hp_); // HPセット
			// その他のパラメータもセット
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

		// 中ボスの種類と生存状態をチェックしてカウント
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
			// コアを殺す（蘇生キャンセル）
			if (coreAlive_) {
				midBossCore_->StartDeathReaction({ 0.0f, 0.0f, 1.0f });
			}
			wave3ReviveInProgress_ = false; // 蘇生フェーズ終了
			wave3CoreTimer_ = 0.0f; // タイマーリセット

			// プレイヤーの参照も切る（nullptrセット）
			if (player_) {
				player_->SetMidBossCore(nullptr);
			}
			midBossCore_.reset(); // 核オブジェクト破棄
		}

		// 中ボスも核も居なければ Wave3 終了 → 次のWave（ボス）へ
		if (enemies_.empty() && !midBossCore_) {
			GoToNextWave();
		}
		// 次のフレームのために生きてる中ボスの数を保存しておく
		wave3PrevAliveMidBossCount_ = aliveMidBossCount_;
		return;
	}

	// ---- 中ボスが 1 体になった瞬間に核を出す ----
	if (aliveMidBossCount_ == 1 && !wave3ReviveInProgress_ && midBossJustDied_) {
		wave3ReviveInProgress_ = true; // 蘇生フェーズ開始
		wave3CoreTimer_ = 0.0f; // タイマーリセット
		SpawnWave3Core(); // 核スポーン

		// プレイヤーに核の参照を渡す
		for (auto& e : enemies_) {
			if (!e) continue;
			if (e->GetType() != EnemyType::Wave3MidBoss) continue;
			if (e->IsDead() || e->IsDying()) continue;

			e->SetAngry(wave3AngryDuration_); // 怒り状態セット（例：核が出たら怒る）
		}
		// プレイヤーに核の参照を渡す
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
		wave3ReviveInProgress_ = false; // 蘇生フェーズ終了
		wave3CoreTimer_ = 0.0f; // タイマーリセット

		// プレイヤーの参照も切る（nullptrセット）
		if (player_) {
			player_->SetMidBossCore(nullptr);
		}
		midBossCore_.reset(); // 核オブジェクト破棄
		// 次のフレームのために生きてる中ボスの数を保存しておく
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
			midBossCore_->StartDeathReaction({ 0.0f, 0.0f, 1.0f }); // 核を殺す（蘇生完了）
		}

		wave3ReviveInProgress_ = false; // 蘇生フェーズ終了
		wave3CoreTimer_ = 0.0f; // タイマーリセット
	}
	// 次のフレームのために生きてる中ボスの数を保存しておく
	wave3PrevAliveMidBossCount_ = aliveMidBossCount_;
}

void EnemyManager::SpawnWave3MidBossStage() {
	if (!&enemies_ || !dx_ || !camera_ || !parent_) {
		return;
	}
	NotifyPlayerBeforeClearEnemies_(); // プレイヤーに敵全削除を通知（ロックオン解除などのため）
	enemies_.clear(); // 敵リストクリア（前のWaveの敵を消す）
	midBossCore_.reset(); // 中ボスの核もリセット（存在すれば）

	wave3ReviveInProgress_ = false; // 蘇生フェーズ開始フラグリセット
	wave3CoreTimer_ = 0.0f; // 蘇生フェーズ用タイマーリセット

	auto camPtr_ = camera_; // ラムダ内でキャプチャするためのローカル変数に、クラスメンバのポインタをコピーしておく
	auto dxPtr_ = dx_; // ラムダ内でキャプチャするためのローカル変数に、クラスメンバのポインタをコピーしておく
	auto parentPtr_ = parent_; // ラムダ内でキャプチャするためのローカル変数に、クラスメンバのポインタをコピーしておく

	EnemyFactory::SpawnLine( // 画面左と右の両端から、等間隔で中ボスを並べてスポーンさせる
		enemies_,
		2,
		wave3LeftPos_.y,
		wave3LeftPos_.z,
		wave3LeftPos_.x,
		(wave3RightPos_.x - wave3LeftPos_.x),
		dxPtr_, camPtr_, parentPtr_,
		[this](Enemy& e) {
			// Wave3の中ボスのパラメータをCSVから取得
			const auto& pm_ = waveConfig_.GetWave3MidBossParams();

			e.SetModel(pm_.model_); // モデルセット
			if (pm_.model_ == "jerryfish.obj") {
				e.SetTentacleModel("tentacle.obj"); // 触手モデルセット
				e.SetTentacleLocal({ 0.0f, 0.0f, 0.0f }, { 0.0f,0.0f,0.0f }, { 1.0f,1.0f,1.0f }); // 触手のローカル位置・回転・スケールセット（例ではモデル原点に配置して等倍スケール）
			}
			e.SetBehavior(pm_.behavior_); // 振る舞いセット
			e.SetFreeRoamArea(pm_.areaMin_, pm_.areaMax_, pm_.normalSpeed_, pm_.rageSpeed_); // 自由に動き回るエリアと、通常移動速度と怒り状態の移動速度をセット
			e.SetHP(pm_.hp_); // HPセット
			e.SetScale({ pm_.scale_, pm_.scale_, pm_.scale_ }); // スケールセット（例では等倍）
			// その他のパラメータもセット
			SetupEnemyForPlayer(e);
			// タイマーで怒り状態にするのではなく、核が出たときに怒るようにするので、ここでは怒り状態にしない
			e.SetType(EnemyType::Wave3MidBoss);
		}
	);

	// 同時出現数の上限を、最初は中ボス2体分にしておく（必要なら後で更新）
	if (maxEnemyCount_) {
		maxEnemyCount_ += 2; // 中ボス2体分追加
	}
	// 次のフレームのために生きてる中ボスの数を保存しておく
	wave3PrevAliveMidBossCount_ = 2;
}

void EnemyManager::SpawnWave3Core() {
	if (!dx_ || !camera_ || !parent_) {
		return;
	}

	// Wave3のコアのスポーン設定をCSVから取得
	const auto& w3_ = waveConfig_.GetWave3();

	// コアのスポーン位置をランダムに決める（Xは左右の範囲でランダム、Zはmin〜maxの範囲でランダム、Yは固定）
	float xRange_ = w3_.coreXRange_;
	float zMin_ = w3_.coreZMin_;
	float zMax_ = w3_.coreZMax_;
	// 0〜1のランダム値を生成
	float rx_ = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	float rz_ = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	// ランダム値を元にスポーン位置を計算
	float x_ = -xRange_ + rx_ * (xRange_ * 2.0f);
	float z_ = zMin_ + rz_ * (zMax_ - zMin_);
	float y_ = w3_.coreY_;

	// すでにコアが居たら一旦消して作り直し
	midBossCore_ = std::make_unique<MidBossCore>();

	// Object3d 用共通（Enemy でも使ってるやつ）
	auto* common_ = TKM::Object3dCommon::GetInstance();

	// ---- コアの初期化 ----
	midBossCore_->Initialize(common_, dx_); // コアの初期化
	midBossCore_->SetCamera(camera_); // コアにカメラ参照をセット
	midBossCore_->SetParentScene(parent_); // コアに親シーン参照をセット
	midBossCore_->SetPosition({ x_, y_, z_ }); // コアのスポーン位置セット
	midBossCore_->SetScale({ 0.8f, 0.8f, 0.8f }); // コアのスケールセット
	midBossCore_->SetHP(wave3CoreHP_); // コアのHPセット
	midBossCore_->SyncTransform(); // コアのワールド行列を計算して反映（位置・回転・スケールをワールド行列に反映させる）

	if (player_) {
		// コアにもロック・プレイヤー情報を渡す
		midBossCore_->SetReticle(player_->GetReticle());
		// コアのプレイヤー位置取得関数に、プレイヤーの位置を返すラムダをセット
		midBossCore_->SetPlayer([this]() { return player_->GetPosition(); });
		// プレイヤーにもコアを教える
		player_->SetMidBossCore(midBossCore_.get());
	}
}

void EnemyManager::SpawnWave3ExtraMidBoss() {
	if (!&enemies_ || !dx_ || !camera_ || !parent_) {
		return;
	}

	bool leftAlive_ = false; // 左側の中ボスが生きているか
	bool rightAlive_ = false; // 右側の中ボスが生きているか

	// どちらの中ボスが生きているかを Enemy リストからチェックして、蘇生させる側を決める
	for (auto& e : enemies_) {
		if (!e) continue;
		if (e->GetType() != EnemyType::Wave3MidBoss) continue;
		if (e->IsDead()) continue;

		Vector3 pos = e->GetWorldPosition(); // 中ボスの位置を取得

		// X座標が負なら左側、正なら右側と判断して、生きている側を記録する
		if (pos.x < 0.0f) {
			leftAlive_ = true;
		} else { // X座標が0以上なら右側
			rightAlive_ = true;
		}
	}
	// 左右両方生きてるなら何もしない（蘇生させない）
	const int state_ = (leftAlive_ ? 1 : 0) | (rightAlive_ ? 2 : 0);
	// state_ の値は 0〜3 の4パターンになる。0=両方死んでる、1=左だけ生きてる、2=右だけ生きてる、3=両方生きてる
	static const int kSpawnSide_[4] = {
		0,
		1,
		0,
		0,
	};

	const Vector3 kSidePos_[2] = { wave3LeftPos_, wave3RightPos_ }; // 左右のスポーン位置
	const Vector3 spawnPos_ = kSidePos_[kSpawnSide_[state_]]; // 蘇生させる側のスポーン位置

	auto camPtr_ = camera_; // ラムダ内でキャプチャするためのローカル変数に、クラスメンバのポインタをコピーしておく
	auto dxPtr_ = dx_; // ラムダ内でキャプチャするためのローカル変数に、クラスメンバのポインタをコピーしておく
	auto parentPtr_ = parent_; // ラムダ内でキャプチャするためのローカル変数に、クラスメンバのポインタをコピーしておく

	EnemyFactory::SpawnLine( // 画面左か右の端から、1体だけ中ボスをスポーンさせる
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
			// Wave3の追加中ボスのパラメータをCSVから取得
			const auto& px_ = waveConfig_.GetWave3ExtraMidBossParams();

			e.SetModel(px_.model_); // モデルセット

			// モデルによっては触手もセットする（例：jerryfish.objならtentacle.objもセットして触手表示）
			if (px_.model_ == "jerryfish.obj") {
				e.SetTentacleModel("tentacle.obj"); // 触手モデルセット
				e.SetTentacleLocal({ 0.0f, 0.0f, 0.0f }, { 0.0f,0.0f,0.0f }, { 1.0f,1.0f,1.0f }); // 触手のローカル位置・回転・スケールセット
			}
			e.SetBehavior(px_.behavior_); // 振る舞いセット
			e.SetVelocity(px_.vel_); // ベロシティセット
			e.SetStopZ(px_.stopZ_); // 停止するZ座標セット（これに達したら止まる）
			e.SetHP(px_.hp_); // HPセット
			e.SetScale({ px_.scale_, px_.scale_, px_.scale_ }); // スケールセット
			e.SetType(EnemyType::Wave3MidBoss); // 蘇生させるやつも中ボスタイプにしておく
			// その他のパラメータもセット
			SetupEnemyForPlayer(e);
		}
	);
}

void EnemyManager::BeginWave1() {
	wave1Phase_ = Wave1Phase::Scatter; // 最初は散開フェーズから
	wave1PhaseTimer_ = 0.0f; // Wave1の状態をリセット
	wave1NormalShotTimer_ = 0.0f;
	wave1SpecialCharging_ = false;
	wave1SpecialCoreBullet_ = nullptr;

	if (maxEnemyCount_) {
		maxEnemyCount_ = wave1DefeatTarget_;
	}

	BuildWave1ScatterPositions_();
	BuildWave1FormationPositions_();
	SpawnWave1Group();
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

	DrawEnemyBullets_(dx); // 敵の弾を全部描画

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

	ImGui::End();

	// 核
	if (midBossCore_) {
		midBossCore_->ImGuiDebug(); // 核のデバッグ表示も呼び出す
	}
#endif
}