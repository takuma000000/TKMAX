#include "BossManager.h"
#include "GameScene.h"
#include <algorithm>
#include "MyMath.h"

// ワールド座標をスクリーンUV座標に変換する
static Vector2 WorldToUV(const Vector3& world, const Matrix4x4& vp) {
	float clipX = world.x * vp.m[0][0] + world.y * vp.m[1][0] + world.z * vp.m[2][0] + 1.0f * vp.m[3][0];
	float clipY = world.x * vp.m[0][1] + world.y * vp.m[1][1] + world.z * vp.m[2][1] + 1.0f * vp.m[3][1];
	float clipW = world.x * vp.m[0][3] + world.y * vp.m[1][3] + world.z * vp.m[2][3] + 1.0f * vp.m[3][3];

	if (fabsf(clipW) < 0.0001f) { return { 0.5f, 0.5f }; }

	float ndcX = clipX / clipW;
	float ndcY = clipY / clipW;

	return { ndcX * 0.5f + 0.5f, -ndcY * 0.5f + 0.5f };
}

namespace { // 無名名前空間
	BossManager::BossBattleConfig MakeBossConfig() {
		// ボス戦設定生成
		BossManager::BossBattleConfig c_{}; // 設定構造体
		c_.spawnPos_ = { 0.0f, 0.0f, 200.0f }; // スポーン位置
		c_.arenaMin_ = { -18.0f, 3.0f, 35.0f }; // アリーナ最小座標
		c_.arenaMax_ = { 18.0f,12.0f, 70.0f }; // アリーナ最大座標
		// 撃破時波紋エフェクト設定
		TKM::WaterRippleEffect::RippleDesc d_{}; // 波紋設定構造体
		d_.duration_ = 0.35f; // 継続秒
		d_.radiusMax_ = 1.45f; // 最大半径(UV)
		d_.amplitude_ = 0.10f; // ゆがみ量
		d_.frequency_ = 85.0f; // 細かさ
		d_.width_ = 10.0f; // 帯の幅（大きいほどシャープ）
		c_.killRipple_ = d_; // 波紋設定代入
		c_.killSlowScale_ = 0.00001f; // 撃破時スローモーション倍率
		c_.killSlowDuration_ = 1.7f; // 撃破時スローモーション継続秒
		return c_;
	}
	// 定数ボス戦設定
	const BossManager::BossBattleConfig kBossConfig_ = MakeBossConfig();

	static float ClampFloat(float v, float mn, float mx) {
		if (v < mn) return mn;
		if (v > mx) return mx;
		return v;
	}

	// AABB(center,size) vs Sphere(center,radius)
	static bool TestAABBSphere(const Vector3& aabbCenter, const Vector3& aabbSize, const Vector3& sphereCenter, float sphereRadius) {
		const float hx = aabbSize.x * 0.5f;
		const float hy = aabbSize.y * 0.5f;
		const float hz = aabbSize.z * 0.5f;

		const float minX = aabbCenter.x - hx;
		const float maxX = aabbCenter.x + hx;
		const float minY = aabbCenter.y - hy;
		const float maxY = aabbCenter.y + hy;
		const float minZ = aabbCenter.z - hz;
		const float maxZ = aabbCenter.z + hz;

		const float cx = ClampFloat(sphereCenter.x, minX, maxX);
		const float cy = ClampFloat(sphereCenter.y, minY, maxY);
		const float cz = ClampFloat(sphereCenter.z, minZ, maxZ);

		const float dx = sphereCenter.x - cx;
		const float dy = sphereCenter.y - cy;
		const float dz = sphereCenter.z - cz;

		return (dx * dx + dy * dy + dz * dz) <= (sphereRadius * sphereRadius);
	}
}

void BossManager::Initialize(TKM::DirectXCommon* dxCommon, TKM::Camera* camera, TKM::BaseScene* parent, Player* player) {
	dxCommon_ = dxCommon;
	camera_ = camera;
	parentScene_ = parent;
	player_ = player;

	bossBattle_ = false; // ボス戦開始フラグ
	bossP2BgmPlayed_ = false; // ボスP2BGM再生フラグ
	boss_.reset(); // ボスオブジェクト
	bossBullets_.clear(); // ボス弾リスト

	slashAttackId_ = 0; // スラッシュ攻撃ID初期化
	currentSlashId_ = -1; // 現在のスラッシュ攻撃ID初期化
	slashIdHoldT_ = 0.0f; // スラッシュ攻撃IDホールド時間初期化

	// HPバーUI初期化
	hpUI_ = std::make_unique<TKM::BossHpBarUI>();
	TKM::BossHpBarUI::Desc d{}; // デフォルト設定
	hpUI_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, parentScene_, d);
	hpUI_->SetVisible(false); // 非表示開始
}

void BossManager::StartBattle() {
	if (bossBattle_) { // すでにボス戦中
		return;
	}
	if (!dxCommon_ || !camera_ || !parentScene_) { // 安全確認
		return;
	}

	// --- ボス本体生成 ---
	bossBattle_ = true; // ボス戦開始フラグセット
	bossP2BgmPlayed_ = false; // P2BGM再生フラグリセット

	// ボス生成
	boss_ = std::make_unique<BossEnemy>();
	boss_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);
	boss_->SetCamera(camera_);
	boss_->SetParentScene(parentScene_); // 親シーンセット

	// プレイヤー位置取得ラムダ
	if (player_) {
		boss_->SetPlayer([this]() { // ラムダ式でプレイヤー位置取得
			return player_->GetPosition();
			});
	}
	// ボス初期位置セット
	boss_->SetPosition(kBossConfig_.spawnPos_);

	// --- ボス挙動コントローラ生成 ---
	bossController_ = std::make_unique<BossController>();
	bossController_->Initialize(kBossConfig_.arenaMin_, kBossConfig_.arenaMax_);

	killSeq_.Reset(); // 撃破シーケンス状態リセット

	if (hpUI_) { // HPバーUI表示
		hpUI_->SetVisible(true); // 表示ON
	}
	if (player_) {
		player_->SetShootingEnabled(true); // ボス戦開始で射撃許可
		player_->SetRumbleEnabled(true); // ボス戦開始でコントローラー振動許可
	}
}

void BossManager::Update(float dt) {
	if (!bossBattle_ || !boss_) { // ボス戦未開始またはボス不在
		UpdateBossBullets(); // ボス弾更新のみ行う
		return;
	}

	if (slashIdHoldT_ > 0.0f) {
		slashIdHoldT_ -= dt;
		if (slashIdHoldT_ < 0.0f) { slashIdHoldT_ = 0.0f; }
	}

	// ================================
	// 撃破演出中：ボスの攻撃を完全停止
	// ================================
	if (boss_ && (boss_->IsDying() || boss_->IsDead())) { // ボスが死亡リアクション中 or 死亡している とき
		// 撃破シーケンス中はプレイヤーの攻撃を止める
		player_->SetShootingEnabled(false);
		// 撃破シーケンス中はコントローラー振動も止める
		player_->SetRumbleEnabled(false);
		// 1回だけ：残ってる弾を消して、以降当たり判定も出さない
		if (!killSeq_.attacksStopped_) {
			bossBullets_.clear(); // 既に出てる弾も全消し
			killSeq_.attacksStopped_ = true; // フラグセット
		}
		// HPバーUI更新（HP減少演出のためにUpdateは呼ぶ）
		if (hpUI_ && boss_) {
			hpUI_->Update(dt, boss_.get()); // HPバーUI更新
		}
		boss_->Update(dt); // ボス本体更新（死亡リアクションのためにUpdateは呼ぶ）
		// 撃破ズーム/スロー開始（既存処理を活かす）
		if (!killSeq_.zoomStarted_ && boss_->IsDying()) { // ボス撃破リアクション開始時
			if (player_) { player_->StartBossDeathCameraZoom(); } // 撃破ズーム開始
			killSeq_.zoomStarted_ = true; // フラグセット

			if (!killSeq_.slowTriggered_ && timeScale_) { // スローモーション開始
				timeScale_->RequestSlow(kBossConfig_.killSlowScale_, kBossConfig_.killSlowDuration_); // 撃破時スローモーションリクエスト
				killSeq_.slowTriggered_ = true; // フラグセット
			}
		}
		return;
	}

	if (bossController_) {
		bossController_->Update(dt, *boss_); // ボス挙動コントローラ更新
	}
	if (boss_ && bossController_) { // ボス触手チャージ演出更新
		boss_->SetTentacleCharge(
			bossController_->IsAnyCharging(), // いずれかの攻撃をチャージ中か？
			bossController_->GetCharge01() // チャージ量0-1
		);
	}
	// --- Missile（通常時攻撃その1） ---
	{
		Vector3 mPos_{}; // 発射位置
		Vector3 mTarget_{}; // ターゲット位置
		float mSpeed_ = 0.0f; // 移動速度
		float mCurveH_ = 0.0f; // 曲線の高さ（0で直線、正で右カーブ、負で左カーブ）
		int mDmg_ = 0; // ダメージ量
		int mLife_ = 0; // 生存フレーム数

		if (bossController_->ConsumeMissileFireRequest(mPos_, mTarget_, mSpeed_, mCurveH_, mDmg_, mLife_)) {
			// BossBulletは「speedが1フレ移動量」なので dt掛けた値を渡す（君が既にやってるやつ）
			const float spPerFrame_ = mSpeed_ * dt;
			auto bullet_ = std::make_unique<BossBullet>(); // 弾オブジェクト生成
			// dirはInitializeのために一応入れる（曲線モードでは使われない）
			Vector3 dir_{ mTarget_.x - mPos_.x, mTarget_.y - mPos_.y, mTarget_.z - mPos_.z };
			dir_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f });

			bullet_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_, camera_, mPos_, dir_, spPerFrame_, mDmg_, mLife_);
			// 曲線設定（好みで調整OK）
			bullet_->SetCurveYaw(0.05f); // 1フレームあたりの曲がる角度（ラジアン）
			bullet_->EnableCurveToTarget(mPos_, mTarget_, mCurveH_, spPerFrame_); // 曲線で終点へ

			bossBullets_.push_back(std::move(bullet_));
		}
	}
	// --- SlashWave（通常時攻撃その2） ---
	{
		Vector3 sPos_{};
		Vector3 sTarget_{};
		float sSpeed_ = 0.0f;
		int sDmg_ = 0;
		int sLife_ = 0;

		if (bossController_->ConsumeSlashFireRequest(sPos_, sTarget_, sSpeed_, sDmg_, sLife_)) {
			const float spPerFrame_ = sSpeed_ * dt;

			if (slashIdHoldT_ <= 0.0f) {
				currentSlashId_ = ++slashAttackId_; // ★このタイミングで「今回の斬撃ID」を確定
			}
			slashIdHoldT_ = 0.5f; // 斬撃の長さに合わせて(0.2〜0.5くらい)

			auto bullet_ = std::make_unique<BossBullet>();

			Vector3 dir_{ sTarget_.x - sPos_.x, sTarget_.y - sPos_.y, sTarget_.z - sPos_.z };
			dir_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f });

			bullet_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_, camera_, sPos_, dir_, spPerFrame_, 1, sLife_);
			bullet_->SetModel("sphere.obj");
			bullet_->SetScale({ 3.8f, 0.7f, 1.2f });
			bullet_->SetFxType(BossBullet::FxType::SlashWave);

			bullet_->SetAttackId(currentSlashId_); // ★斬撃IDセット

			bossBullets_.push_back(std::move(bullet_));
		}
	}
	// HPバーUI更新
	if (hpUI_ && boss_) { // HPバーUI更新
		hpUI_->Update(dt, boss_.get()); // ボスのHP情報を反映
	}

	// ボス本体更新
	boss_->Update(dt);

	// ボス撃破ズーム開始（1回だけ）
	if (!killSeq_.zoomStarted_ && boss_->IsDying()) { // ボス撃破リアクション開始時
		if (player_) { // プレイヤー存在確認
			player_->StartBossDeathCameraZoom(); // 撃破ズーム開始
		}
		killSeq_.zoomStarted_ = true; // フラグセット

		// 波紋
		/*if (!killSeq_.rippleTriggered && waterRipple_ && camera_) {
			Matrix4x4 vp = camera_->GetViewProjectionMatrix();
			Vector2 uv = WorldToUV(boss_->GetWorldPosition(), vp);
			waterRipple_->Trigger(uv, kBossConfig.killRipple);
			killSeq_.rippleTriggered = true;
		}*/

		// スロー
		if (!killSeq_.slowTriggered_ && timeScale_) { // タイムスケールコントローラ存在確認
			timeScale_->RequestSlow(kBossConfig_.killSlowScale_, kBossConfig_.killSlowDuration_); // スロー要求
			killSeq_.slowTriggered_ = true; // フラグセット
		}
	}
	// ボス弾更新
	UpdateBossBullets();

	if (bossController_ && boss_) {
		bossController_->ImGuiDebug(*boss_); // デバッグ表示
	}
}

void BossManager::Draw(TKM::DirectXCommon* dxCommon) {
	if (!bossBattle_ || !boss_) { return; } // ボス戦未開始またはボス不在

	boss_->Draw(dxCommon); // ボス本体描画

	//for (auto& b : bossBullets_) { // ボス弾描画
	//	b->Draw(dxCommon); // 描画
	//}
}

void BossManager::DrawUI() {
	// もしボス戦中ならHPバーUIも描画
	if (hpUI_ && bossBattle_ && boss_) { // HPバーUI描画
		hpUI_->Draw(); // 描画
	}
}

void BossManager::SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame) {
	if (!dxCommon_ || !camera_) { // 安全確認
		return;
	}

	auto bullet_ = std::make_unique<BossBullet>(); // 弾オブジェクト生成
	bullet_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_, camera_, pos, dir, speed, damage, lifeFrame); // 初期化
	bossBullets_.push_back(std::move(bullet_)); // リストに追加
}

bool BossManager::IsBattleActive() const {
	return bossBattle_ && boss_ != nullptr && !boss_->IsDead(); // ボス戦中かつボス生存中
}

bool BossManager::IsBossAlive() const {
	return boss_ && !boss_->IsDead(); // ボス存在かつ生存中
}

bool BossManager::IsBossDead() const {
	return boss_ && boss_->IsDead(); // ボス存在かつ死亡している
}

void BossManager::OnClearSequenceStart() {
	bossBattle_ = false; // ボス戦終了フラグセット
	bossBullets_.clear(); // ボス弾リストクリアa
	boss_.reset(); // ボスオブジェクト破棄
	bossController_.reset(); // ボス挙動コントローラ破棄
	bossP2BgmPlayed_ = false; // P2BGM再生フラグリセット
}

void BossManager::SetCamera(TKM::Camera* camera) {
	camera_ = camera; // カメラセット
	if (boss_) { boss_->SetCamera(camera_); } // ボス本体にカメラセット
	for (auto& b : bossBullets_) { b->SetCamera(camera_); } // ボス弾すべてにカメラセット
}

void BossManager::UpdateBossBullets() {
	for (auto it = bossBullets_.begin(); it != bossBullets_.end();) { // ボス弾更新ループ
		BossBullet* b_ = it->get();

		b_->Update(); // 更新（位置が動くので先に更新）

		// ───────── Player × BossBullet 当たり判定 ─────────
		if (player_ && !player_->IsDead() && !b_->IsDead()) {
			const Vector3 pCenter_ = player_->GetPosition();
			const Vector3 pSize_ = player_->GetColliderScale();

			if (b_->GetFxType() == BossBullet::FxType::SlashWave) {
				if (b_->HitTestSlashX(pCenter_, pSize_)) {

					// ダメージは1回だけ（通らないなら減らない）
					player_->TryDamageFromAttack(b_->Damage(), b_->GetAttackId());

					// でも弾は当たった瞬間に必ず消す
					b_->Kill();
				}
			} else {
				// ミサイル等：従来どおり AABB×Sphere
				const Vector3 sCenter_ = b_->GetPos();
				const float   sR_ = b_->Radius();

				if (TestAABBSphere(pCenter_, pSize_, sCenter_, sR_)) {
					player_->Damage(b_->Damage());
					b_->Kill();
				}
			}
		}

		if (b_->IsDead()) { // 死亡していたらリストから削除
			it = bossBullets_.erase(it);
		} else {
			++it;
		}
	}
}