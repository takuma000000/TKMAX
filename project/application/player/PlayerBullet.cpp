#define NOMINMAX
#include "PlayerBullet.h"
#include <engine/effect/particle/ParticleManager.h>
#include "AABB.h"
#include "Player.h"

void PlayerBullet::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	// 3Dオブジェクト作成
	object_ = std::make_unique<Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");
	object_->SetScale({ 0.2f, 0.2f, 0.2f });

	// 既定グループで一旦初期化（あとで SetTrailGroup で上書き可）
	Vector3 start = object_->GetTranslate();
	trailEmitter_.Initialize(trailGroup_, start);
}

void PlayerBullet::Update() {
	// 現在の座標を取得して、速度分だけ進める
	Vector3 pos = object_->GetTranslate();

	// ▼ 完全追従：毎フレーム、目標の現在位置へ向けて速度ベクトルを再設定
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

		// AABB同士の当たり判定を行う
		if (bulletBox.IsCollidingWithAABB(enemyBox)) { // 当たった場合
			isHit_ = true; // デバッグ用フラグ
			isDead_ = true; // 弾を削除

			// パーティクル発生
			ParticleManager::GetInstance()->Emit("uv", bulletPos, 30); // 衝突位置にパーティクルを発生

			// 正しい順序：敵がまだ死んでない場合のみダメージ処理
			if (enemy_ && !enemy_->IsDead()) { // 敵が死んでなければダメージ処理
				if (isSpecialAttack_) { // 一撃必殺なら大ダメージ
					enemy_->OnHitWithDamage(100); // 引数分のダメージを与える
				} else { // 通常攻撃
					enemy_->OnHitWithDamage(1); // 引数分のダメージを与える
				}
			}

			// カメラシェイク
			if (player_) {
				player_->StartCameraShake(10); // 10フレーム間シェイク
			}

			return;
		}
	}

	// 一定距離（Z方向）を超えたら弾を削除する
	if (pos.z > 50.0f) {
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
