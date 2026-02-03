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

	// オーラボリュームレンダラー初期化
	auraVolume_ = std::make_unique<TKM::AuraVolumeRenderer>();
	auraVolume_->Initialize(dxCommon_);

	// LaserBeam3D 初期化
	laserBeam3D_ = std::make_unique<TKM::LaserBeam3D>();
	laserBeam3D_->Initialize(dxCommon_);
	laserBeam3D_->GetDesc().active_ = false; // 非アクティブ開始
	laserBeam3D_->GetDesc().telegraph_ = false; // 予告モード開始

	// 初期見た目（好みで調整OK）
	laserBeam3D_->GetDesc().color_ = { 0.2f, 0.85f, 1.0f }; // 色
	laserBeam3D_->GetDesc().intensity_ = 3.0f; // 明るさ
	laserBeam3D_->GetDesc().coreSharpness_ = 7.0f; // コアのシャープネス
	laserBeam3D_->GetDesc().edgeSoftness_ = 1.2f; // エッジの柔らかさ
	laserBeam3D_->GetDesc().sliceCount_ = 64; // スライス数
	laserBeam3D_->GetDesc().noiseScale_ = 1.0f; // ノイズの細かさ
	laserBeam3D_->GetDesc().noiseSpeed_ = 1.0f; // ノイズの速さ

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
}

void BossManager::Update(float dt) {
	if (!bossBattle_ || !boss_) { // ボス戦未開始またはボス不在
		UpdateBossBullets(); // ボス弾更新のみ行う
		return;
	}

	if (bossController_) {
		bossController_->Update(dt, *boss_); // ボス挙動コントローラ更新
	}
	// --- Missile（通常時攻撃） ---
	{
		Vector3 mPos_{}; // 発射位置
		Vector3 mTarget_{};
		float mSpeed_ = 0.0f;
		float mCurveH_ = 0.0f;
		int mDmg_ = 0;
		int mLife_ = 0;

		if (bossController_->ConsumeMissileFireRequest(mPos_, mTarget_, mSpeed_, mCurveH_, mDmg_, mLife_)) {
			// BossBulletは「speedが1フレ移動量」なので dt掛けた値を渡す（君が既にやってるやつ）
			const float spPerFrame_ = mSpeed_ * dt;

			auto bullet_ = std::make_unique<BossBullet>();

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
	// --- LaserBeam 更新＆BossControllerのレーザー情報を反映 ---
	if (laserBeam3D_) { // LaserBeam3D 更新
		laserBeam3D_->Update(dt); // 更新
		// BossController からレーザー情報取得＆反映
		LaserInfo li = GetLaserInfo();
		// 描画用LaserBeam3Dに情報セット
		auto& d_ = laserBeam3D_->GetDesc();
		d_.active_ = li.active_; // 発射中/予告中フラグ
		d_.telegraph_ = li.telegraph_; // 予告中フラグ
		d_.startWS_ = li.startWS_; // 開始座標
		d_.endWS_ = li.endWS_; // 終了座標
		d_.radius_ = li.radius_; // 見た目の太さ＝当たり判定半径に一致させる
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

	// --- LaserBeam 描画（空間上） ---
	if (laserBeam3D_ && camera_) { // LaserBeam3D 描画
		const Matrix4x4& camW_ = camera_->GetWorldMatrix(); // カメラワールド行列取得

		// カメラの向きベクトル抽出
		Vector3 right_{ camW_.m[0][0], camW_.m[0][1], camW_.m[0][2] }; // 右方向 ベクトル
		Vector3 up_{ camW_.m[1][0], camW_.m[1][1], camW_.m[1][2] }; // 上方向 ベクトル
		Vector3 fwd_{ camW_.m[2][0], camW_.m[2][1], camW_.m[2][2] }; // 前方向 ベクトル
		// ビュープロジェクション行列取得
		Matrix4x4 vp_ = camera_->GetViewProjectionMatrix();
		laserBeam3D_->Draw(vp_, right_, up_, fwd_); // 描画
	}
}

void BossManager::DrawUI() {
	if (hpUI_ && bossBattle_ && boss_ && !boss_->IsDead()) { // HPバーUI描画
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

BossManager::LaserInfo BossManager::GetLaserInfo() const {
	// BossController からレーザー情報取得
	LaserInfo li_{};
	if (!bossController_) { return li_; } // 安全確認
	li_.active_ = bossController_->IsLaserActive(); // 発射中/予告中フラグ
	li_.telegraph_ = bossController_->IsLaserTelegraph(); // 予告中フラグ
	li_.startWS_ = bossController_->GetLaserStartWS(); // 開始座標
	li_.endWS_ = bossController_->GetLaserEndWS(); // 終了座標
	li_.radius_ = bossController_->GetLaserRadius(); // 当たり判定半径
	return li_;
}

bool BossManager::TestLaserHit(const LaserInfo& laser, const Vector3& sphereCenterWS, float sphereRadius) {
	if (!laser.active_) { return false; } // レーザー非アクティブ時は当たらない
	// レーザー（カプセル）と球体の当たり判定テスト
	const Vector3 p0_ = laser.startWS_; // レーザー開始点
	const Vector3 p1_ = laser.endWS_; // レーザー終了点
	const Vector3 c_ = sphereCenterWS; // 球体中心座標
	// レーザー線分ベクトル
	Vector3 d_ = MyMath::Subtract(p1_, p0_);
	float dlen2_ = MyMath::Dot(d_, d_);

	if (dlen2_ < 1e-6f) { // レーザーがほぼ点の場合
		Vector3 dc_ = MyMath::Subtract(c_, p0_); // 球体中心からレーザー点へのベクトル
		float dist2_ = MyMath::Dot(dc_, dc_); // 距離の二乗
		float r_ = laser.radius_ + sphereRadius; // 合計半径
		return dist2_ <= r_ * r_; // 当たっているか？
	}
	// レーザー線分上の最近接点を求める
	float t_ = MyMath::Dot(MyMath::Subtract(c_, p0_), d_) / dlen2_;
	t_ = std::clamp(t_, 0.0f, 1.0f);
	// 最近接点座標
	Vector3 q_ = MyMath::Add(p0_, MyMath::Multiply(t_, d_));
	Vector3 cq_ = MyMath::Subtract(c_, q_);
	// 距離の二乗を計算して当たり判定
	float dist2_ = MyMath::Dot(cq_, cq_);
	float r_ = laser.radius_ + sphereRadius;
	return dist2_ <= r_ * r_;
}

bool BossManager::IsBattleActive() const {
	return bossBattle_ && boss_ != nullptr && !boss_->IsDead(); // ボス戦中かつボス生存中
}

bool BossManager::IsBossAlive() const {
	return boss_ && !boss_->IsDead(); // ボス存在かつ生存中
}

bool BossManager::IsBossDead() const {
	return boss_ && boss_->IsDead(); // ボス存在かつ死亡中

	if (hpUI_ && boss_ && boss_->IsDead()) { // ボス死亡時HPバーUI非表示
		hpUI_->SetVisible(false); // 非表示
	}
}

void BossManager::OnClearSequenceStart() {
	bossBattle_ = false; // ボス戦終了フラグセット
	bossBullets_.clear(); // ボス弾リストクリアa
	boss_.reset(); // ボスオブジェクト破棄
	bossController_.reset(); // ボス挙動コントローラ破棄
	bossP2BgmPlayed_ = false; // P2BGM再生フラグリセット
}

void BossManager::UpdateBossBullets() {
	for (auto it = bossBullets_.begin(); it != bossBullets_.end();) { // ボス弾更新ループ
		BossBullet* b_ = it->get();

		b_->Update(); // 更新（位置が動くので先に更新）

		// ───────── Player × BossBullet 当たり判定 ─────────
		if (player_ && !player_->IsDead() && !b_->IsDead()) {
			const Vector3 pCenter_ = player_->GetPosition();
			const Vector3 pSize_ = player_->GetColliderScale(); // Player.cpp のワイヤーと同じサイズ
			const Vector3 sCenter_ = b_->GetPos();
			const float   sR_ = b_->Radius();

			if (TestAABBSphere(pCenter_, pSize_, sCenter_, sR_)) {
				player_->Damage(b_->Damage()); // 被弾（HP減る＆赤フラッシュ）
				b_->Kill();                    // 弾消滅
			}
		}

		if (b_->IsDead()) { // 死亡していたらリストから削除
			it = bossBullets_.erase(it);
		} else {
			++it;
		}
	}
}