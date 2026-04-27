#define NOMINMAX
#include "PlayerBullet.h"
#include <ParticleManager.h>
#include "AABB.h"
#include "Player.h"
#include "Enemy.h"
#include "BarrierCore.h"
#include "MyMath.h"
#include "BarrierCoreManager.h"
#include "TrailRibbonRenderer.h"

// 進行方向ベクトルをオイラー角に変換する
static Vector3 DirToEuler_(const Vector3& dir) {
	// 引数をコピーして正規化用に使う
	Vector3 d = dir;

	// ベクトルの長さを取得する
	float len = MyMath::Length(d);

	// 長さが小さすぎる場合は向きを作れないのでゼロ回転を返す
	if (len < 0.0001f) { return { 0,0,0 }; }

	// 向きだけを使うため正規化する
	d = d / len;

	// Z+ を前方向としてヨー角を求める
	float yaw = std::atan2(d.x, d.z);

	// Y方向の傾きからピッチ角を求める
	float pitch = -std::asin(d.y);

	// ロールは使わないので0固定
	return { pitch, yaw, 0.0f };
}

// ベクトルを法線で反射させる
static Vector3 ReflectVector_(const Vector3& velocity, const Vector3& normal) {
	return velocity - normal * (2.0f * MyMath::Dot(velocity, normal));
}

void PlayerBullet::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	// 弾本体の3Dオブジェクトを生成する
	object_ = std::make_unique<TKM::Object3d>();

	// 描画に必要な共通情報を渡して初期化する
	object_->Initialize(common, dxCommon);

	// 通常弾モデルを設定する
	object_->SetModel("normalBullet.obj");

	// 弾の初期スケールを設定する
	object_->SetScale({ kDefaultScale_, kDefaultScale_, kDefaultScale_ });

	// 線分判定用に初期座標を保存する
	prevPos_ = object_->GetTranslate();

	// 現在位置を使ってトレイルエミッタを初期化する
	Vector3 start = object_->GetTranslate();
	trailEmitter_.Initialize(trailGroup_, start);

	// LT弾の場合はリボントレイル用の点列も初期化する
	if (trailGroup_ == "trail_lt") {
		ltTrailPts_.clear();
		ltTrailPts_.push_back(start);
		ltRingDistAcc_ = 0.0f;
		ltTrailDistAcc_ = 0.0f;
	}

	// LT弾リング用の距離蓄積を初期化する
	ltRingDistAcc_ = 0.0f;

	// 寿命タイマーを初期化する
	lifeTimer_ = 0.0f;
}

void PlayerBullet::Update() {

	// 前フレーム座標を保存する
	Vector3 oldPos = object_->GetTranslate();

	// 発射直後のベジェ演出を更新する
	UpdateSpawnBezier();

	// ベジェ演出中は速度移動せず、ベジェ側で位置が決まるのでここで終了する
	if (isSpawningCurve_) {
		object_->Update();
		return;
	}

	// 寿命タイマーを進める
	lifeTimer_ += dt_;

	// 寿命を超えたら死亡扱いにする
	if (lifeTimer_ >= kLifeTime_) {
		isDead_ = true;
	}

	// 現在位置を取得する
	Vector3 pos = object_->GetTranslate();

	// 速度分だけ位置を進める
	pos = pos + velocity_;

	// 新しい位置を反映する
	object_->SetTranslate(pos);

	// 通常トレイルの発生位置を更新する
	trailEmitter_.SetPosition(pos);

	// 線分判定用に前フレーム位置を保存する
	prevPos_ = oldPos;

	// LT弾の場合はリボントレイル用の位置を更新する
	if (trailGroup_ == "trail_lt") {
		// 速度方向を取得する
		Vector3 dir = velocity_;

		// 速度ベクトルの長さを取得する
		float len = MyMath::Length(dir);

		// 十分な長さがあれば正規化する
		if (len > 0.0001f) {
			dir = dir / len;
		} else {
			// 速度がほぼ無い場合は前方向を仮に使う
			dir = { 0,0,1 };
		}

		// トレイルを弾の少し前に出すためのオフセット
		float trailFrontOffset = 1.2f;

		// トレイル用の発生位置を計算する
		Vector3 trailPos = pos + dir * trailFrontOffset;

		// LT用リボントレイルを更新する
		UpdateLTTrail_(trailPos);
	}

	// LT以外で通常トレイルが有効なら更新する
	if (useTrail_ && trailGroup_ != "trail_lt") {
		trailEmitter_.Update();
	}

	//=========================================================
	// 弾AABB vs 相手AABB の共通判定
	//=========================================================

	// 弾の現在位置を取得する
	Vector3 bulletPos = object_->GetTranslate();

	// 弾の現在スケールを取得する
	Vector3 bulletScale = object_->GetScale();

	// 線分判定とAABB判定をまとめた共通ラムダ
	auto CheckSweptHitAABB = [&](const Vector3& targetPos, const Vector3& targetSize) -> bool {
		// 弾側のAABBを作成する
		AABB bulletBox(bulletPos, bulletScale);

		// 相手側のAABBを作成する
		AABB targetBox(targetPos, targetSize);

		// 前フレーム位置から現在位置までの線分が相手AABBと交差しているかを見る
		if (targetBox.IsIntersectSegment(prevPos_, bulletPos)) {
			return true;
		}

		// 念のため、現在位置同士のAABB重なりも見る
		if (bulletBox.IsCollidingWithAABB(targetBox)) {
			return true;
		}

		return false;
		};

	// 線分と楕円体の当たり判定を行う共通ラムダ
	auto CheckSweptHitEllipsoid = [&](const Vector3& center, const Vector3& radius) -> bool {
		// 半径0による除算を防ぐ
		const float rx = std::max(radius.x, 0.0001f);
		const float ry = std::max(radius.y, 0.0001f);
		const float rz = std::max(radius.z, 0.0001f);

		// 前回位置と現在位置を楕円体中心基準に変換する
		Vector3 p0 = prevPos_ - center;
		Vector3 p1 = bulletPos - center;

		// 各軸の半径で割って単位球空間に変換する
		p0.x /= rx; p0.y /= ry; p0.z /= rz;
		p1.x /= rx; p1.y /= ry; p1.z /= rz;

		// 単位球空間での移動ベクトルを作る
		Vector3 d = p1 - p0;

		// 二次方程式の係数を作る
		float a = MyMath::Dot(d, d);
		float b = 2.0f * MyMath::Dot(p0, d);
		float c = MyMath::Dot(p0, p0) - 1.0f;

		// 開始点がすでに内側ならヒット扱い
		if (c <= 0.0f) {
			return true;
		}

		// 判別式を計算する
		float discriminant = b * b - 4.0f * a * c;

		// 交点が無ければヒットしない
		if (discriminant < 0.0f) {
			return false;
		}

		// 交点計算用に平方根を取る
		float sqrtD = std::sqrt(discriminant);

		// 2aの逆数を用意する
		float inv2A = 1.0f / (2.0f * a);

		// 線分上の交点パラメータを求める
		float t1 = (-b - sqrtD) * inv2A;
		float t2 = (-b + sqrtD) * inv2A;

		// 0～1の範囲に交点があれば線分が楕円体に当たっている
		return (t1 >= 0.0f && t1 <= 1.0f) || (t2 >= 0.0f && t2 <= 1.0f);
		};

	//=========================================================
	// Wave1バリアとの当たり判定
	//=========================================================
	if (player_ && player_->IsWave1BarrierActive()) {

		// プレイヤーからバリア中心を取得する
		const Vector3 barrierPos_ = player_->GetWave1BarrierCenter();

		// プレイヤーからバリア半径を取得する
		const Vector3 barrierRadius_ = player_->GetWave1BarrierSize();

		// 楕円体バリアとの線分判定を行う
		const bool barrierHit_ = CheckSweptHitEllipsoid(barrierPos_, barrierRadius_);

		// バリアに当たった場合
		if (barrierHit_) {
			// ヒット済みにする
			isHit_ = true;

			// 弾が進んできた方向を取得する
			Vector3 inDir = velocity_;
			float speed = MyMath::Length(inDir);

			if (speed > 0.0001f) {
				inDir = inDir / speed;
			} else {
				inDir = { 0.0f, 0.0f, 1.0f };
			}

			// バリアの縁でも中心と同じように、来た方向へそのまま押し返す
			Vector3 reflectDir = -inDir;

			// 反射後の速度を設定する
			velocity_ = reflectDir * speed * kBarrierReflectDamping_;

			// めり込み防止：当たる前の位置へ戻して、少しだけ反射方向へ押し出す
			Vector3 safePos = prevPos_ + reflectDir * kBarrierReflectPushOut_;
			object_->SetTranslate(safePos);
			prevPos_ = safePos;

			// 反射後の進行方向へ弾の向きを合わせる
			object_->SetRotate(DirToEuler_(velocity_));

			// パーティクルマネージャーを取得する
			TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();

			// ヒット位置は弾の現在位置を使う
			Vector3 hitPos = bulletPos;

			// バリアヒット演出を出す
			if (pm) {
				pm->Emit("enemyHit_flash", hitPos, 1);
				pm->Emit("enemyHit_ring", hitPos, 1);
				pm->Emit("enemyHit_spark", hitPos, 12);
			}

			// プレイヤー側へバリアヒット情報と演出要求を送る
			if (player_) {
				player_->AddWave1BarrierHit(hitPos);
				player_->RequestWave1BarrierFlash(hitPos);
				player_->StartCameraShake(6);
			}

			return;
		}
	}

	//=========================================================
	// 敵との当たり判定
	//=========================================================
	if (enemy_ && !enemy_->IsDead()) {
		// 敵の現在位置を取得する
		Vector3 enemyPos = enemy_->GetWorldPosition();

		// 敵の当たり判定サイズを取得する
		Vector3 enemySize = enemy_->GetColliderScale();

		// 敵との線分＋AABB判定を行う
		bool hit = CheckSweptHitAABB(enemyPos, enemySize);

		// 敵に当たった場合
		if (hit) {
			// ヒット済みにする
			isHit_ = true;

			// 弾を死亡扱いにする
			isDead_ = true;

			// パーティクルマネージャーを取得する
			TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();

			// ヒット位置は弾の現在位置を使う
			Vector3 hitPos = bulletPos;

			// LT弾かどうかを判定する
			bool isLTBullet = (trailGroup_ == "trail_lt");

			// 通常ダメージか一撃必殺ダメージを決める
			int  damage = isSpecialAttack_ ? 100 : 1;

			// このダメージで敵が死ぬかを先に判定しておく
			bool willDie = (enemy_ && enemy_->GetHP() <= damage);

			// LT弾なら専用の大きいヒット演出にする
			if (isLTBullet) {
				damage = 50;
				pm->Emit("lt_nova_core", hitPos, 1);
				pm->Emit("lt_nova_wave", hitPos, 3);
				pm->Emit("lt_nova_burst", hitPos, 40);
				pm->Emit("lt_nova_debris", hitPos, 120);
				pm->Emit("lt_nova_crack", hitPos, 80);
			} else {
				// 通常弾のヒット演出を出す
				pm->Emit("enemyHit_flash", hitPos, 1);
				pm->Emit("enemyHit_ring", hitPos, 1);
				pm->Emit("enemyHit_rays", hitPos, 18);
				pm->Emit("enemyHit_spark", hitPos, 32);
			}

			// 敵がまだ生きていればダメージを与える
			if (enemy_ && !enemy_->IsDead()) {
				enemy_->OnHitWithDamage(damage);

				// 死亡予定ならノックバック方向を渡して死亡リアクションを開始する
				if (willDie) {
					Vector3 knockDir = velocity_;
					if (MyMath::Length(knockDir) < 0.001f) {
						knockDir = enemyPos - bulletPos;
					}
					enemy_->StartDeathReaction(knockDir);
				}
			}

			// 弾種に応じてカメラシェイクを変える
			if (player_) {
				if (isLTBullet) player_->StartCameraShake(40);
				else            player_->StartCameraShake(10);
			}

			return;
		}
	}

	//=========================================================
	// バリアコア群との当たり判定
	//=========================================================
	if (barrierCoreManager_) {
		// 生存中のコア一覧を取得する
		const auto aliveCores_ = barrierCoreManager_->GetAliveCores();

		// 生存中コアを順番に判定する
		for (BarrierCore* core : aliveCores_) {
			// nullptrは無視する
			if (!core) {
				continue;
			}

			// コア位置を取得する
			Vector3 corePos = core->GetWorldPosition();

			// コア当たり判定サイズを取得する
			Vector3 coreSize = core->GetColliderScale();

			// コアとの線分＋AABB判定を行う
			bool hit = CheckSweptHitAABB(corePos, coreSize);

			// 当たっていなければ次のコアへ
			if (!hit) {
				continue;
			}

			// ヒット済みにする
			isHit_ = true;

			// 弾を死亡扱いにする
			isDead_ = true;

			// パーティクルマネージャーを取得する
			TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();

			// ヒット位置は弾の現在位置を使う
			Vector3 hitPos = bulletPos;

			// LT弾かどうかを判定する
			bool isLTBullet = (trailGroup_ == "trail_lt");

			// 通常ダメージか一撃必殺ダメージを決める
			int  damage = isSpecialAttack_ ? 100 : 1;

			// このダメージでコアが死ぬかを先に判定しておく
			bool willDie = (core->GetHP() <= damage);

			// LT弾なら専用ヒット演出
			if (isLTBullet) {
				damage = 50;
				pm->Emit("lt_nova_core", hitPos, 1);
				pm->Emit("lt_nova_wave", hitPos, 3);
				pm->Emit("lt_nova_burst", hitPos, 40);
				pm->Emit("lt_nova_debris", hitPos, 120);
				pm->Emit("lt_nova_crack", hitPos, 80);
			} else {
				// 通常弾のヒット演出
				pm->Emit("enemyHit_flash", hitPos, 1);
				pm->Emit("enemyHit_ring", hitPos, 1);
				pm->Emit("enemyHit_rays", hitPos, 18);
				pm->Emit("enemyHit_spark", hitPos, 32);
			}

			// コアへダメージを与える
			core->OnHitWithDamage(damage);

			// 死亡予定ならノックバック方向を渡して死亡リアクションを開始する
			if (willDie) {
				Vector3 knockDir = velocity_;
				if (MyMath::Length(knockDir) < 0.001f) {
					knockDir = corePos - bulletPos;
				}
				core->StartDeathReaction(knockDir);
			}

			// 弾種に応じてカメラシェイクを変える
			if (player_) {
				if (isLTBullet) player_->StartCameraShake(40);
				else            player_->StartCameraShake(10);
			}

			return;
		}
	}

	// Z方向に一定距離を超えたら弾を消す
	if (pos.z > kDespawnZ_) {
		isDead_ = true;
	}

	// Object3dを更新する
	object_->Update();
}

void PlayerBullet::Draw(TKM::DirectXCommon* dxCommon) {
	// 弾本体を描画する
	object_->Draw(dxCommon);
}

void PlayerBullet::DrawTrail(TKM::DirectXCommon* dxCommon) {
	// LT弾だけリボントレイルを描画する
	if (trailGroup_ == "trail_lt") {
		if (camera_ && ltTrailPts_.size() >= 1) {
			// リボンレンダラーを取得する
			auto* rr = TKM::TrailRibbonRenderer::GetInstance();

			// デバッグパラメータを取得する
			const auto& p = rr->GetDebugParams();

			// 無効なら描画しない
			if (!p.enable) { return; }

			// 描画用の点列をコピーする
			std::vector<Vector3> drawPts = ltTrailPts_;

			// 現在位置を取得する
			Vector3 currentPos = object_->GetTranslate();

			// 最後の点と現在位置が離れているなら現在位置を追加する
			if (drawPts.empty() || MyMath::Length(currentPos - drawPts.back()) > 0.0001f) {
				drawPts.push_back(currentPos);
			}

			// 2点未満ならリボンを描画できない
			if (drawPts.size() < 2) { return; }

			// リボントレイルを描画する
			rr->DrawRibbon(
				dxCommon,
				*camera_,
				drawPts,
				p.headWidth,
				p.tailWidth,
				p.intensity,
				p.color,
				p.uvTiling,
				p.uvScroll
			);
		}
	}
}

void PlayerBullet::SetPosition(const Vector3& pos) {
	// 弾の座標を設定する
	object_->SetTranslate(pos);

	// 線分判定用の前回位置も更新する
	prevPos_ = pos;

	// 通常トレイルの位置を更新する
	if (useTrail_) {
		trailEmitter_.SetPosition(pos);
	}

	// LT弾ならリボン点列を現在位置から作り直す
	if (trailGroup_ == "trail_lt") {
		ltTrailPts_.clear();
		ltTrailPts_.push_back(pos);
		ltRingDistAcc_ = 0.0f;
		ltTrailDistAcc_ = 0.0f;
	}
}

void PlayerBullet::SetVelocity(const Vector3& vel) {
	// 弾の速度を設定する
	velocity_ = vel;
}

void PlayerBullet::SetCamera(TKM::Camera* camera) {
	// カメラ参照を保持する
	camera_ = camera;

	// Object3dにもカメラを設定する
	if (object_) {
		object_->SetCamera(camera);
	}
}

void PlayerBullet::SetTrailGroup(const std::string& group) {
	// 使用するトレイルグループ名を保存する
	trailGroup_ = group;

	// 現在位置を取得する
	Vector3 pos = object_ ? object_->GetTranslate() : Vector3{};

	// 指定グループでトレイルエミッタを初期化する
	trailEmitter_.Initialize(trailGroup_, pos);

	// LT弾ならリボン点列を現在位置から作り直す
	if (trailGroup_ == "trail_lt") {
		ltTrailPts_.clear();
		ltTrailPts_.push_back(pos);
		ltRingDistAcc_ = 0.0f;
		ltTrailDistAcc_ = 0.0f;
	}
}

void PlayerBullet::SetEnemy(Enemy* enemy) {
	// ヒット対象の敵を設定する
	enemy_ = enemy;
}

void PlayerBullet::SetPlayer(Player* player) {
	// プレイヤー参照を設定する
	player_ = player;
}

void PlayerBullet::SetSpecialAttack(bool flag) {
	// 一撃必殺フラグを設定する
	isSpecialAttack_ = flag;
}

void PlayerBullet::SetCore(BarrierCore* core) {
	// ヒット対象のコアを設定する
	core_ = core;
}

void PlayerBullet::SetUseTrail(bool use) {
	// トレイル使用フラグを設定する
	useTrail_ = use;
}

void PlayerBullet::SetBarrierCoreManager(BarrierCoreManager* manager) {
	// バリアコアマネージャー参照を設定する
	barrierCoreManager_ = manager;
}

void PlayerBullet::StartSpawnBezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float duration, const Vector3& velocityAfter) {
	// LT弾ならベジェ開始位置からリボン点列を作り直す
	if (trailGroup_ == "trail_lt") {
		ltTrailPts_.clear();
		ltTrailPts_.push_back(p0);
		ltRingDistAcc_ = 0.0f;
		ltTrailDistAcc_ = 0.0f;
	}

	// ベジェ曲線の開始点を保存する
	bezP0_ = p0;

	// ベジェ曲線の第1制御点を保存する
	bezP1_ = p1;

	// ベジェ曲線の第2制御点を保存する
	bezP2_ = p2;

	// ベジェ曲線の終点を保存する
	bezP3_ = p3;

	// ベジェ演出時間を下限付きで設定する
	spawnDuration_ = std::max(0.001f, duration);

	// ベジェ進行率をリセットする
	spawnT_ = 0.0f;

	// ベジェ演出中フラグを立てる
	isSpawningCurve_ = true;

	// ベジェ終了後に使う速度を保存する
	postSpawnVelocity_ = velocityAfter;

	// ベジェ中は速度移動させない
	velocity_ = { 0,0,0 };
}

void PlayerBullet::UpdateSpawnBezier() {
	// 現在位置を取得する
	Vector3 pos = object_->GetTranslate();

	// ベジェ演出中だけ更新する
	if (isSpawningCurve_) {
		// ベジェ進行率を進める
		spawnT_ += dt_ / spawnDuration_;

		// 進行率を0～1に収める
		float t = std::clamp(spawnT_, 0.0f, 1.0f);

		// ベジェ曲線上の現在位置を計算する
		Vector3 newPos = MyMath::Bezier3(bezP0_, bezP1_, bezP2_, bezP3_, t);

		// 計算した位置を弾に反映する
		object_->SetTranslate(newPos);

		// LT弾はベジェ曲線に沿ってリボンを更新する
		if (trailGroup_ == "trail_lt") {
			// 前回位置から今回位置への方向を作る
			Vector3 dir = newPos - prevPos_;

			// 方向ベクトルの長さを取得する
			float len = MyMath::Length(dir);

			// 長さがあれば正規化する
			if (len > 0.0001f) {
				dir = dir / len;
			} else {
				// ほぼ動いていないなら前方向を仮に使う
				dir = { 0,0,1 };
			}

			// リボン位置を少し前に出す
			float trailFrontOffset = 1.2f;

			// リボン用の位置を計算する
			Vector3 trailPos = newPos + dir * trailFrontOffset;

			// LTリボン点列を更新する
			UpdateLTTrail_(trailPos);
		} else {
			// 通常トレイルは位置だけ更新してエミッタに任せる
			trailEmitter_.SetPosition(newPos);
			trailEmitter_.Update();
		}

		// ベジェが最後まで進んだら通常速度移動へ切り替える
		if (t >= 1.0f) {
			isSpawningCurve_ = false;
			velocity_ = postSpawnVelocity_;
		}
	}
}

void PlayerBullet::UpdateLTTrail_(const Vector3& p) {
	// 点列が空なら最初の点を追加する
	if (ltTrailPts_.empty()) {
		ltTrailPts_.push_back(p);
		ltRingDistAcc_ = 0.0f;
		ltTrailDistAcc_ = 0.0f;
		return;
	}

	// このフレームで移動した距離を計算する
	float moveDist = MyMath::Length(p - prevPos_);

	// 距離を蓄積する
	ltTrailDistAcc_ += moveDist;

	// 一定距離進んだら点を追加する
	if (ltTrailDistAcc_ >= kLTTrailStep_) {
		ltTrailPts_.push_back(p);

		// 距離蓄積をリセットする
		ltTrailDistAcc_ = 0.0f;

		// 点が増えすぎたら古い点から削除する
		while (ltTrailPts_.size() > kLTTrailHardCap_) {
			ltTrailPts_.erase(ltTrailPts_.begin());
		}
	}
}