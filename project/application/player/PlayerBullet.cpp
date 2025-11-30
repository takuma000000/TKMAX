#define NOMINMAX
#include "MyMath.h"

#include "PlayerBullet.h"
#include <engine/effect/particle/ParticleManager.h>
#include "AABB.h"
#include "Player.h"

void PlayerBullet::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	// 3Dオブジェクト作成
	object_ = std::make_unique<Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");
	object_->SetScale({ kDefaultScale, kDefaultScale, kDefaultScale });

	// 既定グループで一旦初期化（あとで SetTrailGroup で上書き可）
	Vector3 start = object_->GetTranslate();
	trailEmitter_.Initialize(trailGroup_, start);
}

void PlayerBullet::Update() {

	UpdateSpawnBezier(); // 発射の「出方」曲線更新

	// 現在の座標を取得して、速度分だけ進める
	Vector3 pos = object_->GetTranslate();

	// 完全追従：毎フレーム、目標の現在位置へ向けて速度ベクトルを再設定
	if (isHoming_ && enemy_ && !enemy_->IsDead()) {
		Vector3 enemyPos = enemy_->GetWorldPosition(); // 敵の現在位置を取得
		Vector3 dir = enemyPos - pos; // 敵への方向ベクトルを計算
		float len = MyMath::Length(dir); // 方向ベクトルの長さを計算
		if (len > 0.001f) { // ゼロ除算回避
			dir = MyMath::Normalize(dir); // 方向ベクトルを正規化
			velocity_ = dir * homingSpeed_; // 速度の大きさは一定、向きだけ更新
		}
	}

	pos = pos + velocity_; // 速度分だけ進める
	object_->SetTranslate(pos); // 座標を更新
	trailEmitter_.SetPosition(pos); // パーティクル位置更新
	trailEmitter_.Update(); // 毎フレーム放出

	// LTホーミング弾だけ、飛行中にスパークをばら撒く（全部盛りポイント）
	if (trailGroup_ == "trail_lt") {
		ParticleManager* pm = ParticleManager::GetInstance();
		Vector3 emitPos = pos;

		// 空間を裂くような細いレイ
		pm->Emit("enemyHit_rays", emitPos, 1);

		// バチバチ弾けるスパーク
		pm->Emit("enemyHit_spark", emitPos, 1);
	}

	// 敵が存在するなら当たり判定チェック
	if (enemy_ && !enemy_->IsDead()) { // 敵が死んでなければ当たり判定
		// 弾の座標とスケールを取得
		Vector3 bulletPos = object_->GetTranslate();
		Vector3 bulletScale = object_->GetScale();

		// 敵のワールド座標とスケールを取得
		Vector3 enemyPos = enemy_->GetWorldPosition();
		Vector3 enemyScale = enemy_->GetColliderScale();

		// 弾と敵のAABB（軸に沿ったバウンディングボックス）を作成
		AABB bulletBox(bulletPos, bulletScale);
		AABB enemyBox(enemyPos, enemyScale);

		// 当たり判定チェック
		if (bulletBox.IsCollidingWithAABB(enemyBox)) {
			isHit_ = true;
			isDead_ = true;

			ParticleManager* pm = ParticleManager::GetInstance();

			// Emit の第2引数は非const参照なのでローカル変数で
			Vector3 hitPos = bulletPos;

			// ▼ trailGroup_ で「LT弾かどうか」を判定
			bool isLTBullet = (trailGroup_ == "trail_lt");

			// ダメージ値をまず決める
			int damage = isSpecialAttack_ ? 100 : 1;
			// 今のHPから見て「この一撃で死ぬか」を先に判定
			bool willDie = (enemy_ && enemy_->GetHP() <= damage);

			if (isLTBullet) {

				// ===============================
				// LT：ドラゴンボール級 “爆心地誕生” 演出
				// ===============================

				// 核となるコア（めちゃデカい光）
				pm->Emit("lt_nova_core", hitPos, 1);   // サイズは MakeNewParticle 内で6倍へ強化

				// 超巨大ショックウェーブ（2〜3層）
				pm->Emit("lt_nova_wave", hitPos, 3);

				// 炎の大爆発
				pm->Emit("lt_nova_burst", hitPos, 40);

				// デブリ（破片）100個
				pm->Emit("lt_nova_debris", hitPos, 120);

				// （オプション）黒いクラックスパーク（地割れ粒）
				pm->Emit("lt_nova_crack", hitPos, 80);
			} else {
				// ============================
				//  それ以外の弾：通常のヒット演出
				// ============================
				pm->Emit("enemyHit_flash", hitPos, 1);
				pm->Emit("enemyHit_ring", hitPos, 1);
				pm->Emit("enemyHit_rays", hitPos, 18);
				pm->Emit("enemyHit_spark", hitPos, 32);
			}

			// ダメージ適用
			if (enemy_ && !enemy_->IsDead()) {
				enemy_->OnHitWithDamage(damage);

				// ★ 致死だったならノックバック開始
				if (willDie) {
					// ノックバック方向は「弾の進行方向」
					Vector3 knockDir = velocity_;
					if (MyMath::Length(knockDir) < 0.001f) {
						knockDir = enemyPos - bulletPos; // 保険
					}
					enemy_->StartDeathReaction(knockDir);
				}
			}

			// カメラシェイクはLTだけ強め
			if (player_) {
				if (isLTBullet) {
					player_->StartCameraShake(40); // ドーンッ
				} else {
					player_->StartCameraShake(10);
				}
			}

			return;
		}
	}

	// 一定距離（Z方向）を超えたら弾を削除する
	if (pos.z > kDespawnZ) {
		isDead_ = true; // 弾を削除
	}

	// Object3d の更新処理
	object_->Update();
}

void PlayerBullet::Draw(DirectXCommon* dxCommon) {
	object_->Draw(dxCommon); // 3Dオブジェクトの描画
}

void PlayerBullet::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos); // 座標設定
}

void PlayerBullet::SetVelocity(const Vector3& vel) {
	velocity_ = vel; // 速度設定
}

void PlayerBullet::StartSpawnBezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float duration, const Vector3& velocityAfter){
	bezP0_ = p0; bezP1_ = p1; bezP2_ = p2; bezP3_ = p3;
	spawnDuration_ = std::max(0.001f, duration);
	spawnT_ = 0.0f;
	isSpawningCurve_ = true;
	postSpawnVelocity_ = velocityAfter;
	// ベジェ中は速度を使わないので一旦ゼロでもOK（好み）
	velocity_ = { 0,0,0 };
}

void PlayerBullet::UpdateSpawnBezier(){
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
		Vector3 enemyPos = enemy_->GetWorldPosition();
		Vector3 dir = enemyPos - pos;
		float len = MyMath::Length(dir);
		if (len > 0.001f) {
			dir = MyMath::Normalize(dir);
			velocity_ = dir * homingSpeed_;
		}
	}
}