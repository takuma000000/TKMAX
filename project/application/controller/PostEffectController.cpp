#include "PostEffectController.h"
#include "MyMath.h"

namespace TKM {
	void PostEffectController::Initialize(DirectXCommon* dxCommon, Player* player, BossManager* bossManager) {
		dxCommon_ = dxCommon;

		// ──────────────── 画面エフェクトの初期化 ───────────────

		// RadialBlurEffect
		radialBlur_ = std::make_unique<TKM::RadialBlurEffect>();
		radialBlur_->Initialize(dxCommon_);
		dxCommon_->SetRadialBlurEffect(radialBlur_.get());
		if (player) {
			player->SetRadialBlurEffect(radialBlur_.get());
		}

		// VignettingEffect
		vignetting_ = std::make_unique<TKM::VignettingEffect>();
		vignetting_->Initialize(dxCommon_);

		// FogEffect（常時ON）
		fog_ = std::make_unique<TKM::FogEffect>();
		fog_->Initialize(dxCommon_);
		fog_->SetActive(false);
		dxCommon_->SetFogEffect(fog_.get());

		// AuraEffect
		aura_ = std::make_unique<TKM::AuraEffect>();
		aura_->Initialize(dxCommon_);
		dxCommon_->SetAuraEffect(aura_.get());

		// WaterRippleEffect
		waterRipple_ = std::make_unique<TKM::WaterRippleEffect>();
		waterRipple_->Initialize(dxCommon_);
		dxCommon_->SetWaterRippleEffect(waterRipple_.get());
		if (bossManager) {
			bossManager->SetWaterRippleEffect(waterRipple_.get());
		}

		// FogVolume3D
		fogVolume3D_ = std::make_unique<TKM::FogVolume3D>();
		fogVolume3D_->Initialize(dxCommon_);
		{
			auto& d = fogVolume3D_->GetDesc();
			d.centerWS_ = { 0.0f, 6.0f, 20.0f };
			d.halfSizeWS_ = { 900.0f, 220.0f, 900.0f };
			d.sliceCount_ = 80;
			d.density_ = 0.19f;
		}

		// SmokeVolume3D
		smokeVolume3D_ = std::make_unique<TKM::SmokeVolume3D>();
		smokeVolume3D_->Initialize(dxCommon_);
	}

	void PostEffectController::Finalize() {
		if (dxCommon_) {
			dxCommon_->SetRadialBlurEffect(nullptr);
			dxCommon_->SetVignettingEffect(nullptr);
			dxCommon_->SetFogEffect(nullptr);
			dxCommon_->SetAuraEffect(nullptr);
			// WaterRipple は登録してるなら解除しておく（関数がある前提）
			dxCommon_->SetWaterRippleEffect(nullptr);
		}
	}

	void PostEffectController::Update(float dt, BossManager* bossManager) {
		// 画面エフェクトの更新=================================
		if (radialBlur_) {
			radialBlur_->Update(dt);
		}

		if (vignetting_) {
			bool bossWave =
				bossManager &&
				bossManager->IsBattleActive() &&
				!bossManager->IsBossDead();

			vignetting_->SetBossWave(bossWave);
			vignetting_->Update(dt);
		}

		if (fog_) {
			fog_->Update(dt);
		}

		if (waterRipple_) {
			waterRipple_->Update(dt);
		}

		if (fogVolume3D_) {
			fogVolume3D_->Update(dt);
		}

		if (smokeVolume3D_) {
			smokeVolume3D_->Update(dt);
		}
		// ==================================================
	}

	void PostEffectController::OnCameraUpdated(TKM::Camera* activeCamera) {
		if (!fog_ || !activeCamera) {
			return;
		}

		const Matrix4x4& camW = activeCamera->GetWorldMatrix();
		Vector3 camPos{
			camW.m[3][0],
			camW.m[3][1],
			camW.m[3][2]
		};
		fog_->SetWorldPos(camPos);
	}

	void PostEffectController::DrawVolumes(TKM::Camera* activeCamera) {
		if (!activeCamera) {
			return;
		}

		const Matrix4x4& camW = activeCamera->GetWorldMatrix();
		Vector3 right{ camW.m[0][0], camW.m[0][1], camW.m[0][2] };
		Vector3 up{ camW.m[1][0], camW.m[1][1], camW.m[1][2] };
		Vector3 fwd{ camW.m[2][0], camW.m[2][1], camW.m[2][2] };
		Matrix4x4 vp = activeCamera->GetViewProjectionMatrix();

		// FogVolume（空間霧）
		if (fogVolume3D_) {
			fogVolume3D_->Draw(vp, right, up, fwd);
		}

		// SmokeVolume（空間スモーク）
		if (smokeVolume3D_) {
			smokeVolume3D_->Draw(vp, right, up, fwd);
		}
	}

	void PostEffectController::ImGuiDebug() {
#ifdef USE_IMGUI
		static bool showSmoke = true;
		if (ImGui::Begin("ポストエフェクト")) {
			ImGui::Checkbox("煙ボリュームを表示", &showSmoke);
			if (showSmoke) {
				if (smokeVolume3D_) {
					// SmokeVolume3D 側の ImGui はすでに日本語で実装済み
					smokeVolume3D_->ImGuiDebug();
				}
			}
		}
		ImGui::End();
#endif
	}
}