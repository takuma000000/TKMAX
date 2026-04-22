#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;

	//=============================================================
	// FogVolume3Dクラス
	// 3D空間に存在する霧の管理を行うクラス
	//=============================================================
	class FogVolume3D {
	public:
		//=============================================================
		// 設定構造体
		//=============================================================

		struct Desc {
			Vector3 centerWS_{ 0.0f, 0.0f, 0.0f };             // 中心座標（ワールド）
			Vector3 halfSizeWS_{ 900.0f, 220.0f, 900.0f };     // 半サイズ（ワールド）

			//=========================================================
			// 見た目
			//=========================================================

			Vector3 color_{ 0.92f, 0.92f, 0.92f };             // 色
			float density_ = 0.06f;                            // 濃さ
			uint32_t sliceCount_ = 80;                         // スライス数

			//=========================================================
			// ノイズ
			//=========================================================

			float noiseScale_ = 0.18f;                         // ノイズスケール
			float noiseSpeed_ = 0.00f;                         // ノイズ速度
			float noiseStrength_ = 0.85f;                      // ノイズ強度
			float worldScale_ = 1.0f;                          // ワールドスケール
			Vector3 worldPos_{ 0.0f, 0.0f, 0.0f };            // ノイズ基準座標

			//=========================================================
			// 高さ方向
			//=========================================================

			float fogStart_ = 0.10f;                           // 濃く出る開始位置
			float fogEnd_ = 0.90f;                             // 薄くなる終了位置

			//=========================================================
			// 端処理
			//=========================================================

			float softness_ = 1.8f;                            // 端の落ち方
		};

		//=============================================================
		// 初期化・更新・描画
		//=============================================================

		/// <summary>
		/// FogVolume3Dを初期化します。
		/// </summary>
		/// <param name="dx">DirectX共通管理</param>
		void Initialize(DirectXCommon* dx);

		/// <summary>
		/// FogVolume3Dを更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		void Update(float dt);

		/// <summary>
		/// FogVolume3Dを描画します。
		/// </summary>
		/// <param name="viewProj">ビュー射影行列</param>
		/// <param name="camRightWS">カメラ右方向ベクトル</param>
		/// <param name="camUpWS">カメラ上方向ベクトル</param>
		/// <param name="camFwdWS">カメラ前方向ベクトル</param>
		void Draw(const Matrix4x4& viewProj, const Vector3& camRightWS, const Vector3& camUpWS, const Vector3& camFwdWS);

		/// <summary>
		/// ImGuiデバッグ表示を行います。
		/// </summary>
		void ImGuiDebug();

		//=============================================================
		// Getter
		//=============================================================

		/// <summary>
		/// FogVolume3Dが有効かを取得します。
		/// </summary>
		/// <returns>有効ならtrue</returns>
		bool IsActive() const { return active_; }

		/// <summary>
		/// FogVolume3Dの設定を取得します。
		/// </summary>
		/// <returns>現在の設定</returns>
		const Desc& GetDesc() const { return desc_; }

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// FogVolume3Dの有効状態を設定します。
		/// </summary>
		/// <param name="a">有効状態</param>
		void SetActive(bool a) { active_ = a; }

		/// <summary>
		/// FogVolume3Dの設定を設定します。
		/// </summary>
		/// <param name="desc">設定内容</param>
		void SetDesc(const Desc& desc) { desc_ = desc; }

	private:
		//=============================================================
		// 共通参照
		//=============================================================

		DirectXCommon* dxCommon_ = nullptr; // DirectX共通管理

		//=============================================================
		// 状態
		//=============================================================

		Desc desc_{};         // 設定
		bool active_ = true;  // 有効フラグ
		float time_ = 0.0f;   // 経過時間
	};
}