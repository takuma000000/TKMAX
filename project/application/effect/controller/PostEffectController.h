#pragma once
#include <memory>

#include "DirectXCommon.h"
#include "Player.h"
#include "manager/BossManager.h"

#include "RadialBlurEffect.h"
#include "VignettingEffect.h"
#include "FogEffect.h"
#include "AuraEffect.h"
#include "MotionBlurEffect.h"
#include "WaterRippleEffect.h"
#include "FogVolume3D.h"
#include "SmokeVolume3D.h"
#include "Camera.h"
#include "MyMath.h"

namespace TKM {

	//=============================================
	// PostEffectControllerクラス
	// ポストエフェクト管理クラス。
	// 画面全体にかかるエフェクト（放射ブラー、ビネット、画面霧など）や、
	// 3D空間に配置するボリュームエフェクト（立体霧、立体煙など）を管理します。
	//=============================================
	class PostEffectController {
	public:
		/// <summary>
		/// ボリューム系エフェクト管理クラスを初期化します。
		/// </summary>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="player">参照対象となるプレイヤー</param>
		/// <param name="bossManager">ボス管理クラス</param>
		void Initialize(DirectXCommon* dxCommon, Player* player, BossManager* bossManager);
		/// <summary>
		/// 終了処理を行います。
		/// 管理しているリソースや状態を解放します。
		/// </summary>
		void Finalize();
		/// <summary>
		/// 毎フレームの更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="bossManager">状態参照対象となるボス管理クラス</param>
		void Update(float dt, BossManager* bossManager);
		/// <summary>
		/// ボリューム系エフェクトの描画処理を行います。
		/// </summary>
		/// <param name="activeCamera">描画に使用するカメラ</param>
		void DrawVolumes(TKM::Camera* activeCamera);
		/// <summary>
		/// ImGui によるデバッグ情報を表示します。
		/// </summary>
		void ImGuiDebug();

		/// <summary>
		/// カメラ更新時に呼び出される処理を行います。
		/// </summary>
		/// <param name="activeCamera">現在有効なカメラ</param>
		void OnCameraUpdated(TKM::Camera* activeCamera);

		/// <summary>
		/// 放射ブラーの手動制御を解除します。
		/// </summary>
		void ClearRadialBlurManual();

		// Setters========================================
		/// <summary>
		/// 手動で放射ブラーをON/OFFします。
		/// </summary>
		void SetRadialBlurManual(bool enable, float strength = 1.0f);
		// ===============================================
		// Getters========================================
		/// <summary>
		/// 立体煙エフェクトの参照を取得します。
		/// </summary>
		/// <returns>立体煙エフェクトのポインタ</returns>
		SmokeVolume3D* GetSmokeVolume() { return smokeVolume3D_.get(); }
		// ===============================================

	private:
		//==============================
		// 参照
		//==============================
		DirectXCommon* dxCommon_ = nullptr;
		Player* player_ = nullptr;
		//==============================
		// Post Effect（2D / Screen Space）
		//==============================
		std::unique_ptr<TKM::RadialBlurEffect> radialBlur_ = nullptr; // 放射ブラー
		std::unique_ptr<TKM::VignettingEffect> vignetting_ = nullptr; // ビネット
		std::unique_ptr<TKM::FogEffect> fog_ = nullptr; // 画面霧
		std::unique_ptr<TKM::AuraEffect> aura_ = nullptr; // オーラ
		std::unique_ptr<TKM::WaterRippleEffect> waterRipple_ = nullptr; // 水面波紋
		std::unique_ptr<TKM::MotionBlurEffect> motionBlur_ = nullptr; // モーションブラー
		//==============================
		// Volume Effect（3D / World Space）
		//==============================
		std::unique_ptr<TKM::FogVolume3D> fogVolume3D_ = nullptr; // 立体霧
		std::unique_ptr<TKM::SmokeVolume3D> smokeVolume3D_ = nullptr; // 立体煙
	};
}