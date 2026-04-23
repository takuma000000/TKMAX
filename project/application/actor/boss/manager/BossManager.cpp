#include "BossManager.h"
#include "GameScene.h"
#include <algorithm>
#include "MyMath.h"
#include "BossConfigLoader.h"
#include "AudioManager.h"

//=============================================================
// ワールド座標 → スクリーンUV座標変換
//=============================================================
/// <summary>
/// ワールド座標をビュープロジェクション行列で変換し、
/// 0.0～1.0 のスクリーンUV座標へ変換します。
/// </summary>
/// <param name="world">変換元のワールド座標</param>
/// <param name="vp">ビュープロジェクション行列</param>
/// <returns>スクリーンUV座標</returns>
static Vector2 WorldToUV(const Vector3& world, const Matrix4x4& vp) {
	float clipX = world.x * vp.m[0][0] + world.y * vp.m[1][0] + world.z * vp.m[2][0] + 1.0f * vp.m[3][0];
	float clipY = world.x * vp.m[0][1] + world.y * vp.m[1][1] + world.z * vp.m[2][1] + 1.0f * vp.m[3][1];
	float clipW = world.x * vp.m[0][3] + world.y * vp.m[1][3] + world.z * vp.m[2][3] + 1.0f * vp.m[3][3];

	// Wが極端に小さいと除算が不安定になるため、画面中央を返す
	if (fabsf(clipW) < 0.0001f) {
		return { 0.5f, 0.5f };
	}

	float ndcX = clipX / clipW;
	float ndcY = clipY / clipW;

	return { ndcX * 0.5f + 0.5f, -ndcY * 0.5f + 0.5f };
}

namespace {

	//=============================================================
	// 補助関数
	//=============================================================
	/// <summary>
	/// 値を指定した最小値～最大値の範囲に収めます。
	/// </summary>
	/// <param name="v">対象の値</param>
	/// <param name="mn">最小値</param>
	/// <param name="mx">最大値</param>
	/// <returns>クランプ後の値</returns>
	static float ClampFloat(float v, float mn, float mx) {
		if (v < mn) return mn;
		if (v > mx) return mx;
		return v;
	}

	/// <summary>
	/// AABB と Sphere の当たり判定を行います。
	/// </summary>
	/// <param name="aabbCenter">AABB中心座標</param>
	/// <param name="aabbSize">AABBサイズ</param>
	/// <param name="sphereCenter">Sphere中心座標</param>
	/// <param name="sphereRadius">Sphere半径</param>
	/// <returns>接触していれば true</returns>
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

		// Sphere中心をAABB内部の最も近い位置へ寄せる
		const float cx = ClampFloat(sphereCenter.x, minX, maxX);
		const float cy = ClampFloat(sphereCenter.y, minY, maxY);
		const float cz = ClampFloat(sphereCenter.z, minZ, maxZ);

		const float dx = sphereCenter.x - cx;
		const float dy = sphereCenter.y - cy;
		const float dz = sphereCenter.z - cz;

		return (dx * dx + dy * dy + dz * dz) <= (sphereRadius * sphereRadius);
	}
}

//=============================================================
// 初期化
//=============================================================
void BossManager::Initialize(TKM::DirectXCommon* dxCommon, TKM::Camera* camera, TKM::BaseScene* parent, Player* player) {
	// 共通の管理情報を基底クラス側で初期化
	InitializeCommon(dxCommon, camera, parent, player);

	// ボス戦設定をjsonから読み込む
	BossConfigLoader::Load("resources/data/boss_config.json", bossConfig_);

	//=========================================================
	// 戦闘状態初期化
	//=========================================================
	bossBattle_ = false;          // ボス戦開始フラグ
	bossP2BgmPlayed_ = false;     // 第2フェーズBGM再生済みフラグ
	boss_.reset();                // ボス本体
	bossBullets_.clear();         // ボス弾リスト

	//=========================================================
	// スラッシュ攻撃管理初期化
	//=========================================================
	slashAttackId_ = 0;           // 発行用スラッシュ攻撃ID
	currentSlashId_ = -1;         // 現在有効なスラッシュ攻撃ID
	slashIdHoldT_ = 0.0f;         // スラッシュ攻撃ID保持時間

	//=========================================================
	// HPバーUI初期化
	//=========================================================
	hpUI_ = std::make_unique<TKM::BossHpBarUI>();
	TKM::BossHpBarUI::Desc d{};   // デフォルト設定
	hpUI_->Initialize(TKM::SpriteCommon::GetInstance(), dx_, parent_, d);
	hpUI_->SetVisible(false);     // 開始時は非表示
}

//=============================================================
// ボス戦開始要求
//=============================================================
void BossManager::StartBattle() {
	// 既に戦闘中なら何もしない
	if (bossBattle_) {
		return;
	}

	// まだボスが生成されていない場合は登場演出用スポーンを行う
	if (!boss_) {
		SpawnForEntrance();
	}

	// 本戦開始処理へ移行
	BeginBattle();
}

//=============================================================
// 更新
//=============================================================
void BossManager::Update(float dt) {
	// ボス戦が始まっていない、またはボス本体が存在しない場合は
	// ボス弾のみ更新して終了する
	if (!bossBattle_ || !boss_) {
		UpdateBossBullets();
		return;
	}

	//=========================================================
	// スラッシュ攻撃ID保持時間の更新
	//=========================================================
	if (slashIdHoldT_ > 0.0f) {
		slashIdHoldT_ -= dt;

		// 負の値にならないよう補正
		if (slashIdHoldT_ < 0.0f) {
			slashIdHoldT_ = 0.0f;
		}
	}

	//=========================================================
	// 撃破演出中処理
	// ボスが死亡演出中または死亡済みなら攻撃を完全停止する
	//=========================================================
	if (boss_ && (boss_->IsDying() || boss_->IsDead())) {
		// 撃破シーケンス中はプレイヤー操作側の攻撃を停止
		player_->SetShootingEnabled(false);

		// 撃破シーケンス中は振動も停止
		player_->SetRumbleEnabled(false);

		// ボス戦BGM停止
		TKM::AudioManager::GetInstance()->StopSound("bossPhaseBGM");

		// 一度だけ残存弾を消去し、以降の攻撃発生も止める
		if (!killSeq_.attacksStopped_) {
			bossBullets_.clear();
			killSeq_.attacksStopped_ = true;
		}

		// HPバーは減少演出のため更新継続
		if (hpUI_ && boss_) {
			hpUI_->Update(dt, boss_.get());
		}

		// 死亡リアクション進行のためボス本体更新は継続
		boss_->Update(dt);

		// 撃破ズームとスローモーションを1回だけ開始
		if (!killSeq_.zoomStarted_ && boss_->IsDying()) {
			if (player_) {
				player_->StartBossDeathCameraZoom();
			}
			killSeq_.zoomStarted_ = true;

			if (!killSeq_.slowTriggered_ && timeScale_) {
				timeScale_->RequestSlow(
					bossConfig_.bossBattle_.killSlowScale_,
					bossConfig_.bossBattle_.killSlowDuration_
				);
				killSeq_.slowTriggered_ = true;
			}
		}
		return;
	}

	//=========================================================
	// ボス行動コントローラ更新
	//=========================================================
	if (bossController_) {
		bossController_->Update(dt, *boss_);
	}

	//=========================================================
	// 触手チャージ演出反映
	//=========================================================
	if (boss_ && bossController_) {
		boss_->SetTentacleCharge(
			bossController_->IsAnyCharging(), // いずれかの攻撃をチャージ中か
			bossController_->GetCharge01()    // チャージ量 0.0～1.0
		);
	}

	//=========================================================
	// ミサイル攻撃生成
	//=========================================================
	{
		Vector3 mPos_{};                            // 発射位置
		Vector3 mTarget_{};                         // ターゲット位置
		float mSpeed_ = 0.0f;                       // 移動速度
		Vector3 mControlOffset_{ 0.0f, 0.0f, 0.0f }; // 曲線制御オフセット
		int mDmg_ = 0;                              // ダメージ量
		int mLife_ = 0;                             // 生存フレーム数

		while (bossController_->ConsumeMissileFireRequest(mPos_, mTarget_, mSpeed_, mControlOffset_, mDmg_, mLife_)) {
			// BossBulletのspeedは「1フレームあたり移動量」を想定しているため、
			// 秒基準の速度をdtで変換して渡す
			const float spPerFrame_ = mSpeed_ * dt;

			auto bullet_ = std::make_unique<BossBullet>();

			// 曲線移動モードでは使わないが、初期化用に進行方向を作る
			Vector3 dir_{ mTarget_.x - mPos_.x, mTarget_.y - mPos_.y, mTarget_.z - mPos_.z };
			dir_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f });

			// ミサイルは見た目と当たり判定を一致させるため、
			// 弾自体が当たり判定付きエフェクトとして機能する
			bullet_->Initialize(TKM::Object3dCommon::GetInstance(), dx_, camera_, mPos_, dir_, spPerFrame_, mDmg_, mLife_);
			bullet_->EnableCurveToTargetWithControlOffset(mPos_, mTarget_, mControlOffset_, spPerFrame_);
			bullet_->SetFxType(BossBullet::FxType::MissileEvil);

			bossBullets_.push_back(std::move(bullet_));
		}
	}

	//=========================================================
	// スラッシュ攻撃生成
	//=========================================================
	{
		Vector3 sPos_{};
		Vector3 sTarget_{};
		float sSpeed_ = 0.0f;
		int sDmg_ = 0;
		int sLife_ = 0;

		if (bossController_->ConsumeSlashFireRequest(sPos_, sTarget_, sSpeed_, sDmg_, sLife_)) {
			const float spPerFrame_ = sSpeed_ * dt;

			// 一定時間を超えたら新しい斬撃IDを発行する
			if (slashIdHoldT_ <= 0.0f) {
				currentSlashId_ = ++slashAttackId_;
			}

			// 同一スラッシュ判定として扱う保持時間
			slashIdHoldT_ = 0.5f;

			auto bullet_ = std::make_unique<BossBullet>();

			Vector3 dir_{ sTarget_.x - sPos_.x, sTarget_.y - sPos_.y, sTarget_.z - sPos_.z };
			dir_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f });

			// スラッシュも見た目と当たり判定を兼用する
			bullet_->Initialize(TKM::Object3dCommon::GetInstance(), dx_, camera_, sPos_, dir_, spPerFrame_, 1, sLife_);
			bullet_->SetModel("sphere.obj");
			bullet_->SetScale({ 3.8f, 0.7f, 1.2f }); // 細長い楕円柱状の見た目にする
			bullet_->SetFxType(BossBullet::FxType::SlashWave);

			// 同一斬撃の多段ヒット制御用ID
			bullet_->SetAttackId(currentSlashId_);

			// スラッシュは移動させず、その場で向きや見た目を使って当たり判定を行う想定
			bossBullets_.push_back(std::move(bullet_));
		}
	}

	//=========================================================
	// HPバーUI更新
	//=========================================================
	if (hpUI_ && boss_) {
		hpUI_->Update(dt, boss_.get());
	}

	//=========================================================
	// ボス本体更新
	//=========================================================
	boss_->Update(dt);

	//=========================================================
	// ボス撃破ズーム開始
	//=========================================================
	if (!killSeq_.zoomStarted_ && boss_->IsDying()) {
		if (player_) {
			player_->StartBossDeathCameraZoom();
		}
		killSeq_.zoomStarted_ = true;

		// 撃破時スローモーション開始
		if (!killSeq_.slowTriggered_ && timeScale_) {
			timeScale_->RequestSlow(
				bossConfig_.bossBattle_.killSlowScale_,
				bossConfig_.bossBattle_.killSlowDuration_
			);
			killSeq_.slowTriggered_ = true;
		}
	}

	//=========================================================
	// ボス弾更新
	//=========================================================
	UpdateBossBullets();

	//=========================================================
	// デバッグ表示
	//=========================================================
	if (bossController_ && boss_) {
		bossController_->ImGuiDebug(*boss_);
	}
}

//=============================================================
// 描画
//=============================================================
void BossManager::Draw(TKM::DirectXCommon* dxCommon) {
	// ボスがいない場合は弾更新だけ行って終了
	if (!boss_) {
		UpdateBossBullets();
		return;
	}

	// ボス戦中でなく、かつ登場演出描画中でもない場合は
	// ボス本体を描画しない
	if (!bossBattle_ && !isEntranceDrawing_) {
		UpdateBossBullets();
		return;
	}

	//=========================================================
	// ボス本体描画
	//=========================================================
	boss_->Draw(dxCommon);

	//=========================================================
	// ボス弾トレイル描画
	//=========================================================
	for (auto& b : bossBullets_) {
		b->DrawTrail(dxCommon);
	}
}

//=============================================================
// UI描画
//=============================================================
void BossManager::DrawUI() {
	// ボス戦中のみHPバーを表示
	if (hpUI_ && bossBattle_ && boss_) {
		hpUI_->Draw();
	}
}

//=============================================================
// 登場演出用スポーン
//=============================================================
void BossManager::SpawnForEntrance() {
	if (boss_) {
		return;
	}
	if (!dx_ || !camera_ || !parent_) {
		return;
	}

	// 第2フェーズBGM再生状態リセット
	bossP2BgmPlayed_ = false;

	//=========================================================
	// ボス本体生成
	//=========================================================
	boss_ = std::make_unique<BossEnemy>();
	boss_->SetConfig(&bossConfig_.bossEnemy_);
	boss_->Initialize(TKM::Object3dCommon::GetInstance(), dx_);
	boss_->SetCamera(camera_);
	boss_->SetParentScene(parent_);
	boss_->SetPosition(bossConfig_.bossBattle_.spawnPos_);
	boss_->SyncTransform();

	//=========================================================
	// プレイヤー参照設定
	//=========================================================
	if (player_) {
		boss_->SetPlayer([this]() {
			return player_->GetPosition();
			});
	}

	//=========================================================
	// 行動コントローラ生成
	//=========================================================
	bossController_ = std::make_unique<BossController>();
	bossController_->SetConfig(&bossConfig_.bossController_);
	bossController_->Initialize(
		bossConfig_.bossBattle_.arenaMin_,
		bossConfig_.bossBattle_.arenaMax_
	);

	//=========================================================
	// 撃破シーケンス状態初期化
	//=========================================================
	killSeq_.Reset();

	//=========================================================
	// UI・描画状態初期化
	//=========================================================
	hpUI_->SetVisible(false);   // 登場演出中はHPバー非表示
	isEntranceDrawing_ = true;  // 登場演出描画中フラグON
}

//=============================================================
// 本戦開始
//=============================================================
void BossManager::BeginBattle() {
	if (bossBattle_) {
		return;
	}
	if (!boss_) {
		return;
	}

	bossBattle_ = true;
	bossP2BgmPlayed_ = false;

	//=========================================================
	// UI表示
	//=========================================================
	if (hpUI_) {
		hpUI_->SetVisible(true);
	}

	//=========================================================
	// プレイヤー側の戦闘状態有効化
	//=========================================================
	if (player_) {
		player_->SetShootingEnabled(true);
		player_->SetRumbleEnabled(true);
	}

	// 登場演出終了後は通常の戦闘描画更新へ移行
	isEntranceDrawing_ = false;
}

//=============================================================
// ボス弾生成
//=============================================================
void BossManager::SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame) {
	// 生成に必要な参照が無ければ何もしない
	if (!dx_ || !camera_) {
		return;
	}

	auto bullet_ = std::make_unique<BossBullet>();
	bullet_->Initialize(TKM::Object3dCommon::GetInstance(), dx_, camera_, pos, dir, speed, damage, lifeFrame);
	bossBullets_.push_back(std::move(bullet_));
}

//=============================================================
// 状態取得
//=============================================================
bool BossManager::IsBattleActive() const {
	return bossBattle_ && boss_ != nullptr && !boss_->IsDead();
}

bool BossManager::IsBossAlive() const {
	return boss_ && !boss_->IsDead();
}

bool BossManager::IsBossDead() const {
	return boss_ && boss_->IsDead();
}

//=============================================================
// クリア演出開始時処理
//=============================================================
void BossManager::OnClearSequenceStart() {
	bossBattle_ = false;         // ボス戦終了
	bossBullets_.clear();        // 残存弾破棄
	boss_.reset();               // ボス本体破棄
	bossController_.reset();     // 行動コントローラ破棄
	bossP2BgmPlayed_ = false;    // BGM再生フラグリセット
	isEntranceDrawing_ = false;  // 登場演出描画フラグリセット
}

//=============================================================
// 外部参照設定
//=============================================================
void BossManager::SetTimeScaleController(TKM::TimeScaleController* t) {
	timeScale_ = t;
}

void BossManager::SetWaterRippleEffect(TKM::WaterRippleEffect* r) {
	waterRipple_ = r;
}

void BossManager::SetCamera(TKM::Camera* camera) {
	BattleActorManagerBase::SetCamera(camera);
}

//=============================================================
// カメラ差し替え時処理
//=============================================================
void BossManager::OnCameraChanged() {
	// ボス本体へ新しいカメラを反映
	if (boss_) {
		boss_->SetCamera(camera_);
	}

	// 既存の全ボス弾へも新しいカメラを反映
	for (auto& b : bossBullets_) {
		b->SetCamera(camera_);
	}
}

//=============================================================
// ボス弾更新
//=============================================================
void BossManager::UpdateBossBullets() {
	for (auto it = bossBullets_.begin(); it != bossBullets_.end();) {
		BossBullet* b_ = it->get();

		//=========================================================
		// 弾更新
		//=========================================================
		b_->Update();

		//=========================================================
		// Player × BossBullet 当たり判定
		//=========================================================
		if (player_ && !player_->IsDead() && !b_->IsDead()) {
			const Vector3 pCenter_ = player_->GetPosition();      // プレイヤー中心座標
			const Vector3 pSize_ = player_->GetColliderScale();   // プレイヤーAABBサイズ

			// スラッシュは専用当たり判定を使用
			if (b_->GetFxType() == BossBullet::FxType::SlashWave) {
				if (b_->HitTestSlashX(pCenter_, pSize_)) {
					// 同一攻撃IDによる多段ヒット制御込みでダメージを試行
					player_->TryDamageFromAttack(b_->Damage(), b_->GetAttackId());

					// 当たった瞬間に弾は消す
					b_->Kill();
				}
			} else {
				// ミサイルなど通常弾は AABB × Sphere 判定
				const Vector3 sCenter_ = b_->GetPos();
				const float sR_ = b_->Radius();

				if (TestAABBSphere(pCenter_, pSize_, sCenter_, sR_)) {
					player_->Damage(b_->Damage());
					b_->Kill();
				}
			}
		}

		//=========================================================
		// 生死判定による削除
		//=========================================================
		if (b_->IsDead()) {
			it = bossBullets_.erase(it);
		} else {
			++it;
		}
	}
}

//=============================================================
// 撃破シーケンス状態リセット
//=============================================================
void BossManager::KillSequenceState::Reset() {
	zoomStarted_ = false;       // 撃破ズーム開始済みフラグ
	slowTriggered_ = false;     // スローモーション発動済みフラグ
	rippleTriggered_ = false;   // 波紋エフェクト発動済みフラグ
	attacksStopped_ = false;    // 撃破中の攻撃停止実行済みフラグ
}