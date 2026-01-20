#include "BossManager.h"
#include "GameScene.h"
#include <algorithm>

static Vector2 WorldToUV(const Vector3& world, const Matrix4x4& vp) {
	float clipX = world.x * vp.m[0][0] + world.y * vp.m[1][0] + world.z * vp.m[2][0] + 1.0f * vp.m[3][0];
	float clipY = world.x * vp.m[0][1] + world.y * vp.m[1][1] + world.z * vp.m[2][1] + 1.0f * vp.m[3][1];
	float clipW = world.x * vp.m[0][3] + world.y * vp.m[1][3] + world.z * vp.m[2][3] + 1.0f * vp.m[3][3];

	if (fabsf(clipW) < 0.0001f) { return { 0.5f, 0.5f }; }

	float ndcX = clipX / clipW;
	float ndcY = clipY / clipW;

	return { ndcX * 0.5f + 0.5f, -ndcY * 0.5f + 0.5f };
}

namespace {
	BossManager::BossBattleConfig MakeBossConfig() {
		BossManager::BossBattleConfig c{};
		c.spawnPos = { 0.0f, 0.0f, 200.0f };

		c.arenaMin = { -18.0f, 3.0f, 35.0f };
		c.arenaMax = { 18.0f,12.0f, 70.0f };

		TKM::WaterRippleEffect::RippleDesc d{};
		d.duration = 0.35f;
		d.radiusMax = 1.45f;
		d.amplitude = 0.10f;
		d.frequency = 85.0f;
		d.width = 10.0f;
		c.killRipple = d;

		c.killSlowScale = 0.00001f;
		c.killSlowDuration = 1.7f;
		return c;
	}

	const BossManager::BossBattleConfig kBossConfig = MakeBossConfig();
}

void BossManager::Initialize(TKM::DirectXCommon* dxCommon, TKM::Camera* camera, TKM::BaseScene* parent, Player* player) {
	dxCommon_ = dxCommon;
	camera_ = camera;
	parentScene_ = parent;
	player_ = player;

	bossBattle_ = false;
	bossP2BgmPlayed_ = false;
	boss_.reset();
	bossBullets_.clear();

	// オーラボリュームレンダラー初期化
	auraVolume_ = std::make_unique<TKM::AuraVolumeRenderer>();
	auraVolume_->Initialize(dxCommon_);

	// LaserBeam3D 初期化
	laserBeam3D_ = std::make_unique<TKM::LaserBeam3D>();
	laserBeam3D_->Initialize(dxCommon_);
	laserBeam3D_->GetDesc().active = false;
	laserBeam3D_->GetDesc().telegraph = false;

	// 初期見た目（好みで調整OK）
	laserBeam3D_->GetDesc().color = { 0.2f, 0.85f, 1.0f };
	laserBeam3D_->GetDesc().intensity = 3.0f;
	laserBeam3D_->GetDesc().coreSharpness = 7.0f;
	laserBeam3D_->GetDesc().edgeSoftness = 1.2f;
	laserBeam3D_->GetDesc().sliceCount = 64;
	laserBeam3D_->GetDesc().noiseScale = 1.0f;
	laserBeam3D_->GetDesc().noiseSpeed = 1.0f;

	hpUI_ = std::make_unique<TKM::BossHpBarUI>();
	TKM::BossHpBarUI::Desc d{};
	hpUI_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, parentScene_, d);
	hpUI_->SetVisible(false);
}

void BossManager::StartBattle() {
	if (bossBattle_) {
		return;
	}

	if (!dxCommon_ || !camera_ || !parentScene_) {
		return;
	}

	bossBattle_ = true;
	bossP2BgmPlayed_ = false;

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

	boss_->SetPosition(kBossConfig.spawnPos);

	// --- ボス挙動コントローラ生成 ---
	bossController_ = std::make_unique<BossController>();
	bossController_->Initialize(kBossConfig.arenaMin, kBossConfig.arenaMax);

	killSeq_.Reset();

	if (hpUI_) {
		hpUI_->SetVisible(true);
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
		auto& d = laserBeam3D_->GetDesc();
		d.active = li.active;
		d.telegraph = li.telegraph;
		d.startWS = li.startWS;
		d.endWS = li.endWS;
		d.radius = li.radius; // 見た目の太さ＝当たり判定半径に一致させる
	}

	if (hpUI_ && boss_) {
		hpUI_->Update(dt, boss_.get());
	}

	// ボス本体更新
	boss_->Update(dt);

	// ボス撃破ズーム開始（1回だけ）
	if (!killSeq_.zoomStarted && boss_->IsDying()) {
		if (player_) {
			player_->StartBossDeathCameraZoom();
		}
		killSeq_.zoomStarted = true;

		// 波紋
		/*if (!killSeq_.rippleTriggered && waterRipple_ && camera_) {
			Matrix4x4 vp = camera_->GetViewProjectionMatrix();
			Vector2 uv = WorldToUV(boss_->GetWorldPosition(), vp);
			waterRipple_->Trigger(uv, kBossConfig.killRipple);
			killSeq_.rippleTriggered = true;
		}*/

		// スロー
		if (!killSeq_.slowTriggered && timeScale_) {
			timeScale_->RequestSlow(kBossConfig.killSlowScale, kBossConfig.killSlowDuration);
			killSeq_.slowTriggered = true;
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
		const Matrix4x4& camW = camera_->GetWorldMatrix();

		// ※GameSceneと同じ取り方（translationが m[3]、基底が row0/1/2 前提）
		Vector3 right{ camW.m[0][0], camW.m[0][1], camW.m[0][2] };
		Vector3 up{ camW.m[1][0], camW.m[1][1], camW.m[1][2] };
		Vector3 fwd{ camW.m[2][0], camW.m[2][1], camW.m[2][2] };

		Matrix4x4 vp = camera_->GetViewProjectionMatrix();
		laserBeam3D_->Draw(vp, right, up, fwd);
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

	auto bullet = std::make_unique<BossBullet>();
	bullet->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_, camera_, pos, dir, speed, damage, lifeFrame);
	bossBullets_.push_back(std::move(bullet));
}

BossManager::LaserInfo BossManager::GetLaserInfo() const {
	LaserInfo li{};
	if (!bossController_) { return li; }
	li.active = bossController_->IsLaserActive();
	li.telegraph = bossController_->IsLaserTelegraph();
	li.startWS = bossController_->GetLaserStartWS();
	li.endWS = bossController_->GetLaserEndWS();
	li.radius = bossController_->GetLaserRadius();
	return li;
}

static float Dot3(const Vector3& a, const Vector3& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float LengthSq3(const Vector3& v) {
	return Dot3(v, v);
}

static Vector3 Sub3(const Vector3& a, const Vector3& b) {
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

static Vector3 Add3(const Vector3& a, const Vector3& b) {
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

static Vector3 Mul3(const Vector3& a, float s) {
	return { a.x * s, a.y * s, a.z * s };
}

bool BossManager::TestLaserHit(const LaserInfo& laser, const Vector3& sphereCenterWS, float sphereRadius) {
	if (!laser.active) { return false; }

	const Vector3 p0 = laser.startWS;
	const Vector3 p1 = laser.endWS;
	const Vector3 c = sphereCenterWS;

	Vector3 d = Sub3(p1, p0);
	float dlen2 = LengthSq3(d);

	if (dlen2 < 1e-6f) {
		Vector3 dc = Sub3(c, p0);
		float dist2 = LengthSq3(dc);
		float r = laser.radius + sphereRadius;
		return dist2 <= r * r;
	}

	float t = Dot3(Sub3(c, p0), d) / dlen2;
	t = std::clamp(t, 0.0f, 1.0f);

	Vector3 q = Add3(p0, Mul3(d, t));
	Vector3 cq = Sub3(c, q);

	float dist2 = LengthSq3(cq);
	float r = laser.radius + sphereRadius;
	return dist2 <= r * r;
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