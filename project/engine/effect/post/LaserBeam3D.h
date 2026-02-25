#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;

	//=============================================================
	// LaserBeam3Dクラス
	// 3D空間に存在するレーザービームの管理を行うクラス。
	//=============================================================
	class LaserBeam3D {
	public:
		struct Desc {
			Vector3 startWS_{ 0.0f, 0.0f, 0.0f };
			Vector3 endWS_{ 0.0f, 0.0f, 0.0f };

			// ビームの太さ（ワールド半径）
			float radius_ = 2.2f;

			// 見た目
			Vector3 color_{ 0.2f, 0.85f, 1.0f };
			float intensity_ = 3.0f;     // 発光強さ（加算）
			float coreSharpness_ = 7.0f; // 中心コアの締まり（大きいほど細く強い）
			float edgeSoftness_ = 1.2f;  // 外側の落ち方

			// 分割（ビーム方向に何枚置くか）
			uint32_t sliceCount_ = 64;

			// ゆらぎ（ちらつき/波）
			float noiseScale_ = 1.0f; // 1D的なノイズのスケール
			float noiseSpeed_ = 1.0f; // 時間変化

			// 状態
			bool active_ = false;
			bool telegraph_ = false; // 予告（点滅弱めなど）
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
		void Draw(const Matrix4x4& viewProj,
			const Vector3& camRightWS,
			const Vector3& camUpWS,
			const Vector3& camFwdWS);
		/// <summary>
		/// デバッグ用ImGui表示
		/// </summary>
		void ImGuiDebug();

		/// <summary>
		/// アクティブか？
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return desc_.active_; }

		// Setter=====================================
		/// <summary>
		/// アクティブ状態を設定します。
		/// </summary>
		/// <param name="a">有効にする場合 true、無効にする場合 false</param>
		void SetActive(bool a) { desc_.active_ = a; }
		/// <summary>
		/// 設定情報を設定します。
		/// </summary>
		/// <param name="desc">設定する FogVolume3D のパラメータ</param>
		void SetDesc(const Desc& desc) { desc_ = desc; }
		// ===========================================
		// Getter=====================================
		/// <summary>
		/// 説明取得（const）
		/// </summary>
		/// <returns></returns>
		const Desc& GetDesc() const { return desc_; }
		// ===========================================
	private:
		DirectXCommon* dxCommon_ = nullptr;
		Desc desc_{};

		float time_ = 0.0f;
	};
}