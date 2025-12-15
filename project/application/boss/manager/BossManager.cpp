#include "BossManager.h"
#include "GameScene.h"

static Vector2 WorldToUV(const Vector3& world, const Matrix4x4& vp) {
	float clipX = world.x * vp.m[0][0] + world.y * vp.m[1][0] + world.z * vp.m[2][0] + 1.0f * vp.m[3][0];
	float clipY = world.x * vp.m[0][1] + world.y * vp.m[1][1] + world.z * vp.m[2][1] + 1.0f * vp.m[3][1];
	float clipW = world.x * vp.m[0][3] + world.y * vp.m[1][3] + world.z * vp.m[2][3] + 1.0f * vp.m[3][3];

	if (fabsf(clipW) < 0.0001f) { return { 0.5f, 0.5f }; }

	float ndcX = clipX / clipW;
	float ndcY = clipY / clipW;

	return { ndcX * 0.5f + 0.5f, -ndcY * 0.5f + 0.5f };
}

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

	// 初期位置セット
	boss_->SetPosition({ 0, 0, 200 });

	// --- ボス挙動コントローラ生成 ---
	bossController_ = std::make_unique<BossController>();
	// ボスの行動範囲
	Vector3 arenaMin{ -18.0f, 3.0f, 35.0f }; // Y軸は地面から少し上
	Vector3 arenaMax{ 18.0f, 12.0f, 70.0f }; // Y軸は天井より少し下
	bossController_->Initialize(arenaMin, arenaMax); // 行動範囲セット
}

void BossManager::Update(float dt) {
	if (!bossBattle_ || !boss_) {
		// ボス戦中でないなら弾だけ掃除しておく
		UpdateBossBullets();
		return;
	}

	// --- ボス挙動更新 ---
	if (bossController_) {
		bossController_->Update(dt, *boss_); // ボス挙動更新
	}

	// ===== Aura（予備動作中だけ）=====
	if (dxCommon_ && camera_ && bossController_) {
		AuraEffect* aura = dxCommon_->GetAuraEffect();
		if (aura) {
			const bool active = bossController_->IsAuraActive(); // DashWindup中 true :contentReference[oaicite:6]{index=6}
			aura->SetActive(active);

			if (active) {
				// ボス中心をスクリーンUVへ
				Matrix4x4 vp = camera_->GetViewProjectionMatrix();
				Vector3 bossPos = boss_->GetWorldPosition();
				Vector2 uv = WorldToUV(bossPos, vp);

				// Controllerのパラメータをそのまま反映
				aura->SetCenterUV(uv);
				aura->SetIntensity(bossController_->GetAuraIntensity());
				aura->SetUseRing(bossController_->GetAuraUseRing());
				aura->SetColorA(bossController_->GetAuraColor()); // とりあえず単色運用

				// いったんB色は固定（あとでImGuiで混色運用にする）
				aura->SetColorB({ 1.0f, 0.85f, 0.2f });
				aura->SetMix(0.0f);

				// スケールは画面上の広がり（0.15〜0.35くらいで調整）
				// ボス倍率は Controller の値を使う
				float scale = 0.22f * bossController_->GetAuraScaleMul();
				aura->SetScale(scale);

				// リングは仮（欲しければ後で詰める）
				aura->SetRingRadius(0.12f);
				aura->SetRingWidth(22.0f);

				// GPUへ送る
				aura->PushToGpu();
			}
		}
	}

	// ボス本体更新
	boss_->Update();

	// ボス撃破ズーム開始（1回だけ）
	if (!bossZoomStarted_ && boss_->IsDying()) {
		if (player_) {
			player_->StartBossDeathCameraZoom();
		}
		bossZoomStarted_ = true;
	}

	// ボス弾更新（共通処理にまとめた）
	UpdateBossBullets();

#ifdef USE_IMGUI
	if (bossController_ && boss_) {
		bossController_->ImGuiDebug(*boss_);
	}
#endif
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