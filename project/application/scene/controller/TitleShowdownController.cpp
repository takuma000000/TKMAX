#define NOMINMAX
#include "TitleShowdownController.h"
#include "TitleScene.h"
#include "ParticleManager.h"
#include "MyMath.h"
#include <algorithm>
#include <cmath>

namespace {
	// 度数法の角度をラジアンに変換する
	float DegToRad_(float deg) {
		return deg * 3.14159265f / 180.0f;
	}

	// fromからtoを見るためのPitch角を求める
	float LookAtPitch_(const Vector3& from, const Vector3& to) {
		// fromからtoへの方向ベクトルを作る
		Vector3 d = { to.x - from.x, to.y - from.y, to.z - from.z };

		// XZ平面上の距離を求める
		const float horiz = std::sqrt(d.x * d.x + d.z * d.z);

		// 上下方向の角度をPitchとして返す
		return -std::atan2f(d.y, (horiz < 0.0001f ? 0.0001f : horiz));
	}
}

void TitleShowdownController::Initialize(
	TitleScene* ownerScene,
	TKM::DirectXCommon* dxCommon,
	TKM::SrvManager* srvManager,
	TKM::Camera* camera
) {
	// 所有シーン参照を保持する
	ownerScene_ = ownerScene;

	// DirectX共通情報を保持する
	dxCommon_ = dxCommon;

	// SRV管理参照を保持する
	srvManager_ = srvManager;

	// カメラ参照を保持する
	camera_ = camera;

	// プレイヤーとボスの表示用アクターを生成する
	CreateActors_();

	//=========================================================
	// ビーム押し合い状態初期化
	//=========================================================

	// ビーム衝突位置を中央にする
	beamT_ = kTitleBeamCenterT_;

	// ビーム衝突位置の目標も中央にする
	beamTargetT_ = kTitleBeamCenterT_;

	// 目標変更タイマーを初期化する
	beamTargetTimer_ = 0.0f;

	// 微細揺れ用タイマーを初期化する
	beamMicroOscTime_ = 0.0f;

	// ビームクラッシュ発生間隔の蓄積時間を初期化する
	clashEmitAcc_ = 0.0f;

	// 最初のビーム目標位置を設定する
	ResetBeamTarget_();
}

void TitleShowdownController::Update(float dt, bool enableBeam) {
	// プレイヤーまたはボスが無ければ更新しない
	if (!player_ || !boss_) { return; }

	// プレイヤーをタイトル用の固定位置に配置する
	player_->SetPosition(playerPos_);

	// ボスをタイトル用の固定位置に配置する
	boss_->SetPosition(bossPos_);

	// プレイヤー回転の初期値
	Vector3 pRotRad{ 0.0f, 0.0f, 0.0f };

	// ボス回転の初期値
	Vector3 bRotRad{ 0.0f, 0.0f, 0.0f };

	//=========================================================
	// 自動見つめ合い回転
	//=========================================================

	// 自動見つめ合いが有効なら、お互いの方向を向くように回転を計算する
	if (autoLookAt_) {
		// プレイヤーからボスを見るYaw角を求める
		const float py = LookAtYaw_(playerPos_, bossPos_);

		// ボスからプレイヤーを見るYaw角を求める
		const float by = LookAtYaw_(bossPos_, playerPos_);

		// プレイヤーからボスを見るPitch角を求める
		const float pp = LookAtPitch_(playerPos_, bossPos_);

		// ボスからプレイヤーを見るPitch角を求める
		const float bp = LookAtPitch_(bossPos_, playerPos_);

		// プレイヤー回転に見つめ合い角度を入れる
		pRotRad = { pp, py, 0.0f };

		// ボス回転に見つめ合い角度を入れる
		bRotRad = { bp, by, 0.0f };
	}

	//=========================================================
	// 回転オフセット反映
	//=========================================================

	// プレイヤーのX回転オフセットを加算する
	pRotRad.x += DegToRad_(playerRotDeg_.x);

	// プレイヤーのY回転オフセットを加算する
	pRotRad.y += DegToRad_(playerRotDeg_.y);

	// プレイヤーのZ回転オフセットを加算する
	pRotRad.z += DegToRad_(playerRotDeg_.z);

	// ボスのX回転オフセットを加算する
	bRotRad.x += DegToRad_(bossRotDeg_.x);

	// ボスのY回転オフセットを加算する
	bRotRad.y += DegToRad_(bossRotDeg_.y);

	// ボスのZ回転オフセットを加算する
	bRotRad.z += DegToRad_(bossRotDeg_.z);

	// プレイヤー回転を反映する
	player_->SetRotate(pRotRad);

	// ボス回転を反映する
	boss_->SetRotate(bRotRad);

	// タイトル用のプレイヤー待機更新を行う
	player_->UpdateTitleIdle(dt);

	// ボスを更新する
	boss_->Update(dt);

	// ボス更新後もタイトル用固定位置へ戻す
	boss_->SetPosition(bossPos_);

	// ビーム押し合い位置を更新する
	UpdateBeamPush_(dt);

	// ビームが有効な時だけクラッシュ演出を更新する
	if (enableBeam && beamActive_) {
		UpdateBeamClash_(dt);
	}
}

void TitleShowdownController::Draw(TKM::DirectXCommon* dxCommon) {
	// プレイヤーまたはボスが無ければ描画しない
	if (!player_ || !boss_) { return; }

	// プレイヤーを描画する
	player_->Draw(dxCommon);

	// ボスを描画する
	boss_->Draw(dxCommon);
}

void TitleShowdownController::SetBeamActive(bool active) {
	// ビーム演出の有効状態を設定する
	beamActive_ = active;
}

void TitleShowdownController::CreateActors_() {
	//=========================================================
	// プレイヤーアクター生成
	//=========================================================

	// タイトル表示用プレイヤーを生成する
	player_ = std::make_unique<Player>();

	// プレイヤーを初期化する
	player_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);

	// プレイヤーの親シーンを設定する
	player_->SetParentScene(ownerScene_);

	// プレイヤーのカメラを設定する
	player_->SetCamera(camera_);

	// プレイヤーをタイトル用位置に配置する
	player_->SetPosition(playerPos_);

	// タイトル演出中はプレイヤー操作を無効にする
	player_->SetControlEnabled(false);

	// タイトル演出中は射撃を無効にする
	player_->SetShootingEnabled(false);

	// タイトル演出中はレティクルを非表示にする
	player_->SetReticleVisible(false);

	// タイトル演出中はジェットスモークを無効にする
	player_->SetEnableJetSmoke(false);

	// タイトル用なのでターゲット敵は設定しない
	player_->SetEnemy(nullptr);

	// タイトル中の不要な振動を止める
	player_->StopRumble();

	//=========================================================
	// ボスアクター生成
	//=========================================================

	// タイトル表示用ボスを生成する
	boss_ = std::make_unique<BossEnemy>();

	// ボスのカメラを設定する
	boss_->SetCamera(camera_);

	// ボスの親シーンを設定する
	boss_->SetParentScene(ownerScene_);

	// ボスを初期化する
	boss_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);

	// ボスをタイトル用位置に配置する
	boss_->SetPosition(bossPos_);

	// タイトル演出用に触手チャージ状態を設定する
	boss_->SetTentacleCharge(true, 0.35f);

	// 初期Transformを同期する
	boss_->SyncTransform();

	//=========================================================
	// 初期回転設定
	//=========================================================

	// プレイヤーからボスを見るYaw角を求める
	const float py = LookAtYaw_(playerPos_, bossPos_);

	// ボスからプレイヤーを見るYaw角を求める
	const float by = LookAtYaw_(bossPos_, playerPos_);

	// プレイヤーの初期Yawを保存する
	playerRot_.y = py;

	// ボスの初期Yawを保存する
	bossRot_.y = by;

	// プレイヤーをボス方向へ向ける
	player_->SetYaw(playerRot_.y);

	// ボスをプレイヤー方向へ向ける
	boss_->SetRotate(bossRot_);
}

float TitleShowdownController::LookAtYaw_(const Vector3& from, const Vector3& to) const {
	// fromからtoへの方向ベクトルを作る
	Vector3 d = { to.x - from.x, to.y - from.y, to.z - from.z };

	// XZ平面上の向きからYaw角を返す
	return std::atan2f(d.x, d.z);
}

void TitleShowdownController::UpdateBeamPush_(float dt) {
	// ビームの微細な揺れ時間を進める
	beamMicroOscTime_ += dt;

	//=========================================================
	// ビーム非アクティブ時の中央戻し
	//=========================================================

	// ビーム演出が無効なら、衝突位置を中央へ戻す
	if (!beamActive_) {
		// 現在位置から中央までの差分を求める
		const float toCenter = kTitleBeamCenterT_ - beamT_;

		// 中央へなめらかに近づける
		beamT_ += toCenter * std::clamp(dt * kTitleBeamNeutralReturnSpeed_, 0.0f, 1.0f);

		// ビーム位置を許可範囲に収める
		beamT_ = std::clamp(beamT_, kTitleBeamMinT_, kTitleBeamMaxT_);
		return;
	}

	// 目標変更までの時間を減らす
	beamTargetTimer_ -= dt;

	// タイマーが切れたら新しい目標位置を決める
	if (beamTargetTimer_ <= 0.0f) {
		ResetBeamTarget_();
	}

	// 現在位置を目標位置へ近づける割合を計算する
	const float approachRate = std::clamp(dt * kTitleBeamApproachSpeed_, 0.0f, 1.0f);

	// ビーム位置を目標へ近づける
	beamT_ += (beamTargetT_ - beamT_) * approachRate;

	// 微細な揺れを作る
	const float microOsc = std::sin(beamMicroOscTime_ * kTitleBeamMicroOscSpeed_) * kTitleBeamMicroOscAmp_;

	// 揺れを加える前の基準位置を保存する
	const float baseT = beamT_;

	// 微細揺れを加えた位置を許可範囲に収める
	beamT_ = std::clamp(baseT + microOsc, kTitleBeamMinT_, kTitleBeamMaxT_);
}

void TitleShowdownController::ResetBeamTarget_() {
	// 次に目標位置を変えるまでの時間分布を作る
	std::uniform_real_distribution<float> timeDist(
		kTitleBeamTargetChangeMinSec_,
		kTitleBeamTargetChangeMaxSec_
	);

	// ビーム衝突位置の目標値分布を作る
	std::uniform_real_distribution<float> sideDist(
		kTitleBeamMinT_,
		kTitleBeamMaxT_
	);

	// 次の目標変更までの時間をランダムに決める
	beamTargetTimer_ = timeDist(rng_);

	// 次のビーム衝突位置をランダムに決める
	beamTargetT_ = sideDist(rng_);
}

void TitleShowdownController::UpdateBeamClash_(float dt) {
	// プレイヤーまたはボスが無ければ更新しない
	if (!player_ || !boss_) { return; }

	// パーティクルマネージャーを取得する
	auto* pm = TKM::ParticleManager::GetInstance();

	// パーティクルマネージャーが無ければ演出を出せない
	if (!pm) { return; }

	// プレイヤー側ビーム開始位置を作る
	const Vector3 p0 = playerPos_ + Vector3{ 0.0f, kTitleBeamPlayerStartOffsetY_, 0.0f };

	// ボス側ビーム開始位置を作る
	const Vector3 b0 = bossPos_ + Vector3{ 0.0f, kTitleBeamBossStartOffsetY_, 0.0f };

	// ビーム衝突位置の補間率を取得する
	const float t = std::clamp(beamT_, 0.0f, 1.0f);

	// プレイヤー側とボス側の間で衝突位置を計算する
	const Vector3 hit = {
		p0.x + (b0.x - p0.x) * t,
		p0.y + (b0.y - p0.y) * t,
		p0.z + (b0.z - p0.z) * t
	};

	//=========================================================
	// プレイヤー側ビーム粒子
	//=========================================================

	// プレイヤーから衝突点までを分割して粒子を出す
	for (int i = 0; i < kTitleBeamSegments_; ++i) {
		// 0.0〜1.0の線形補間率を作る
		const float u = static_cast<float>(i) / static_cast<float>(kTitleBeamSegments_ - 1);

		// プレイヤー側ビーム上の発生位置を計算する
		const Vector3 p = {
			p0.x + (hit.x - p0.x) * u,
			p0.y + (hit.y - p0.y) * u,
			p0.z + (hit.z - p0.z) * u
		};

		// プレイヤー側ビーム粒子を発生させる
		pm->Emit("titleBeam_player", p, kTitleBeamPerSegment_);
	}

	//=========================================================
	// ボス側ビーム粒子
	//=========================================================

	// ボスから衝突点までを分割して粒子を出す
	for (int i = 0; i < kTitleBeamSegments_; ++i) {
		// 0.0〜1.0の線形補間率を作る
		const float u = static_cast<float>(i) / static_cast<float>(kTitleBeamSegments_ - 1);

		// ボス側ビーム上の発生位置を計算する
		const Vector3 p = {
			b0.x + (hit.x - b0.x) * u,
			b0.y + (hit.y - b0.y) * u,
			b0.z + (hit.z - b0.z) * u
		};

		// ボス側ビーム粒子を発生させる
		pm->Emit("titleBeam_boss", p, kTitleBeamPerSegment_);
	}

	//=========================================================
	// ビーム衝突点演出
	//=========================================================

	// 衝突点演出の発生タイマーを進める
	clashEmitAcc_ += dt;

	// 指定Hzから発生間隔を求める
	const float emitStep = 1.0f / kTitleBeamClashHz_;

	// 一定間隔ごとにクラッシュ演出を発生させる
	while (clashEmitAcc_ >= emitStep) {
		// 発生間隔分だけ蓄積時間を減らす
		clashEmitAcc_ -= emitStep;

		// 衝突点の中心光を出す
		pm->Emit("titleBeamClash_core", hit, kTitleClashCoreCount_);

		// 衝突点から伸びるレイを出す
		pm->Emit("titleBeamClash_rays", hit, kTitleClashRaysCount_);

		// 衝突点のリング衝撃波を出す
		pm->Emit("titleBeamClash_ring", hit, kTitleClashRingCount_);
	}
}