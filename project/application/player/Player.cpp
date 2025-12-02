#include "Player.h"
#include <engine/effect/particle/ParticleManager.h>

void Player::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	common_ = common; // Object3d共通
	dxCommon_ = dxCommon; // DirectX共通

	// 3Dオブジェクト作成
	object_ = std::make_unique<Object3d>();
	object_->Initialize(common_, dxCommon_);
	object_->SetModel("jett.obj");
	object_->SetEnvironment("./resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");

	reticle_ = std::make_unique<Reticle>();
	reticle_->Initialize(common_, dxCommon_, "reticle_big.obj"); // モデル指定可
	if (camera) reticle_->SetCamera(camera);
	// Player から位置とヨー角(radians)を渡す（循環依存を避けるためコールバック）
	reticle_->BindOwner(
		[this]() { return object_->GetTranslate(); },
		[this]() { return object_->GetRotate().y; }
	);
	reticle_->GetCenterWorldPos(); // 中心位置取得用

	// パーティクルグループ作成
	ParticleManager::GetInstance()->CreateParticleGroup(
		"jetSmoke", "./resources/circle.png", ParticleManager::ParticleType::NORMAL); // ジェット煙
	ParticleManager::GetInstance()->CreateParticleGroup(
		"damageSpark", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL); // 故障スパーク（バチバチ）
	ParticleManager::GetInstance()->CreateParticleGroup("trail_rb", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL); // 弾の軌跡
	ParticleManager::GetInstance()->CreateParticleGroup("trail_lb", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL); // 弾の軌跡
	ParticleManager::GetInstance()->CreateParticleGroup("trail_rt", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL); // 弾の軌跡
	ParticleManager::GetInstance()->CreateParticleGroup("trail_lt", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL); // 弾の軌跡

	if (enableJetSmoke_) { // ジェット煙初期化
		Vector3 jetPos = object_->GetTranslate();
		jetPos.z -= kJetSmokeOffsetZ;           // 機体のケツあたり
		jetEmitter_.Initialize("jetSmoke", jetPos);
	}
}

void Player::Update() {
	// クリア演出などで操作禁止中は、通常のUpdateを流さない
	if (!controlEnabled_) {
		return;
	}

	if (reticle_) reticle_->Update(dt);

	HandleGamePadMove(); // ゲームパッドのスティック入力で移動
	HandleFollowCamera(); // カメラの追従処理
	RemoveEnemyIfDead(); // 敵が死んでたら参照をクリア

	// RTホールド中はターゲットをロック表示（切り替わり時は前の敵を解除）
	{
		Input* input = Input::GetInstance();
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

	Death(); // 撃墜処理

	// ---- ジェット煙（HPが0なら停止）----
	if (enableJetSmoke_ && hp_ > 0) {
		Vector3 jetPos = object_->GetTranslate();
		jetPos.z -= kJetSmokeOffsetZ;
		jetEmitter_.SetPosition(jetPos);
		jetEmitter_.Update();
	}

	ParticleManager::GetInstance()->Update(); // パーティクルマネージャー更新
	object_->Update(); // プレイヤー本体更新
}


void Player::ImGuiDebug() {
#ifdef USE_IMGUI
	if (!object_) return;

	Vector3 pos = object_->GetTranslate();
	Vector3 rot = object_->GetRotate();
	Vector3 scale = object_->GetScale();

	//---------------- プレイヤー本体 ----------------
	ImGui::Begin("プレイヤー");
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
	ImGui::Text("RT 一撃必殺: %s", canUseSpecial_ ? "READY" : "NOT READY"); // 一撃必殺の使用可能状態を表示
	ImGui::Checkbox("RT 無制限", &debugUnlimitedSpecial_);
	ImGui::Separator(); // 区切り線
	ImGui::Text("HP: %d", hp_);// 1
	ImGui::SameLine();// 1 と 2 を同じ行に配置
	if (ImGui::Button("HPリセット")) { hp_ = 1; } // 2
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
	ImGui::SliderFloat("弾速度(RB,RT,LB)", &normalBulletSpeed_, 0.1f, 5.0f); // RB,RT,LBの弾速度調整
	ImGui::Separator();
	int idx = 0;
	for (const auto& bullet : bullets_) {   // Player が持ってる bullets_ :contentReference[oaicite:1]{index=1}
		ImGui::Text("Bullet %d : %s",
			idx++,
			bullet->IsHit() ? "Hit" : "Flying");  // PlayerBullet::IsHit() 
	}
	if (idx == 0) {
		ImGui::Text("弾なし");
	}
	ImGui::End();
#endif
}

void Player::RemoveEnemyIfDead()
{
	if (enemy_ && enemy_->IsDead()) { // 敵が死んでたら参照をクリア
		enemy_ = nullptr;
	}
}

void Player::Death()
{
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
					ParticleManager::GetInstance()->Emit("damageSpark", p, perSpot);
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
				ParticleManager::GetInstance()->Emit("uv", pos, 20);

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
				ParticleManager::GetInstance()->Emit("uv", ep, 2);
			}

			object_->Update();
			return; // 撃墜中は他処理停止
		}
	}
}

void Player::UpdateVisualOnly() {
	// クリア演出用：
	// GameScene 側から SetPosition などで座標だけ動かしておいて、
	// ここで行列更新だけ行う
	if (object_) {
		object_->Update();
	}
}

void Player::Draw(DirectXCommon* dxCommon) {
	object_->Draw(dxCommon); // プレイヤー本体描画

	// クリア演出中などで隠したいときはフラグでOFF
	if (reticle_ && reticleVisible_) {
		reticle_->Draw(dxCommon);
	}

	//for (auto& bullet : bullets_) {
	//	bullet->Draw(dxCommon); // 弾描画
	//}
}

void Player::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos); // 位置設定
}

void Player::SetParentScene(BaseScene* scene) {
	parentScene_ = scene; // 親シーン設定
}

void Player::StartCameraShake(int frameCount) {
	cameraShakeFrame_ = frameCount; // シェイクフレーム数セット
}

void Player::HandleGamePadMove() {
	if (!object_) return;

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
	RBShoot(); // RB弾処理
	RTShoot(); // RT弾処理
	LBShoot(); // LB弾処理
	LTShoot(); // LT弾処理
}

void Player::RBShoot() {
	Input* input = Input::GetInstance();
	// ▼ RB：通常弾（レティクルが描いているガイドライン通りに発射）
	if (input->TriggerButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
		auto bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize(common_, dxCommon_);

		// 発射位置＝プレイヤー位置（レティクルもここを起点に線を伸ばしている）
		Vector3 startPos = object_->GetTranslate();
		bullet->SetPosition(startPos); // 弾位置設定

		// ---- 向き：Reticle が計算した「aimDir」をそのまま使う ----
		Vector3 dir = { 0, 0, 1 }; // デフォは前方

		if (reticle_) {
			dir = reticle_->GetAimDirection(); // ★ 新規に追加した関数を使う
			float len = MyMath::Length(dir);
			if (len <= 0.01f) {
				dir = { 0, 0, 1 }; // 念のための保険
			}
		}

		bullet->SetVelocity(dir * normalBulletSpeed_); // 速度設定
		bullet->SetCamera(camera);
		bullet->SetEnemy(enemy_);     // RBは敵ロックなしでOKなら null に
		bullet->SetPlayer(this);      // プレイヤー設定
		bullet->SetTrailGroup("trail_rb");

		bullets_.push_back(std::move(bullet)); // 弾リストに追加
	}
}

void Player::RTShoot() {
	Input* input = Input::GetInstance();
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
			bullet->SetCamera(camera); // カメラ設定
			bullet->SetEnemy(enemy_); // 敵設定
			bullet->SetPlayer(this); // プレイヤー設定
			bullet->SetSpecialAttack(true); // 一撃必殺フラグON

			// RT専用の軌跡
			bullet->SetTrailGroup("trail_rt");

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
	Input* input = Input::GetInstance();
	// ▼ LB：全敵必中弾
	if (input->TriggerButton(XINPUT_GAMEPAD_LEFT_SHOULDER) && allEnemies_) {
		for (auto& enemy : *allEnemies_) { // 全敵ループ
			if (enemy->IsDead()) continue;
			// 敵ごとに弾を生成
			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize(common_, dxCommon_);

			Vector3 startPos = object_->GetTranslate(); // 発射位置
			Vector3 enemyPos = enemy->GetWorldPosition(); // 敵位置
			Vector3 dir = MyMath::Normalize(enemyPos - startPos); // 方向計算

			// 弾設定
			bullet->SetPosition(startPos); // 弾位置設定
			bullet->SetVelocity(dir * normalBulletSpeed_); // 速度設定
			bullet->SetCamera(camera); // カメラ設定
			bullet->SetEnemy(enemy.get()); // 敵設定
			bullet->SetPlayer(this); // プレイヤー設定

			// LB専用の軌跡
			bullet->SetTrailGroup("trail_lb");

			bullets_.push_back(std::move(bullet)); // 弾リストに追加
		}
	}
}

void Player::LTShoot() {
	Input* input = Input::GetInstance();
	if ((input->GetLeftTrigger() > kTriggerThreshold) && !ltHeld_) {
		auto bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize(common_, dxCommon_);

		// 発射位置＝プレイヤー位置
		Vector3 p0 = object_->GetTranslate();
		bullet->SetPosition(p0);
		bullet->SetEnemy(enemy_);
		bullet->SetHoming(true, kHomingBulletSpeed); // ベジェ終了後に効く追尾速度
		bullet->SetCamera(camera);
		bullet->SetPlayer(this);
		bullet->SetTrailGroup("trail_lt");

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
		float reach = std::clamp(distToEnemy * 1.10f, 18.0f, 48.0f); // Z前進量（合流点まで）
		float sweep = std::clamp(distToEnemy * 1.00f, 18.0f, 40.0f); // 横張り（画面外へ）
		float lift = std::clamp(distToEnemy * 0.60f, 8.0f, 22.0f); // 上げ量（上にもはみ出す）
		float bezTime = std::clamp(distToEnemy * 0.16f, 1.2f, 3.5f); // ベジェ飛行時間

		// 制御点：P0(開始) → P1(強く外へ) → P2(外を保ちつつ敵方向へ) → P3(敵手前で合流)
		Vector3 enemyPos = (enemy_ && !enemy_->IsDead())
			? enemy_->GetWorldPosition()
			: p0 + toEnemyDir * reach; // 保険で前方
		// P3 を「敵位置」にする（敵に向かって弧のまま当たる）
		Vector3 p3 = enemyPos;
		// P1, P2 は今まで通りだけど、終点がp3に変わったので弧が自然に敵に吸い込まれる
		Vector3 p1 = p0 + right * (side * sweep)
			+ Vector3{ 0.0f, lift * 0.7f, 0.0f }
		+ toEnemyDir * (reach * 0.25f);
		Vector3 p2 = p3 - right * (side * sweep * 0.85f)
			+ Vector3{ 0.0f, lift, 0.0f };

		// ベジェ後は軽く前へ押し出してからホーミング
		Vector3 vAfter = toEnemyDir * 0.40f; // 前方速度
		// ベジェ弾道開始
		bullet->StartSpawnBezier(p0, p1, p2, p3, bezTime, vAfter);
		// ホーミング設定
		bullet->SetHomingDelay(0.12f); // ベジェ完了から追尾開始までの遅延時間


		bullets_.push_back(std::move(bullet));

		// 見せ場用の軽いズーム＆シェイク
		ZoomCamera();
		StartCameraShake(10);
	}
	ltHeld_ = (input->GetLeftTrigger() > kTriggerThreshold);
}

void Player::UpdateCameraFollowThirdPerson(float dt) {
	if (!camera) return;

	Vector3 playerPos = object_->GetTranslate();
	Vector3 camRot = camera->GetRotate();

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

	float distance = baseDistance / camZoom_; // 距離はズーム係数で調整
	float height = baseHeight; // 高さは据え置き（必要なら *camZoom_ でもOK）

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
	camera->SetTranslate(cameraPos);
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