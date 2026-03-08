#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;

	//=============================================================
	// SmokeVolume3Dクラス
	// 3D空間に存在する煙の管理を行うクラス。
	//=============================================================
	class SmokeVolume3D {
	public:
		struct Desc {
			Vector3 centerWS_{ 0.0f, 0.0f, 0.0f };
			Vector3 halfSizeWS_{ 600.0f, 220.0f, 600.0f };

			// 見た目
			Vector3 color_{ 0.92f, 0.92f, 0.92f };
			float density_ = 0.12f;

			uint32_t sliceCount_ = 57;

			// ノイズ（モクモク）
			float baseScale_ = 0.124f;        // 大きい塊
			float detailScale_ = 0.741f;      // 細かいディテール
			float detailStrength_ = 0.33f;   // 0..1

			// 雲化（threshold/softness）
			float threshold_ = 0.52f;  // 0..1（高いほど薄くなる）
			float softness_ = 0.115f;   // 0..1（大きいほど境界が柔らかい）

			// 流れ（画面手前方向：-CamFwd へ流す）
			float flowSpeed_ = 4.140f;  // 速さ
			float riseSpeed_ = 0.15f;  // 上昇（煙っぽさ）

			float alphaMax_ = 0.66f;   // 上限

			// ノイズ座標の基準
			float worldScale_ = 1.0f;
			Vector3 worldPos_{ 0.0f, 0.0f, 0.0f }; // 基本 centerWS に同期
		};

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="dx"></param>
		void Initialize(DirectXCommon* dx);
		/// <summary>
		/// 更新
		/// </summary>
		/// <param name="dt"></param>
		void Update(float dt);
		/// <summary>
		/// 描画
		/// </summary>
		/// <param name="viewProj"></param>
		/// <param name="camRightWS"></param>
		/// <param name="camUpWS"></param>
		/// <param name="camFwdWS"></param>
		void Draw(const Matrix4x4& viewProj, const Vector3& camRightWS, const Vector3& camUpWS, const Vector3& camFwdWS);
		/// <summary>
		/// デバッグ用ImGui表示
		/// </summary>
		void ImGuiDebug();

		/// <summary>
		/// 有効かどうか
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return active_; }

		// Setter=======================================
		/// <summary>
		/// 有効/無効設定
		/// </summary>
		/// <param name="a"></param>
		void SetActive(bool a) { active_ = a; }
		/// <summary>
		/// 設定情報設定
		/// </summary>
		/// <param name="desc">設定するパラメータ</param>
		void SetDesc(const Desc& desc) { desc_ = desc; }
		// =============================================
		// Getter=======================================
		/// <summary>
		/// 設定取得（const版）
		/// </summary>
		/// <returns></returns>
		const Desc& GetDesc() const { return desc_; }
		// =============================================
	private:

		//==============================================
		// 共通参照
		//==============================================
		DirectXCommon* dxCommon_ = nullptr;
		//==============================================
		// 設定
		//==============================================
		Desc desc_{}; // エフェクトの見た目や動きのパラメータをまとめた構造体
		//==============================================
		// 状態
		//==============================================
		bool active_ = true; // エフェクトが有効かどうか
		//==============================================
		// タイマー
		//==============================================
		float time_ = 0.0f; // 経過時間。ノイズのアニメーションや流れの計算に使用します。
	};
}