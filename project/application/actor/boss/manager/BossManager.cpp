#include "BossManager.h"
#include "GameScene.h"
#include <algorithm>
#include "MyMath.h"
#include "BossConfigLoader.h"

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

namespace {
	// 値をmn～mxの範囲にクランプする
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
	// 基底クラスの初期化
	InitializeCommon(dxCommon, camera, parent, player);
	// ボス戦設定ファイルから読み込み
	BossConfigLoader::Load("resources/data/boss_config.json", bossConfig_);

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
	hpUI_->Initialize(TKM::SpriteCommon::GetInstance(), dx_, parent_, d);
	hpUI_->SetVisible(false); // 非表示開始
}

void BossManager::StartBattle() {
	// すでにボス戦開始している場合は何もしない
	if (bossBattle_) {
		return;
	}

	if (!boss_) {
		// まだボスが生成されていない場合は、まず登場演出用に生成する
		SpawnForEntrance();
	}

	// ボス戦開始
	BeginBattle();
}

void BossManager::Update(float dt) {
	if (!bossBattle_ || !boss_) { // ボス戦未開始またはボス不在
		UpdateBossBullets(); // ボス弾更新のみ行う
		return;
	}

	if (slashIdHoldT_ > 0.0f) { // スラッシュ攻撃IDホールド中
		slashIdHoldT_ -= dt; // 経過時間減算
		if (slashIdHoldT_ < 0.0f) { slashIdHoldT_ = 0.0f; } // 負にならないようにクランプ
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
				// 撃破ズームと同時にスローモーション開始
				timeScale_->RequestSlow(
					bossConfig_.bossBattle_.killSlowScale_,
					bossConfig_.bossBattle_.killSlowDuration_
				);
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
		Vector3 mControlOffset_{ 0.0f, 0.0f, 0.0f }; // 曲線制御用オフセット（ワールド座標で、弾の進行方向に対して左右どちらかにオフセットする想定）
		int mDmg_ = 0; // ダメージ量
		int mLife_ = 0; // 生存フレーム数

		while (bossController_->ConsumeMissileFireRequest(mPos_, mTarget_, mSpeed_, mControlOffset_, mDmg_, mLife_)) {
			// BossBulletは「speedが1フレ移動量」なので dt掛けた値を渡す（君が既にやってるやつ）
			const float spPerFrame_ = mSpeed_ * dt;
			auto bullet_ = std::make_unique<BossBullet>(); // 弾オブジェクト生成
			// dirはInitializeのために一応入れる（曲線モードでは使われない）
			Vector3 dir_{ mTarget_.x - mPos_.x, mTarget_.y - mPos_.y, mTarget_.z - mPos_.z };
			dir_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f });

			// ミサイルエフェクトは当たり判定も兼ねるので、ダメージと生存フレーム数をしっかり設定する
			bullet_->Initialize(TKM::Object3dCommon::GetInstance(), dx_, camera_, mPos_, dir_, spPerFrame_, mDmg_, mLife_);
			// ミサイルエフェクトは見た目と当たり判定を合わせるために、曲線移動モードで出現位置からターゲット位置に向かって移動させる
			bullet_->EnableCurveToTargetWithControlOffset(mPos_, mTarget_, mControlOffset_, spPerFrame_);
			// ミサイルエフェクトは、bossEvil_シリーズのモデルを使用する想定
			bullet_->SetFxType(BossBullet::FxType::MissileEvil);

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
				currentSlashId_ = ++slashAttackId_; // このタイミングで「今回の斬撃ID」を確定
			}
			slashIdHoldT_ = 0.5f; // 斬撃の長さに合わせて(0.2〜0.5くらい)

			auto bullet_ = std::make_unique<BossBullet>();

			Vector3 dir_{ sTarget_.x - sPos_.x, sTarget_.y - sPos_.y, sTarget_.z - sPos_.z };
			dir_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f });

			// 斬撃エフェクトは当たり判定も兼ねるので、ダメージと生存フレーム数をしっかり設定する
			bullet_->Initialize(TKM::Object3dCommon::GetInstance(), dx_, camera_, sPos_, dir_, spPerFrame_, 1, sLife_);
			bullet_->SetModel("sphere.obj");
			bullet_->SetScale({ 3.8f, 0.7f, 1.2f }); // スラッシュエフェクトは細長い楕円柱みたいな形にする
			bullet_->SetFxType(BossBullet::FxType::SlashWave); // スラッシュエフェクトタイプセット

			bullet_->SetAttackId(currentSlashId_); // 斬撃IDセット
			// 斬撃エフェクトは見た目と当たり判定を合わせるために、移動はせずに出現位置で回転するだけにする
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
	if (!killSeq_.zoomStarted_ && boss_->IsDying()) {
		if (player_) {
			player_->StartBossDeathCameraZoom();
		}
		killSeq_.zoomStarted_ = true;

		// スロー
		if (!killSeq_.slowTriggered_ && timeScale_) {
			timeScale_->RequestSlow(
				bossConfig_.bossBattle_.killSlowScale_,
				bossConfig_.bossBattle_.killSlowDuration_
			);
			killSeq_.slowTriggered_ = true;
		}
	}
	// ボス弾更新
	UpdateBossBullets();

	if (bossController_ && boss_) {
		bossController_->ImGuiDebug(*boss_); // デバッグ表示
	}
}

void BossManager::Draw(TKM::DirectXCommon* dxCommon) {
	if (!boss_) {
		UpdateBossBullets();
		return;
	}

	// ボス戦中でなければ、ボスは描画せず弾だけ描画する
	if (!bossBattle_) {
		UpdateBossBullets();
		return;
	}

	// ボス
	boss_->Draw(dxCommon);
	// トレイル
	for (auto& b : bossBullets_) {
		b->DrawTrail(dxCommon);
	}
}

void BossManager::DrawUI() {
	// もしボス戦中ならHPバーUIも描画
	if (hpUI_ && bossBattle_ && boss_) { // HPバーUI描画
		hpUI_->Draw(); // 描画
	}
}

void BossManager::SpawnForEntrance() {
	if (boss_) {
		return;
	}
	if (!dx_ || !camera_ || !parent_) {
		return;
	}

	bossP2BgmPlayed_ = false;

	boss_ = std::make_unique<BossEnemy>();
	boss_->SetConfig(&bossConfig_.bossEnemy_);
	boss_->Initialize(TKM::Object3dCommon::GetInstance(), dx_);
	boss_->SetCamera(camera_);
	boss_->SetParentScene(parent_);
	boss_->SetPosition(bossConfig_.bossBattle_.spawnPos_);
	boss_->SyncTransform();

	if (player_) {
		boss_->SetPlayer([this]() {
			return player_->GetPosition();
			});
	}

	bossController_ = std::make_unique<BossController>();
	bossController_->SetConfig(&bossConfig_.bossController_);
	bossController_->Initialize(
		bossConfig_.bossBattle_.arenaMin_,
		bossConfig_.bossBattle_.arenaMax_
	);

	killSeq_.Reset();

	if (hpUI_) {
		hpUI_->SetVisible(false);
	}
}

void BossManager::BeginBattle() {
	if (bossBattle_) {
		return;
	}
	if (!boss_) {
		return;
	}

	bossBattle_ = true;
	bossP2BgmPlayed_ = false;

	if (hpUI_) {
		hpUI_->SetVisible(true);
	}
	if (player_) {
		player_->SetShootingEnabled(true);
		player_->SetRumbleEnabled(true);
	}
}

void BossManager::SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame) {
	if (!dx_ || !camera_) { // 安全確認
		return;
	}

	auto bullet_ = std::make_unique<BossBullet>(); // 弾オブジェクト生成
	bullet_->Initialize(TKM::Object3dCommon::GetInstance(), dx_, camera_, pos, dir, speed, damage, lifeFrame); // 初期化
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

void BossManager::SetTimeScaleController(TKM::TimeScaleController* t) {
	timeScale_ = t; // タイムスケールコントローラーセット
}

void BossManager::SetWaterRippleEffect(TKM::WaterRippleEffect* r) {
	waterRipple_ = r; // 波紋エフェクトセット
}

void BossManager::SetCamera(TKM::Camera* camera) {
	BattleActorManagerBase::SetCamera(camera); // 基底クラスのカメラセット処理呼び出し
}

void BossManager::OnCameraChanged() {
	if (boss_) { boss_->SetCamera(camera_); } // カメラが変わったらボスと弾の両方に新しいカメラをセットする
	for (auto& b : bossBullets_) { b->SetCamera(camera_); } // ボス弾ループしてカメラセット
}

void BossManager::UpdateBossBullets() {
	for (auto it = bossBullets_.begin(); it != bossBullets_.end();) { // ボス弾更新ループ
		BossBullet* b_ = it->get();

		b_->Update(); // 更新（位置が動くので先に更新）

		// ───────── Player × BossBullet 当たり判定 ─────────
		if (player_ && !player_->IsDead() && !b_->IsDead()) {
			const Vector3 pCenter_ = player_->GetPosition(); // プレイヤー中心座標
			const Vector3 pSize_ = player_->GetColliderScale(); // プレイヤー当たり判定サイズ（AABBの幅・高さ・奥行き）

			if (b_->GetFxType() == BossBullet::FxType::SlashWave) { // スラッシュエフェクト：見た目と当たり判定を合わせるために、専用の当たり判定関数で判定する
				if (b_->HitTestSlashX(pCenter_, pSize_)) { // スラッシュのX軸方向の当たり判定（細長い楕円柱の当たり判定に近い感じ）

					// ダメージは1回だけ（通らないなら減らない）
					player_->TryDamageFromAttack(b_->Damage(), b_->GetAttackId());

					// でも弾は当たった瞬間に必ず消す
					b_->Kill();
				}
			} else {
				// ミサイル等：従来どおり AABB×Sphere
				const Vector3 sCenter_ = b_->GetPos();
				const float   sR_ = b_->Radius();

				if (TestAABBSphere(pCenter_, pSize_, sCenter_, sR_)) { // 当たった
					player_->Damage(b_->Damage()); // ダメージを与える
					b_->Kill(); // 弾は当たったら消す
				}
			}
		}

		if (b_->IsDead()) { // 死亡していたらリストから削除
			it = bossBullets_.erase(it);
		} else { // 生存していたら次へ
			++it;
		}
	}
}

void BossManager::KillSequenceState::Reset() {
	zoomStarted_ = false; // ズーム開始フラグ
	slowTriggered_ = false; // スローモーション発動済みフラグ
	rippleTriggered_ = false; // スローモーション/波紋エフェクト発動済みフラグ
	attacksStopped_ = false; // 撃破中に攻撃を止めたか
}