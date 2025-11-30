#define NOMINMAX
#include "Enemy.h"
#include "ModelManager.h"
#include <algorithm>

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
		const float dt = 1.0f / 60.0f;              // 疑似的なフレーム時間
		deathTimer_ += dt;
		float t = std::min(deathTimer_ / deathDuration_, 1.0f); // 0.0 → 1.0

		// ---- ノックバック移動（最初速い → 最後ゆっくり）
		float speed = 1.0f - t;                     // 1 → 0
		Vector3 pos = object_->GetTranslate();
		pos += deathVelocity_ * speed * dt;         // 「速度 × dt」でじわっと動かす
		object_->SetTranslate(pos);

		// ---- 回転（こっちも dt でフレーム依存をなくす）
		Vector3 rot = object_->GetRotate();
		rot.x += deathRotateSpeed_.x * dt;
		rot.y += deathRotateSpeed_.y * dt;
		rot.z += deathRotateSpeed_.z * dt;
		object_->SetRotate(rot);

		// ---- スケールもだんだん小さくする（ここ追加）
		// t = 0.0 → 1.0 の間で 1.0 → 0.0 に縮む
		float scaleT = 1.0f - t;
		Vector3 deathScale = {
			baseScale_.x * scaleT,
			baseScale_.y * scaleT,
			baseScale_.z * scaleT
		};
		object_->SetScale(deathScale);

		// ---- フェード（演出の最初から最後までずっと薄くしていく）
		//   t=0.0 のとき 1.0（完全不透明）
		//   t=1.0 のとき 0.0（完全透明）
		deathAlpha_ = 1.0f - t;
		object_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });
		object_->Update();

		if (deathTimer_ >= deathDuration_) {
			// 敵が完全に消える瞬間に専用エフェクトを出す
			ParticleManager* pm = ParticleManager::GetInstance();
			Vector3 emitPos = GetWorldPosition(); // 敵の現在ワールド座標

			// 中心でフッと光るコア
			pm->Emit("enemyDeath_core", emitPos, 1);
			// バラバラに飛び散る破片
			pm->Emit("enemyDeath_shard", emitPos, 18);  // 数はお好みで 12〜24 くらい
			// ふわっと残る煙
			pm->Emit("enemyDeath_smoke", emitPos, 6);   // ちょっとだけ

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

	if (ImGui::DragFloat3("Position", &pos.x, 0.01f)) {
		object_->SetTranslate(pos);
	}
	if (ImGui::DragFloat3("Rotation", &rot.x, 0.01f)) {
		object_->SetRotate(rot);
	}
	if (ImGui::DragFloat3("Scale", &scale.x, 0.01f)) {
		object_->SetScale(scale);
	}

	ImGui::Text("N_EnemyHP: %d", hp_);
	ImGui::Text("Dead: %s", isDead_ ? "true" : "false");

	ImGui::End();

#endif // USE_IMGUI
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
	// フェードをしっかり見せたいので 1.5秒に伸ばす
	deathDuration_ = 1.5f;
	deathAlpha_ = 1.0f;

	// ノックバック方向
	Vector3 dir = hitDir;
	if (MyMath::Length(dir) < 0.001f) {
		dir = { 0.0f, 0.0f, 1.0f };
	}
	dir = MyMath::Normalize(dir);

	// 1秒あたりどれくらい飛ぶか（ここは今の感覚が良ければそのままでOK）
	deathVelocity_ = dir * 4.0f;
	// 回転速度も dt 前提（今の値で問題なければそのままでOK）
	deathRotateSpeed_ = { 1.5f, 2.0f, 0.8f };
}