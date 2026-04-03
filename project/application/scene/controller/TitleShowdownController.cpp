#define NOMINMAX
#include "TitleShowdownController.h"
#include "TitleScene.h"
#include "ParticleManager.h"
#include "MyMath.h"
#include <algorithm>
#include <cmath>

namespace {
	// 度をラジアンに変換する関数
	float DegToRad_(float deg) {
		return deg * 3.14159265f / 180.0f;
	}
	// from から to へのPitchを求めます。
	float LookAtPitch_(const Vector3& from, const Vector3& to) {
		Vector3 d = { to.x - from.x, to.y - from.y, to.z - from.z };
		const float horiz = std::sqrt(d.x * d.x + d.z * d.z);
		return -std::atan2f(d.y, (horiz < 0.0001f ? 0.0001f : horiz));
	}
}

void TitleShowdownController::Initialize(
	TitleScene* ownerScene,
	TKM::DirectXCommon* dxCommon,
	TKM::SrvManager* srvManager,
	TKM::Camera* camera
) {
	ownerScene_ = ownerScene;
	dxCommon_ = dxCommon;
	srvManager_ = srvManager;
	camera_ = camera;

	// アクター生成
	CreateActors_();

	// ビーム関連初期化
	beamT_ = kTitleBeamCenterT_;
	beamTargetT_ = kTitleBeamCenterT_;
	beamTargetTimer_ = 0.0f;
	beamMicroOscTime_ = 0.0f;
	// クラッシュ演出関連初期化
	clashEmitAcc_ = 0.0f;

	// 最初の目標位置を設定
	ResetBeamTarget_();
}

void TitleShowdownController::Update(float dt, bool enableBeam) {
	if (!player_ || !boss_) { return; }

	// プレイヤーとボスの位置を設定する
	player_->SetPosition(playerPos_);
	boss_->SetPosition(bossPos_); 

	// 自動見つめ合い
	Vector3 pRotRad{ 0.0f, 0.0f, 0.0f };
	Vector3 bRotRad{ 0.0f, 0.0f, 0.0f };

	// 自動見つめ合いが有効な場合、プレイヤーとボスがお互いを向くように回転を計算する
	if (autoLookAt_) {
		// プレイヤーから見たボスの方向と、ボスから見たプレイヤーの方向を計算して、それぞれのYawとPitchを求める
		const float py = LookAtYaw_(playerPos_, bossPos_);
		const float by = LookAtYaw_(bossPos_, playerPos_);
		const float pp = LookAtPitch_(playerPos_, bossPos_);
		const float bp = LookAtPitch_(bossPos_, playerPos_);
		// プレイヤーとボスの回転を、見つめ合いの角度 + オフセット角度（Deg）で設定する
		pRotRad = { pp, py, 0.0f };
		bRotRad = { bp, by, 0.0f };
	}

	// プレイヤーとボスの回転オフセット（Deg）をラジアンに変換して加算する
	pRotRad.x += DegToRad_(playerRotDeg_.x);
	pRotRad.y += DegToRad_(playerRotDeg_.y);
	pRotRad.z += DegToRad_(playerRotDeg_.z);
	// プレイヤーとボスの回転オフセット（Deg）をラジアンに変換して加算する
	bRotRad.x += DegToRad_(bossRotDeg_.x);
	bRotRad.y += DegToRad_(bossRotDeg_.y);
	bRotRad.z += DegToRad_(bossRotDeg_.z);

	// 計算した回転をプレイヤーとボスに設定する
	player_->SetRotate(pRotRad); 
	boss_->SetRotate(bRotRad);
	// プレイヤーとボスの更新処理を呼び出す
	player_->UpdateTitleIdle(dt);
	boss_->Update(dt);
	boss_->SetPosition(bossPos_);

	// ビーム押し合い位置の更新
	UpdateBeamPush_(dt);

	// ビームクラッシュ演出の更新
	if (enableBeam && beamActive_) {
		UpdateBeamClash_(dt);
	}
}

void TitleShowdownController::Draw(TKM::DirectXCommon* dxCommon) {
	if (!player_ || !boss_) { return; }

	// プレイヤーとボスの描画処理を呼び出す
	player_->Draw(dxCommon);
	boss_->Draw(dxCommon);
}

void TitleShowdownController::SetBeamActive(bool active) {
	beamActive_ = active; // ビーム演出の有効状態を設定する
}

void TitleShowdownController::CreateActors_() {
	// プレイヤーとボスのアクターを生成して初期化する
	player_ = std::make_unique<Player>();
	player_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);
	player_->SetParentScene(ownerScene_); // プレイヤーの親シーンを設定する
	player_->SetCamera(camera_); // プレイヤーのカメラを設定する
	player_->SetPosition(playerPos_); // プレイヤーの初期位置を設定する
	player_->SetControlEnabled(false); // プレイヤーの操作を無効にする
	player_->SetShootingEnabled(false); // プレイヤーの射撃を無効にする
	player_->SetReticleVisible(false); // プレイヤーの照準を非表示にする
	player_->SetEnableJetSmoke(false); // プレイヤーのジェットスモークを無効にする
	player_->SetEnemy(nullptr); // プレイヤーのターゲット敵を設定しない
	player_->StopRumble(); // プレイヤーのゲームパッド振動を停止する
	// プレイヤーのHPを最大にする
	boss_ = std::make_unique<BossEnemy>();
	boss_->SetCamera(camera_); // ボスのカメラを設定する
	boss_->SetParentScene(ownerScene_); // ボスの親シーンを設定する
	boss_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);
	boss_->SetPosition(bossPos_); // ボスの初期位置を設定する
	boss_->SetTentacleCharge(true, 0.35f); // ボスの触手チャージを有効にしてチャージ量を設定する
	boss_->SyncTransform();

	// プレイヤーとボスの回転を、見つめ合いの角度で設定する
	const float py = LookAtYaw_(playerPos_, bossPos_);
	const float by = LookAtYaw_(bossPos_, playerPos_);
	// プレイヤーとボスの回転を、見つめ合いの角度で設定する
	playerRot_.y = py;
	bossRot_.y = by;
	// プレイヤーとボスの回転を、見つめ合いの角度で設定する
	player_->SetYaw(playerRot_.y);
	boss_->SetRotate(bossRot_);
}

float TitleShowdownController::LookAtYaw_(const Vector3& from, const Vector3& to) const {
	// from から to への方向ベクトルを計算して、そのYawを求める
	Vector3 d = { to.x - from.x, to.y - from.y, to.z - from.z };
	return std::atan2f(d.x, d.z);
}

void TitleShowdownController::UpdateBeamPush_(float dt) {
	beamMicroOscTime_ += dt; // ビームの微細な揺れの時間を更新する

	// ビーム演出が非アクティブな場合、ビームの位置を中心に戻す処理を行う
	if (!beamActive_) {
		const float toCenter = kTitleBeamCenterT_ - beamT_;
		beamT_ += toCenter * std::clamp(dt * kTitleBeamNeutralReturnSpeed_, 0.0f, 1.0f);
		beamT_ = std::clamp(beamT_, kTitleBeamMinT_, kTitleBeamMaxT_);
		return;
	}
	
	beamTargetTimer_ -= dt; // ビームの目標位置のタイマーを減算する

	// タイマーが0以下になったら、次の目標位置を再設定する
	if (beamTargetTimer_ <= 0.0f) {
		ResetBeamTarget_();
	}

	// ビームの現在位置を、目標位置に近づける処理を行う
	const float approachRate = std::clamp(dt * kTitleBeamApproachSpeed_, 0.0f, 1.0f);
	beamT_ += (beamTargetT_ - beamT_) * approachRate;

	// ビームの位置に微細な揺れを加える処理を行う
	const float microOsc = std::sin(beamMicroOscTime_ * kTitleBeamMicroOscSpeed_) * kTitleBeamMicroOscAmp_;
	const float baseT = beamT_;
	beamT_ = std::clamp(baseT + microOsc, kTitleBeamMinT_, kTitleBeamMaxT_);
}

void TitleShowdownController::ResetBeamTarget_() {
	// ビームの目標位置をランダムに再設定する処理を行う
	std::uniform_real_distribution<float> timeDist(
		kTitleBeamTargetChangeMinSec_,
		kTitleBeamTargetChangeMaxSec_
	);
	// ビームの目標位置をランダムに再設定する処理を行う
	std::uniform_real_distribution<float> sideDist(
		kTitleBeamMinT_,
		kTitleBeamMaxT_
	);

	// ビームの目標位置をランダムに再設定する処理を行う
	beamTargetTimer_ = timeDist(rng_);
	beamTargetT_ = sideDist(rng_);
}

void TitleShowdownController::UpdateBeamClash_(float dt) {
	if (!player_ || !boss_) { return; }

	// ビームの衝突位置にパーティクルを発生させる処理を行う
	auto* pm = TKM::ParticleManager::GetInstance();
	if (!pm) { return; }
	// プレイヤーとボスのビーム開始位置を計算する
	const Vector3 p0 = playerPos_ + Vector3{ 0.0f, kTitleBeamPlayerStartOffsetY_, 0.0f };
	const Vector3 b0 = bossPos_ + Vector3{ 0.0f, kTitleBeamBossStartOffsetY_, 0.0f };
	// ビームの衝突位置を、ビームの開始位置と目標位置に基づいて計算する
	const float t = std::clamp(beamT_, 0.0f, 1.0f);
	const Vector3 hit = {
		p0.x + (b0.x - p0.x) * t,
		p0.y + (b0.y - p0.y) * t,
		p0.z + (b0.z - p0.z) * t
	};

	// ビームの衝突位置に、プレイヤー側とボス側のビームパーティクルを、ビームのセグメント数に基づいて等間隔に発生させる
	for (int i = 0; i < kTitleBeamSegments_; ++i) {
		const float u = static_cast<float>(i) / static_cast<float>(kTitleBeamSegments_ - 1);
		const Vector3 p = {
			p0.x + (hit.x - p0.x) * u,
			p0.y + (hit.y - p0.y) * u,
			p0.z + (hit.z - p0.z) * u
		};
		pm->Emit("titleBeam_player", p, kTitleBeamPerSegment_);
	}
	// ビームの衝突位置に、プレイヤー側とボス側のビームパーティクルを、ビームのセグメント数に基づいて等間隔に発生させる
	for (int i = 0; i < kTitleBeamSegments_; ++i) {
		const float u = static_cast<float>(i) / static_cast<float>(kTitleBeamSegments_ - 1);
		const Vector3 p = {
			b0.x + (hit.x - b0.x) * u,
			b0.y + (hit.y - b0.y) * u,
			b0.z + (hit.z - b0.z) * u
		};
		pm->Emit("titleBeam_boss", p, kTitleBeamPerSegment_);
	}

	// ビームの衝突位置に、ビームクラッシュのコア、レイ、リングパーティクルを、一定のHzで発生させる
	clashEmitAcc_ += dt;
	const float emitStep = 1.0f / kTitleBeamClashHz_;

	// ビームの衝突位置に、ビームクラッシュのコア、レイ、リングパーティクルを、一定のHzで発生させる処理を行う
	while (clashEmitAcc_ >= emitStep) {
		clashEmitAcc_ -= emitStep;
		pm->Emit("titleBeamClash_core", hit, kTitleClashCoreCount_);
		pm->Emit("titleBeamClash_rays", hit, kTitleClashRaysCount_);
		pm->Emit("titleBeamClash_ring", hit, kTitleClashRingCount_);
	}
}