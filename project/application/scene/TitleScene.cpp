#define NOMINMAX
#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"
#include <algorithm>
#include <Windows.h>
#include <cmath>
#include "AudioCatalog.h"
#include "TextureCatalog.h"
#include "ModelCatalog.h"
#include "ParticleGroupsCatalog.h"
#include "TitleFlowStates.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

using TKM::Camera;
using TKM::TextureManager;
using TKM::ModelManager;
using TKM::Sprite;

/// <summary>
/// 度数法の角度をラジアンに変換します。
/// </summary>
/// <param name="deg">度数法の角度</param>
/// <returns>ラジアン角</returns>
static float DegToRad_(float deg) {
	return deg * 3.14159265f / 180.0f;
}

/// <summary>
/// 指定したZ座標における、カメラに映る範囲の半分の幅・高さを求めます。
/// </summary>
/// <param name="outHalfW">映る範囲の半分の横幅</param>
/// <param name="outHalfH">映る範囲の半分の高さ</param>
/// <param name="z">調べたいZ座標</param>
/// <param name="camZ">カメラのZ座標</param>
/// <param name="fovYRad">縦方向の視野角</param>
/// <param name="aspect">画面アスペクト比</param>
static void VisibleHalfExtentsAtZ_(
	float& outHalfW, float& outHalfH,
	float z, float camZ,
	float fovYRad, float aspect
) {
	// カメラから指定Z座標までの距離を求める
	float d = z - camZ;

	// 距離が小さすぎるとtan計算の結果が不安定になるため最低値を保証する
	if (d < 0.01f) { d = 0.01f; }

	// 縦方向の見える半分の高さを計算する
	outHalfH = std::tan(fovYRad * 0.5f) * d;

	// アスペクト比から横方向の見える半分の幅を計算する
	outHalfW = outHalfH * aspect;
}

/// <summary>
/// 指定位置から対象位置を見るためのピッチ角を求めます。
/// </summary>
/// <param name="from">見る側の位置</param>
/// <param name="to">見られる側の位置</param>
/// <returns>ピッチ角</returns>
static float LookAtPitch_(const Vector3& from, const Vector3& to) {
	// fromからtoへ向かう方向ベクトルを作る
	Vector3 d = { to.x - from.x, to.y - from.y, to.z - from.z };

	// XZ平面上での水平距離を求める
	const float horiz = std::sqrt(d.x * d.x + d.z * d.z);

	// 水平距離が小さすぎる場合は0除算に近い状態を避ける
	return -std::atan2f(d.y, (horiz < 0.0001f ? 0.0001f : horiz));
}

void TitleScene::Initialize() {
	/// ──────────────── カメラ初期化 ───────────────
	const Vector3 mainRot = { 0.0f, 0.0f, 0.0f };
	const Vector3 mainPos = { 0.0f, camY_, camDist_ };
	const Vector3 debugTarget = { 0.0f, 0.0f, 0.0f };

	// タイトルシーン用のメインカメラをCameraManager側で初期化する
	TKM::CameraManager::GetInstance()->Initialize(mainRot, mainPos, debugTarget);

	// 初期化したメインカメラをこのシーンで使用する
	camera_ = TKM::CameraManager::GetInstance()->GetMainCamera();

	/// ──────────────── テクスチャ読み込み ───────────────
	TextureCatalog::LoadTextureCatalogs();

	/// ──────────────── モデル読み込み ───────────────
	ModelCatalog::LoadModelCatalogs(dxCommon_);

	/// ──────────────── パーティクル初期化 ───────────────
	// タイトルシーンで使用するカメラを渡してパーティクルを初期化する
	TKM::ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, TKM::CameraManager::GetInstance()->GetMainCamera());

	// 前シーンなどのパーティクルグループが残らないように一度クリアする
	TKM::ParticleManager::GetInstance()->ClearAllGroups();

	// タイトルシーン用のパーティクルグループを登録する
	TKM::ParticleGroupsCatalog::RegisterScene(TKM::ParticleManager::GetInstance());

	/// ──────────────── タイトル演出フロー初期化 ───────────────
	// タイトル演出を管理するステートマシンを初期化する
	flowSM_.Initialize(this);

	// 最初はアイリスを開く演出から開始する
	flowSM_.Change(std::make_unique<TitleFlowIntroIrisOpenState>());

	/// ──────────────── タイトルスプライト初期化 ───────────────
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, "./resources/texture/title_kuraran.dds");

	// タイトル画像の表示位置を設定する
	sprite_->SetPosition({ -10.0f,-290.0f });

	// タイトル画像のサイズを設定する
	sprite_->SetSize({ 1.0f, 1.0f });

	/// ──────────────── ライト初期化 ───────────────
	dirLight_ = std::make_unique<TKM::DirectionalLight>();

	// 白色の平行光源を上方向から当てる
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f, -1.0f, 0.0f }, 1.0f);

	/// ──────────────── スカイボックス初期化 ───────────────
	skybox_ = std::make_unique<TKM::Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");

	// スカイボックスをメインカメラに追従させる
	skybox_->SetCamera(TKM::CameraManager::GetInstance()->GetMainCamera());

	/// ──────────────── アイリス初期化 ───────────────
	iris_ = CreateCenteredIrisSprite(dxCommon_, irisMax_, "./resources/texture/circle2.png");

	// アイリスの色を白に設定する
	iris_->SetColor({ 1,1,1,1 });

	// 開始時は画面を覆うサイズから始める
	irisStartScale_ = irisMax_;
	irisEndScale_ = 0.0f;
	irisScale_ = irisStartScale_;
	iris_->SetSize({ irisScale_, irisScale_ });

	// アイリスを開くTweenを設定する
	irisTween_.Reset(
		irisStartScale_,
		irisEndScale_,
		kIrisDurationSec_,
		Ease::Type::OutBack
	);

	// 開始直後はアイリスオープン中にする
	irisOpening_ = true;
	irisClosing_ = false;

	/// ──────────────── 水面波紋エフェクト初期化 ───────────────
	rippleEffect_ = std::make_unique<TKM::WaterRippleEffect>();
	rippleEffect_->Initialize(dxCommon_);

	// DirectXCommon側へ現在使用する波紋エフェクトを渡す
	dxCommon_->SetWaterRippleEffect(rippleEffect_.get());

	/// ──────────────── BGM読み込み・再生 ───────────────
	AudioCatalog::LoadTitleAudios();

	// タイトルBGMをループ再生する
	TKM::AudioManager::GetInstance()->PlaySound("title", 0.1f, true);

	/// ──────────────── タイトルメニュー初期化 ───────────────
	titleMenu_ = std::make_unique<TitleMenuController>();
	titleMenu_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		this,
		1280.0f,
		720.0f
	);

	/// ──────────────── タイトル敵初期化 ───────────────
	CreateTitleEnemies_();

	/// ──────────────── 見つめ合い演出初期化 ───────────────
	titleShowdown_ = std::make_unique<TitleShowdownController>();
	titleShowdown_->Initialize(this, dxCommon_, srvManager_, camera_);

	// タイトルメニュー表示時のビーム演出を有効にする
	titleShowdown_->SetBeamActive(true);

	/// ──────────────── 初期表示状態設定 ───────────────
	// 最初はUIを表示しない
	showUi_ = false;

	// タイトルメニューも非表示から始める
	titleMenu_->SetVisible(false);

	/// ──────────────── 分岐フラグ初期化 ───────────────
	showMenuAfterVanish_ = false;

	/// ──────────────── タイマー初期化 ───────────────
	seqTimer_ = 0.0f;
	vanishTimer_ = 0.0f;
	rippleTimer_ = 0.0f;
}

void TitleScene::Finalize() {
	/// ──────────────── 各種終了処理 ───────────────
	// タイトルシーンで使用していた音声を終了する
	TKM::AudioManager::GetInstance()->Finalize();

	// タイトルシーンで出していたパーティクルを残さないように削除する
	TKM::ParticleManager::GetInstance()->ClearAllGroups();
}

void TitleScene::Update() {
	/// ──────────────── パフォーマンス情報更新 ───────────────
	UpdatePerformanceInfo();

	/// ──────────────── 入力更新 ───────────────
	TKM::Input::GetInstance()->Update();

	/// ──────────────── 基本システム更新 ───────────────
	// ライト情報を更新する
	dirLight_->Update();

	// カメラマネージャ側のカメラ更新を行う
	TKM::CameraManager::GetInstance()->Update();

	// タイトル画像スプライトを更新する
	sprite_->Update();

	// 波紋エフェクトを更新する
	rippleEffect_->Update(dt_);

	/// ──────────────── タイトル演出フロー更新 ───────────────
	// ステート側で早期returnを要求するかどうかを毎フレーム初期化する
	earlyExitUpdate_ = false;

	// 現在のタイトル演出ステートを更新する
	flowSM_.Update(dt_);

	// ステート側でシーン遷移などを行った場合は、この後の通常更新を止める
	if (earlyExitUpdate_) { return; }

	/// ──────────────── デバッグ遷移 ───────────────
	// Tキーで直接ゲームクリアシーンへ遷移する
	if (TKM::Input::GetInstance()->TriggerKey(DIK_T)) {
		sceneManager_->SetNextScene(
			std::make_unique<GameClearScene>(dxCommon_, srvManager_)
		);
		return;
	}

	/// ──────────────── スカイボックス回転更新 ───────────────
	constexpr float kTwoPi = 6.2831853f;

	// スカイボックスのピッチ角を少しずつ動かす
	skyPitch_ -= skyRotSpeedX_;

	// 角度が大きくなりすぎないように0～2π付近へ戻す
	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;

	// 回転をスカイボックスへ反映する
	skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });

	/// ──────────────── パーティクル更新 ───────────────
	TKM::ParticleManager::GetInstance()->Update(dt_);

#ifdef USE_IMGUI
	/// ──────────────── ImGuiデバッグ表示 ───────────────
	ImGui::Begin("タイトルシーン デバッグ");

	/// ──────────────── パフォーマンス情報 ───────────────
	ImGuiDebugInfo();

	/// ──────────────── タイトル敵情報 ───────────────
	if (ImGui::CollapsingHeader("タイトル敵情報", ImGuiTreeNodeFlags_DefaultOpen)) {

		int aliveCount = 0;
		int totalCount = static_cast<int>(titleEnemies_.size());

		// 現在生きているタイトル敵の数を数える
		for (const auto& u : titleEnemies_) {
			if (u.alive_) {
				aliveCount++;
			}
		}

		// タイトル敵の状態を表示する
		ImGui::Text("生存数 : %d", aliveCount);
		ImGui::Text("総数   : %d", totalCount);
		ImGui::Text("消滅数 : %d", totalCount - aliveCount);

		ImGui::Separator();
	}

	/// ──────────────── Flow・タイマー情報 ───────────────
	if (ImGui::CollapsingHeader("状態 / タイマー", ImGuiTreeNodeFlags_DefaultOpen)) {

		const char* flowName = "（なし）";

		// 現在のステート型を見て、表示用の名前に変換する
		if (flowSM_.GetState()) {
			if (dynamic_cast<TitleFlowIntroIrisOpenState*>(flowSM_.GetState())) { flowName = "アイリスオープン中"; } else if (dynamic_cast<TitleFlowIdleState*>(flowSM_.GetState())) { flowName = "待機中"; } else if (dynamic_cast<TitleFlowVanishingState*>(flowSM_.GetState())) { flowName = "消滅演出中"; } else if (dynamic_cast<TitleFlowRippleState*>(flowSM_.GetState())) { flowName = "波紋演出中"; } else if (dynamic_cast<TitleFlowIrisCloseState*>(flowSM_.GetState())) { flowName = "アイリスクローズ中"; }
		}

		// 現在の演出ステートと各タイマーを表示する
		ImGui::Text("現在の状態 : %s", flowName);
		ImGui::Separator();
		ImGui::Text("シーケンスタイマー : %.2f 秒", seqTimer_);
		ImGui::Text("消滅タイマー       : %.2f 秒", vanishTimer_);
		ImGui::Text("波紋タイマー       : %.2f 秒", rippleTimer_);
	}

	ImGui::End();
#endif
}

void TitleScene::Draw() {
	// 背景、3D、2Dの順で描画する
	DrawBack();
	Draw3D();
	DrawSprite();
}

void TitleScene::Draw3D() {
	/// ──────────────── 3D描画共通設定 ───────────────
	TKM::Object3dCommon::GetInstance()->DrawSetCommon();

	/// ──────────────── タイトル敵描画 ───────────────
	for (auto& u : titleEnemies_) {
		// 消滅済み、または実体がない敵は描画しない
		if (!u.alive_ || !u.enemy_) { continue; }

		u.enemy_->Draw(dxCommon_);
	}

	/// ──────────────── 見つめ合い演出描画 ───────────────
	// UIとメニューが表示されている間だけ、見つめ合い演出を描画する
	if (showUi_ && titleMenu_->IsVisible()) {
		titleShowdown_->Draw(dxCommon_);
	}

	/// ──────────────── パーティクル描画 ───────────────
	TKM::ParticleManager::GetInstance()->Draw();
}

void TitleScene::DrawSprite() {
	/// ──────────────── 2D描画共通設定 ───────────────
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	/// ──────────────── メニュー描画 ───────────────
	if (showUi_) {
		titleMenu_->Draw();
	}

	/// ──────────────── アイリス描画 ───────────────
	// シーン開始時、または遷移時だけアイリスを描画する
	if (irisOpening_ || irisClosing_) {
		iris_->Draw();
	}
}

void TitleScene::DrawBack() {
	/// ──────────────── スカイボックス描画 ───────────────
	skybox_->Draw();

	/// ──────────────── 背景スプライト描画共通設定 ───────────────
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	/// ──────────────── タイトル画像描画 ───────────────
	if (showUi_) {
		sprite_->Draw();
	}
}

void TitleScene::CreateTitleEnemies_() {
	/// ──────────────── タイトル敵配列初期化 ───────────────
	// 既存のタイトル敵を削除する
	titleEnemies_.clear();

	// 生成数が決まっているため、先に容量を確保する
	titleEnemies_.reserve(kEnemyCount);

	/// ──────────────── 画面・カメラ情報設定 ───────────────
	const float screenW = 1280.0f;
	const float screenH = 720.0f;
	const float aspect = screenW / screenH;

	// 敵配置に使う縦視野角
	const float fovY = DegToRad_(60.0f);

	// 現在のカメラZ座標
	const float camZ = camera_->GetTranslate().z;

	/// ──────────────── 出現Z範囲設定 ───────────────
	// カメラに近い敵のZ範囲
	std::uniform_real_distribution<float> nearZ(6.0f, 14.0f);

	// カメラから遠い敵のZ範囲
	std::uniform_real_distribution<float> farZ(14.0f, 28.0f);

	// 全体のうち近距離側に配置する割合
	const float kNearRatio = 0.65f;

	// 近距離側に配置する数
	const int nearCount = static_cast<int>(kEnemyCount * kNearRatio);

	/// ──────────────── タイトル敵生成 ───────────────
	for (int i = 0; i < kEnemyCount; ++i) {
		// 前半は近距離、後半は遠距離として扱う
		const bool isNear = (i < nearCount);

		// 近距離用、遠距離用のどちらかからZ座標を決める
		float z = isNear ? nearZ(rng_) : farZ(rng_);

		float halfW = 0.0f, halfH = 0.0f;

		// 現在のZ座標で画面に映る範囲を計算する
		VisibleHalfExtentsAtZ_(halfW, halfH, z, camZ, fovY, aspect);

		// 画面端ギリギリに出ないように、近距離・遠距離で余白を変える
		const float margin = isNear ? 6.0f : 10.0f;

		// 余白を引いた後も最低限の範囲は残す
		halfW = std::max(1.0f, halfW - margin);
		halfH = std::max(1.0f, halfH - margin);

		// 計算した表示範囲内でランダムなX・Yを作る
		std::uniform_real_distribution<float> rx(-halfW, halfW);
		std::uniform_real_distribution<float> ry(-halfH, halfH);

		// 敵の初期位置を決める
		Vector3 pos = { rx(rng_), ry(rng_), z };

		TitleEnemyUnit u{};

		// タイトル用の敵を生成する
		u.enemy_ = std::make_unique<Enemy>();

		// 敵にタイトルシーンのカメラと親シーンを渡す
		u.enemy_->SetCamera(camera_);
		u.enemy_->SetParentScene(this);

		// 敵本体を初期化する
		u.enemy_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);

		// 敵本体と触手モデルを設定する
		u.enemy_->SetModel("jerryfish.obj");
		u.enemy_->SetTentacleModel("tentacle.obj");

		// 触手のローカルTransformを初期化する
		u.enemy_->SetTentacleLocal({ 0,0,0 }, { 0,0,0 }, { 1,1,1 });

		// 近くの敵は少し大きめ、遠くの敵は少し小さめにする
		u.enemy_->SetScale(isNear ? Vector3{ 1.35f, 1.35f, 1.35f } : Vector3{ 1.10f, 1.10f, 1.10f });

		// 初期位置を反映する
		u.enemy_->SetPosition(pos);

		// タイトル画面では自由移動挙動にする
		u.enemy_->SetBehavior(EnemyBehavior::FreeRoam);

		// 敵が自由移動できる範囲を、画面内の見える範囲に合わせる
		Vector3 roamMin = { -halfW, -halfH, z };
		Vector3 roamMax = { halfW,  halfH, z };
		u.enemy_->SetRoamArea(roamMin, roamMax);

		// タイトル画面ではゆっくり漂うように移動させる
		u.enemy_->SetRoamSpeed(0.10f, 0.10f);

		// 生成直後は生存状態にする
		u.alive_ = true;

		// 消滅タイミングは後でScheduleVanish_側で設定する
		u.vanishDelay_ = 0.0f;

		// タイトル敵配列へ追加する
		titleEnemies_.push_back(std::move(u));
	}
}

void TitleScene::ScheduleVanish_() {
	/// ──────────────── 消滅タイミング設定 ───────────────
	// 0秒～最大遅延時間の間でランダムな消滅遅延を作る
	std::uniform_real_distribution<float> d(0.0f, kVanishDelayMaxSec_);

	for (auto& u : titleEnemies_) {
		// 敵ごとに消滅するタイミングをずらす
		u.vanishDelay_ = d(rng_);
	}

	/// ──────────────── 消滅タイマー初期化 ───────────────
	vanishTimer_ = 0.0f;
}

void TitleScene::EmitTitleExplode_(const Vector3& pos) {
	/// ──────────────── タイトル敵消滅爆発 ───────────────
	Vector3 p = pos;
	auto* pm = TKM::ParticleManager::GetInstance();

	// 爆発の中心
	pm->Emit("titleExplode_core", p, 28);

	// 放射状の光
	pm->Emit("titleExplode_rays", p, 140);

	// 飛び散る破片
	pm->Emit("titleExplode_debris", p, 90);

	// 広がるリング
	pm->Emit("titleExplode_ring", p, 2);
}

bool TitleScene::AllEnemiesGone_() const {
	/// ──────────────── タイトル敵全消滅判定 ───────────────
	for (const auto& u : titleEnemies_) {
		// 1体でも生きていれば、まだ全消滅ではない
		if (u.alive_) { return false; }
	}

	// すべて消えている
	return true;
}