#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;

	//=============================================================
	// FogVolume3Dクラス
	// 3D空間に存在する霧の管理を行うクラス。
	//=============================================================
	class FogVolume3D {
	public:
		struct Desc {
			Vector3 centerWS_{ 0.0f, 0.0f, 0.0f }; // 中心座標（ワールド）
			Vector3 halfSizeWS_{ 900.0f, 220.0f, 900.0f }; // 範囲サイズ（半径）

			// --- 見た目（煙っぽく） ---
			Vector3 color_{ 0.92f, 0.92f, 0.92f };
			float density_ = 0.06f; // 全体の濃さ（煙は少し濃い目が映える）

			uint32_t sliceCount_ = 80; // スライス数

			// --- もくもく感（大きい塊 + ゆっくり） ---
			float noiseScale_ = 0.18f;  // PS側で低周波メインに組むので、ここは少し大きめでOK
			float noiseSpeed_ = 0.00f;  // 煙はゆっくり

			// --- 高さ方向（下が濃い / 上が薄い）---
			// 0..1（0=体積の下端, 1=上端）
			float fogStart_ = 0.10f; // この高さまでは濃い
			float fogEnd_ = 0.90f;   // この高さでほぼ消える

			// --- ノイズ強さ / 基準座標 ---
			float noiseStrength_ = 0.85f; // もくもくのムラ（強め）
			float worldScale_ = 1.0f;     // ノイズ座標のスケール（基本1）
			Vector3 worldPos_{ 0.0f, 0.0f, 0.0f }; // ノイズの基準（基本=centerWSに同期）

			float softness_ = 1.8f; // 端の落ち方（大きいほど中心寄りに残る）
		};

		/// <summary>
		/// FogVolume3Dを初期化します。
		/// </summary>
		/// <param name="dx"></param>
		void Initialize(DirectXCommon* dx);
		/// <summary>
		/// FogVolume3Dを更新します。
		/// </summary>
		/// <param name="dt"></param>
		void Update(float dt);
		/// <summary>
		/// FogVolume3Dを描画します。
		/// </summary>
		/// <param name="viewProj"></param>
		/// <param name="camRightWS"></param>
		/// <param name="camUpWS"></param>
		/// <param name="camFwdWS"></param>
		void Draw(const Matrix4x4& viewProj, const Vector3& camRightWS, const Vector3& camUpWS, const Vector3& camFwdWS);
		/// <summary>
		/// ImGuiデバッグ表示。
		/// </summary>
		void ImGuiDebug();

		/// <summary>
		/// FogVolume3Dがアクティブかどうかを取得します。
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return active_; }

		// Setter===================================
		/// <summary>
		/// FogVolume3D のアクティブ状態を設定します。
		/// </summary>
		/// <param name="a">有効にする場合 true、無効にする場合 false</param>
		void SetActive(bool a) { active_ = a; }
		/// <summary>
		/// FogVolume3D の設定をセットします。
		/// </summary>
		/// <param name="desc">設定する FogVolume3D のパラメータ</param>
		void SetDesc(const Desc& desc) { desc_ = desc; }
		// =========================================
		// Getter===================================
		/// <summary>
		/// FogVolume3Dの設定取得（const版）。
		/// </summary>
		/// <returns></returns>
		const Desc& GetDesc() const { return desc_; }
		// =========================================

	private:
		DirectXCommon* dxCommon_ = nullptr;
		Desc desc_{};

		bool active_ = true;
		float time_ = 0.0f;
	};
}