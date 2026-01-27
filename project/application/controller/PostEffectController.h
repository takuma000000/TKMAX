#pragma once
#include <memory>

#include "DirectXCommon.h"
#include "Player.h"
#include "manager/BossManager.h"

#include "RadialBlurEffect.h"
#include "VignettingEffect.h"
#include "FogEffect.h"
#include "AuraEffect.h"
#include "WaterRippleEffect.h"
#include "FogVolume3D.h"
#include "SmokeVolume3D.h"

#include "camera/Camera.h"
#include "MyMath.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {
	class PostEffectController {
	public:
		/// <summary>
		/// 初期化。
		/// </summary>
		/// <param name="dxCommon"></param>
		/// <param name="player"></param>
		/// <param name="bossManager"></param>
		void Initialize(DirectXCommon* dxCommon, Player* player, BossManager* bossManager);
		/// <summary>
		/// 終了処理。
		/// </summary>
		void Finalize();
		/// <summary>
		/// 更新。
		/// </summary>
		/// <param name="dt"></param>
		/// <param name="bossManager"></param>
		void Update(float dt, BossManager* bossManager);
		/// <summary>
		/// カメラ更新時の処理。
		/// </summary>
		/// <param name="activeCamera"></param>
		void OnCameraUpdated(TKM::Camera* activeCamera);
		/// <summary>
		/// ボリューム系エフェクトの描画。
		/// </summary>
		/// <param name="activeCamera"></param>
		void DrawVolumes(TKM::Camera* activeCamera);
		/// <summary>
		/// ImGuiデバッグ表示。
		/// </summary>
		void ImGuiDebug();

	private:
		DirectXCommon* dxCommon_ = nullptr;

		std::unique_ptr<TKM::RadialBlurEffect> radialBlur_ = nullptr;
		std::unique_ptr<TKM::VignettingEffect> vignetting_ = nullptr;
		std::unique_ptr<TKM::FogEffect> fog_ = nullptr;
		std::unique_ptr<TKM::AuraEffect> aura_ = nullptr;
		std::unique_ptr<TKM::WaterRippleEffect> waterRipple_ = nullptr;

		std::unique_ptr<TKM::FogVolume3D> fogVolume3D_ = nullptr;
		std::unique_ptr<TKM::SmokeVolume3D> smokeVolume3D_ = nullptr;
	};
}