#include "Player.h"
#include "Enemy.h"
#include <ParticleManager.h>
#include "AABB.h"
#include <limits>
#include "RadialBlurEffect.h"

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
	reticle_->GetCenterWorldPos(); // 中心位置取得用

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
	// RB弾
	rbAmmo_ = kRbAmmoMax_;// ロケット弾初期弾数
	rbEmptyTimer_ = 0.0f; // ロケット弾空タイマー初期化
	rbRefilling_ = false; // ロケット弾リフィル中フラグ初期化
	rbRefillValue_ = float(rbAmmo_); // ロケット弾リフィル値初期化
	// LB弾
	lbAmmo_ = kLbAmmoMax_; // LB弾初期弾数
	lbNoFireTimer_ = 0.0f; // LB弾発射不可タイマー初期化

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

	HandleFollowCamera(); // カメラの追従処理
	RemoveEnemyIfDead(); // 敵が死んでたら参照をクリア
	HandleFollowCamera(); // カメラの追従処理
	RemoveEnemyIfDead(); // 敵が死んでたら参照をクリア

	if (controlEnabled_) {
		HandleGamePadMove(); // ゲームパッドのスティック入力で移動
		HandleDodge(dt); // 回避処理

		// RTホールド中はターゲットをロック表示（切り替わり時は前の敵を解除）
		{
			TKM::Input* input = TKM::Input::GetInstance();
			Enemy* cur = (enemy_ && !enemy_->IsDead()) ? enemy_ : nullptr;

			bool hold = (input->GetRightTrigger() > kTriggerThreshold) && (canUseSpecial_ || debugUnlimitedSpecial_);

			// ターゲットが切り替わったら前のロックを解除
			if (lastLockedEnemy_ && lastLockedEnemy_ != cur) {
				lastLockedEnemy_->SetLocked(false);
			}

			if (cur && hold) { // ロック中
				cur->SetLocked(true);
				lastLockedEnemy_ = cur;
			} else { // ロック解除
				if (cur) cur->SetLocked(false);
				lastLockedEnemy_ = nullptr;
			}
		}

		if (shootingEnabled_) { // 射撃処理
			HandleShooting(); // 先にプレイヤーの操作より下に置くと自然
		}
	} else {
		// 操作不能中はロック表示を解除しておく
		if (lastLockedEnemy_) {
			lastLockedEnemy_->SetLocked(false);
			lastLockedEnemy_ = nullptr;
		}
	}

	for (auto it = bullets_.begin(); it != bullets_.end(); ) { // 弾更新と削除
		(*it)->Update();
		if ((*it)->IsDead()) { // 弾が死んでたら削除
			it = bullets_.erase(it);
		} else { // 生存してたら次へ
			++it;
		}
	}
	for (auto it = homingBullets_.begin(); it != homingBullets_.end(); ) {
		(*it)->Update();
		if ((*it)->IsDead()) {
			it = homingBullets_.erase(it);
		} else {
			++it;
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

	UpdateFlipperAnim_(dt); // ヒレのアニメーション更新
	UpdateFloatBob_(dt); // 浮遊のアニメーション更新

	// TrailRibbonRendererの更新
	TKM::TrailRibbonRenderer::GetInstance()->Update(dt);

	TKM::ParticleManager::GetInstance()->Update(dt); // パーティクルマネージャー更新
	object_->Update(); // プレイヤー本体更新
	flipper_->Update(); // ヒレ更新
}

void Player::DrawTrails(TKM::DirectXCommon* dxCommon) {
	for (auto& bullet : bullets_) {
		if (!bullet) { continue; }
		bullet->DrawTrail(dxCommon); // 弾のトレイル描画
	}
	for (auto& bullet : homingBullets_) {
		if (!bullet) { continue; }
		bullet->DrawTrail(dxCommon);
	}
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
	if (enemy_ && enemy_->IsDead()) { // 敵が死んでたら参照をクリア
		enemy_ = nullptr;
	}
}

void Player::OnEnemyDestroyed(Enemy* e) {
	if (enemy_ == e) {
		enemy_ = nullptr;
	}
	for (auto& b : bullets_) { // 弾が追従している敵も解除する
		if (!b) continue; // 安全確認
		if (b->GetEnemy() == e) { b->SetEnemy(nullptr); } // 敵解除
	}
	for (auto& b : homingBullets_) {
		if (!b) continue;
		if (b->GetEnemy() == e) { b->SetEnemy(nullptr); }
	}
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
	// ---- HPが0になったら「故障スパーク → 撃墜」二段階 ----
	if (hp_ <= 0) {

		// まだ死亡演出に入ってなければ、故障スパークから開始
		if (deathPhase_ == DeathPhase::None) {
			deathPhase_ = DeathPhase::FaultSparks;
			isDead_ = true;        // 以後の通常操作を停止
			faultTimer_ = 0.0f;
			faultFrameCounter_ = 0;
			flyInit_ = false;
		}

		// === フェーズ1：故障スパーク（機体の周囲に複数スポット）===
		if (deathPhase_ == DeathPhase::FaultSparks) {
			faultTimer_ += dt;
			++faultFrameCounter_;

			// 調整用ローカル（必要なら後でImGui化）
			const float kSpreadRadius = 2.0f; // 機体中心からどれくらい外側まで
			const int   kSpotCount = 6;    // 同時に噴くスポット数

			// 一定フレーム毎にスパーク発生 & カメラシェイク
			if ((faultFrameCounter_ % std::max(1, faultTickInterval_)) == 0) {

				// 1スポットあたりの粒数（全体の発生数を均等割）
				const int perSpot = std::max(1, faultBurstPerTick_ / std::max(1, kSpotCount));

				// 簡易乱数ユーティリティ
				auto frand = [](float a, float b) {
					return a + (b - a) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
					};

				Vector3 base = object_->GetTranslate(); // 機体中心

				for (int i = 0; i < kSpotCount; ++i) {
					// ランダムな方向ベクトル（球面上）＋半径ランダム
					Vector3 dir = { frand(-1.f, 1.f), frand(-1.f, 1.f), frand(-1.f, 1.f) };
					if (MyMath::Length(dir) < 0.001f) dir = { 0,0,1 };
					dir = MyMath::Normalize(dir);

					float r = kSpreadRadius * frand(0.25f, 1.0f); // 内側～外側へ散らす
					Vector3 p = base + dir * r;                   // スポット位置

					// Emitの第2引数は非const参照なのでローカル変数を渡す
					TKM::ParticleManager::GetInstance()->Emit("damageSpark", p, perSpot);
				}

				// 激しさ”演出：軽めシェイクを継続
				StartCameraShake(20);
			}

			// ほんの少しだけ姿勢が乱れる感じ（お好み）
			Vector3 rot = object_->GetRotate();
			rot.z += 0.02f; // バンク方向に微揺れ
			object_->SetRotate(rot);

			object_->Update(); // 故障中も更新

			// 規定時間でフェーズ2へ
			if (faultTimer_ >= faultDuration_) {
				deathPhase_ = DeathPhase::FlyAway;
			}
			return; // 故障中は他処理停止
		}

		// === フェーズ2：緩やかな吹き飛び（穏やか版） ===
		if (deathPhase_ == DeathPhase::FlyAway) {
			if (!flyInit_) {
				flyInit_ = true;

				// 横ブレ・上向き控えめ、+Zへ
				float side = (rand() % 200 - 100) / 100.0f;   // -1..1
				float up = 0.15f + (rand() % 100) / 100.0f * 0.20f; // 0.15..0.35
				Vector3 dir = MyMath::Normalize(Vector3{ side * 0.25f, up, 1.6f });

				deathVelocity_ = dir * 0.55f;

				deathRotateSpeed_.x = 0.03f + (rand() % 30) / 100.0f;
				deathRotateSpeed_.y = 0.04f + (rand() % 30) / 100.0f;
				deathRotateSpeed_.z = 0.05f + (rand() % 30) / 100.0f;

				// スパーク直後は余韻の弱シェイク
				StartCameraShake(60);

				// パーティクル少なめの爆散
				Vector3 pos = object_->GetTranslate();
				TKM::ParticleManager::GetInstance()->Emit("uv", pos, 20);

				deathTimer_ = 0.0f;
			}

			deathTimer_ += dt; // 経過時間更新

			// ゆっくり減速しつつ、わずかに浮き
			deathVelocity_ *= 0.992f;
			deathVelocity_.y += 0.02f * dt;

			// 速度上限
			const float maxSpeed = 1.2f;
			float sp = MyMath::Length(deathVelocity_);
			if (sp > maxSpeed) {
				deathVelocity_ = MyMath::Normalize(deathVelocity_) * maxSpeed;
			}

			// 位置
			Vector3 pos = object_->GetTranslate();
			pos += deathVelocity_;
			object_->SetTranslate(pos);

			// 緩いスピン
			float t = std::clamp(deathTimer_ / deathDuration_, 0.0f, 1.0f);
			Vector3 rot = object_->GetRotate();
			float spinScale = 1.0f + 0.3f * (1.0f - std::cosf(t * MyMath::GetPI()));
			rot.x += deathRotateSpeed_.x * spinScale;
			rot.y += deathRotateSpeed_.y * spinScale;
			rot.z += deathRotateSpeed_.z * spinScale;
			object_->SetRotate(rot);

			// ほんの少し縮小
			Vector3 sc = object_->GetScale();
			sc *= 0.999f;
			object_->SetScale(sc);

			// まばらなチリ
			if (static_cast<int>(deathTimer_ * 60.0f) % 10 == 0) {
				Vector3 ep = object_->GetTranslate();
				TKM::ParticleManager::GetInstance()->Emit("uv", ep, 2);
			}

			object_->Update();
			return; // 撃墜中は他処理停止
		}
	}
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

	for (auto& bullet : bullets_) {
		bullet->Draw(dxCommon); // 弾の描画はしない(今後も予定なし)
	}

	/*for (auto& bullet : homingBullets_) {
		bullet->Draw(dxCommon);
	}*/
}

void Player::SetCamera(TKM::Camera* camera) {
	this->camera_ = camera;
	if (object_) { object_->SetCamera(camera); }
	if (reticle_) { reticle_->SetCamera(camera); }
	if (flipper_) { flipper_->SetCamera(camera); }
}

void Player::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos); // 位置設定
}

void Player::SetParentScene(TKM::BaseScene* scene) {
	parentScene_ = scene; // 親シーン設定
}

void Player::SetEnemy(Enemy* enemy) {
	enemy_ = enemy; // 敵1をセット（ロックオン対象）
}

void Player::SetAllEnemies(std::vector<std::unique_ptr<Enemy>>* enemies) {
	allEnemies_ = enemies; // 敵全体の参照をセット（弾の追従用）
}

void Player::SetControlEnabled(bool enabled) {
	controlEnabled_ = enabled; // プレイヤー操作の有効 / 無効を切り替えるフラグ
}

void Player::SetReticleVisible(bool visible) {
	reticleVisible_ = visible; // レティクルの表示 / 非表示を切り替えるフラグ
}

void Player::SetMidBossCore(MidBossCore* core) {
	core_ = core; // レティクルのターゲットにコアを追加
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

void Player::OnMidBossCoreDestroyed(MidBossCore* core) {
	if (!core) {
		return;
	}

	for (auto& b : bullets_) {
		if (!b) { continue; }
		// PlayerBullet に GetCore() が無いなら、SetCore(nullptr) を無条件で入れてもいい
		b->SetCore(nullptr);
	}

	for (auto& b : homingBullets_) {
		if (!b) { continue; }
		b->SetCore(nullptr);
	}
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
	shootingEnabled_ = enabled; // シューティングの有効 / 無効を切り替えるフラグ
	if (!enabled) { // 無効にするなら、関連する状態もリセットしておく
		// 押しっぱなし判定が残らないようにする（復帰時の暴発防止）
		rtHeld_ = false; // LB/RBは今のところ特に持続処理がないのでリセット不要
		ltHeld_ = false; // LB/RBは今のところ特に持続処理がないのでリセット不要
	}
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

	if (movingThisFrame) {
		float targetBank = -vx * 0.8f;
		float k = 0.25f;
		float d = 0.45f;
		bankVel_ += (targetBank - bankAngle_) * k - bankVel_ * d;
		bankAngle_ += bankVel_;
	} else {
		float resetK = 0.25f;
		float resetD = 0.5f;
		bankVel_ += (0.0f - bankAngle_) * resetK - bankVel_ * resetD;
		bankAngle_ += bankVel_;
		if (std::fabs(bankAngle_) < 0.001f && std::fabs(bankVel_) < 0.001f) {
			bankAngle_ = 0.0f;
			bankVel_ = 0.0f;
		}
	}

	object_->SetTranslate(newPos);
	Vector3 rot = object_->GetRotate();
	rot.z = bankAngle_;
	object_->SetRotate(rot);
}
void Player::HandleFollowCamera() {
	const float dt = 1.0f / 60.0f;
	// FPV分岐はしない（ズームは追従側で処理）
	UpdateCameraFollowThirdPerson(dt);
}

void Player::HandleShooting() {
	//====================
	// RB弾 リチャージ更新（0回復 + アイドル回復）
	//====================
	{
		// 「撃ってない時間」を進める（回復中は進めなくてOK）
		if (!rbRefilling_) {
			rbNoFireTimer_ += dt;
		}

		// --- 回復開始条件 ---
		// A) 弾が0で、一定時間経過
		// B) 弾が残っていても、一定時間撃っていない（アイドル）
		const bool empty = (rbAmmo_ <= 0);
		const bool idleReady = (!empty && rbNoFireTimer_ >= kRbEmptyWaitSec_);
		const bool emptyReady = (empty && (rbEmptyTimer_ >= kRbEmptyWaitSec_));

		// 弾が0なら空タイマーを進める（0じゃないなら0に戻す）
		if (!rbRefilling_) {
			if (empty) {
				rbEmptyTimer_ += dt;
			} else {
				rbEmptyTimer_ = 0.0f;
			}
		}

		// 回復開始
		if (!rbRefilling_ && (idleReady || emptyReady)) {
			rbRefilling_ = true;

			// 回復開始時の初期値：
			// 0回復なら 0 から
			// アイドル回復なら 現在弾数から一気に増える
			rbRefillValue_ = empty ? 0.0f : float(rbAmmo_);
		}

		// 回復中：一気に増えて全回復
		if (rbRefilling_) {
			const float speed = float(kRbAmmoMax_) / std::max(0.001f, kRbRefillSec_); // 弾/秒
			rbRefillValue_ += speed * dt;

			rbAmmo_ = std::clamp(int(rbRefillValue_), 0, kRbAmmoMax_);

			if (rbAmmo_ >= kRbAmmoMax_) {
				rbAmmo_ = kRbAmmoMax_;
				rbRefilling_ = false;
				rbEmptyTimer_ = 0.0f;
				rbNoFireTimer_ = 0.0f; // 満タンになったらアイドル判定もリセット
			}
		}
	}
	//====================
	// LB弾 自動満タン回復（一定時間LBを撃ってないとMaxへ）
	//====================
	{
		if (!debugUnlimitedLB_ && lbAmmo_ < kLbAmmoMax_) { // まだ満タンじゃないなら
			lbNoFireTimer_ += dt; // 撃ってない時間を進める

			if (lbNoFireTimer_ >= kLbRefillWaitSec_) { // 一定時間撃ってないなら満タンにする
				lbAmmo_ = kLbAmmoMax_; // 満タンにする
				lbNoFireTimer_ = 0.0f; // 満タンになったらアイドル判定もリセット
			}
		} else {
			// 満タンならタイマーは不要なのでリセット
			lbNoFireTimer_ = 0.0f;
		}
	}

	//====================
	// RB弾 クールダウンタイマー更新（撃ってから一定時間は撃てない）
	// ====================
	if (rbShotCooldownTimer_ > 0.0f) {
		rbShotCooldownTimer_ -= dt;
		if (rbShotCooldownTimer_ < 0.0f) {
			rbShotCooldownTimer_ = 0.0f;
		}
	}

	RBShoot(); // RB弾処理
	RTShoot(); // RT弾処理
	LBShoot(); // LB弾処理
	LTShoot(); // LT弾処理
}

void Player::RBShoot() {
	TKM::Input* input = TKM::Input::GetInstance();

	// ▼ RB：通常弾
	const bool padRB = input->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER);
	const bool keyK = input->PushKey(DIK_K);

	if (!padRB && !keyK) {
		return;
	}
	if (rbShotCooldownTimer_ > 0.0f) {
		return;
	}
	if (rbAmmo_ <= 0 || rbRefilling_) { // 弾切れ中は発射不可
		return;
	}

	auto bullet = std::make_unique<PlayerBullet>();
	bullet->Initialize(common_, dxCommon_);

	// 発射位置＝プレイヤー位置
	Vector3 startPos = object_->GetTranslate();
	bullet->SetPosition(startPos);

	// ---- 向き：Reticle の aimDir を使う ----
	Vector3 dir = { 0, 0, 1 };

	if (reticle_) {
		dir = reticle_->GetAimDirection();
		float len = MyMath::Length(dir);
		if (len <= 0.01f) {
			dir = { 0, 0, 1 };
		}
	}

	// 弾の基本設定
	bullet->SetVelocity(dir * normalBulletSpeed_);
	bullet->SetCamera(camera_);
	bullet->SetPlayer(this);
	bullet->SetUseTrail(false);
	bullet->SetCore(core_);

	// ==============================
	// ターゲット決定
	// ==============================
	Enemy* targetEnemy = nullptr;

	if (allEnemies_) {
		Vector3 rayDir = dir;
		float len = MyMath::Length(rayDir);
		if (len > 0.001f) {
			rayDir = rayDir / len;
		}

		Vector3 rayEnd = startPos + rayDir * 150.0f;
		float closestDist = std::numeric_limits<float>::max();

		for (auto& e : *allEnemies_) {
			if (!e) continue;
			if (e->IsDead() || e->IsDying()) continue;

			Vector3 center = e->GetWorldPosition();
			Vector3 size = e->GetColliderScale();
			AABB box(center, size);

			if (box.IsIntersectSegment(startPos, rayEnd)) {
				float dist = MyMath::Length(center - startPos);
				if (dist < closestDist) {
					closestDist = dist;
					targetEnemy = e.get();
				}
			}
		}
	}

	// ロック中の敵（ボス含む）
	if (!targetEnemy) {
		if (enemy_ && !enemy_->IsDead()) {
			targetEnemy = enemy_;
		}
	}

	bullet->SetEnemy(targetEnemy);
	bullets_.push_back(std::move(bullet));

	// クールダウン開始
	rbShotCooldownTimer_ = kRbShotCooldownSec_;

	//  発射成功したら消費
	rbAmmo_ = std::max(0, rbAmmo_ - 1);
	// 「撃ってない時間」リセット
	rbNoFireTimer_ = 0.0f;

}

void Player::RTShoot() {
	TKM::Input* input = TKM::Input::GetInstance();
	// RT：一撃必殺（最も近い敵に必中弾）
	const bool pressed = (input->GetRightTrigger() > kTriggerThreshold);

	// 押している間：ホールド状態にする（発射はしない）
	if (pressed && (canUseSpecial_ || debugUnlimitedSpecial_) && enemy_ && !enemy_->IsDead()) {
		rtHeld_ = true; // ロックの見た目は Update() 側でON
	}

	// 離した瞬間：発射
	if (!pressed && rtHeld_) {
		if ((canUseSpecial_ || debugUnlimitedSpecial_) && enemy_ && !enemy_->IsDead()) {
			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize(common_, dxCommon_);

			Vector3 startPos = object_->GetTranslate(); // 発射位置
			Vector3 enemyPos = enemy_->GetWorldPosition(); // 敵位置
			Vector3 dir = MyMath::Normalize(enemyPos - startPos); // 方向計算

			// 弾設定
			bullet->SetPosition(startPos); // 弾位置設定
			bullet->SetVelocity(dir * normalBulletSpeed_); // 速度設定
			bullet->SetCamera(camera_); // カメラ設定
			bullet->SetEnemy(enemy_); // 敵設定
			bullet->SetPlayer(this); // プレイヤー設定
			bullet->SetSpecialAttack(true); // 一撃必殺フラグON

			// RT専用の軌跡
			bullet->SetTrailGroup("trail_rt");

			bullet->SetCore(core_);

			bullets_.push_back(std::move(bullet)); // 弾リストに追加

			// 見た目のロックは解除
			enemy_->SetLocked(false);
			if (!debugUnlimitedSpecial_) { // 一撃必殺使用済みにする
				canUseSpecial_ = false;
			}
		}
		rtHeld_ = false; // 次に備えて解除
	}
}

void Player::LBShoot() {
	TKM::Input* input = TKM::Input::GetInstance();

	// ▼ LB：山なりホーミング弾（ロックオンしてる敵に向かう、LB弾は自動で満タン回復する）
	if ((input->TriggerButton(XINPUT_GAMEPAD_LEFT_SHOULDER) || input->TriggerKey(DIK_L)) && !ltHeld_) {

		// デバッグ無限LBモードでないなら、弾数が0のときは発射できない
		if (!debugUnlimitedLB_ && lbAmmo_ <= 0) {
			return;
		}
		// LB弾はロックオンしてる敵に向かう山なりホーミング弾
		auto bullet = std::make_unique<HomingBullet>();
		bullet->Initialize(common_, dxCommon_);

		Vector3 start = object_->GetTranslate(); // 発射位置

		// 終点
		Vector3 end = start + Vector3{ 0.0f, 0.0f, 28.0f };

		// ロック中の敵（ボス含む）を終点にする
		if (enemy_ && !enemy_->IsDead()) {
			end = enemy_->GetWorldPosition();
		}

		// 山なり制御点を作る
		Vector3 flat = end - start; // 開始から終点へのベクトル
		flat.y = 0.0f; // 水平方向のベクトルだけ抜き取る
		float flatLen = MyMath::Length(flat); // 水平距離

		Vector3 forward = { 0.0f, 0.0f, 1.0f }; // デフォルトの前方向

		// 水平距離が十分あるなら、そこから前方向を計算する
		if (flatLen > 0.001f) {
			forward = flat / flatLen;
		}

		float arcHeight = std::clamp(flatLen * 0.25f, 6.0f, 18.0f); // 水平距離に応じた高さ（最小6、最大18）

		// 制御点は、開始から終点へのベクトルの途中に、上方向へのオフセットを加えた位置にする
		Vector3 c1 = start + forward * (flatLen * 0.25f) + Vector3{ 0.0f, arcHeight, 0.0f };
		Vector3 c2 = end - forward * (flatLen * 0.20f) + Vector3{ 0.0f, arcHeight * 0.85f, 0.0f };

		// 弾の基本設定
		bullet->SetPosition(start);
		bullet->SetEnemy(enemy_);
		bullet->SetCamera(camera_);
		bullet->SetPlayer(this);
		bullet->SetCore(core_);
		bullet->StartArc(start, c1, c2, end, 0.4f);

		//if (radialBlur_) {
		//	radialBlur_->BulrStartShock(2.0f, 0.35f); // 強さ = 2.0f、時間 = 0.35秒
		//}

		homingBullets_.push_back(std::move(bullet)); // ホーミング弾リストに追加

		if (!debugUnlimitedLB_) {
			lbAmmo_ = std::max(0, lbAmmo_ - 1); // 発射成功したら消費
		}
		lbNoFireTimer_ = 0.0f; // 「撃ってない時間」リセット

		ZoomCamera(); // LTの一時ズームアウト開始
		//StartCameraShake(10); // 軽いシェイクも同時に開始
		StartRumble(0.12f, 42000, 42000); // 振動も同時に開始（0.12秒、強め）

		ltHeld_ = true;
	}

	// 離した瞬間：ホールド状態解除
	if (!input->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER)) {
		ltHeld_ = false;
	}
}

void Player::LTShoot() {
#ifdef _DEBUG
	TKM::Input* input = TKM::Input::GetInstance();

	// ▼ LT：全敵必中弾
	const bool padLT = (input->GetLeftTrigger() > kTriggerThreshold);
	const bool keyL = input->PushKey(DIK_Z);

	if ((padLT || keyL) && allEnemies_) {

		// デバッグ無限LBモードでないなら、弾数が0のときは発射できない
		for (auto& enemy : *allEnemies_) {
			if (enemy->IsDead()) continue; // 死んでる敵はスキップ

			// LB弾はロックオンしてる敵に向かう山なりホーミング弾
			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize(common_, dxCommon_);

			// 発射位置＝プレイヤー位置
			Vector3 startPos = object_->GetTranslate();
			Vector3 enemyPos = enemy->GetWorldPosition();
			Vector3 dir = MyMath::Normalize(enemyPos - startPos);

			// 弾の基本設定
			bullet->SetPosition(startPos);
			bullet->SetVelocity(dir * normalBulletSpeed_);
			bullet->SetCamera(camera_);
			bullet->SetEnemy(enemy.get());
			bullet->SetPlayer(this);
			bullet->SetCore(core_);
			bullet->SetTrailGroup("trail_lb");
			// LT弾は全敵必中なので、ターゲットは個々の敵に設定する（LB弾はロック中の敵1体だけだった）
			bullets_.push_back(std::move(bullet));
		}
	}
#endif
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