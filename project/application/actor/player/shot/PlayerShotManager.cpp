#include "PlayerShotManager.h"

#include "Player.h"
#include "Enemy.h"
#include "BarrierCore.h"
#include "BarrierCoreManager.h"
#include "AABB.h"
#include "MyMath.h"

#include <limits>

void PlayerShotManager::Initialize(Player* owner, TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	// 所有者であるPlayer参照を保持する
	owner_ = owner;

	// Object3d共通情報を保持する
	common_ = common;

	// DirectX共通情報を保持する
	dxCommon_ = dxCommon;

	//=========================================================
	// LB弾状態初期化
	//=========================================================

	// LB弾数を初期化する
	lbAmmo_ = 0;

	// LB未発射時間タイマーを初期化する
	lbNoFireTimer_ = 0.0f;
}

void PlayerShotManager::Update(float dt, bool canShoot) {
	// 死亡済みターゲット参照を整理する
	RemoveDeadTargets();

	// 射撃可能ならロック状態と射撃処理を更新する
	if (canShoot && shootingEnabled_) {
		UpdateLockState_();
		HandleShooting_(dt);
	} else {
		// 射撃不可ならロック状態を解除する
		ClearLockState();
	}

	//=========================================================
	// 通常弾更新
	//=========================================================
	for (auto it = bullets_.begin(); it != bullets_.end();) {
		// 弾を更新する
		(*it)->Update();

		// 死亡済みの弾はリストから削除する
		if ((*it)->IsDead()) {
			// 死亡した弾は削除せず、待機プールへ戻す
			bulletPool_.push_back(std::move(*it));

			// 使用中リストからは外す
			it = bullets_.erase(it);
		} else {
			++it;
		}
	}

	//=========================================================
	// ホーミング弾更新
	//=========================================================
	for (auto it = homingBullets_.begin(); it != homingBullets_.end();) {
		// ホーミング弾を更新する
		(*it)->Update();

		// 死亡済みのホーミング弾はリストから削除する
		if ((*it)->IsDead()) {
			// 死亡したホーミング弾は削除せず、待機プールへ戻す
			homingBulletPool_.push_back(std::move(*it));

			// 使用中リストからは外す
			it = homingBullets_.erase(it);
		} else {
			++it;
		}
	}
}

void PlayerShotManager::UpdateLockState_() {
	// 入力管理を取得する
	TKM::Input* input = TKM::Input::GetInstance();

	// 現在のロック対象候補を取得する
	Enemy* cur = (enemy_ && !enemy_->IsDead()) ? enemy_ : nullptr;

	// RTが押されていて、かつ特殊攻撃が使用可能ならロック状態にする
	bool hold = (input->GetRightTrigger() > kTriggerThreshold) && (canUseSpecial_ || debugUnlimitedSpecial_);

	// 前回ロックしていた敵と現在の敵が違うなら、前回の敵のロックを外す
	if (lastLockedEnemy_ && lastLockedEnemy_ != cur) {
		lastLockedEnemy_->SetLocked(false);
	}

	// 現在の敵がいてロック入力中ならロック表示をONにする
	if (cur && hold) {
		cur->SetLocked(true);
		lastLockedEnemy_ = cur;
	} else {
		// ロック条件を満たさないなら現在敵のロックを外す
		if (cur) {
			cur->SetLocked(false);
		}

		// 最後にロックしていた敵情報を消す
		lastLockedEnemy_ = nullptr;
	}
}

void PlayerShotManager::ClearLockState() {
	// 最後にロックしていた敵がいればロック表示を解除する
	if (lastLockedEnemy_) {
		lastLockedEnemy_->SetLocked(false);
		lastLockedEnemy_ = nullptr;
	}
}

void PlayerShotManager::RemoveDeadTargets() {
	// 現在ターゲットの敵が死んでいたら参照を外す
	if (enemy_ && enemy_->IsDead()) {
		enemy_ = nullptr;
	}

	// 現在ターゲットのコアが死んでいたら参照を外す
	if (core_ && core_->IsDead()) {
		core_ = nullptr;
	}
}

void PlayerShotManager::SetShootingEnabled(bool enabled) {
	// 射撃有効フラグを更新する
	shootingEnabled_ = enabled;

	// 射撃を無効にする場合は射撃関連状態をまとめてリセットする
	if (!enabled) {
		// RT/LTの押しっぱなし状態を解除する
		rtHeld_ = false;
		ltHeld_ = false;

		// ロック状態を解除する
		ClearLockState();

		// 画面上の通常弾を待機プールへ戻す
		while (!bullets_.empty()) {
			// 死亡した弾は削除せず、待機プールへ戻す
			bulletPool_.push_back(std::move(bullets_.front()));
			bullets_.pop_front(); // 管理リストからは外す
		}

		// 画面上のホーミング弾を待機プールへ戻す
		while (!homingBullets_.empty()) {
			// 死亡したホーミング弾は削除せず、待機プールへ戻す
			homingBulletPool_.push_back(std::move(homingBullets_.front()));
			homingBullets_.pop_front(); // 管理リストからは外す
		}

		// RB発射クールダウンをリセットする
		rbShotCooldownTimer_ = 0.0f;

		// LB未発射タイマーをリセットする
		lbNoFireTimer_ = 0.0f;
	}
}

void PlayerShotManager::DrawTrails(TKM::DirectXCommon* dxCommon) {
	// 通常弾のトレイルを描画する
	for (auto& bullet : bullets_) {
		if (!bullet) { continue; }
		bullet->DrawTrail(dxCommon);
	}

	// ホーミング弾のトレイルを描画する
	for (auto& bullet : homingBullets_) {
		if (!bullet) { continue; }
		bullet->DrawTrail(dxCommon);
	}
}

void PlayerShotManager::DrawBullets(TKM::DirectXCommon* dxCommon) {
	// 通常弾を描画する
	for (auto& bullet : bullets_) {
		if (!bullet) { continue; }
		bullet->Draw(dxCommon);
	}
}

void PlayerShotManager::OnEnemyDestroyed(Enemy* e) {
	// 現在ターゲットが破壊された敵なら参照を外す
	if (enemy_ == e) {
		enemy_ = nullptr;
	}

	// 通常弾が持っている敵参照を整理する
	for (auto& b : bullets_) {
		if (!b) { continue; }
		if (b->GetEnemy() == e) {
			b->SetEnemy(nullptr);
		}
	}

	// ホーミング弾が持っている敵参照を整理する
	for (auto& b : homingBullets_) {
		if (!b) { continue; }
		if (b->GetEnemy() == e) {
			b->SetEnemy(nullptr);
		}
	}
}

void PlayerShotManager::OnBarrierCoreDestroyed(BarrierCore* core) {
	// 無効なコアなら何もしない
	if (!core) {
		return;
	}

	// 通常弾が持っているコア参照を外す
	for (auto& b : bullets_) {
		if (!b) { continue; }
		b->SetCore(nullptr);
	}

	// ホーミング弾が持っているコア参照を外す
	for (auto& b : homingBullets_) {
		if (!b) { continue; }
		b->SetCore(nullptr);
	}

	// 現在ターゲットのコア参照も外す
	core_ = nullptr;
}

void PlayerShotManager::SetConfig(const PlayerShotConfig* config) {
	// 設定データ参照を保持する
	config_ = config;

	// 設定が無ければ停止する
	assert(config_ && "PlayerShotConfig が未設定です");

	// LB弾数を設定値の最大値で初期化する
	lbAmmo_ = std::max(0, config_->GetLB().ammoMax_);
	// LB未発射タイマーをリセットする
	lbNoFireTimer_ = 0.0f;
	// RB発射クールダウンをリセットする
	rbShotCooldownTimer_ = 0.0f;
}

void PlayerShotManager::HandleShooting_(float dt) {
	// 設定が無ければ停止する
	assert(config_ && "PlayerShotConfig が未設定です");

	// LB設定を取得する
	const PlayerShotConfig::LBConfig& lb = config_->GetLB();

	//=========================================================
	// LB弾 自動満タン回復
	//=========================================================
	{
		// 無限LBでなく、弾数が最大未満なら回復待ち時間を進める
		if (!debugUnlimitedLB_ && lbAmmo_ < std::max(1, lb.ammoMax_)) {
			lbNoFireTimer_ += dt;

			// 一定時間経過で満タンまで回復する
			if (lbNoFireTimer_ >= lb.refillWaitSec_) {
				lbAmmo_ = std::max(1, lb.ammoMax_);
				lbNoFireTimer_ = 0.0f;
			}
		} else {
			// 回復不要なら待ち時間をリセットする
			lbNoFireTimer_ = 0.0f;
		}
	}

	//=========================================================
	// RB弾クールダウン更新
	//=========================================================
	if (rbShotCooldownTimer_ > 0.0f) { // クールダウン中ならクールダウンを減らす
		rbShotCooldownTimer_ -= dt;
		// 0未満にならないようにする
		if (rbShotCooldownTimer_ < 0.0f) {
			rbShotCooldownTimer_ = 0.0f;
		}
	}

	// RB弾発射処理
	RBShoot_();
	// LB弾発射処理
	LBShoot_();
}

void PlayerShotManager::RBShoot_() {
	// 設定が無ければ停止する
	assert(config_ && "PlayerShotConfig が未設定です");

	// RB設定を取得する
	const PlayerShotConfig::RBConfig& rb = config_->GetRB();

	// 入力管理を取得する
	TKM::Input* input = TKM::Input::GetInstance();

	// RBボタン入力状態を取得する
	const bool padRB = input->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER);

	// キーボードK入力状態を取得する
	const bool keyK = input->PushKey(DIK_K);

	// 入力が無ければ発射しない
	if (!padRB && !keyK) {
		return;
	}

	// クールダウン中なら発射しない
	if (rbShotCooldownTimer_ > 0.0f) {
		return;
	}

	// 発射元オブジェクトが無ければ発射しない
	if (!ownerObject_) {
		return;
	}

	//=========================================================
	// RB弾生成
	//=========================================================
	
	// ObjectPoolから弾を取得する。空いていなければ新規生成する。
	std::unique_ptr<PlayerBullet> bullet;

	// 待機プールに空きがあればそこから取り出す
	if (!bulletPool_.empty()) {
		// 待機プールから弾を取り出す
		bullet = std::move(bulletPool_.front());
		bulletPool_.pop_front(); // プールから取り出した弾は再利用する
		// 再利用用に状態を初期化する
		bullet->ResetForReuse();
	} else {
		// プールに空きが無い場合だけ新しく生成する
		bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize(common_, dxCommon_);
		bullet->ResetForReuse(); // 新規生成でも状態を初期化する
	}

	// 発射開始位置を取得する
	Vector3 startPos = ownerObject_->GetTranslate();

	// 弾の初期位置を設定する
	bullet->SetPosition(startPos);

	// デフォルト発射方向はZ+
	Vector3 dir = { 0, 0, 1 };

	// レティクルがある場合はレティクルの狙い方向を使う
	if (reticle_) {
		dir = reticle_->GetAimDirection();
		float len = MyMath::Length(dir);

		// 方向が極端に短い場合はZ+を使う
		if (len <= 0.01f) {
			dir = { 0, 0, 1 };
		}
	}

	// 弾速度を設定する
	bullet->SetVelocity(dir * rb.bulletSpeed_);

	// カメラ参照を設定する
	bullet->SetCamera(camera_);

	// プレイヤー参照を設定する
	bullet->SetPlayer(owner_);

	// RB弾では通常トレイルを使わない
	bullet->SetUseTrail(false);

	// バリアコアマネージャー参照を渡す
	bullet->SetBarrierCoreManager(barrierCoreManager_);

	//=========================================================
	// レイ上のターゲット敵検索
	//=========================================================

	// 発射方向上で最も近い敵を入れる
	Enemy* targetEnemy = nullptr;

	if (allEnemies_) {
		// レイ方向を正規化する
		Vector3 rayDir = dir;
		float len = MyMath::Length(rayDir);
		if (len > 0.001f) {
			rayDir = rayDir / len;
		}

		// レイ終点を十分遠くに設定する
		Vector3 rayEnd = startPos + rayDir * 150.0f;

		// 最も近い交差敵の距離
		float closestDist = std::numeric_limits<float>::max();

		// 全敵からレイに当たる敵を探す
		for (auto& e : *allEnemies_) {
			if (!e) continue;
			if (e->IsDead() || e->IsDying()) continue;

			// 敵AABBを作る
			Vector3 center = e->GetWorldPosition();
			Vector3 size = e->GetColliderScale();
			AABB box(center, size);

			// レイが敵AABBに交差しているか確認する
			if (box.IsIntersectSegment(startPos, rayEnd)) {
				float dist = MyMath::Length(center - startPos);

				// より近い敵を採用する
				if (dist < closestDist) {
					closestDist = dist;
					targetEnemy = e.get();
				}
			}
		}
	}

	// レイ上に敵がいなければ、現在の敵参照を使う
	if (!targetEnemy) {
		if (enemy_ && !enemy_->IsDead()) {
			targetEnemy = enemy_;
		}
	}

	// 弾にターゲット敵を設定する
	bullet->SetEnemy(targetEnemy);

	// 弾リストへ追加する
	bullets_.push_back(std::move(bullet));

	// 発射クールダウンを開始する
	rbShotCooldownTimer_ = rb.shotCooldownSec_;
}

void PlayerShotManager::LBShoot_() {
	// 設定が無ければ停止する
	assert(config_ && "PlayerShotConfig が未設定です");

	// LB設定を取得する
	const PlayerShotConfig::LBConfig& lb = config_->GetLB();

	// 入力管理を取得する
	TKM::Input* input = TKM::Input::GetInstance();

	//=========================================================
	// LB弾発射入力
	//=========================================================
	if ((input->TriggerButton(XINPUT_GAMEPAD_LEFT_SHOULDER) || input->TriggerKey(DIK_L)) && !ltHeld_) {

		// 無限LBでない場合、弾数0なら発射しない
		if (!debugUnlimitedLB_ && lbAmmo_ <= 0) {
			return;
		}

		// 発射元オブジェクトが無ければ発射しない
		if (!ownerObject_) {
			return;
		}

		//=====================================================
		// LBホーミング弾生成
		//=====================================================

		// ObjectPoolからホーミング弾を取得する。空いていなければ新規生成する。
		std::unique_ptr<HomingBullet> bullet;

		// 待機プールに空きがあればそこから取り出す
		if (!homingBulletPool_.empty()) {
			// 待機プールからホーミング弾を取り出す
			bullet = std::move(homingBulletPool_.front());
			homingBulletPool_.pop_front(); // プールから取り出したホーミング弾は再利用する

			// 再利用用に状態を初期化する
			bullet->ResetForReuse();
		} else {
			// プールに空きが無い場合だけ新しく生成する
			bullet = std::make_unique<HomingBullet>();
			bullet->Initialize(common_, dxCommon_);
			bullet->ResetForReuse(); // 新規生成でも状態を初期化する
		}

		// 発射位置を取得する
		Vector3 start = ownerObject_->GetTranslate();

		// 前方固定の仮終点を作る
		Vector3 end = start + Vector3{ 0.0f, 0.0f, lb.forwardOffsetZ_ };

		// コアがあれば最優先で終点にする
		if (core_ && !core_->IsDead()) {
			end = core_->GetWorldPosition();
		} else if (enemy_ && !enemy_->IsDead()) {
			// コアが無ければ敵を終点にする
			end = enemy_->GetWorldPosition();
		}

		//=====================================================
		// 山なり弾道制御点作成
		//=====================================================

		// 開始点から終点までの水平差分を作る
		Vector3 flat = end - start;
		flat.y = 0.0f;

		// 水平方向距離を取得する
		float flatLen = MyMath::Length(flat);

		// 基本前方向
		Vector3 forward = { 0.0f, 0.0f, 1.0f };

		// 距離があれば終点方向を前方向として使う
		if (flatLen > 0.001f) {
			forward = flat / flatLen;
		}

		// 山なり高さを計算する
		float arcHeight = std::clamp(flatLen * 0.25f, 6.0f, lb.arcHeight_);

		// 1つ目の制御点を作る
		Vector3 c1 = start + forward * (flatLen * 0.25f) + Vector3{ 0.0f, arcHeight, 0.0f };

		// 2つ目の制御点を作る
		Vector3 c2 = end - forward * (flatLen * 0.20f) + Vector3{ 0.0f, arcHeight * 0.85f, 0.0f };

		//=====================================================
		// 弾設定
		//=====================================================

		// 弾の初期位置を設定する
		bullet->SetPosition(start);

		// 現在の敵参照を設定する
		bullet->SetEnemy(enemy_);

		// カメラ参照を設定する
		bullet->SetCamera(camera_);

		// プレイヤー参照を設定する
		bullet->SetPlayer(owner_);

		// 現在のコア参照を設定する
		bullet->SetCore(core_);

		// 山なり弾道を開始する
		bullet->StartArc(start, c1, c2, end, 0.4f);

		// ホーミング弾リストへ追加する
		homingBullets_.push_back(std::move(bullet));

		// ホーミング弾発射時の集中線をリクエストする
		if (owner_) {
			owner_->RequestHomingSpeedLine();
		}

		// 無限LBでないなら弾数を1減らす
		if (!debugUnlimitedLB_) {
			lbAmmo_ = std::max(0, lbAmmo_ - 1);
		}

		// LB未発射タイマーをリセットする
		lbNoFireTimer_ = 0.0f;

		// プレイヤー側へズームと振動を要求する
		if (owner_) {
			owner_->ZoomCamera();
			owner_->StartRumble(0.12f, 42000, 42000);
		}

		// 押しっぱなしによる連続発射を防ぐ
		ltHeld_ = true;
	}

	// LB入力を離したらホールド状態を解除する
	if (!input->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER) && !input->PushKey(DIK_L)) {
		ltHeld_ = false;
	}
}