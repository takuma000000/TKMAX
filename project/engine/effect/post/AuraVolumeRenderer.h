#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;
}

namespace TKM {

	//=============================================================
	// AuraVolumeRendererクラス
	// オーラボリュームの描画を行うクラス。
	//=============================================================
	class AuraVolumeRenderer {
	public:
		/// <summary>
		/// オーラボリュームレンダラーを初期化します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void Initialize(TKM::DirectXCommon* dxCommon);
		/// <summary>
		/// オーラボリュームレンダラーを終了処理します。
		/// </summary>
		/// <param name="viewProj"></param>
		/// <param name="bossCenter"></param>
		/// <param name="collider"></param>
		/// <param name="active"></param>
		void Draw(
			const Matrix4x4& viewProj,
			const Vector3& bossCenter,
			const Vector3& collider,
			bool active);
		/// <summary>
		/// ImGui描画。
		/// </summary>
		/// <param name="label"></param>
		void DrawImGui(const char* label = "AuraVolume");

		/// <summary>
		/// スライス数のゲッター。
		/// </summary>
		/// <returns></returns>
		bool IsAlwaysOn() const { return alwaysOn_; }

		// Setter===================================
		/// <summary>
		/// スライス数の設定。
		/// </summary>
		/// <param name="v"></param>
		void SetSliceCount(uint32_t v) { sliceCount_ = v; }
		/// <summary>
		/// 半径倍率の設定。
		/// </summary>
		/// <param name="v"></param>
		void SetRadiusMul(float v) { radiusMul_ = v; }
		/// <summary>
		/// 高さ倍率の設定。
		/// </summary>
		/// <param name="v"></param>
		void SetHeightMul(float v) { heightMul_ = v; }
		/// <summary>
		/// 色の設定。
		/// </summary>
		/// <param name="v"></param>
		void SetIntensity(float v) { intensity_ = v; }
		/// <summary>
		/// 常にONにするかどうかの設定。
		/// </summary>
		/// <param name="v"></param>
		void SetAlwaysOn(bool v) { alwaysOn_ = v; }
		// =========================================

	private:
		TKM::DirectXCommon* dxCommon_ = nullptr;

		uint32_t sliceCount_ = 12;   // 8〜16が目安
		float radiusMul_ = 1.25f;    // collider.x に掛ける
		float heightMul_ = 1.60f;    // collider.y に掛ける

		Vector3 color_ = { 0.2f, 0.6f, 1.0f };
		float intensity_ = 1.35f;

		// 炎パラメータ
		float noiseScale_ = 10.0f;
		float noiseSpeed_ = 1.6f;
		float rimPower_ = 2.2f;
		float alphaBase_ = 1.0f;

		float time_ = 0.0f;

		// デバッグ用
		uint32_t drawCallsThisFrame_ = 0;

		bool alwaysOn_ = false; // デバッグ用：常にONにする
	};
}