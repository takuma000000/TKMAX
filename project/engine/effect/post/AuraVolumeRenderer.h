#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;
}

namespace TKM {

	//=============================================================
	// AuraVolumeRendererクラス
	// オーラボリュームの描画を行うクラス
	//=============================================================
	class AuraVolumeRenderer {
	public:
		//=============================================================
		// 初期化・描画
		//=============================================================

		/// <summary>
		/// オーラボリュームレンダラーを初期化します。
		/// </summary>
		/// <param name="dxCommon">DirectX共通管理</param>
		void Initialize(TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// オーラボリュームを描画します。
		/// </summary>
		/// <param name="viewProj">ビュー射影行列</param>
		/// <param name="bossCenter">描画中心座標</param>
		/// <param name="collider">コライダーサイズ</param>
		/// <param name="active">有効状態</param>
		void Draw(
			const Matrix4x4& viewProj,
			const Vector3& bossCenter,
			const Vector3& collider,
			bool active);

		/// <summary>
		/// ImGui描画を行います。
		/// </summary>
		/// <param name="label">表示ラベル</param>
		void DrawImGui(const char* label = "AuraVolume");

		//=============================================================
		// Getter
		//=============================================================

		/// <summary>
		/// 常時ONかどうかを取得します。
		/// </summary>
		/// <returns>常時ONならtrue</returns>
		bool IsAlwaysOn() const { return alwaysOn_; }

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// スライス数を設定します。
		/// </summary>
		/// <param name="v">スライス数</param>
		void SetSliceCount(uint32_t v) { sliceCount_ = v; }

		/// <summary>
		/// 半径倍率を設定します。
		/// </summary>
		/// <param name="v">半径倍率</param>
		void SetRadiusMul(float v) { radiusMul_ = v; }

		/// <summary>
		/// 高さ倍率を設定します。
		/// </summary>
		/// <param name="v">高さ倍率</param>
		void SetHeightMul(float v) { heightMul_ = v; }

		/// <summary>
		/// 強度を設定します。
		/// </summary>
		/// <param name="v">強度</param>
		void SetIntensity(float v) { intensity_ = v; }

		/// <summary>
		/// 常にONにするかを設定します。
		/// </summary>
		/// <param name="v">常時ONフラグ</param>
		void SetAlwaysOn(bool v) { alwaysOn_ = v; }

	private:
		//=============================================================
		// 共通参照
		//=============================================================

		TKM::DirectXCommon* dxCommon_ = nullptr; // DirectX共通管理

		//=============================================================
		// 描画パラメータ
		//=============================================================

		uint32_t sliceCount_ = 12; // スライス数
		float radiusMul_ = 1.25f;  // 半径倍率
		float heightMul_ = 1.60f;  // 高さ倍率

		Vector3 color_ = { 0.2f, 0.6f, 1.0f }; // 色
		float intensity_ = 1.35f;              // 強度

		//=============================================================
		// 炎パラメータ
		//=============================================================

		float noiseScale_ = 10.0f; // ノイズスケール
		float noiseSpeed_ = 1.6f;  // ノイズ速度
		float rimPower_ = 2.2f;    // リム強度
		float alphaBase_ = 1.0f;   // 基本アルファ

		//=============================================================
		// 状態
		//=============================================================

		float time_ = 0.0f;                 // 経過時間
		uint32_t drawCallsThisFrame_ = 0;   // このフレームの描画回数
		bool alwaysOn_ = false;             // 常時ONフラグ
	};
}