#include "Player.h"
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
		"jetSmoke", "./resources/circle.png", TKM::ParticleManager::ParticleType::NORMAL); // ジェット煙
	TKM::ParticleManager::GetInstance()->CreateParticleGroup(
		"damageSpark", "./resources/circle2.png", TKM::ParticleManager::ParticleType::NORMAL); // 故障スパーク（バチバチ）
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_rb", "./resources/circle2.png", TKM::ParticleManager::ParticleType::NORMAL); // 弾の軌跡
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lb", "./resources/circle2.png", TKM::ParticleManager::ParticleType::NORMAL); // 弾の軌跡
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_rt", "./resources/circle2.png", TKM::ParticleManager::ParticleType::NORMAL); // 弾の軌跡
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lt", "./resources/circle2.png", TKM::ParticleManager::ParticleType::NORMAL); // 弾の軌跡

	if (enableJetSmoke_) { // ジェット煙初期化
		Vector3 jetPos = object_->GetTranslate();
		jetPos.z -= kJetSmokeOffsetZ_;           // 機体のケツあたり
		jetEmitter_.Initialize("jetSmoke", jetPos);
	}

	rbAmmo_ = kRbAmmoMax_;// ロケット弾初期弾数
	rbEmptyTimer_ = 0.0f; // ロケット弾空タイマー初期化
	rbRefilling_ = false; // ロケット弾リフィル中フラグ初期化
	rbRefillValue_ = float(rbAmmo_); // ロケット弾リフィル値初期化
}

void Player::Update(float dt) {
	UpdateRumble(dt); // コントローラー振動更新

	if (!controlEnabled_) {
		HandleFollowCamera(); // カメラ演出は動かす
		return;
	}

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

	HandleGamePadMove(); // ゲームパッドのスティック入力で移動
	HandleFollowCamera(); // カメラの追従処理
	RemoveEnemyIfDead(); // 敵が死んでたら参照をクリア
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

	HandleShooting(); // 先にプレイヤーの操作より下に置くと自然

	for (auto it = bullets_.begin(); it != bullets_.end(); ) { // 弾更新と削除
		(*it)->Update();
		if ((*it)->IsDead()) { // 弾が死んでたら削除
			it = bullets_.erase(it);
		} else { // 生存してたら次へ
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

	TKM::ParticleManager::GetInstance()->Update(dt); // パーティクルマネージャー更新
	object_->Update(); // プレイヤー本体更新
	flipper_->Update(); // ヒレ更新
}

void Player::ImGuiDebug() {
#ifdef USE_IMGUI
	if (!object_) return;

	Vector3 pos = object_->GetTranslate();
	Vector3 rot = object_->GetRotate();
	Vector3 scale = object_->GetScale();

	//---------------- プレイヤー本体 ----------------
	ImGui::Begin("プレイヤー");


	ImGui::Text("直前に当たった攻撃ID: %d", lastHitAttackId_);
	ImGui::Text("同一攻撃ダメージ無効時間: %.2f", sameAttackLockT_);

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
	ImGui::Text("RT 一撃必殺: %s", canUseSpecial_ ? "READY" : "NOT READY"); // 一撃必殺の使用可能状態を表示
	ImGui::Checkbox("RT 無制限", &debugUnlimitedSpecial_);
	ImGui::Separator(); // 区切り線
	ImGui::Text("HP: %d", hp_);// 1
	ImGui::SameLine();// 1 と 2 を同じ行に配置
	if (ImGui::Button("HPリセット")) { hp_ = 5; } // 2
	ImGui::SeparatorText("カメラシェイク");
	ImGui::SliderFloat("強度のベース", &shakeBaseStrength_, 0.0f, 5.0f); // ベースとなるカメラシェイク強度
	ImGui::SliderFloat("ズーム強調", &shakeZoomBoost_, 0.0f, 15.0f); // ズーム時の追加倍率
	ImGui::Text("現在の増幅量 : %.2f", shakeBaseStrength_ + (1.0f - camZoom_) * shakeZoomBoost_); // 現在の倍率を表示
	ImGui::End();
	//---------------- レティクル ----------------
	ImGui::Begin("レティクル");
	if (reticle_) {
		ImGui::Separator();
		reticle_->ImGuiDebug();
	}
	ImGui::End();
	//---------------- プレイヤー弾ステータス ----------------
	ImGui::Begin("P弾ステータス");
	ImGui::SliderFloat("弾速度(RB,RT,LB)", &normalBulletSpeed_, 0.1f, 15.0); // RB,RT,LBの弾速度調整

	ImGui::Separator();

	float rate = float(rbAmmo_) / float(kRbAmmoMax_);
	char label[64];
	std::snprintf(
		label,
		sizeof(label),
		"RB弾数 %d / %d",
		rbAmmo_,
		kRbAmmoMax_
	);
	ImGui::ProgressBar(rate, ImVec2(260.0f, 18.0f), label);

	if (rbRefilling_) {
		ImGui::Text("RB回復中...");
	} else if (rbAmmo_ <= 0) {
		ImGui::Text("RB回復まで %.2f 秒", std::max(0.0f, kRbEmptyWaitSec_ - rbEmptyTimer_));
	} else {
		ImGui::Text("RBアイドル回復まで %.2f 秒", std::max(0.0f, kRbEmptyWaitSec_ - rbNoFireTimer_));
	}

	ImGui::End();
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

	// だいぶ引きたいのでかなり小さめにする
	// camZoom_ と掛け算される前提で、
	// 0.35f くらいだと「約 1 / 0.35 ≒ 2.85 倍」引きになるイメージ
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

	//for (auto& bullet : bullets_) {
	//	bullet->Draw(dxCommon); // 弾描画
	//}
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

void Player::StartCameraShake(int frameCount) {
	cameraShakeFrame_ = frameCount; // シェイクフレーム数セット
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

	RBShoot(); // RB弾処理
	RTShoot(); // RT弾処理
	LBShoot(); // LB弾処理
	LTShoot(); // LT弾処理
}

void Player::RBShoot() {
	TKM::Input* input = TKM::Input::GetInstance();

	// ▼ RB：通常弾
	if (!input->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
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

	bullet->SetVelocity(dir * normalBulletSpeed_);
	bullet->SetCamera(camera_);
	bullet->SetPlayer(this);
	bullet->SetTrailGroup("trail_rb");
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

	// ▼ LB：ホーミング弾（元LT）
	// ※「1押し1発」のため、ltHeld_をそのまま流用（名前は気にしなくてOK）
	if (input->TriggerButton(XINPUT_GAMEPAD_LEFT_SHOULDER) && !ltHeld_) {
		auto bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize(common_, dxCommon_);

		// 発射位置＝プレイヤー位置
		Vector3 p0 = object_->GetTranslate();
		bullet->SetPosition(p0);
		bullet->SetEnemy(enemy_);
		bullet->SetHoming(true, kHomingBulletSpeed_); // ベジェ終了後に効く追尾速度
		bullet->SetCamera(camera_);
		bullet->SetPlayer(this);
		bullet->SetTrailGroup("trail_lt"); // 見た目もLBに寄せるなら "trail_lb" にしてOK

		// ここでラジアルブラー発火
		if (radialBlur_) {
			radialBlur_->BulrStartShock(2.0f, 0.35f); // 強さ、長さ
		}

		bullet->SetCore(core_);

		// --- 敵方向基準（いなければ前方） ---
		Vector3 toEnemyDir = { 0,0,1 };
		float   distToEnemy = 12.0f;
		if (enemy_ && !enemy_->IsDead()) {
			Vector3 v = enemy_->GetWorldPosition() - p0;
			distToEnemy = std::max(4.0f, MyMath::Length(v));
			toEnemyDir = (distToEnemy > 0.01f) ? MyMath::Normalize(v) : Vector3{ 0,0,1 };
		}

		// 右方向（Y軸回り 90°回転）
		Vector3 right = { toEnemyDir.z, 0.0f, -toEnemyDir.x };
		float rl = MyMath::Length(right);
		right = (rl > 0.001f) ? right * (1.0f / rl) : Vector3{ 1,0,0 };

		// 画面右側の敵なら右回り、左なら左回り
		int side = +1;
		if (enemy_ && !enemy_->IsDead()) {
			Vector3 v = enemy_->GetWorldPosition() - p0;
			float lateral = MyMath::DotOnXZ(right, MyMath::Normalize(Vector3{ v.x,0,v.z }));
			side = (lateral >= 0.0f) ? +1 : -1;
		}

		// --- 大きな弧のパラメータ（距離で自動スケール） ---
		float reach = std::clamp(distToEnemy * 1.10f, 18.0f, 48.0f);
		float sweep = std::clamp(distToEnemy * 1.00f, 18.0f, 40.0f);
		float lift = std::clamp(distToEnemy * 0.60f, 8.0f, 22.0f);
		float bezTime = std::clamp(distToEnemy * 0.16f, 1.2f, 3.5f);

		Vector3 enemyPos = (enemy_ && !enemy_->IsDead())
			? enemy_->GetWorldPosition()
			: p0 + toEnemyDir * reach;

		Vector3 p3 = enemyPos;
		Vector3 p1 = p0 + right * (side * sweep)
			+ Vector3{ 0.0f, lift * 0.7f, 0.0f }
		+ toEnemyDir * (reach * 0.25f);
		Vector3 p2 = p3 - right * (side * sweep * 0.85f)
			+ Vector3{ 0.0f, lift, 0.0f };

		Vector3 vAfter = toEnemyDir * 0.40f; // 前方速度
		bullet->StartSpawnBezier(p0, p1, p2, p3, bezTime, vAfter);
		bullet->SetHomingDelay(0.12f);

		bullets_.push_back(std::move(bullet));

		ZoomCamera();
		StartCameraShake(10);

		// 発射の瞬間だけ軽く振動
		StartRumble(0.12f, 42000, 42000);
	}

	// 押しっぱなし防止ラッチ（名前ltHeld_のまま流用）
	ltHeld_ = input->TriggerButton(XINPUT_GAMEPAD_LEFT_SHOULDER);
}

void Player::LTShoot() {
	TKM::Input* input = TKM::Input::GetInstance();

	// ▼ LT：全敵必中弾（元LB）
	if ((input->GetLeftTrigger() > kTriggerThreshold) && allEnemies_) {
		for (auto& enemy : *allEnemies_) {
			if (enemy->IsDead()) continue;

			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize(common_, dxCommon_);

			Vector3 startPos = object_->GetTranslate();
			Vector3 enemyPos = enemy->GetWorldPosition();
			Vector3 dir = MyMath::Normalize(enemyPos - startPos);

			bullet->SetPosition(startPos);
			bullet->SetVelocity(dir * normalBulletSpeed_);
			bullet->SetCamera(camera_);
			bullet->SetEnemy(enemy.get());
			bullet->SetPlayer(this);

			bullet->SetCore(core_);

			// LT側に移したので trail も合わせたいなら "trail_lt" にしてOK
			bullet->SetTrailGroup("trail_lb");

			bullets_.push_back(std::move(bullet));
		}
	}
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

		cameraShakeOffset_.x = ((rand() % 100 - 50) / 500.0f) * shakeGain;
		cameraShakeOffset_.y = ((rand() % 100 - 50) / 500.0f) * shakeGain;
		cameraShakeOffset_.z = ((rand() % 100 - 50) / 500.0f) * shakeGain;

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
			// ※ここでResetすると戻りが始まらず伸び続ける原因になる
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
	// 既に鳴ってる場合は「強い方」「長い方」を優先（重なっても破綻しにくい）
	rumbleT_ = std::max(rumbleT_, sec);
	rumbleLeft_ = std::max(rumbleLeft_, leftMotor);
	rumbleRight_ = std::max(rumbleRight_, rightMotor);

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

void Player::StartDodge() {
	if (!object_) return;
	if (isDodging_) return;

	auto* in = TKM::Input::GetInstance();

	// スティック方向（左スティック）
	float rx = static_cast<float>(in->GetLeftStickX());
	float ry = static_cast<float>(in->GetLeftStickY());

	// デッドゾーン（Reticleと同じノリ）
	const float dz = 6000.0f;
	if (std::fabs(rx) < dz) rx = 0.0f;
	if (std::fabs(ry) < dz) ry = 0.0f;

	const float norm = 32767.0f;
	rx /= norm;
	ry /= norm;

	// カメラRight/Up基準でワールド方向へ
	Vector3 camRight = { 1,0,0 };
	Vector3 camUp = { 0,1,0 };
	if (camera_) {
		const auto& W = camera_->GetWorldMatrix();
		camRight = MyMath::Normalize({ W.m[0][0], W.m[0][1], W.m[0][2] });
		camUp = MyMath::Normalize({ W.m[1][0], W.m[1][1], W.m[1][2] });
	}

	Vector3 dir = camRight * rx + camUp * ry;
	dir.z = 0.0f;

	if (MyMath::Length(dir) < 0.001f) {
		return; // 方向入力なしなら回避しない（好みで前方向にしてもOK）
	}
	dir = MyMath::Normalize(dir);

	isDodging_ = true;
	dodgeT_ = 0.0f;
	dodgeStartPos_ = object_->GetTranslate();
	dodgeDir_ = dir; // 入力方向
}

void Player::HandleDodge(float dt) {
	auto* in = TKM::Input::GetInstance();

	// X押した瞬間に開始
	if (!isDodging_ && in->PushButton(XINPUT_GAMEPAD_X)) {
		StartDodge();
	}

	if (!isDodging_) return;

	dodgeT_ += dt;

	// 進行（移動/回転）
	// ※もし Player.h が dodgeMoveDuration_ になってるなら、ここだけ名前を合わせてね
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
		Vector3 rot = object_->GetRotate();
		rot.z = bankAngle_ + (MyMath::GetPI() * 2.0f) * dodgeSpinTurns_ * eSpin;
		object_->SetRotate(rot);
	}

	// 終了：移動と回転の両方が終わってから
	if (uMove >= 1.0f && uSpin >= 1.0f) {
		isDodging_ = false;

		// 回転を戻して通常のバンクに復帰
		Vector3 r = object_->GetRotate();
		r.z = bankAngle_;
		object_->SetRotate(r);
	}
}