#include "BossManager.h"
#include "GameScene.h"
#include <algorithm>
#include "MyMath.h"

// ワールド座標をスクリーンUV座標に変換する
static Vector2 WorldToUV(const Vector3& world, const Matrix4x4& vp) {
	float clipX = world.x * vp.m[0][0] + world.y * vp.m[1][0] + world.z * vp.m[2][0] + 1.0f * vp.m[3][0];
	float clipY = world.x * vp.m[0][1] + world.y * vp.m[1][1] + world.z * vp.m[2][1] + 1.0f * vp.m[3][1];
	float clipW = world.x * vp.m[0][3] + world.y * vp.m[1][3] + world.z * vp.m[2][3] + 1.0f * vp.m[3][3];

	if (fabsf(clipW) < 0.0001f) { return { 0.5f, 0.5f }; }

	float ndcX = clipX / clipW;
	float ndcY = clipY / clipW;

	return { ndcX * 0.5f + 0.5f, -ndcY * 0.5f + 0.5f };
}

namespace { // 無名名前空間
	BossManager::BossBattleConfig MakeBossConfig() {
		BossManager::BossBattleConfig c_{};
		c_.spawnPos_ = { 0.0f, 0.0f, 200.0f };

		c_.arenaMin_ = { -18.0f, 3.0f, 35.0f };
		c_.arenaMax_ = { 18.0f,12.0f, 70.0f };

		TKM::WaterRippleEffect::RippleDesc d_{};
		d_.duration_ = 0.35f;
		d_.radiusMax_ = 1.45f;
		d_.amplitude_ = 0.10f;
		d_.frequency_ = 85.0f;
		d_.width_ = 10.0f;
		c_.killRipple_ = d_;

		c_.killSlowScale_ = 0.00001f;
		c_.killSlowDuration_ = 1.7f;
		return c_;
	}

	const BossManager::BossBattleConfig kBossConfig_ = MakeBossConfig();
}

void BossManager::Initialize(TKM::DirectXCommon* dxCommon, TKM::Camera* camera, TKM::BaseScene* parent, Player* player) {
	dxCommon_ = dxCommon;
	camera_ = camera;
	parentScene_ = parent;
	player_ = player;

	bossBattle_ = false; // ボス戦開始フラグ
	bossP2BgmPlayed_ = false; // ボスP2BGM再生フラグ
	boss_.reset(); // ボスオブジェクト
	bossBullets_.clear(); // ボス弾リスト

	// オーラボリュームレンダラー初期化
	auraVolume_ = std::make_unique<TKM::AuraVolumeRenderer>();
	auraVolume_->Initialize(dxCommon_);

	// LaserBeam3D 初期化
	laserBeam3D_ = std::make_unique<TKM::LaserBeam3D>();
	laserBeam3D_->Initialize(dxCommon_);
	laserBeam3D_->GetDesc().active_ = false; // 非アクティブ開始
	laserBeam3D_->GetDesc().telegraph_ = false; // 予告モード開始

	// 初期見た目（好みで調整OK）
	laserBeam3D_->GetDesc().color_ = { 0.2f, 0.85f, 1.0f }; // 色
	laserBeam3D_->GetDesc().intensity_ = 3.0f; // 明るさ
	laserBeam3D_->GetDesc().coreSharpness_ = 7.0f; // コアのシャープネス
	laserBeam3D_->GetDesc().edgeSoftness_ = 1.2f; // エッジの柔らかさ
	laserBeam3D_->GetDesc().sliceCount_ = 64; // スライス数
	laserBeam3D_->GetDesc().noiseScale_ = 1.0f; // ノイズの細かさ
	laserBeam3D_->GetDesc().noiseSpeed_ = 1.0f; // ノイズの速さ

	// HPバーUI初期化
	hpUI_ = std::make_unique<TKM::BossHpBarUI>();
	TKM::BossHpBarUI::Desc d{};
	hpUI_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, parentScene_, d);
	hpUI_->SetVisible(false);
}

void BossManager::StartBattle() {
	if (bossBattle_) { // すでにボス戦中
		return;
	}
	if (!dxCommon_ || !camera_ || !parentScene_) { // 安全確認
		return;
	}

	// --- ボス本体生成 ---
	bossBattle_ = true; // ボス戦開始フラグセット
	bossP2BgmPlayed_ = false; // P2BGM再生フラグリセット
	
	// ボス生成
	boss_ = std::make_unique<BossEnemy>();
	boss_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);
	boss_->SetCamera(camera_);
	boss_->SetParentScene(parentScene_);

	// プレイヤー位置取得ラムダ
	if (player_) {
		boss_->SetPlayer([this]() {
			return player_->GetPosition();
			});
	}
	// ボス初期位置セット
	boss_->SetPosition(kBossConfig_.spawnPos_);

	// --- ボス挙動コントローラ生成 ---
	bossController_ = std::make_unique<BossController>();
	bossController_->Initialize(kBossConfig_.arenaMin_, kBossConfig_.arenaMax_);

	killSeq_.Reset(); // 撃破シーケンス状態リセット

	if (hpUI_) { // HPバーUI表示
		hpUI_->SetVisible(true); // 表示ON
	}
}

void BossManager::Update(float dt) {
	if (!bossBattle_ || !boss_) {
		UpdateBossBullets();
		return;
	}

	if (bossController_) {
		bossController_->Update(dt, *boss_);
	}

	// --- LaserBeam 更新＆BossControllerのレーザー情報を反映 ---
	if (laserBeam3D_) {
		laserBeam3D_->Update(dt);

		LaserInfo li = GetLaserInfo();
		auto& d_ = laserBeam3D_->GetDesc();
		d_.active_ = li.active_;
		d_.telegraph_ = li.telegraph_;
		d_.startWS_ = li.startWS_;
		d_.endWS_ = li.endWS_;
		d_.radius_ = li.radius_; // 見た目の太さ＝当たり判定半径に一致させる
	}

	if (hpUI_ && boss_) {
		hpUI_->Update(dt, boss_.get());
	}

	// ボス本体更新
	boss_->Update(dt);

	// ボス撃破ズーム開始（1回だけ）
	if (!killSeq_.zoomStarted_ && boss_->IsDying()) {
		if (player_) {
			player_->StartBossDeathCameraZoom();
		}
		killSeq_.zoomStarted_ = true;

		// 波紋
		/*if (!killSeq_.rippleTriggered && waterRipple_ && camera_) {
			Matrix4x4 vp = camera_->GetViewProjectionMatrix();
			Vector2 uv = WorldToUV(boss_->GetWorldPosition(), vp);
			waterRipple_->Trigger(uv, kBossConfig.killRipple);
			killSeq_.rippleTriggered = true;
		}*/

		// スロー
		if (!killSeq_.slowTriggered_ && timeScale_) {
			timeScale_->RequestSlow(kBossConfig_.killSlowScale_, kBossConfig_.killSlowDuration_);
			killSeq_.slowTriggered_ = true;
		}
	}

	UpdateBossBullets();

#ifdef USE_IMGUI
	if (bossController_ && boss_) {
		bossController_->ImGuiDebug(*boss_);
	}
#endif
}

void BossManager::Draw(TKM::DirectXCommon* dxCommon) {
	if (!bossBattle_ || !boss_) { return; }

	boss_->Draw(dxCommon);

	for (auto& b : bossBullets_) {
		b->Draw(dxCommon);
	}

	// --- LaserBeam 描画（空間上） ---
	if (laserBeam3D_ && camera_) {
		const Matrix4x4& camW_ = camera_->GetWorldMatrix();

		// ※GameSceneと同じ取り方（translationが m[3]、基底が row0/1/2 前提）
		Vector3 right_{ camW_.m[0][0], camW_.m[0][1], camW_.m[0][2] };
		Vector3 up_{ camW_.m[1][0], camW_.m[1][1], camW_.m[1][2] };
		Vector3 fwd_{ camW_.m[2][0], camW_.m[2][1], camW_.m[2][2] };

		Matrix4x4 vp_ = camera_->GetViewProjectionMatrix();
		laserBeam3D_->Draw(vp_, right_, up_, fwd_);
	}
}

void BossManager::DrawUI() {
	if (hpUI_ && bossBattle_ && boss_ && !boss_->IsDead()) {
		hpUI_->Draw();
	}
}

void BossManager::SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame) {
	if (!dxCommon_ || !camera_) {
		return;
	}

	auto bullet_ = std::make_unique<BossBullet>();
	bullet_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_, camera_, pos, dir, speed, damage, lifeFrame);
	bossBullets_.push_back(std::move(bullet_));
}

BossManager::LaserInfo BossManager::GetLaserInfo() const {
	LaserInfo li_{};
	if (!bossController_) { return li_; }
	li_.active_ = bossController_->IsLaserActive();
	li_.telegraph_ = bossController_->IsLaserTelegraph();
	li_.startWS_ = bossController_->GetLaserStartWS();
	li_.endWS_ = bossController_->GetLaserEndWS();
	li_.radius_ = bossController_->GetLaserRadius();
	return li_;
}

bool BossManager::TestLaserHit(const LaserInfo& laser, const Vector3& sphereCenterWS, float sphereRadius) {
	if (!laser.active_) { return false; }

	const Vector3 p0_ = laser.startWS_;
	const Vector3 p1_ = laser.endWS_;
	const Vector3 c_ = sphereCenterWS;

	Vector3 d_ = MyMath::Subtract(p1_, p0_);
	float dlen2_ = MyMath::Dot(d_, d_);

	if (dlen2_ < 1e-6f) {
		Vector3 dc_ = MyMath::Subtract(c_, p0_);
		float dist2_ = MyMath::Dot(dc_, dc_);
		float r_ = laser.radius_ + sphereRadius;
		return dist2_ <= r_ * r_;
	}

	float t_ = MyMath::Dot(MyMath::Subtract(c_, p0_), d_) / dlen2_;
	t_ = std::clamp(t_, 0.0f, 1.0f);

	Vector3 q_ = MyMath::Add(p0_, MyMath::Multiply(t_, d_));
	Vector3 cq_ = MyMath::Subtract(c_, q_);

	float dist2_ = MyMath::Dot(cq_, cq_);
	float r_ = laser.radius_ + sphereRadius;
	return dist2_ <= r_ * r_;
}

bool BossManager::IsBattleActive() const {
	return bossBattle_ && boss_ != nullptr && !boss_->IsDead();
}

bool BossManager::IsBossAlive() const {
	return boss_ && !boss_->IsDead();
}

bool BossManager::IsBossDead() const {
	return boss_ && boss_->IsDead();

	if (hpUI_ && boss_ && boss_->IsDead()) {
		hpUI_->SetVisible(false);
	}
}

void BossManager::OnClearSequenceStart() {
	bossBattle_ = false;
	bossBullets_.clear();
	boss_.reset();
	bossController_.reset();
	bossP2BgmPlayed_ = false;
}

void BossManager::UpdateBossBullets() {
	for (auto it = bossBullets_.begin(); it != bossBullets_.end();) {
		(*it)->Update();
		if ((*it)->IsDead()) {
			it = bossBullets_.erase(it);
		} else {
			++it;
		}
	}
}