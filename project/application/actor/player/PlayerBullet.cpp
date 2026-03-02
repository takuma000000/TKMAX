#define NOMINMAX
#include "PlayerBullet.h"
#include <ParticleManager.h>
#include "AABB.h"
#include "Player.h"
#include "Enemy.h"
#include "MidBossCore.h"
#include "MyMath.h"

// 弾の初期スケール
static Vector3 DirToEuler_(const Vector3& dir) {
	Vector3 d = dir;
	float len = MyMath::Length(d);
	if (len < 0.0001f) { return { 0,0,0 }; }
	d = d / len;

	// Z+ が前方向の想定（yaw/pitch）
	float yaw = std::atan2(d.x, d.z);
	float pitch = -std::asin(d.y);
	return { pitch, yaw, 0.0f };
}

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

	ltRingDistAcc_ = 0.0f; // LT弾リングの距離加算値初期化
}

void PlayerBullet::Update() {

	Vector3 oldPos = object_->GetTranslate(); // 前フレームの座標保存

	UpdateSpawnBezier(); // 発射の「出方」曲線更新

	// 現在の座標を取得して、速度分だけ進める
	Vector3 pos = object_->GetTranslate();
	pos = pos + velocity_; // 速度分だけ進める
	object_->SetTranslate(pos); // 座標を更新
	trailEmitter_.SetPosition(pos); // パーティクル位置更新
	if (trailGroup_ != "trail_lt") {
		trailEmitter_.Update(); // 通常弾は今まで通り
	}

	prevPos_ = oldPos; // 前フレームの座標を保存

	// LT弾：プレイヤーから“常に”伸びる線
	if (trailGroup_ == "trail_lt") {
		Vector3 start = oldPos; // fallback
		if (player_) {
			start = player_->GetWorldPosition(); // ←銃口があるなら GetMuzzleWorldPosition() みたいなのに差し替え
			// 例：少し前に出したいなら
			// start = start + Vector3{ 0.0f, 0.8f, 1.0f };
		}
		EmitLTFairyTrail_(start, pos);
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

void PlayerBullet::SetCamera(TKM::Camera* camera) {
	if (object_) {
		object_->SetCamera(camera); // Object3d にカメラを設定
	}
}

void PlayerBullet::SetTrailGroup(const std::string& group) {
	trailGroup_ = group; // トレイルグループ名を保存
	// 位置は現在地で再初期化（生成直後や途中でもOK）
	Vector3 pos = object_ ? object_->GetTranslate() : Vector3{}; // Object3d がまだない場合は原点で初期化
	trailEmitter_.Initialize(trailGroup_, pos); // トレイルエミッターを新しいグループで初期化
}

void PlayerBullet::SetEnemy(Enemy* enemy) {
	enemy_ = enemy; // ヒット対象の敵を設定
}

void PlayerBullet::SetPlayer(Player* player) {
	player_ = player; // プレイヤー参照を設定
}

void PlayerBullet::SetSpecialAttack(bool flag) {
	isSpecialAttack_ = flag; // 一撃必殺フラグを設定
}

void PlayerBullet::SetHoming(bool enable, float speed) {
	isHoming_ = enable; // ホーミングの有効/無効を設定
	homingSpeed_ = speed; // ホーミング速度を設定
}

void PlayerBullet::SetHomingDelay(float sec) {
	homingDelay_ = std::max(0.0f, sec); // ホーミング開始までの遅延時間を設定（負の値は0に補正）
}

void PlayerBullet::SetCore(MidBossCore* core) {
	core_ = core; // ヒット対象の核を設定
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
		if (trailGroup_ == "trail_lt") { // LT弾はベジェ曲線区間も“線”で埋める
			EmitLTFairyTrail_(pos, newPos); // pos は関数冒頭で取ってる“更新前”位置
		} else { // 通常弾はベジェ曲線区間もエミッターを追従させるだけ
			trailEmitter_.SetPosition(newPos); // エミッター位置更新
			trailEmitter_.Update(); // エミッター更新（通常弾はベジェ区間もエミッターを追従させる）
		}
		// ベジェ曲線が終わったら通常の速度に切り替える
		if (t >= 1.0f) {
			isSpawningCurve_ = false; // ベジェ曲線終了
			velocity_ = postSpawnVelocity_; // ベジェ終了後の速度を適用
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

void PlayerBullet::EmitLTFairyTrail_(const Vector3& from, const Vector3& to) {
	TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
	if (!pm) { return; }

	Vector3 d = to - from;
	float L = MyMath::Length(d);
	if (L < 0.001f) { return; }

	Vector3 dir = d / L;

	// 線分の中心に1枚置く（パンツァみたいに“伸びる線”になる）
	Vector3 mid = from + d * 0.5f;

	TKM::ParticleManager::Transform tr{};
	tr.translate_ = mid;

	// レーザーの太さ
	const float thickness = 0.55f;

	// リボン頂点は Xが横幅 / Yが厚み / Zは0 なので
	// Z方向を「長さ」に使うため、スケールZにLを入れる
	tr.scale_ = { thickness, thickness, L * 0.5f };

	// 進行方向へ向ける
	tr.rotate_ = DirToEuler_(dir);

	// メルヘン全振り（白なし寄り）
	Vector4 col = { 1.25f, 0.35f, 1.35f, 0.95f };

	// 1本の線分として出す
	pm->EmitWithTransform("trail_lt_ribbon", tr, col, 1);

	// 周辺のキラキラ（これは点でOK）
	pm->Emit("trail_lt_sparkle", tr.translate_, 2);

	// たまにリング（道筋演出）
	ltRingDistAcc_ += L;
	const float kRingEvery = 1.8f;
	if (ltRingDistAcc_ >= kRingEvery) {
		Vector3 p = to;
		pm->Emit("trail_lt_ring", p, 1);
		ltRingDistAcc_ = 0.0f;
	}
}