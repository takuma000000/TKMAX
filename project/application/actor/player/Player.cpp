#include "Player.h"
#include "Enemy.h"
#include <ParticleManager.h>
#include "AABB.h"
#include <limits>
#include "RadialBlurEffect.h"
#include "BarrierCore.h"
#include "AudioManager.h"
#include "PlayerShotManager.h"
#include "PlayerShotConfig.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void Player::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	common_ = common; // Object3d共通
	dxCommon_ = dxCommon; // DirectX共通

	// TrailRibbonRenderer初期化
	TKM::TrailRibbonRenderer::GetInstance()->Initialize(dxCommon_);

	// 3Dオブジェクト作成
	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common_, dxCommon_);
	object_->SetModel("turtle.obj");

	// ヒレ（4枚入り）
	flipper_ = std::make_unique<TKM::Object3d>();
	flipper_->Initialize(common_, dxCommon_);
	flipper_->SetModel("turtle_flipper.obj");
	// 親子付け
	flipper_->SetParent(object_.get());
	flipperBaseRot_ = flipper_->GetRotate(); // ヒレの回転の基準値を保存
	flipperAnimT_ = 0.0f; // ヒレのアニメーション用タイマー

	reticle_ = std::make_unique<Reticle>();
	reticle_->Initialize(common_, dxCommon_, "reticle_big.obj"); // モデル指定可
	// Player から位置とヨー角(radians)を渡す（循環依存を避けるためコールバック）
	reticle_->BindOwner(
		[this]() { return object_->GetTranslate(); },
		[this]() { return object_->GetRotate().y; }
	);
	reticle_->SetMoveRange(moveMin_, moveMax_); // レティクルの移動範囲を指定
	reticle_->GetCenterWorldPos(); // 中心位置取得用

	// ショットマネージャー初期化
	shotManager_ = std::make_unique<PlayerShotManager>();
	shotManager_->Initialize(this, common_, dxCommon_);

	const bool loaded = shotConfig_.Load("./resources/data/playerShotConfig.json");
	assert(loaded && "playerShotConfig.json の読込に失敗しました");

	shotManager_->SetConfig(&shotConfig_);

	shotManager_->SetOwnerObject(object_.get());
	shotManager_->SetReticle(reticle_.get());
	shotManager_->SetCamera(camera_);

	// パーティクルグループ作成
	TKM::ParticleManager::GetInstance()->CreateParticleGroup(
		"jetSmoke", "./resources/texture/circle.png", TKM::ParticleManager::ParticleType::NORMAL); // ジェット煙
	TKM::ParticleManager::GetInstance()->CreateParticleGroup(
		"damageSpark", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL); // 故障スパーク（バチバチ）
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_rb", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL); // 弾の軌跡
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lb", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL); // 弾の軌跡
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_rt", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL); // 弾の軌跡
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lt", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL); // 弾の軌跡
	// --- LT弾：メルヘン弾道（3レイヤー）---
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lt_ribbon", "./resources/texture/firework_star.png", TKM::ParticleManager::ParticleType::RIBBON);
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lt_sparkle", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL);
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lt_ring", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::RING);
	TKM::ParticleManager::GetInstance()->CreateParticleGroup(
		"trail_lb_glitter",
		"./resources/texture/firework_star.png",
		TKM::ParticleManager::ParticleType::NORMAL);
	TKM::ParticleManager::GetInstance()->CreateParticleGroup(
		"trail_lb_bolt_main",
		"./resources/texture/gradationLine.png",
		TKM::ParticleManager::ParticleType::NORMAL);
	TKM::ParticleManager::GetInstance()->CreateParticleGroup(
		"trail_lb_bolt_core",
		"./resources/texture/gradationLine.png",
		TKM::ParticleManager::ParticleType::NORMAL);

	if (enableJetSmoke_) { // ジェット煙初期化
		Vector3 jetPos = object_->GetTranslate();
		jetPos.z -= kJetSmokeOffsetZ_;           // 機体のケツあたり
		jetEmitter_.Initialize("jetSmoke", jetPos);
	}

	// ワンウェイバリア
	wave1BarrierHits_.clear(); // ワンウェイバリアヒット情報リスト初期化
}

void Player::Update(float dt) {
	UpdateRumble(dt); // コントローラー振動更新

	// --- 無敵時間 更新（操作無効でも進める）---
	if (isInvincible_) {
		invincibleT_ += dt;
		blinkT_ += dt;

		if (blinkT_ >= kBlinkInterval_) {
			blinkT_ = 0.0f;
			invincibleVisible_ = !invincibleVisible_;
		}

		if (invincibleT_ >= kInvincibleSec_) {
			isInvincible_ = false;
			invincibleT_ = 0.0f;
			blinkT_ = 0.0f;
			invincibleVisible_ = true; // 最後は必ず表示
		}
	}

	// 被弾フラッシュ用タイマー更新
	if (hitFlashTimer_ > 0.0f) {
		hitFlashTimer_ -= dt;
		if (hitFlashTimer_ < 0.0f) {
			hitFlashTimer_ = 0.0f;
		}
	}

	if (sameAttackLockT_ > 0.0f) { // 同一攻撃IDロックタイマー更新
		sameAttackLockT_ -= dt;
		if (sameAttackLockT_ < 0.0f) { sameAttackLockT_ = 0.0f; }
	}

	if (reticle_) reticle_->Update(dt);

	// Wave1バリアのヒット情報を更新
	for (auto it = wave1BarrierHits_.begin(); it != wave1BarrierHits_.end();) {
		it->age_ += dt;
		// 寿命が尽きてたら削除
		if (it->age_ >= it->life_) {
			it = wave1BarrierHits_.erase(it);
		} else { // 生存してたら次へ
			++it;
		}
	}

	// 操作有効かつ生存中のみゲームプレイ処理
	if (controlEnabled_ && !isDead_) {
		HandleGamePadMove();
		HandleDodge(dt);

		if (shotManager_) {
			shotManager_->Update(dt, shootingEnabled_);
		}
	} else {
		if (shotManager_) {
			shotManager_->ClearLockState();
		}
	}

#ifdef USE_IMGUI
	// ───────── 自機当たり判定ワイヤーボックス描画 ─────────
	{
		Vector3 center = object_->GetTranslate();
		Vector3 size = colliderScale_;

		auto* lr = TKM::LineRenderer::GetInstance();

		TKM::LineRenderer::Color col =
			(hitFlashTimer_ > 0.0f)
			? TKM::LineRenderer::Color{ 1.0f, 0.0f, 0.0f, 1.0f }
		: TKM::LineRenderer::Color{ 0.0f, 1.0f, 0.0f, 1.0f };

		lr->AddAABB(center, size, col);
	}
#endif

	Death(); // 撃墜処理

	// ---- ジェット煙（HPが0なら停止）----
	if (enableJetSmoke_ && hp_ > 0) {
		Vector3 jetPos = object_->GetTranslate();
		jetPos.z -= kJetSmokeOffsetZ_;
		jetEmitter_.SetPosition(jetPos);
		jetEmitter_.Update();
	}

	RemoveEnemyIfDead(); // 敵が死んでたら参照をクリア
	UpdateFlipperAnim_(dt); // ヒレのアニメーション更新
	if (!isDead_) {
		UpdateFloatBob_(dt); // 浮遊のアニメーション更新
		HandleFollowCamera(); // 生存中だけカメラ追従
	}

	// TrailRibbonRendererの更新
	TKM::TrailRibbonRenderer::GetInstance()->Update(dt);

	TKM::ParticleManager::GetInstance()->Update(dt); // パーティクルマネージャー更新
	object_->Update(); // プレイヤー本体更新
	flipper_->Update(); // ヒレ更新
}

void Player::DrawTrails(TKM::DirectXCommon* dxCommon) {
	// TrailRibbonRendererの描画
	shotManager_->DrawTrails(dxCommon);

}

void Player::ImGuiDebug() {
#ifdef USE_IMGUI
	if (!object_) return;

	Vector3 pos = object_->GetTranslate();
	Vector3 rot = object_->GetRotate();
	Vector3 scale = object_->GetScale();

	//---------------- プレイヤー本体 ----------------
	ImGui::Begin("プレイヤー");

	ImGui::Text("バリア状態: %s", wave1BarrierActive_ ? "ON" : "OFF");
	ImGui::Text("ヒット数: %d", static_cast<int>(wave1BarrierHits_.size()));

	ImGui::Text("直前に当たった攻撃ID: %d", lastHitAttackId_);

	if (ImGui::DragFloat3("位置", &pos.x, 0.01f)) {
		object_->SetTranslate(pos);
	}
	if (ImGui::DragFloat3("回転", &rot.x, 0.01f)) {
		object_->SetRotate(rot);
	}
	if (ImGui::DragFloat3("拡縮cale", &scale.x, 0.01f)) {
		object_->SetScale(scale);
	}
	ImGui::Separator(); // 区切り線
	// 当たり判定サイズ
	Vector3 col = colliderScale_;
	if (ImGui::DragFloat3("当たり判定サイズ(自機)", &col.x, 0.01f, 0.01f, 50.0f)) {
		colliderScale_ = col;
	}
	ImGui::Separator(); // 区切り線
	if (ImGui::Button("HPリセット")) { hp_ = 5; } // 2
	ImGui::SeparatorText("カメラシェイク");
	ImGui::SliderFloat("強度のベース", &shakeBaseStrength_, 0.0f, 5.0f); // ベースとなるカメラシェイク強度
	ImGui::SliderFloat("ズーム強調", &shakeZoomBoost_, 0.0f, 15.0f); // ズーム時の追加倍率
	ImGui::Text("現在の増幅量 : %.2f", shakeBaseStrength_ + (1.0f - camZoom_) * shakeZoomBoost_); // 現在の倍率を表示
	ImGui::End();
	////---------------- プレイヤー弾ステータス ----------------
	//ImGui::Begin("P弾ステータス");
	//ImGui::SliderFloat("弾速度(RB,RT,LB)", &normalBulletSpeed_, 0.1f, 15.0); // RB,RT,LBの弾速度調整

	//ImGui::Separator();

	//float rate = float(rbAmmo_) / float(kRbAmmoMax_);
	//char label[64];
	//std::snprintf(
	//	label,
	//	sizeof(label),
	//	"RB弾数 %d / %d",
	//	rbAmmo_,
	//	kRbAmmoMax_
	//);
	//ImGui::ProgressBar(rate, ImVec2(260.0f, 18.0f), label);

	//if (rbRefilling_) {
	//	ImGui::Text("RB回復中...");
	//} else if (rbAmmo_ <= 0) {
	//	ImGui::Text("RB回復まで %.2f 秒", std::max(0.0f, kRbEmptyWaitSec_ - rbEmptyTimer_));
	//} else {
	//	ImGui::Text("RBアイドル回復まで %.2f 秒", std::max(0.0f, kRbEmptyWaitSec_ - rbNoFireTimer_));
	//}

	//ImGui::End();
#endif
}

void Player::RemoveEnemyIfDead() {
	// 敵が死んでたらターゲットを解除
	shotManager_->RemoveDeadTargets();
}

void Player::EnableSpecialAttack() {
	shotManager_->EnableSpecialAttack(); // ショットマネージャーに一撃必殺使用可能を通知
}

void Player::OnEnemyDestroyed(Enemy* e) {
	// 敵が破壊されたときの処理をショットマネージャーに通知（ロック解除や一撃必殺の解放など）
	shotManager_->OnEnemyDestroyed(e);
}

void Player::Damage(int value) {

	if (isInvincible_) { return; }

	hp_ -= value;
	if (hp_ < 0) hp_ = 0;

	// ==========================
	// 被弾Rumble（発射と違う感触）
	// 1段目: 左強めで「ドン」
	// 2段目: 少し遅らせて右で「ビリ」
	// ==========================
	StartRumble(0.10f, 52000, 18000);  // ドン（重い）
	rumble2Pending_ = true;
	rumble2DelayT_ = 0.07f;            // ちょい遅らせる
	rumble2Sec_ = 0.08f;
	rumble2Left_ = 0;
	rumble2Right_ = 42000;             // ビリ（細かい）

	// 被弾したので当たり判定ボックスをしばらく赤くする
	hitFlashTimer_ = 0.15f;

	// --- 無敵開始（2秒）---
	isInvincible_ = true;
	invincibleT_ = 0.0f;
	blinkT_ = 0.0f;
	invincibleVisible_ = true;
}

void Player::Death() {
	if (hp_ > 0) {
		return;
	}

	// 死亡開始時に一度だけ行う処理
	if (!deathStartHandled_) {
		deathStartHandled_ = true;
		isDead_ = true;

		SetControlEnabled(false);   // 操作停止
		SetShootingEnabled(false);  // 射撃停止
		SetReticleVisible(false);   // レティクル非表示

		// ロック状態クリア（撃墜後はターゲットロックも意味ないので）
		shotManager_->ClearLockState();

		// 画面手前(-Z)に一発だけ弾かれて、そのまま落ちる
		deathBackwardDir_ = { 0.0f, 0.0f, -1.0f };
		deathVelocity_ = { 0.0f, -kDeathFallStartSpeed_, -kDeathBackwardSpeed_ * 0.8f };

		// 姿勢は少しだけ崩す
		float rollSign = (rand() % 2 == 0) ? -1.0f : 1.0f;
		deathAngularVelocity_.x = 0.012f;
		deathAngularVelocity_.y = 0.0f;
		deathAngularVelocity_.z = 0.020f * rollSign;

		StartCameraShake(20);
	}

	// 下方向へ加速
	deathVelocity_.y -= kDeathGravity_;
	if (deathVelocity_.y < -kDeathFallMaxSpeed_) {
		deathVelocity_.y = -kDeathFallMaxSpeed_;
	}

	// 手前方向(Z)の勢いは少しずつだけ抜ける
	deathVelocity_.z *= kDeathBackwardDamping_;

	// 位置反映
	Vector3 pos = object_->GetTranslate();
	pos += deathVelocity_;
	object_->SetTranslate(pos);

	// 姿勢更新
	Vector3 newRot = object_->GetRotate();
	newRot.x += deathAngularVelocity_.x;
	newRot.z += deathAngularVelocity_.z;

	if (newRot.x > kDeathMaxPitch_) {
		newRot.x = kDeathMaxPitch_;
	}

	newRot.z = std::clamp(newRot.z, -kDeathMaxRoll_, kDeathMaxRoll_);
	object_->SetRotate(newRot);

	deathAngularVelocity_ *= kDeathRotateDamping_;
}

void Player::UpdateVisualOnly(float dt) {
	// クリア演出用：入力や弾処理は回さず、見た目（親子付け/アニメ）だけ更新する
	UpdateFlipperAnim_(dt);
	// クリア演出用
	if (object_) { object_->Update(); }
	if (flipper_) { flipper_->Update(); }
}

void Player::StartBossDeathCameraZoom() {
	// すでにボス用ズーム中なら二重起動しない
	if (bossZoomActive_) {
		return;
	}

	// 調整用定数
	const float kTargetZoom = 0.35f;
	const float kZoomTime = 1.2f;   // カメラが引ききるまでの時間
	const float kBlurTime = 4.795f;   // ブラー継続時間

	// ---- ズームアウト用トゥイーン設定 ----
	bossZoomActive_ = true;
	bossZoomTween_.Reset(1.0f, kTargetZoom, kZoomTime, Ease::Type::OutCubic);
	bossZoom_ = 1.0f;

	// ---- ラジアルブラー発火 ----
	if (radialBlur_) {
		// 強さ = 2.0f、時間 = kBlurTime
		radialBlur_->BulrStartShock(2.0f, kBlurTime);
	}
}

bool Player::TryDamageFromAttack(int damage, int attackId) {
	if (isInvincible_) {
		return false;
	}

	// lock中で同じ攻撃IDなら無視
	if (sameAttackLockT_ > 0.0f && attackId == lastHitAttackId_) {
		return false;
	}

	// 通す
	Damage(damage);
	lastHitAttackId_ = attackId;
	sameAttackLockT_ = 0.20f; // 0.2秒くらい（好みで）
	return true;
}

void Player::Draw(TKM::DirectXCommon* dxCommon) {

	// --- 無敵点滅：見えないタイミングは自機だけ描画しない ---
	if (isInvincible_ && !invincibleVisible_) {
		// 自機（胴体＋ヒレ）を両方スキップ
	} else {
		object_->Draw(dxCommon);
		if (flipper_) flipper_->Draw(dxCommon);
	}
	// クリア演出中などで隠したいときはフラグでOFF
	if (reticle_ && reticleVisible_) {
		reticle_->Draw(dxCommon);
	}

	shotManager_->DrawBullets(dxCommon);
}

void Player::SetCamera(TKM::Camera* camera) {
	this->camera_ = camera;
	if (object_) { object_->SetCamera(camera); }
	if (reticle_) { reticle_->SetCamera(camera); }
	if (flipper_) { flipper_->SetCamera(camera); }
	shotManager_->SetCamera(camera); // ショットマネージャー
}

void Player::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos); // 位置設定
}

void Player::SetParentScene(TKM::BaseScene* scene) {
	parentScene_ = scene; // 親シーン設定
}

void Player::SetEnemy(Enemy* enemy) {
	shotManager_->SetEnemy(enemy); // ショットマネージャーに敵の参照をセット（ロックオンや追従弾のターゲット用）
}

void Player::SetAllEnemies(std::vector<std::unique_ptr<Enemy>>* enemies) {
	shotManager_->SetAllEnemies(enemies); // ショットマネージャーに敵のリストの参照をセット（全体ロックオンや追従弾のターゲット用）
}

void Player::SetControlEnabled(bool enabled) {
	controlEnabled_ = enabled;

	if (reticle_) {
		reticle_->SetInputEnabled(enabled);
	}
}

void Player::SetReticleVisible(bool visible) {
	reticleVisible_ = visible; // レティクルの表示 / 非表示を切り替えるフラグ
}

void Player::SetBarrierCore(BarrierCore* core) {
	shotManager_->SetBarrierCore(core); // ショットマネージャーにミッドボスコアの参照をセット（ロックオンや一撃必殺のターゲット用）
}

void Player::SetColliderScale(const Vector3& s) {
	colliderScale_ = s;
}

void Player::SetRadialBlurEffect(TKM::RadialBlurEffect* effect) {
	radialBlur_ = effect; // ラジアルブラーエフェクトの参照をセット
}

void Player::SetRotation(const Vector3& r) {
	object_->SetRotate(r); // 回転設定（直接指定版）
}

void Player::StartCameraShake(int frameCount) {
	cameraShakeFrame_ = frameCount; // シェイクフレーム数セット
}

void Player::StopRumble() {
	// タイマー・強度を全部リセット
	rumbleT_ = 0.0f;
	rumbleLeft_ = 0;
	rumbleRight_ = 0;

	// 追い振動も潰す
	rumble2Pending_ = false;
	rumble2DelayT_ = 0.0f;
	rumble2Sec_ = 0.0f;
	rumble2Left_ = 0;
	rumble2Right_ = 0;

	// 実際に振動も止める
	TKM::Input::GetInstance()->SetVibration(0, 0);
}

void Player::SetYaw(float yawRad) {
	if (!object_) { return; } // 安全確認
	Vector3 r = object_->GetRotate(); // 現在の回転を取得
	r.y = yawRad; // ヨー角だけ更新
	object_->SetRotate(r); // ヨー角だけ更新
}

void Player::SetWave1BarrierInfo(bool active, const Vector3& center, const Vector3& size) {
	wave1BarrierActive_ = active;
	wave1BarrierCenter_ = center;
	wave1BarrierSize_ = size;

	if (!wave1BarrierActive_) {
		wave1BarrierHits_.clear();
	}
}

void Player::SetBarrierCoreManager(BarrierCoreManager* manager) {
	shotManager_->SetBarrierCoreManager(manager); // ショットマネージャーにバリアコアマネージャーの参照をセット（ワンウェイバリアのエフェクト用）
}

void Player::UpdateTitleIdle(float dt) {
	// タイトル専用：入力/射撃/移動/ロックオン等は一切触らない
	// ただし Draw に必要な行列更新だけは行う

	// ヒレだけパタパタ（既存の内部関数を使う）
	UpdateFlipperAnim_(dt);

	// 行列更新（これをしないと描画が古いままになることがある）
	if (object_) { object_->Update(); }
	if (flipper_) { flipper_->Update(); }
}

void Player::AddWave1BarrierHit(const Vector3& worldPos) {
	// ワンウェイバリアに当たった位置を記録（エフェクト描画用）
	Wave1BarrierHit hit_;
	hit_.worldPos_ = worldPos;
	hit_.age_ = 0.0f;
	hit_.life_ = 0.35f;

	// 古いヒット情報を消しつつ追加
	wave1BarrierHits_.push_back(hit_);

	// 上限を超えたら古いのから消す
	if (wave1BarrierHits_.size() > kWave1BarrierHitMax_) {
		wave1BarrierHits_.erase(wave1BarrierHits_.begin()); // 最初の要素を削除
	}
}

void Player::OnBarrierCoreDestroyed(BarrierCore* core) {
	// コアが破壊されたときの処理をショットマネージャーに通知（ロック解除や一撃必殺の解放など）
	shotManager_->OnBarrierCoreDestroyed(core);
}

void Player::RequestWave1BarrierFlash(const Vector3& worldPos) {
	wave1BarrierFlashRequested_ = true;
	wave1BarrierFlashPos_ = worldPos;
}

bool Player::ConsumeWave1BarrierFlashRequest(Vector3& outWorldPos) {
	if (!wave1BarrierFlashRequested_) {
		return false;
	}

	outWorldPos = wave1BarrierFlashPos_;
	wave1BarrierFlashRequested_ = false;
	return true;
}

void Player::SetShootingEnabled(bool enabled) {
	shootingEnabled_ = enabled; // 射撃の有効 / 無効を切り替えるフラグ

	shotManager_->SetShootingEnabled(enabled); // ショットマネージャーにも通知して、射撃処理全体をON/OFF
}

const std::list<std::unique_ptr<PlayerBullet>>& Player::GetBullets() const {
	return shotManager_->GetBullets();
}

bool Player::IsRbRefilling() const {
	return shotManager_ ? shotManager_->IsRbRefilling() : false;
}

int Player::GetRbAmmo() const {
	return shotManager_ ? shotManager_->GetRbAmmo() : 0;
}

int Player::GetRbAmmoMax() const {
	return shotManager_ ? shotManager_->GetRbAmmoMax() : 0;
}

int Player::GetLbAmmo() const {
	return shotManager_ ? shotManager_->GetLbAmmo() : 0;
}

int Player::GetLbAmmoMax() const {
	return shotManager_ ? shotManager_->GetLbAmmoMax() : 0;
}

void Player::SetRumbleEnabled(bool enabled) {
	rumbleEnabled_ = enabled; // コントローラー振動の有効 / 無効を切り替えるフラグ
	if (!enabled) { // 無効にするなら、今鳴ってるのも即停止（追い振動も潰す）
		StopRumble(); // 鳴ってる最中のも即停止（追い振動も潰す）
	}
}

void Player::SetRotate(const Vector3& rotRad) {
	if (!object_) { return; } // 安全確認
	object_->SetRotate(rotRad); // 回転設定（直接指定版）
}

void Player::HandleGamePadMove() {
	if (!object_) return;
	if (isDodging_) return;

	Vector3 pos = object_->GetTranslate();
	Vector3 newPos = pos;
	bool movingThisFrame = false;

	if (reticle_) {
		Vector3 target = pos;

		// レティクル中心のワールド座標（今は「世界に固定される」）
		Vector3 aim = reticle_->GetCenterWorldPos();

		// X/Y だけ追従、Zは固定
		target.x = std::clamp(aim.x, moveMin_.x, moveMax_.x);
		target.y = std::clamp(aim.y, moveMin_.y, moveMax_.y);
		target.z = 0.0f;

		Vector3 diff = { target.x - pos.x, target.y - pos.y, 0.0f };
		float dist2 = diff.x * diff.x + diff.y * diff.y;

		const float stopDist = 0.02f; // これ以内なら「追いついた」とみなす

		if (dist2 > stopDist * stopDist) {
			const float follow = 0.12f; // 追従のキモ（大きいほどキビキビ）
			newPos.x = MyMath::Lerp(pos.x, target.x, follow);
			newPos.y = MyMath::Lerp(pos.y, target.y, follow);
			newPos.z = 0.0f;
			movingThisFrame = true;
		} else {
			newPos = target; // ほぼ同じなら座標を揃えてピタッと停止
		}

		// 画面外に行かないようにクランプ
		newPos.x = std::clamp(newPos.x, moveMin_.x, moveMax_.x);
		newPos.y = std::clamp(newPos.y, moveMin_.y, moveMax_.y);
	}

	// ---- バンク処理は今のロジックを流用 ----
	float vx = newPos.x - pos.x;
	float vy = newPos.y - pos.y;

	if (movingThisFrame) {
		const float kBankStrength_ = 0.8f;
		const float kPitchStrength_ = 0.45f;
		const float kSpring_ = 0.25f;
		const float kDamping_ = 0.45f;

		float targetBank = -vx * kBankStrength_;
		float targetPitch = -vy * kPitchStrength_;

		bankVel_ += (targetBank - bankAngle_) * kSpring_ - bankVel_ * kDamping_;
		bankAngle_ += bankVel_;

		pitchVel_ += (targetPitch - pitchAngle_) * kSpring_ - pitchVel_ * kDamping_;
		pitchAngle_ += pitchVel_;
	} else {
		const float kResetSpring_ = 0.25f;
		const float kResetDamping_ = 0.5f;

		bankVel_ += (0.0f - bankAngle_) * kResetSpring_ - bankVel_ * kResetDamping_;
		bankAngle_ += bankVel_;

		pitchVel_ += (0.0f - pitchAngle_) * kResetSpring_ - pitchVel_ * kResetDamping_;
		pitchAngle_ += pitchVel_;

		if (std::fabs(bankAngle_) < 0.001f && std::fabs(bankVel_) < 0.001f) {
			bankAngle_ = 0.0f;
			bankVel_ = 0.0f;
		}

		if (std::fabs(pitchAngle_) < 0.001f && std::fabs(pitchVel_) < 0.001f) {
			pitchAngle_ = 0.0f;
			pitchVel_ = 0.0f;
		}
	}

	// 傾き
	object_->SetTranslate(newPos);
	Vector3 rot = object_->GetRotate();
	rot.x = pitchAngle_; // X軸回転(上下)
	rot.z = bankAngle_; // Z軸回転(左右)
	object_->SetRotate(rot);
}
void Player::HandleFollowCamera() {
	const float dt = 1.0f / 60.0f;
	// FPV分岐はしない（ズームは追従側で処理）
	UpdateCameraFollowThirdPerson(dt);
}

void Player::UpdateCameraFollowThirdPerson(float dt) {
	if (!camera_) return;

	Vector3 playerPos = object_->GetTranslate();
	Vector3 camRot = camera_->GetRotate();

	// ベース値は従来どおり
	const float baseDistance = 40.0f;
	const float baseHeight = 4.0f;

	// ---- LT一時ズームアウト更新 ----
	if (ltZoomActive_) {
		camZoom_ = ltZoomTween_.Update(dt);  // 係数を更新
		// 最小到達＆ホールドが残っていれば消化
		if (ltZoomTween_.Finished() && ltZoomTween_.end < ltZoomTween_.start) {
			if (ltZoomHold_ > 0.0f) {
				ltZoomHold_ -= dt;
			} else {
				// 逆方向に戻すトゥイーン開始（0.25秒で 0.82→1.0）
				ltZoomTween_.Reset(ltZoomTween_.end, 1.0f, 0.25f, Ease::Type::OutCubic);
			}
		}
		// 完全に戻り切ったら終了
		if (ltZoomTween_.Finished() && ltZoomTween_.end == 1.0f) {
			ltZoomActive_ = false;
			camZoom_ = 1.0f;
		}
	} else {
		camZoom_ = 1.0f;
	}

	// ---- ボス撃破ズームアウト更新 ----
	if (bossZoomActive_) {
		bossZoom_ = bossZoomTween_.Update(dt);
		if (bossZoomTween_.Finished()) {
			// トゥイーン完了 → ここで止めるだけ。bossZoom_ の値はそのまま保持。
			bossZoomActive_ = false;
		}
	}

	float zoom = camZoom_ * bossZoom_;
	float distance = baseDistance / zoom; // ← これで LT & ボス両方が効く
	float height = baseHeight;

	float angleY = camRot.y;
	Vector3 offset = {
		std::sinf(angleY) * -distance,
		height,
		std::cosf(angleY) * -distance
	};

	// ---- カメラシェイク処理 ----
	if (cameraShakeFrame_ > 0) {
		float zoomKick = std::max(0.0f, 1.0f - camZoom_);
		float shakeGain = shakeBaseStrength_ + zoomKick * shakeZoomBoost_;
		// ランダムオフセットを生成（距離に応じて強さ変化）
		cameraShakeOffset_.x = ((rand() % 100 - 50) / 500.0f) * shakeGain;
		cameraShakeOffset_.y = ((rand() % 100 - 50) / 500.0f) * shakeGain;
		cameraShakeOffset_.z = ((rand() % 100 - 50) / 500.0f) * shakeGain;
		// フレームを減らす
		cameraShakeFrame_--;
	} else {
		cameraShakeOffset_ = { 0,0,0 };
	}


	Vector3 cameraPos = playerPos + offset + cameraShakeOffset_;
	camera_->SetTranslate(cameraPos);
}

void Player::ZoomCamera() {
	// === LT押下時の一時カメラズーム ===
	const float kInTarget = 0.6f; // ズーム到達目標値
	const float kInTime = 0.12f;  // 再ターゲット時の寄り時間（短め）
	const float kOutTime = 0.25f;  // 戻り時間
	const float kHoldUnit = 1.5f;  // 1回の押下で与えるホールド秒
	const float kHoldMax = 1.2f;  // 連打してもここまで（上限）

	if (!ltZoomActive_) {
		// まだズームしていなければ通常起動
		ltZoomActive_ = true;
		ltZoomTween_.Reset(1.0f, kInTarget, 0.18f, Ease::Type::OutCubic);
		ltZoomHold_ = kHoldUnit; // 初回ホールド
		return;
	}

	// 既にズーム中
	const bool isInPhase = (ltZoomTween_.end < ltZoomTween_.start);  // IN方向
	const bool finished = ltZoomTween_.Finished();

	if (isInPhase) {
		if (!finished) {
			// まだ「寄りアニメ」進行中 → 何もしない（Resetしない）
			return;
		}
		// INが完了して「HOLD中」→ ホールドを上限まで延長（積み上げない）
		ltZoomHold_ = std::min(kHoldMax, std::max(ltZoomHold_, kHoldUnit));
		return;
	}

	// ここに来るのは「OUT（戻り）中」→ 現在値から再びINへ
	ltZoomTween_.Reset(camZoom_, kInTarget, kInTime, Ease::Type::OutCubic);
	ltZoomHold_ = kHoldUnit; // 再度短くホールド
}

void Player::StartRumble(float sec, WORD leftMotor, WORD rightMotor) {
	if (!rumbleEnabled_) { return; } // 振動禁止中は無視
	// すでに鳴ってるときは、時間は長い方、強さは強い方を優先して上書きするイメージ
	rumbleT_ = std::max(rumbleT_, sec);
	rumbleLeft_ = std::max(rumbleLeft_, leftMotor);
	rumbleRight_ = std::max(rumbleRight_, rightMotor);
	// 追い振動は、今鳴ってるのにさらに強い振動が来たときに、上書きせずに追加するイメージ
	TKM::Input::GetInstance()->SetVibration(rumbleLeft_, rumbleRight_);
}

void Player::UpdateRumble(float dt) {
	// 2段目（追い振動）を時間になったら発火
	if (rumble2Pending_) {
		rumble2DelayT_ -= dt;
		if (rumble2DelayT_ <= 0.0f) {
			rumble2Pending_ = false;
			StartRumble(rumble2Sec_, rumble2Left_, rumble2Right_);
		}
	}

	if (rumbleT_ <= 0.0f) { return; } // 鳴ってない

	// 継続時間を減らす
	rumbleT_ -= dt;
	if (rumbleT_ <= 0.0f) {
		rumbleT_ = 0.0f;
		rumbleLeft_ = 0;
		rumbleRight_ = 0;
		TKM::Input::GetInstance()->SetVibration(0, 0);
	}
}

void Player::UpdateFlipperAnim_(float dt) {
	if (!flipper_) { return; }

	flipperAnimT_ += dt;

	// サイン波（-1..+1）
	float w = 2.0f * MyMath::GetPI() * flipperFlapHz_;
	float s = std::sinf(flipperAnimT_ * w);

	// パタパタ（上下フラップ）
	float flap = s * flipperFlapAmp_;

	// ちょい横揺れ（左右の水かき感）
	float sway = std::sinf(flipperAnimT_ * (w * 0.55f) + 1.2f) * flipperYawSwayAmp_;

	Vector3 r = flipperBaseRot_;

	// どの軸で曲げるかはモデル向き次第
	// まずは X をフラップ、Y を揺れにしてみる（合わなければ X<->Z を入れ替え）
	r.x += flap;
	r.y += sway;

	flipper_->SetRotate(r);
}

void Player::UpdateFloatBob_(float dt) {
	if (!enableFloatBob_) { return; }
	if (!object_) { return; }

	floatT_ += dt;

	// -1..+1 のサイン波
	float w = 2.0f * MyMath::GetPI() * floatHz_;
	float s = std::sinf(floatT_ * w);

	// いまの座標に「見た目だけ」足す（Yだけ）
	Vector3 pos = object_->GetTranslate();
	pos.y += s * floatAmp_;
	object_->SetTranslate(pos);
}

void Player::StartDodge() {
	if (!object_) return;
	if (isDodging_) return;

	auto* in = TKM::Input::GetInstance();

	//========================================
	// ゲームパッド方向入力（左スティック）
	//========================================
	float rx = static_cast<float>(in->GetLeftStickX());
	float ry = static_cast<float>(in->GetLeftStickY());

	const float dz = 6000.0f;
	if (std::fabs(rx) < dz) { rx = 0.0f; }
	if (std::fabs(ry) < dz) { ry = 0.0f; }

	const float norm = 32767.0f;
	rx /= norm;
	ry /= norm;

	//========================================
	// キーボード方向入力
	// WASD と 方向キー 両対応
	//========================================
	float keyX = 0.0f;
	float keyY = 0.0f;

	if (in->PushKey(DIK_A) || in->PushKey(DIK_LEFT)) { keyX -= 1.0f; }
	if (in->PushKey(DIK_D) || in->PushKey(DIK_RIGHT)) { keyX += 1.0f; }
	if (in->PushKey(DIK_W) || in->PushKey(DIK_UP)) { keyY += 1.0f; }
	if (in->PushKey(DIK_S) || in->PushKey(DIK_DOWN)) { keyY -= 1.0f; }

	// 斜めをちゃんと扱うため正規化
	if (std::fabs(keyX) > 0.0001f || std::fabs(keyY) > 0.0001f) {
		float keyLen = std::sqrt(keyX * keyX + keyY * keyY);
		if (keyLen > 0.0001f) {
			keyX /= keyLen;
			keyY /= keyLen;
		}
	}

	//========================================
	// パッド + キーボードを合成
	// キーボード入力があるならそっちを優先
	//========================================
	float moveX = rx;
	float moveY = ry;

	if (std::fabs(keyX) > 0.0001f || std::fabs(keyY) > 0.0001f) {
		moveX = keyX;
		moveY = keyY;
	}

	// カメラRight/Up基準でワールド方向へ
	Vector3 camRight = { 1,0,0 };
	Vector3 camUp = { 0,1,0 };
	if (camera_) {
		const auto& W = camera_->GetWorldMatrix();
		camRight = MyMath::Normalize({ W.m[0][0], W.m[0][1], W.m[0][2] });
		camUp = MyMath::Normalize({ W.m[1][0], W.m[1][1], W.m[1][2] });
	}

	Vector3 dir = camRight * moveX + camUp * moveY;
	dir.z = 0.0f;

	if (MyMath::Length(dir) < 0.001f) {
		return; // 方向入力なしなら回避しない
	}
	dir = MyMath::Normalize(dir);

	isDodging_ = true;
	dodgeT_ = 0.0f;
	dodgeStartPos_ = object_->GetTranslate();
	dodgeDir_ = dir;

	// 回避の音
	TKM::AudioManager::GetInstance()->PlaySound("avoid", 0.1f);

	dodgeBaseRot_ = object_->GetRotate();
	{
		const float ax = std::fabs(dodgeDir_.x);
		const float ay = std::fabs(dodgeDir_.y);

		const bool horizontal = (ax >= ay);

		if (horizontal) {
			// 左右回避：Z回転
			dodgeSpinWRoll_ = 1.0f;
			dodgeSpinWPitch_ = 0.0f;

			dodgeSpinRollSign_ = (dodgeDir_.x >= 0.0f) ? -1.0f : +1.0f;
			dodgeSpinPitchSign_ = +1.0f;
		} else {
			// 上下回避：X回転
			dodgeSpinWRoll_ = 0.0f;
			dodgeSpinWPitch_ = 1.0f;

			dodgeSpinPitchSign_ = (dodgeDir_.y >= 0.0f) ? +1.0f : -1.0f;
			dodgeSpinRollSign_ = +1.0f;
		}
	}
}

void Player::HandleDodge(float dt) {
	auto* in = TKM::Input::GetInstance();

	// Xボタン または Jキーを押した瞬間に開始
	if (!isDodging_ && (in->PushButton(XINPUT_GAMEPAD_X) || in->TriggerKey(DIK_J))) {
		StartDodge();
	}

	if (!isDodging_) return;

	dodgeT_ += dt;

	// 進行（移動/回転）
	float uMove = dodgeT_ / std::max(0.001f, dodgeDuration_);
	if (uMove > 1.0f) uMove = 1.0f;

	float uSpin = dodgeT_ / std::max(0.001f, dodgeSpinDuration_);
	if (uSpin > 1.0f) uSpin = 1.0f;

	// EaseInOutSine（移動も回転も丁寧になる）
	float eMove = 0.5f - 0.5f * std::cos(MyMath::GetPI() * uMove);
	float eSpin = 0.5f - 0.5f * std::cos(MyMath::GetPI() * uSpin);

	// 位置：毎フレーム更新（これで「移動し終わってから回転」にならない）
	{
		Vector3 pos = dodgeStartPos_ + dodgeDir_ * (dodgeDistance_ * eMove);
		pos.x = std::clamp(pos.x, moveMin_.x, moveMax_.x);
		pos.y = std::clamp(pos.y, moveMin_.y, moveMax_.y);
		pos.z = 0.0f;
		object_->SetTranslate(pos);
	}

	// 回転：毎フレーム更新（丁寧に1回転）
	{
		Vector3 rot = object_->GetRotate(); // Z回転だけ上書き
		{ // まずは回転の進み具合をイーズする
			// uSpin をイーズして、さらに回転の進み具合を 0..2回転くらいの範囲で調整
			float spin = (MyMath::GetPI() * 2.0f) * dodgeSpinTurns_ * eSpin;
			// ここで回転の軸・量を決める（StartDodgeで決めたやつを使う）
			Vector3 rot = object_->GetRotate();
			// 上下：X回転（前転/後転）
			rot.x = dodgeBaseRot_.x + dodgeSpinPitchSign_ * spin * dodgeSpinWPitch_;
			// 左右：Z回転（ロール）
			rot.z = bankAngle_ + dodgeSpinRollSign_ * spin * dodgeSpinWRoll_;
			// 反映
			object_->SetRotate(rot);
		}
	}

	// 終了：移動と回転の両方が終わってから
	if (uMove >= 1.0f && uSpin >= 1.0f) {
		isDodging_ = false; // フラグを戻す
		// 最終的な回転のリセット
		Vector3 r = object_->GetRotate(); // 現在の回転をベースに
		r.x = dodgeBaseRot_.x; // ピッチ戻す
		r.z = bankAngle_;      // バンクに復帰
		object_->SetRotate(r); // 反映
	}
}