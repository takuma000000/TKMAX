#include "Player.h"
#include "Enemy.h"
#include <ParticleManager.h>
#include "AABB.h"
#include <limits>
#include "RadialBlurEffect.h"
#include "BarrierCore.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void Player::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	// Object3d共通への参照を保持する
	common_ = common;
	// DirectX共通への参照を保持する
	dxCommon_ = dxCommon;
	// トレイル描画システムを初期化する
	TKM::TrailRibbonRenderer::GetInstance()->Initialize(dxCommon_);

	//=========================================================
	// プレイヤー本体生成
	//=========================================================

	// プレイヤー本体の3Dオブジェクトを生成する
	object_ = std::make_unique<TKM::Object3d>();
	// 本体の描画に必要な情報を渡して初期化する
	object_->Initialize(common_, dxCommon_);
	// 本体モデルを設定する
	object_->SetModel("turtle.obj");

	//=========================================================
	// ヒレ生成
	//=========================================================

	// ヒレ用3Dオブジェクトを生成する
	flipper_ = std::make_unique<TKM::Object3d>();
	// ヒレの描画に必要な情報を渡して初期化する
	flipper_->Initialize(common_, dxCommon_);
	// ヒレモデルを設定する
	flipper_->SetModel("turtle_flipper.obj");
	// ヒレを本体の子にして追従させる
	flipper_->SetParent(object_.get());
	// ヒレ回転の基準姿勢を保存する
	flipperBaseRot_ = flipper_->GetRotate();
	// ヒレアニメ用タイマーを初期化する
	flipperAnimT_ = 0.0f;

	//=========================================================
	// 回避システム生成
	//=========================================================

	// 回避システムを生成する
	dodge_ = std::make_unique<PlayerDodge>();
	// 回避システムを初期化する
	dodge_->Initialize(common_, dxCommon_, camera_);

	//=========================================================
	// HP管理生成
	//=========================================================

	// HP管理クラスを生成する
	health_ = std::make_unique<PlayerHealth>();
	// HP管理を初期化する
	health_->Initialize(5);

	//=========================================================
	// 撃墜管理生成
	//=========================================================

	// 撃墜管理クラスを生成する
	death_ = std::make_unique<PlayerDeath>();
	// 撃墜管理を初期化する
	death_->Initialize();

	//=========================================================
	// レティクル生成
	//=========================================================

	// レティクルを生成する
	reticle_ = std::make_unique<Reticle>();
	// レティクルを初期化する
	reticle_->Initialize(common_, dxCommon_, "reticle_big.obj");
	// プレイヤーの位置とヨー角をレティクルへ渡すコールバックを登録する
	reticle_->BindOwner(
		[this]() { return object_->GetTranslate(); },
		[this]() { return object_->GetRotate().y; }
	);
	// レティクルの移動範囲を設定する
	reticle_->SetMoveRange(moveMin_, moveMax_);
	// レティクル中心取得処理を一度呼んでおく
	reticle_->GetCenterWorldPos();

	//=========================================================
	// ショットマネージャー生成
	//=========================================================

	// ショットマネージャーを生成する
	shotManager_ = std::make_unique<PlayerShotManager>();
	// プレイヤー参照と描画情報を渡して初期化する
	shotManager_->Initialize(this, common_, dxCommon_);
	// ショット設定JSONを読み込む
	const bool loaded = shotConfig_.Load("./resources/data/playerShotConfig.json");
	// 読み込み失敗時は停止する
	assert(loaded && "playerShotConfig.json の読込に失敗しました");
	// 読み込んだ設定をショットマネージャーへ渡す
	shotManager_->SetConfig(&shotConfig_);
	// 自機オブジェクト参照を渡す
	shotManager_->SetOwnerObject(object_.get());
	// レティクル参照を渡す
	shotManager_->SetReticle(reticle_.get());
	// カメラ参照を渡す
	shotManager_->SetCamera(camera_);

	//=========================================================
	// パーティクルグループ作成
	//=========================================================

	// ジェット煙パーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup(
		"jetSmoke", "./resources/texture/circle.png", TKM::ParticleManager::ParticleType::NORMAL);

	// 被弾スパークパーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup(
		"damageSpark", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL);

	// RB弾の軌跡パーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_rb", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL);

	// LB弾の軌跡パーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lb", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL);

	// RT弾の軌跡パーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_rt", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL);

	// LT弾の軌跡パーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lt", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL);

	// LT弾リボン軌跡パーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lt_ribbon", "./resources/texture/firework_star.png", TKM::ParticleManager::ParticleType::RIBBON);

	// LT弾キラキラパーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lt_sparkle", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL);

	// LT弾リングパーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup("trail_lt_ring", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::RING);

	// LB弾のキラキラ演出パーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup(
		"trail_lb_glitter",
		"./resources/texture/firework_star.png",
		TKM::ParticleManager::ParticleType::NORMAL);

	// LB弾の稲光メイン演出パーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup(
		"trail_lb_bolt_main",
		"./resources/texture/gradationLine.png",
		TKM::ParticleManager::ParticleType::NORMAL);

	// LB弾の稲光コア演出パーティクルを作成する
	TKM::ParticleManager::GetInstance()->CreateParticleGroup(
		"trail_lb_bolt_core",
		"./resources/texture/gradationLine.png",
		TKM::ParticleManager::ParticleType::NORMAL);

	//=========================================================
	// ジェット煙エミッタ初期化
	//=========================================================

	// ジェット煙が有効なら初期化する
	if (enableJetSmoke_) {
		// 機体後方に煙の初期位置を作る
		Vector3 jetPos = object_->GetTranslate();
		jetPos.z -= kJetSmokeOffsetZ_;

		// エミッタを初期化する
		jetEmitter_.Initialize("jetSmoke", jetPos);
	}

	//=========================================================
	// ワンウェイバリア関連初期化
	//=========================================================

	// バリアヒット情報リストを空にする
	wave1BarrierHits_.clear();
}

void Player::Update(float dt) {
	// コントローラー振動を更新する
	UpdateRumble(dt);

	// HP管理を更新する
	health_->Update(dt);

	//=========================================================
	// ゲーム開始時の前進演出更新
	//=========================================================
	UpdateIntroForwardMove_(dt);

	// 前進演出が終わるまではレティクルを完全に更新しない
	if (reticle_ && !introForwardActive_ && !health_->IsDead()) {
		reticle_->Update(dt);
	}

	//=========================================================
	// バリアヒット履歴更新
	//=========================================================
	for (auto it = wave1BarrierHits_.begin(); it != wave1BarrierHits_.end();) {
		// 各ヒット情報の経過時間を進める
		it->age_ += dt;

		// 寿命を超えたものは削除する
		if (it->age_ >= it->life_) {
			it = wave1BarrierHits_.erase(it);
		} else {
			// まだ生きているものは次へ進む
			++it;
		}
	}

	//=========================================================
	// ゲームプレイ処理
	//=========================================================
	if (controlEnabled_ && !IsDead() && !introForwardActive_) {
		// 通常移動処理
		if (!dodge_->IsDodging()) {
			HandleGamePadMove();
		}

		//=========================================================
		// 回避更新
		//=========================================================

		// 回避システムを更新する
		dodge_->Update(
			dt,
			object_.get(),
			true, // コントロール有効
			moveMin_,
			moveMax_,
			bankAngle_
		);

		// ショット処理
		shotManager_->Update(dt, shootingEnabled_);
		// RB/LB弾数が変化していたらHUDへ通知する
		NotifyHudState_();

	} else {
		// コントロール無効時はショットを全て消す
		shotManager_->ClearLockState();
		// 射撃停止などで弾状態が変わった場合に備えてHUDへ通知する
		NotifyHudState_();
		// 回避システムを更新する（コントロール無効で移動もさせない）
		dodge_->Update(
			dt,
			object_.get(),
			false, // コントロール無効
			moveMin_,
			moveMax_,
			bankAngle_
		);

	}

#ifdef USE_IMGUI
	//=========================================================
	// 自機当たり判定可視化
	//=========================================================
	{
		// 現在位置を取得する
		Vector3 center = object_->GetTranslate();

		// 当たり判定サイズを取得する
		Vector3 size = colliderScale_;

		// ライン描画システムを取得する
		auto* lr = TKM::LineRenderer::GetInstance();

		// 被弾フラッシュ中は赤、それ以外は緑で表示する
		TKM::LineRenderer::Color col =
			(health_ && health_->IsHitFlashActive())
			? TKM::LineRenderer::Color{ 1.0f, 0.0f, 0.0f, 1.0f }
		: TKM::LineRenderer::Color{ 0.0f, 1.0f, 0.0f, 1.0f };

		// AABBを描画する
		lr->AddAABB(center, size, col);
	}
#endif

	// 撃墜処理を更新する
	Death();

	//=========================================================
	// ジェット煙更新
	//=========================================================
	if (enableJetSmoke_ && health_ && !health_->IsDead()) {
		// 機体後方に煙の発生位置を置く
		Vector3 jetPos = object_->GetTranslate();
		jetPos.z -= kJetSmokeOffsetZ_;

		// エミッタ位置を更新する
		jetEmitter_.SetPosition(jetPos);

		// エミッタを更新する
		jetEmitter_.Update();
	}

	// 死亡済みターゲット参照を整理する
	RemoveEnemyIfDead();

	// ヒレアニメを更新する
	UpdateFlipperAnim_(dt);

	// 生きている間だけ浮遊とカメラ追従を更新する
	if (!IsDead()) {
		UpdateFloatBob_(dt);
		HandleFollowCamera();
	}

	// トレイル描画システムを更新する
	TKM::TrailRibbonRenderer::GetInstance()->Update(dt);

	// パーティクルマネージャーを更新する
	TKM::ParticleManager::GetInstance()->Update(dt);

	// プレイヤー本体の行列などを更新する
	object_->Update();

	// ヒレの行列などを更新する
	flipper_->Update();
}

void Player::DrawTrails(TKM::DirectXCommon* dxCommon) {
	// ショットマネージャー経由でトレイルを描画する
	shotManager_->DrawTrails(dxCommon);
}

void Player::ImGuiDebug() {
#ifdef USE_IMGUI
	// 本体がなければ何もしない
	if (!object_) return;

	// 現在のTransformを取得する
	Vector3 pos = object_->GetTranslate();
	Vector3 rot = object_->GetRotate();
	Vector3 scale = object_->GetScale();

	//=========================================================
	// プレイヤー本体デバッグUI
	//=========================================================
	ImGui::Begin("プレイヤー");

	// バリア状態表示
	ImGui::Text("バリア状態: %s", wave1BarrierActive_ ? "ON" : "OFF");

	// バリアヒット数表示
	ImGui::Text("ヒット数: %d", static_cast<int>(wave1BarrierHits_.size()));

	// 位置編集
	if (ImGui::DragFloat3("位置", &pos.x, 0.01f)) {
		object_->SetTranslate(pos);
	}

	// 回転編集
	if (ImGui::DragFloat3("回転", &rot.x, 0.01f)) {
		object_->SetRotate(rot);
	}

	// 拡縮編集
	if (ImGui::DragFloat3("拡縮cale", &scale.x, 0.01f)) {
		object_->SetScale(scale);
	}

	ImGui::Separator();

	// 当たり判定サイズ編集
	Vector3 col = colliderScale_;
	if (ImGui::DragFloat3("当たり判定サイズ(自機)", &col.x, 0.01f, 0.01f, 50.0f)) {
		colliderScale_ = col;
	}

	ImGui::Separator();

	// HPリセットボタン
	if (ImGui::Button("HPリセット")) {
		// HPを最大値に戻す
		health_->Reset();
		NotifyHudState_(); // HUDへ状態変更を通知する

	}

	ImGui::SeparatorText("カメラシェイク");

	// シェイク基本強度調整
	ImGui::SliderFloat("強度のベース", &shakeBaseStrength_, 0.0f, 5.0f);

	// ズーム時追加強度調整
	ImGui::SliderFloat("ズーム強調", &shakeZoomBoost_, 0.0f, 15.0f);

	// 現在のシェイク倍率表示
	ImGui::Text("現在の増幅量 : %.2f", shakeBaseStrength_ + (1.0f - camZoom_) * shakeZoomBoost_);

	ImGui::End();
#endif
}

void Player::AddHudObserver(const HudObserver& observer) {
	// HUD状態変更時に呼び出す通知先を登録する
	hudObservers_.push_back(observer);

	// 登録直後に現在状態を一度通知して、初期表示を正しくする
	NotifyHudState_();
}

bool Player::IsHudStateChanged_(const HudState& state) const {
	// 前回の状態がないなら常に変化とみなす
	if (!hasLastHudState_) {
		return true;
	}

	// 前回の状態と比較してどれか一つでも違うものがあれば変化とみなす
	return
		state.currentHp_ != lastHudState_.currentHp_ ||
		state.maxHp_ != lastHudState_.maxHp_ ||
		state.rbAmmo_ != lastHudState_.rbAmmo_ ||
		state.rbAmmoMax_ != lastHudState_.rbAmmoMax_ ||
		state.rbRefilling_ != lastHudState_.rbRefilling_ ||
		state.lbAmmo_ != lastHudState_.lbAmmo_ ||
		state.lbAmmoMax_ != lastHudState_.lbAmmoMax_;
}

void Player::NotifyHudState_() {
	// 現在の状態を構造体にまとめる
	HudState state{};
	state.currentHp_ = GetHP(); // 現在HP
	state.maxHp_ = GetMaxHP(); // 最大HP
	// RB弾の残弾数、最大残弾数、回復中かどうかを取得して構造体にセットする
	state.rbAmmo_ = GetRbAmmo(); // RB弾の残弾数
	state.rbAmmoMax_ = GetRbAmmoMax(); // RB弾の最大残弾数
	state.rbRefilling_ = IsRbRefilling(); // RB弾が回復中かどうか
	state.rbRefillingFromEmpty_ = IsRbRefillingFromEmpty(); // RB弾が0発から回復中かどうか
	// LB弾の残弾数、最大残弾数を取得して構造体にセットする
	state.lbAmmo_ = GetLbAmmo(); // LB弾の残弾数
	state.lbAmmoMax_ = GetLbAmmoMax(); // LB弾の最大残弾数

	// 状態に変化がなければ通知しない
	if (!IsHudStateChanged_(state)) {
		return;
	}

	lastHudState_ = state; // 状態を保存する
	hasLastHudState_ = true; // 状態があることを示すフラグを立てる

	// 登録されている通知先すべてに現在の状態を渡して呼び出す
	for (const auto& observer : hudObservers_) {
		if (observer) { // 登録されている通知先すべてに現在の状態を渡して呼び出す
			observer(state);
		}
	}
}

void Player::RemoveEnemyIfDead() {
	// ショットマネージャー側で死亡済みターゲット参照を外す
	shotManager_->RemoveDeadTargets();
}

void Player::EnableSpecialAttack() {
	// 一撃必殺の使用可能化をショットマネージャーへ通知する
	shotManager_->EnableSpecialAttack();
}

void Player::OnEnemyDestroyed(Enemy* e) {
	// 敵破壊時の後処理をショットマネージャーへ通知する
	shotManager_->OnEnemyDestroyed(e);
}

void Player::Damage(int value) {
	// HP管理側でダメージを処理する
	if (!health_ || !health_->Damage(value)) {
		return;
	}
	// HPが変化したのでHUDへ通知する
	NotifyHudState_();

	//=========================================================
	// 被弾振動設定
	//=========================================================

	// 1段目の重い振動を開始する
	StartRumble(0.10f, 52000, 18000);
	// 2段目振動を予約する
	rumble2Pending_ = true;
	// 2段目開始までの遅延
	rumble2DelayT_ = 0.07f;
	// 2段目の継続時間
	rumble2Sec_ = 0.08f;
	// 2段目左モーター強度
	rumble2Left_ = 0;
	// 2段目右モーター強度
	rumble2Right_ = 42000;
}

void Player::Death() {
	// HPが残っているなら撃墜処理しない
	if (!health_ || !health_->IsDead()) {
		return;
	}
	// 撃墜管理が無ければ処理しない
	if (!death_) {
		return;
	}

	// 撃墜を開始する
	death_->Start();

	//=========================================================
	// 死亡開始時に一度だけ行う処理
	//=========================================================

	// 撃墜開始要求があるなら
	if (death_->IsStartRequested()) {
		// 操作を止める
		SetControlEnabled(false);
		// 射撃を止める
		SetShootingEnabled(false);
		// レティクルを隠す
		SetReticleVisible(false);
		// ロック状態を解除する
		shotManager_->ClearLockState();
		// 軽くカメラシェイクする
		StartCameraShake(20);
		// 撃墜開始要求を消費する
		death_->ConsumeStartRequest();
	}

	// 撃墜演出を更新する
	death_->Update(object_.get());
}

void Player::UpdateVisualOnly(float dt) {
	// クリア演出用にヒレアニメだけ更新する
	UpdateFlipperAnim_(dt);
	// 本体行列だけ更新する
	if (object_) { object_->Update(); }
	// ヒレ行列だけ更新する
	if (flipper_) { flipper_->Update(); }
}

void Player::StartBossDeathCameraZoom() {
	// すでにズーム中なら二重起動しない
	if (bossZoomActive_) {
		return;
	}

	// ズーム先の目標倍率
	const float kTargetZoom = 0.35f;
	// ズーム時間
	const float kZoomTime = 1.2f;
	// ブラー継続時間
	const float kBlurTime = 4.795f;

	//=========================================================
	// ズームトゥイーン開始
	//=========================================================
	bossZoomActive_ = true;
	bossZoomTween_.Reset(1.0f, kTargetZoom, kZoomTime, Ease::Type::OutCubic);
	bossZoom_ = 1.0f;

	//=========================================================
	// ラジアルブラー開始
	//=========================================================
	if (radialBlur_) {
		radialBlur_->BlurStartShock(2.0f, kBlurTime);
	}
}

bool Player::TryDamageFromAttack(int damage, int attackId) {
	// HP管理側で攻撃ID付きダメージを処理する
	if (!health_ || !health_->TryDamageFromAttack(damage, attackId)) {
		return false;
	}

	// HPが変化したのでHUDへ通知する
	NotifyHudState_();

	//=========================================================
	// 被弾振動設定
	//=========================================================

	// 1段目の重い振動を開始する
	StartRumble(0.10f, 52000, 18000);
	// 2段目振動を予約する
	rumble2Pending_ = true;
	// 2段目開始までの遅延
	rumble2DelayT_ = 0.07f;
	// 2段目の継続時間
	rumble2Sec_ = 0.08f;
	// 2段目左モーター強度
	rumble2Left_ = 0;
	// 2段目右モーター強度
	rumble2Right_ = 42000;

	return true;
}

void Player::Draw(TKM::DirectXCommon* dxCommon) {

	// 回避残像を描画する
	dodge_->Draw(dxCommon);

	//=========================================================
	// 無敵点滅中の本体描画制御
	//=========================================================
	if (health_ && health_->IsInvincible() && !health_->IsVisible()) {
		// 点滅の非表示タイミングなので描かない
	} else {
		// 本体を描画する
		object_->Draw(dxCommon);

		// ヒレがあれば描画する
		if (flipper_) flipper_->Draw(dxCommon);
	}

	// レティクル表示フラグが立っていれば描画する
	if (reticle_ && reticleVisible_ && !health_->IsDead()) {
		reticle_->Draw(dxCommon);
	}

	// 弾を描画する
	shotManager_->DrawBullets(dxCommon);
}

float Player::GetDodgeCooldownGaugeRate() const {
	return dodge_ ? dodge_->GetCooldownGaugeRate() : 1.0f;
}

Vector2 Player::GetDodgeCooldownGaugeScreenPos(float screenW, float screenH) const {
	// 本体またはカメラがなければ画面中央を返す
	if (!object_ || !camera_) {
		return { screenW * 0.5f, screenH * 0.5f };
	}

	// プレイヤーの足元にしたいワールド座標
	Vector3 worldPos = object_->GetTranslate();

	// モデルの少し下に出す
	constexpr float kGaugeWorldOffsetY = -0.9f;
	worldPos.y += kGaugeWorldOffsetY;

	// ワールド座標をスクリーン座標へ変換
	const Matrix4x4 viewProjectionMatrix =
		camera_->GetViewMatrix() * camera_->GetProjectionMatrix();

	// ビューポート行列を作る
	const Matrix4x4 viewportMatrix =
		MyMath::MakeViewportMatrix(
			0.0f,    // ビューポート左上X
			0.0f,    // ビューポート左上Y
			screenW, // ビューポート幅
			screenH, // ビューポート高さ
			0.0f,    // ニアクリップ距離
			1.0f     // ファークリップ距離
		);

	// ワールド座標をスクリーン座標へ変換する
	Vector3 screenPos =
		MyMath::Transform(
			MyMath::Transform(worldPos, viewProjectionMatrix), // ワールド→スクリーン変換	
			viewportMatrix
		);

	return { screenPos.x, screenPos.y }; // 
}

void Player::SetHP(int hp) {
	// HP管理側へHPを設定する
	health_->SetHP(hp);

	// HPが変化したのでHUDへ通知する
	NotifyHudState_();
}

void Player::SetCamera(TKM::Camera* camera) {
	// カメラ参照を保持する
	this->camera_ = camera;

	// 本体へカメラを渡す
	if (object_) { object_->SetCamera(camera); }

	// レティクルへカメラを渡す
	if (reticle_) { reticle_->SetCamera(camera); }

	// ヒレへカメラを渡す
	if (flipper_) { flipper_->SetCamera(camera); }

	// 回避残像にもカメラを渡す
	if (dodge_) { dodge_->SetCamera(camera); }

	// ショットマネージャーへもカメラを渡す
	shotManager_->SetCamera(camera);
}

void Player::SetPosition(const Vector3& pos) {
	// 本体位置を設定する
	object_->SetTranslate(pos);
}

void Player::SetParentScene(TKM::BaseScene* scene) {
	// 親シーンを保持する
	parentScene_ = scene;
}

void Player::SetEnemy(Enemy* enemy) {
	// ショットマネージャーへ敵参照を渡す
	shotManager_->SetEnemy(enemy);
}

void Player::SetAllEnemies(std::vector<std::unique_ptr<Enemy>>* enemies) {
	// ショットマネージャーへ敵リスト参照を渡す
	shotManager_->SetAllEnemies(enemies);
}

void Player::SetControlEnabled(bool enabled) {
	// 操作有効フラグを更新する
	controlEnabled_ = enabled;

	// レティクル入力有効状態も合わせる
	if (reticle_) {
		reticle_->SetInputEnabled(enabled);
	}
}

void Player::SetReticleVisible(bool visible) {
	// レティクル描画フラグを更新する
	reticleVisible_ = visible;
}

void Player::SetBarrierCore(BarrierCore* core) {
	// ショットマネージャーへバリアコア参照を渡す
	shotManager_->SetBarrierCore(core);
}

void Player::SetColliderScale(const Vector3& s) {
	// 当たり判定サイズを更新する
	colliderScale_ = s;
}

void Player::SetRadialBlurEffect(TKM::RadialBlurEffect* effect) {
	// ラジアルブラー参照を保持する
	radialBlur_ = effect;
}

void Player::SetRotation(const Vector3& r) {
	// 本体回転を直接設定する
	object_->SetRotate(r);
}

void Player::StartCameraShake(int frameCount) {
	// カメラシェイク継続フレーム数を設定する
	cameraShakeFrame_ = frameCount;
}

void Player::StopRumble() {
	//=========================================================
	// 振動状態リセット
	//=========================================================
	rumbleT_ = 0.0f;
	rumbleLeft_ = 0;
	rumbleRight_ = 0;

	// 追い振動もリセット
	rumble2Pending_ = false;
	rumble2DelayT_ = 0.0f;
	rumble2Sec_ = 0.0f;
	rumble2Left_ = 0;
	rumble2Right_ = 0;

	// 実際の振動も停止する
	TKM::Input::GetInstance()->SetVibration(0, 0);
}

void Player::SetYaw(float yawRad) {
	// 本体未生成なら何もしない
	if (!object_) { return; }

	// 現在回転を取得する
	Vector3 r = object_->GetRotate();

	// ヨーだけ更新する
	r.y = yawRad;

	// 回転を反映する
	object_->SetRotate(r);
}

void Player::SetWave1BarrierInfo(bool active, const Vector3& center, const Vector3& size) {
	// バリア有効状態を更新する
	wave1BarrierActive_ = active;

	// バリア中心位置を更新する
	wave1BarrierCenter_ = center;

	// バリアサイズを更新する
	wave1BarrierSize_ = size;

	// 無効化されたらヒット履歴を消す
	if (!wave1BarrierActive_) {
		wave1BarrierHits_.clear();
	}
}

void Player::SetBarrierCoreManager(BarrierCoreManager* manager) {
	// ショットマネージャーへバリアコアマネージャー参照を渡す
	shotManager_->SetBarrierCoreManager(manager);
}

void Player::UpdateTitleIdle(float dt) {
	// タイトル中はヒレだけ動かす
	UpdateFlipperAnim_(dt);

	// 本体行列だけ更新する
	if (object_) { object_->Update(); }

	// ヒレ行列だけ更新する
	if (flipper_) { flipper_->Update(); }
}

void Player::AddWave1BarrierHit(const Vector3& worldPos) {
	//=========================================================
	// バリアヒット情報生成
	//=========================================================
	Wave1BarrierHit hit_;
	hit_.worldPos_ = worldPos;
	hit_.age_ = 0.0f;
	hit_.life_ = 0.35f;

	// 履歴へ追加する
	wave1BarrierHits_.push_back(hit_);

	// 上限超過時は古いものから削除する
	if (wave1BarrierHits_.size() > kWave1BarrierHitMax_) {
		wave1BarrierHits_.erase(wave1BarrierHits_.begin());
	}
}

void Player::OnBarrierCoreDestroyed(BarrierCore* core) {
	// コア破壊時の処理をショットマネージャーへ通知する
	shotManager_->OnBarrierCoreDestroyed(core);
}

void Player::RequestWave1BarrierFlash(const Vector3& worldPos) {
	// フラッシュ要求フラグを立てる
	wave1BarrierFlashRequested_ = true;

	// フラッシュ位置を記録する
	wave1BarrierFlashPos_ = worldPos;
}

bool Player::ConsumeWave1BarrierFlashRequest(Vector3& outWorldPos) {
	// 要求が無ければ false を返す
	if (!wave1BarrierFlashRequested_) {
		return false;
	}

	// 呼び出し元へ位置を返す
	outWorldPos = wave1BarrierFlashPos_;

	// 要求フラグを消す
	wave1BarrierFlashRequested_ = false;
	return true;
}

void Player::SetShootingEnabled(bool enabled) {
	// 射撃有効フラグを更新する
	shootingEnabled_ = enabled;

	// ショットマネージャーへも通知する
	shotManager_->SetShootingEnabled(enabled);
}

bool Player::IsDodgeCooldown() const {
	return dodge_ ? dodge_->IsCooldown() : false;
}

bool Player::IsDodgeCooldownGaugeVisible() const {
	return dodge_ ? dodge_->IsCooldownGaugeVisible() : false;
}

const std::list<std::unique_ptr<PlayerBullet>>& Player::GetBullets() const {
	// ショットマネージャーが管理する弾リストを返す
	return shotManager_->GetBullets();
}

bool Player::IsRbRefilling() const {
	// RB回復状態を返す
	return shotManager_ ? shotManager_->IsRbRefilling() : false;
}

bool Player::IsRbRefillingFromEmpty() const {
	return shotManager_ ? shotManager_->IsRbRefillingFromEmpty() : false;
}

int Player::GetRbAmmo() const {
	// 現在RB弾数を返す
	return shotManager_ ? shotManager_->GetRbAmmo() : 0;
}

int Player::GetRbAmmoMax() const {
	// 最大RB弾数を返す
	return shotManager_ ? shotManager_->GetRbAmmoMax() : 0;
}

int Player::GetLbAmmo() const {
	// 現在LB弾数を返す
	return shotManager_ ? shotManager_->GetLbAmmo() : 0;
}

int Player::GetLbAmmoMax() const {
	// 最大LB弾数を返す
	return shotManager_ ? shotManager_->GetLbAmmoMax() : 0;
}

void Player::SetRumbleEnabled(bool enabled) {
	// 振動許可フラグを更新する
	rumbleEnabled_ = enabled;

	// 無効化時は現在の振動も即停止する
	if (!enabled) {
		StopRumble();
	}
}

void Player::SetRotate(const Vector3& rotRad) {
	// 本体未生成なら何もしない
	if (!object_) { return; }

	// 回転を直接設定する
	object_->SetRotate(rotRad);
}

void Player::HandleGamePadMove() {
	// 本体未生成なら何もしない
	if (!object_) return;

	// 現在位置を取得する
	Vector3 pos = object_->GetTranslate();

	// 新しい位置の初期値は現在位置
	Vector3 newPos = pos;

	// このフレームで移動したかどうか
	bool movingThisFrame = false;

	if (reticle_) {
		// 目標位置を現在位置で初期化する
		Vector3 target = pos;

		// レティクル中心ワールド座標を取得する
		Vector3 aim = reticle_->GetCenterWorldPos();

		// X/Y だけレティクルへ追従し、Z は固定する
		target.x = std::clamp(aim.x, moveMin_.x, moveMax_.x);
		target.y = std::clamp(aim.y, moveMin_.y, moveMax_.y);
		target.z = 0.0f;

		// 目標との差分を求める
		Vector3 diff = { target.x - pos.x, target.y - pos.y, 0.0f };

		// 距離の二乗を求める
		float dist2 = diff.x * diff.x + diff.y * diff.y;

		// これ以下なら追いついたとみなす
		const float stopDist = 0.02f;

		if (dist2 > stopDist * stopDist) {
			// 補間率
			const float follow = 0.12f;

			// X方向を補間する
			newPos.x = MyMath::Lerp(pos.x, target.x, follow);

			// Y方向を補間する
			newPos.y = MyMath::Lerp(pos.y, target.y, follow);

			// Z は常に固定
			newPos.z = 0.0f;

			// このフレームで移動あり
			movingThisFrame = true;
		} else {
			// ほぼ追いついたら目標座標へ揃える
			newPos = target;
		}

		// 範囲外へ出ないようにクランプする
		newPos.x = std::clamp(newPos.x, moveMin_.x, moveMax_.x);
		newPos.y = std::clamp(newPos.y, moveMin_.y, moveMax_.y);
	}

	//=========================================================
	// バンク・ピッチ更新
	//=========================================================
	float vx = newPos.x - pos.x;
	float vy = newPos.y - pos.y;

	if (movingThisFrame) {
		// 左右傾き強度
		const float kBankStrength_ = 0.8f;

		// 上下傾き強度
		const float kPitchStrength_ = 0.45f;

		// 追従バネ強度
		const float kSpring_ = 0.25f;

		// 減衰
		const float kDamping_ = 0.45f;

		// 目標バンク角を計算する
		float targetBank = -vx * kBankStrength_;

		// 目標ピッチ角を計算する
		float targetPitch = -vy * kPitchStrength_;

		// バンクをばねで追従させる
		bankVel_ += (targetBank - bankAngle_) * kSpring_ - bankVel_ * kDamping_;
		bankAngle_ += bankVel_;

		// ピッチをばねで追従させる
		pitchVel_ += (targetPitch - pitchAngle_) * kSpring_ - pitchVel_ * kDamping_;
		pitchAngle_ += pitchVel_;
	} else {
		// バンク戻し用バネ強度
		const float kResetSpring_ = 0.25f;

		// バンク戻し用減衰
		const float kResetDamping_ = 0.5f;

		// バンクを0へ戻す
		bankVel_ += (0.0f - bankAngle_) * kResetSpring_ - bankVel_ * kResetDamping_;
		bankAngle_ += bankVel_;

		// ピッチを0へ戻す
		pitchVel_ += (0.0f - pitchAngle_) * kResetSpring_ - pitchVel_ * kResetDamping_;
		pitchAngle_ += pitchVel_;

		// ほぼ止まったら完全に0へ吸着する
		if (std::fabs(bankAngle_) < 0.001f && std::fabs(bankVel_) < 0.001f) {
			bankAngle_ = 0.0f;
			bankVel_ = 0.0f;
		}

		// ほぼ止まったら完全に0へ吸着する
		if (std::fabs(pitchAngle_) < 0.001f && std::fabs(pitchVel_) < 0.001f) {
			pitchAngle_ = 0.0f;
			pitchVel_ = 0.0f;
		}
	}

	//=========================================================
	// 位置と姿勢反映
	//=========================================================
	object_->SetTranslate(newPos);

	Vector3 rot = object_->GetRotate();
	rot.x = pitchAngle_;
	rot.z = bankAngle_;
	object_->SetRotate(rot);
}

void Player::HandleFollowCamera() {
	// 内部固定dtで三人称追従を更新する
	const float dt = 1.0f / 60.0f;

	// 三人称追従カメラ更新
	UpdateCameraFollowThirdPerson(dt);
}

void Player::UpdateCameraFollowThirdPerson(float dt) {
	// カメラ未設定なら何もしない
	if (!camera_) return;

	// プレイヤー位置を取得する
	Vector3 playerPos = object_->GetTranslate();

	// カメラ回転を取得する
	Vector3 camRot = camera_->GetRotate();

	// 基本距離
	const float baseDistance = 40.0f;

	// 基本高さ
	const float baseHeight = 4.0f;

	//=========================================================
	// LT一時ズーム更新
	//=========================================================
	if (ltZoomActive_) {
		// 現在のズーム係数を更新する
		camZoom_ = ltZoomTween_.Update(dt);

		// INフェーズ完了後はホールドを消化する
		if (ltZoomTween_.Finished() && ltZoomTween_.end < ltZoomTween_.start) {
			if (ltZoomHold_ > 0.0f) {
				ltZoomHold_ -= dt;
			} else {
				// ホールド終了後は元へ戻す
				ltZoomTween_.Reset(ltZoomTween_.end, 1.0f, 0.25f, Ease::Type::OutCubic);
			}
		}

		// 完全に戻り切ったらズーム状態を終了する
		if (ltZoomTween_.Finished() && ltZoomTween_.end == 1.0f) {
			ltZoomActive_ = false;
			camZoom_ = 1.0f;
		}
	} else {
		// ズームしていない時は等倍
		camZoom_ = 1.0f;
	}

	//=========================================================
	// ボス撃破ズーム更新
	//=========================================================
	if (bossZoomActive_) {
		// 現在のボスズーム係数を更新する
		bossZoom_ = bossZoomTween_.Update(dt);

		// 到達したらその値を保持したままズーム更新終了
		if (bossZoomTween_.Finished()) {
			bossZoomActive_ = false;
		}
	}

	// 両ズームを掛け合わせた最終ズーム係数
	float zoom = camZoom_ * bossZoom_;

	// ズームに応じた実距離
	float distance = baseDistance / zoom;

	// カメラ高さ
	float height = baseHeight;

	// 現在のヨー角
	float angleY = camRot.y;

	// プレイヤー後方オフセットを計算する
	Vector3 offset = {
		std::sinf(angleY) * -distance,
		height,
		std::cosf(angleY) * -distance
	};

	//=========================================================
	// カメラシェイク更新
	//=========================================================
	if (cameraShakeFrame_ > 0) {
		// LTズーム時の補正量
		float zoomKick = std::max(0.0f, 1.0f - camZoom_);

		// 最終シェイク倍率
		float shakeGain = shakeBaseStrength_ + zoomKick * shakeZoomBoost_;

		// ランダムシェイクオフセットを作る
		cameraShakeOffset_.x = ((rand() % 100 - 50) / 500.0f) * shakeGain;
		cameraShakeOffset_.y = ((rand() % 100 - 50) / 500.0f) * shakeGain;
		cameraShakeOffset_.z = ((rand() % 100 - 50) / 500.0f) * shakeGain;

		// 残りフレームを減らす
		cameraShakeFrame_--;
	} else {
		// シェイク無し
		cameraShakeOffset_ = { 0,0,0 };
	}

	//=========================================================
	// カメラ位置反映
	//=========================================================
	Vector3 cameraPos = playerPos + offset + cameraShakeOffset_;
	camera_->SetTranslate(cameraPos);
}

void Player::ZoomCamera() {
	// LTズーム時の目標倍率
	const float kInTarget = 0.6f;

	// 寄る時間
	const float kInTime = 0.12f;

	// 戻る時間
	const float kOutTime = 0.25f;

	// 1回押しあたりのホールド時間
	const float kHoldUnit = 1.5f;

	// ホールド上限
	const float kHoldMax = 1.2f;

	// まだズーム中でなければ通常起動する
	if (!ltZoomActive_) {
		ltZoomActive_ = true;
		ltZoomTween_.Reset(1.0f, kInTarget, 0.18f, Ease::Type::OutCubic);
		ltZoomHold_ = kHoldUnit;
		return;
	}

	// 現在がINフェーズかを判定する
	const bool isInPhase = (ltZoomTween_.end < ltZoomTween_.start);

	// 現在トゥイーンが終了しているか
	const bool finished = ltZoomTween_.Finished();

	if (isInPhase) {
		if (!finished) {
			// まだ寄っている途中なら何もしない
			return;
		}

		// 寄りきってホールド中ならホールド時間だけ上書きする
		ltZoomHold_ = std::min(kHoldMax, std::max(ltZoomHold_, kHoldUnit));
		return;
	}

	// 戻り中に再入力されたら現在値から再度寄り始める
	ltZoomTween_.Reset(camZoom_, kInTarget, kInTime, Ease::Type::OutCubic);
	ltZoomHold_ = kHoldUnit;
}

void Player::StartRumble(float sec, WORD leftMotor, WORD rightMotor) {
	// 振動禁止中なら無視する
	if (!rumbleEnabled_) { return; }

	// 時間は長い方を残す
	rumbleT_ = std::max(rumbleT_, sec);

	// 左モーターは強い方を残す
	rumbleLeft_ = std::max(rumbleLeft_, leftMotor);

	// 右モーターは強い方を残す
	rumbleRight_ = std::max(rumbleRight_, rightMotor);

	// 実際に振動を設定する
	TKM::Input::GetInstance()->SetVibration(rumbleLeft_, rumbleRight_);
}

void Player::StartIntroForwardMove(float startOffsetZ, float durationSec) {
	if (!object_) { return; }

	// Z方向だけオフセットした位置を目標位置とする
	introForwardTargetPos_ = object_->GetTranslate();
	// 目標位置からさらにZ方向へオフセットした位置を開始位置とする
	introForwardStartPos_ = introForwardTargetPos_;
	introForwardStartPos_.z += startOffsetZ;

	// 開始位置に移動させる
	Vector3 pos = object_->GetTranslate();
	pos.z = introForwardStartPos_.z; // XとYは今のまま、Zだけ開始位置にする
	object_->SetTranslate(pos); // 開始位置に移動させる

	introForwardDuration_ = durationSec; // 移動にかける時間

	// 時間が0以下なら最低限の時間を設定する（0だと割り算で困るし、あまりに短いと見た目も良くない）
	if (introForwardDuration_ <= 0.0f) {
		introForwardDuration_ = 0.01f;
	}

	introForwardT_ = 0.0f; // 移動開始からの経過時間
	introForwardActive_ = true; // 移動開始
	reticle_->SetInputEnabled(false); // レティクルの入力を無効化しておく
}

void Player::UpdateIntroForwardMove_(float dt) {
	if (!introForwardActive_) { return; }

	// 経過時間を進める
	introForwardT_ += dt;

	// 補間率を0..1の範囲で計算する
	float t = introForwardT_ / introForwardDuration_;
	t = std::clamp(t, 0.0f, 1.0f);

	float easedT = 1.0f - std::pow(1.0f - t, 3.0f); // イーズアウトキューブで緩やかに開始する補間率

	Vector3 pos = object_->GetTranslate(); // 現在位置を取得

	// XとYは今のまま、Zだけ補間する
	pos.z = MyMath::Lerp(introForwardStartPos_.z, introForwardTargetPos_.z, easedT);

	object_->SetTranslate(pos); // 位置を更新

	// 終了判定
	if (t >= 1.0f) {
		// 念のため目標位置に揃える
		pos = object_->GetTranslate();
		pos.z = introForwardTargetPos_.z;
		object_->SetTranslate(pos); // 位置を更新

		introForwardActive_ = false; // 移動終了
		reticle_->SetInputEnabled(true); // 前進終了後にレティクル入力を戻す
	}
}

void Player::UpdateRumble(float dt) {
	//=========================================================
	// 2段目振動発火
	//=========================================================
	if (rumble2Pending_) {
		// 遅延時間を減らす
		rumble2DelayT_ -= dt;

		// 発火タイミングになったら2段目振動を開始する
		if (rumble2DelayT_ <= 0.0f) {
			rumble2Pending_ = false;
			StartRumble(rumble2Sec_, rumble2Left_, rumble2Right_);
		}
	}

	// 鳴っていなければ何もしない
	if (rumbleT_ <= 0.0f) { return; }

	//=========================================================
	// 振動継続時間更新
	//=========================================================
	rumbleT_ -= dt;

	if (rumbleT_ <= 0.0f) {
		// 振動状態をクリアする
		rumbleT_ = 0.0f;
		rumbleLeft_ = 0;
		rumbleRight_ = 0;

		// 実際の振動を止める
		TKM::Input::GetInstance()->SetVibration(0, 0);
	}
}

void Player::UpdateFlipperAnim_(float dt) {
	// ヒレが無ければ何もしない
	if (!flipper_) { return; }

	// アニメ時間を進める
	flipperAnimT_ += dt;

	// フラップ周波数から角速度を作る
	float w = 2.0f * MyMath::GetPI() * flipperFlapHz_;

	// サイン波を作る
	float s = std::sinf(flipperAnimT_ * w);

	// 上下フラップ量
	float flap = s * flipperFlapAmp_;

	// 横揺れ量
	float sway = std::sinf(flipperAnimT_ * (w * 0.55f) + 1.2f) * flipperYawSwayAmp_;

	// 基準回転から開始する
	Vector3 r = flipperBaseRot_;

	// Xを上下フラップに使う
	r.x += flap;

	// Yを横揺れに使う
	r.y += sway;

	// 回転を反映する
	flipper_->SetRotate(r);
}

void Player::UpdateFloatBob_(float dt) {
	// 浮遊演出無効なら何もしない
	if (!enableFloatBob_) { return; }

	// 本体が無ければ何もしない
	if (!object_) { return; }

	// 浮遊アニメ時間を進める
	floatT_ += dt;

	// サイン波の角速度を計算する
	float w = 2.0f * MyMath::GetPI() * floatHz_;

	// -1..+1 のサイン波
	float s = std::sinf(floatT_ * w);

	// 現在位置を取得する
	Vector3 pos = object_->GetTranslate();

	// Y方向へだけ揺らす
	pos.y += s * floatAmp_;

	// 位置を反映する
	object_->SetTranslate(pos);
}