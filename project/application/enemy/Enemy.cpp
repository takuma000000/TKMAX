#define NOMINMAX
#include "Enemy.h"
#include "ModelManager.h"
#include <algorithm>
#include <cstdlib> 

void Enemy::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	object_ = std::make_unique<Object3d>(); // Object3d のインスタンスを生成
	object_->Initialize(common, dxCommon); // 初期化
	object_->SetModel("enemy.obj"); // モデル名は適宜変更
	ModelManager::GetInstance()->LoadModel("enemy.obj", dxCommon); // モデル読み込み

	// カメラ設定
	if (camera) {
		object_->SetCamera(camera);
	}

	baseScale_ = object_->GetScale(); // 元のスケールを保持
	colliderScale_ = baseScale_; // 当たり判定用スケールも初期化
	startX_ = object_->GetTranslate().x; // サイン波の基準用
}

void Enemy::Update() {

	// 死亡演出中ならこっちを優先
	if (isDying_) {
		const float dt = 1.0f / 60.0f;
		deathTimer_ += dt;
		float t = std::min(deathTimer_ / deathDuration_, 1.0f); // 0.0 → 1.0
		// 基本値を取得
		Vector3 pos = object_->GetTranslate();
		Vector3 rot = object_->GetRotate();
		Vector3 scale = baseScale_;

		switch (deathReaction_) {
		case EnemyDeathReaction::BlowAway: {
			// いままでの「吹っ飛び＋縮小」
			float speed = 1.0f - t;                    // だんだん減速
			pos += deathVelocity_ * speed * dt;        // 吹っ飛び

			rot.x += deathRotateSpeed_.x * dt;
			rot.y += deathRotateSpeed_.y * dt;
			rot.z += deathRotateSpeed_.z * dt;

			float s = 1.0f - t;                        // 全体的に縮む
			scale = { baseScale_.x * s, baseScale_.y * s, baseScale_.z * s };
			break;
		}
		case EnemyDeathReaction::RiseAbsorb: {
			// その場付近で上に吸い込まれるように消える
			pos += deathVelocity_ * dt;                // 上方向へ一定速度で移動

			// 少しだけY軸回転
			rot.y += deathRotateSpeed_.y * dt;

			// XZだけ細くなっていく（縦方向はあまり潰さない）
			float s = 1.0f - t;
			scale = {
				baseScale_.x * s * 0.5f,
				baseScale_.y * (1.0f - t * 0.2f),
				baseScale_.z * s * 0.5f
			};
			break;
		}
		case EnemyDeathReaction::Collapse: {
			// 前のめりに崩れ落ちる
			pos += deathVelocity_ * dt;                // 下＋ちょっと前に落ちる

			// X 回転を強めに（前に倒れ込む）
			rot.x += deathRotateSpeed_.x * dt;

			// Yだけペシャンと潰れる感じ
			float s = 1.0f - t;
			scale = {
				baseScale_.x,
				baseScale_.y * s * 0.2f,
				baseScale_.z
			};
			break;
		}
		}

		object_->SetTranslate(pos);
		object_->SetRotate(rot);
		object_->SetScale(scale);

		// 全パターン共通：アルファは 1 → 0 にフェード
		deathAlpha_ = 1.0f - t;
		object_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });
		object_->Update();

		if (deathTimer_ >= deathDuration_) {
			// 敵が完全に消える瞬間に専用エフェクトを出す
			ParticleManager* pm = ParticleManager::GetInstance();
			Vector3 emitPos = GetWorldPosition();

			switch (deathReaction_) {
			case EnemyDeathReaction::BlowAway:
				// 吹っ飛び系：破片多め
				pm->Emit("enemyDeath_core", emitPos, 1);
				pm->Emit("enemyDeath_shard", emitPos, 20);
				pm->Emit("enemyDeath_smoke", emitPos, 4);
				break;
			case EnemyDeathReaction::RiseAbsorb:
				// 吸い込み系：ビット＋縦ラインメイン
				pm->Emit("enemyDeath_core", emitPos, 1);
				pm->Emit("enemyDeath_shard", emitPos, 14);
				pm->Emit("enemyDeath_smoke", emitPos, 6);
				break;
			case EnemyDeathReaction::Collapse:
				// 崩れ落ち系：破片少なめ＋控えめなライン
				pm->Emit("enemyDeath_shard", emitPos, 10);
				pm->Emit("enemyDeath_smoke", emitPos, 3);
				break;
			}

			isDead_ = true;
		}
		return;
	}

	// ---- 位置更新（挙動別）----
	Vector3 pos = object_->GetTranslate();

	switch (behavior_) { // 挙動別移動
	case EnemyBehavior::StraightStop: { // いまの「Z手前に進んでstopZで止まる」
		if (!stopMove_) {
			pos += velocity_;
			if (pos.z <= stopZ_) { pos.z = stopZ_; stopMove_ = true; }
		}
		break;
	}
	case EnemyBehavior::SineX: { // Xをサイン波で揺らしながら前進
		t_ += 0.05f;
		pos.z += velocity_.z; // 手前へ
		pos.x = startX_ + std::sinf(sinePhase_ + t_ * sineFreq_) * sineAmpX_;
		if (pos.z <= stopZ_) { pos.z = stopZ_; }
		break;
	}
	case EnemyBehavior::StrafeLtoR: { // Xを左右往復しながら前進
		pos.z += velocity_.z;
		// 簡易左右往復
		strafePosX_ += strafeSpeed_ * strafeDir_;
		if (strafePosX_ > strafeRight_) { strafePosX_ = strafeRight_; strafeDir_ = -1; }
		if (strafePosX_ < strafeLeft_) { strafePosX_ = strafeLeft_;  strafeDir_ = +1; }
		pos.x = strafePosX_;
		if (pos.z <= stopZ_) { pos.z = stopZ_; }
		break;
	}
	case EnemyBehavior::ChasePlayer: { // プレイヤー方向にじわっと追尾
		pos.z += velocity_.z;
		if (playerGetter_) {
			Vector3 toP = playerGetter_() - pos;
			Vector3 desire = { toP.x, toP.y, 0.0f };
			float len = MyMath::Length(desire);
			if (len > 0.001f) {
				Vector3 dir = MyMath::Normalize(desire);
				pos.x += dir.x * chaseSpeed_;
				pos.y += dir.y * chaseSpeed_;
			}
		}
		if (pos.z <= stopZ_) { pos.z = stopZ_; }
		break;
	}
	}

	object_->SetTranslate(pos); // 位置反映

	// ---- 当たり判定の可視化（ワイヤーボックス）----
	{
		Vector3 center = GetWorldPosition();

		float hx = colliderScale_.x * 0.5f;
		float hy = colliderScale_.y * 0.5f;
		float hz = colliderScale_.z * 0.5f;

		auto* lr = LineRenderer::GetInstance();

		// 軸ごとに色分けしても分かりやすい
		LineRenderer::Color colEdge{ 0.0f, 1.0f, 0.0f, 1.0f }; // とりあえず全部同じ色

		// 8頂点（AABBの隅）
		Vector3 p[8] = {
			{ center.x - hx, center.y - hy, center.z - hz }, // 0: 左下手前
			{ center.x + hx, center.y - hy, center.z - hz }, // 1: 右下手前
			{ center.x - hx, center.y + hy, center.z - hz }, // 2: 左上手前
			{ center.x + hx, center.y + hy, center.z - hz }, // 3: 右上手前
			{ center.x - hx, center.y - hy, center.z + hz }, // 4: 左下奥
			{ center.x + hx, center.y - hy, center.z + hz }, // 5: 右下奥
			{ center.x - hx, center.y + hy, center.z + hz }, // 6: 左上奥
			{ center.x + hx, center.y + hy, center.z + hz }, // 7: 右上奥
		};

		auto add = [&](int a, int b) {
			lr->AddLine(p[a], p[b], colEdge);
			};

		// 手前面（z - hz）
		add(0, 1);
		add(1, 3);
		add(3, 2);
		add(2, 0);

		// 奥面（z + hz）
		add(4, 5);
		add(5, 7);
		add(7, 6);
		add(6, 4);

		// 側面（縦の4本）
		add(0, 4);
		add(1, 5);
		add(2, 6);
		add(3, 7);
	}

	// ---- ロック中のパルス（既存）----
	if (isLocked_) {
		pulseT_ += 0.12f;
		float s = 1.0f + 0.15f * sinf(pulseT_);
		object_->SetScale({ baseScale_.x * s, baseScale_.y * s, baseScale_.z * s });
	} else { // 通常スケールに戻す
		object_->SetScale(baseScale_);
	}

	// ---- 将来の射撃フック（必要になったら実装）----
	if (canShoot_) {
		shootTimer_++;
		if (shootTimer_ >= shootInterval_) {
			shootTimer_ = 0.0f;
		}
	}

	object_->Update(); // Object3d の更新
}

void Enemy::Draw(DirectXCommon* dxCommon) {
	object_->Draw(dxCommon); // Object3d の描画
}

void Enemy::SetCamera(Camera* camera) {
	this->camera = camera; // メンバ変数に保存
	if (object_) {
		object_->SetCamera(camera); // Object3d に反映
	}
}
void Enemy::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos); // 位置設定
}
void Enemy::SetParentScene(BaseScene* scene) {
	parentScene_ = scene; // メンバ変数に保存
}
Vector3 Enemy::GetWorldPosition() const {
	return object_->GetTranslate(); // ワールド位置を返す
}

void Enemy::ImGuiDebug() {
#ifdef USE_IMGUI
	if (!object_) return;

	ImGui::Begin("Enemy");

	Vector3 pos = object_->GetTranslate();
	Vector3 rot = object_->GetRotate();
	Vector3 scale = object_->GetScale();

	if (ImGui::DragFloat3("位置", &pos.x, 0.01f)) {
		object_->SetTranslate(pos);
	}
	if (ImGui::DragFloat3("回転", &rot.x, 0.01f)) {
		object_->SetRotate(rot);
	}
	if (ImGui::DragFloat3("拡縮", &scale.x, 0.01f)) {
		SetScale(scale);   // モデルと当たり判定両方に反映される
	}

	// 当たり判定スケール編集
	Vector3 col = colliderScale_;
	if (ImGui::DragFloat3("当たり判定サイズ", &col.x, 0.01f, 0.01f, 999.0f)) {
		SetColliderScale(col);
	}

	ImGui::Text("HP: %d / %d", hp_, maxHP_);
	ImGui::Text("生死: %s", isDead_ ? "死" : "生");

	ImGui::End();
#endif
}

void Enemy::OnHitWithDamage(int damage){
	hp_ -= damage; // 指定ダメージ分減らす
	if (hp_ < 0) {
		hp_ = 0;
	}
}

void Enemy::StartDeathReaction(const Vector3& hitDir) {
	if (isDying_) {
		return;
	}

	isDying_ = true;
	deathTimer_ = 0.0f;
	deathAlpha_ = 1.0f;

	// 0,1,2 のどれかをランダムに選ぶ
	int r = std::rand() % 3;

	// 共通で使うノックバック方向
	Vector3 dir = hitDir;
	if (MyMath::Length(dir) < 0.001f) {
		dir = { 0.0f, 0.0f, 1.0f };
	}
	dir = MyMath::Normalize(dir);

	switch (r) {
	case 0: // 吹っ飛び
	default:
		deathReaction_ = EnemyDeathReaction::BlowAway;
		deathDuration_ = 1.0f;
		deathVelocity_ = dir * 4.0f;             // ヒット方向へ吹っ飛ぶ
		deathRotateSpeed_ = { 1.5f, 2.0f, 0.8f }; // ぐるっと回転
		break;
	case 1: // 上に吸い込まれる
		deathReaction_ = EnemyDeathReaction::RiseAbsorb;
		deathDuration_ = 1.2f;
		deathVelocity_ = { 0.0f, 3.0f, 0.0f };   // 上方向にスッと上がる
		deathRotateSpeed_ = { 0.0f, 2.0f, 0.0f }; // 少しだけY回転
		break;
	case 2: // 崩れ落ち
		deathReaction_ = EnemyDeathReaction::Collapse;
		deathDuration_ = 0.9f;
		// ちょい前＋下に崩れ落ちる
		deathVelocity_ = { dir.x * 1.5f, -3.0f, dir.z * 1.5f };
		deathRotateSpeed_ = { 3.0f, 0.5f, 0.0f }; // 前に倒れ込む感じ
		break;
	}
}