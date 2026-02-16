#define NOMINMAX
#include "MyMath.h"

#include "PlayerBullet.h"
#include <ParticleManager.h>
#include "AABB.h"
#include "Player.h"
#include "MidBossCore.h"

void PlayerBullet::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	// 3Dオブジェクト作成
	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");
	object_->SetScale({ kDefaultScale_, kDefaultScale_, kDefaultScale_ });

	prevPos_ = object_->GetTranslate(); // 初期座標を保存

	// 既定グループで一旦初期化（あとで SetTrailGroup で上書き可）
	Vector3 start = object_->GetTranslate();
	trailEmitter_.Initialize(trailGroup_, start);
}

void PlayerBullet::Update() {

	Vector3 oldPos = object_->GetTranslate(); // 前フレームの座標保存

	UpdateSpawnBezier(); // 発射の「出方」曲線更新

	// 現在の座標を取得して、速度分だけ進める
	Vector3 pos = object_->GetTranslate();
	pos = pos + velocity_; // 速度分だけ進める
	object_->SetTranslate(pos); // 座標を更新
	trailEmitter_.SetPosition(pos); // パーティクル位置更新
	trailEmitter_.Update(); // 毎フレーム放出

	prevPos_ = oldPos; // 前フレームの座標を保存

	// LTホーミング弾だけ、飛行中にスパークをばら撒く（全部盛りポイント）
	if (trailGroup_ == "trail_lt") {
		TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
		Vector3 emitPos = pos;

		// 軽い光の尾
		pm->Emit("trail_lt_path", emitPos, 1);
		// バチバチ弾けるスパーク
		pm->Emit("enemyHit_spark", emitPos, 1);
	}

	// =========================================
	// 弾AABB vs 相手AABB（線分＋AABB）の共通判定
	// =========================================
	Vector3 bulletPos = object_->GetTranslate();
	Vector3 bulletScale = object_->GetScale();

	auto CheckSweptHitAABB = [&](const Vector3& targetPos, const Vector3& targetSize) -> bool {
		AABB bulletBox(bulletPos, bulletScale);
		AABB targetBox(targetPos, targetSize);

		// 1) 弾の移動線分(prevPos_ → bulletPos)と相手AABBの交差判定
		if (targetBox.IsIntersectSegment(prevPos_, bulletPos)) {
			return true;
		}
		// 2) 念のため AABB vs AABB も見る（弾が中からスタートした場合など）
		if (bulletBox.IsCollidingWithAABB(targetBox)) {
			return true;
		}
		return false;
		};

	// =========================================
	// 敵が存在するなら当たり判定チェック
	// =========================================
	if (enemy_ && !enemy_->IsDead()) { // 敵が存在していて生きているなら
		// 敵の位置と当たり判定用スケールを取得
		Vector3 enemyPos = enemy_->GetWorldPosition();
		Vector3 enemySize = enemy_->GetColliderScale();
		// 当たり判定チェック
		bool hit = CheckSweptHitAABB(enemyPos, enemySize);

		if (hit) { // 当たった！
			// 当たりフラグを立てて弾を消す
			isHit_ = true;
			isDead_ = true;

			// エフェクト発生
			TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
			Vector3 hitPos = bulletPos;

			bool isLTBullet = (trailGroup_ == "trail_lt");
			int  damage = isSpecialAttack_ ? 100 : 1;
			bool willDie = (enemy_ && enemy_->GetHP() <= damage);

			// ▼ エフェクト（元のまま）
			if (isLTBullet) {
				damage = 50;
				pm->Emit("lt_nova_core", hitPos, 1);
				pm->Emit("lt_nova_wave", hitPos, 3);
				pm->Emit("lt_nova_burst", hitPos, 40);
				pm->Emit("lt_nova_debris", hitPos, 120);
				pm->Emit("lt_nova_crack", hitPos, 80);
			} else {
				pm->Emit("enemyHit_flash", hitPos, 1);
				pm->Emit("enemyHit_ring", hitPos, 1);
				pm->Emit("enemyHit_rays", hitPos, 18);
				pm->Emit("enemyHit_spark", hitPos, 32);
			}

			if (enemy_ && !enemy_->IsDead()) {
				enemy_->OnHitWithDamage(damage);
				if (willDie) {
					Vector3 knockDir = velocity_;
					if (MyMath::Length(knockDir) < 0.001f) {
						knockDir = enemyPos - bulletPos;
					}
					enemy_->StartDeathReaction(knockDir);
				}
			}

			if (player_) {
				if (isLTBullet) player_->StartCameraShake(40);
				else            player_->StartCameraShake(10);
			}

			return;
		}
	}

	// =========================================
	// 核（MidBossCore）との当たり判定
	// =========================================
	if (core_ && !core_->IsDead()) {
		Vector3 corePos = core_->GetWorldPosition();
		Vector3 coreSize = core_->GetColliderScale();

		bool hit = CheckSweptHitAABB(corePos, coreSize);

		if (hit) {
			isHit_ = true;
			isDead_ = true;

			TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
			Vector3 hitPos = bulletPos;

			bool isLTBullet = (trailGroup_ == "trail_lt");
			int  damage = isSpecialAttack_ ? 100 : 1;
			bool willDie = (core_ && core_->GetHP() <= damage);

			if (isLTBullet) {
				damage = 10;
				pm->Emit("lt_nova_core", hitPos, 1);
				pm->Emit("lt_nova_wave", hitPos, 3);
				pm->Emit("lt_nova_burst", hitPos, 40);
				pm->Emit("lt_nova_debris", hitPos, 120);
				pm->Emit("lt_nova_crack", hitPos, 80);
			} else {
				pm->Emit("enemyHit_flash", hitPos, 1);
				pm->Emit("enemyHit_ring", hitPos, 1);
				pm->Emit("enemyHit_rays", hitPos, 18);
				pm->Emit("enemyHit_spark", hitPos, 32);
			}

			if (core_ && !core_->IsDead()) {
				core_->OnHitWithDamage(damage);
				if (willDie) {
					Vector3 knockDir = velocity_;
					if (MyMath::Length(knockDir) < 0.001f) {
						knockDir = corePos - bulletPos;
					}
					core_->StartDeathReaction(knockDir);
				}
			}

			if (player_) {
				if (isLTBullet) player_->StartCameraShake(40);
				else            player_->StartCameraShake(10);
			}

			return;
		}
	}

	// 一定距離（Z方向）を超えたら弾を削除する
	if (pos.z > kDespawnZ_) {
		isDead_ = true; // 弾を削除
	}

	// Object3d の更新処理
	object_->Update();
}

void PlayerBullet::Draw(TKM::DirectXCommon* dxCommon) {
	object_->Draw(dxCommon); // 3Dオブジェクトの描画
}

void PlayerBullet::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos); // 座標設定
	prevPos_ = pos;
}

void PlayerBullet::SetVelocity(const Vector3& vel) {
	velocity_ = vel; // 速度設定
}

void PlayerBullet::StartSpawnBezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float duration, const Vector3& velocityAfter) {
	bezP0_ = p0; bezP1_ = p1; bezP2_ = p2; bezP3_ = p3;
	spawnDuration_ = std::max(0.001f, duration);
	spawnT_ = 0.0f;
	isSpawningCurve_ = true;
	postSpawnVelocity_ = velocityAfter;
	// ベジェ中は速度を使わないので一旦ゼロでもOK（好み）
	velocity_ = { 0,0,0 };
}

void PlayerBullet::UpdateSpawnBezier() {
	const float dt = 1.0f / 60.0f;
	// 現在の座標を取得して、速度分だけ進める
	Vector3 pos = object_->GetTranslate();

	// --- 発射の“出方”をベジェで演出 ---
	if (isSpawningCurve_) {
		spawnT_ += dt / spawnDuration_;
		float t = std::clamp(spawnT_, 0.0f, 1.0f);

		Vector3 newPos = MyMath::Bezier3(bezP0_, bezP1_, bezP2_, bezP3_, t);
		object_->SetTranslate(newPos);
		trailEmitter_.SetPosition(newPos);
		trailEmitter_.Update();

		if (t >= 1.0f) {
			isSpawningCurve_ = false;
			velocity_ = postSpawnVelocity_;
		}
	}

	if (isHoming_ && homingDelay_ > 0.0f) {
		homingDelay_ -= (1.0f / 60.0f);
	}

	if (isHoming_ && homingDelay_ <= 0.0f && enemy_ && !enemy_->IsDead()) {
		if (!enemy_) return;
		Vector3 enemyPos = enemy_->GetWorldPosition();
		Vector3 dir = enemyPos - pos;
		float len = MyMath::Length(dir);
		if (len > 0.001f) {
			dir = MyMath::Normalize(dir);
			velocity_ = dir * homingSpeed_;
		}
	}
}