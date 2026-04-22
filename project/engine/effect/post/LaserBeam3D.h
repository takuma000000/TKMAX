#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;

	//=============================================================
	// LaserBeam3Dクラス
	// 3D空間に存在するレーザービームの管理を行うクラス
	//=============================================================
	class LaserBeam3D {
	public:
		//=============================================================
		// 設定構造体
		//=============================================================

		struct Desc {
			Vector3 startWS_{ 0.0f, 0.0f, 0.0f }; // 開始座標
			Vector3 endWS_{ 0.0f, 0.0f, 0.0f };   // 終了座標

			//=========================================================
			// ビーム形状
			//=========================================================

			float radius_ = 2.2f; // 半径

			//=========================================================
			// 見た目
			//=========================================================

			Vector3 color_{ 0.2f, 0.85f, 1.0f }; // 色
			float intensity_ = 3.0f;             // 発光強さ
			float coreSharpness_ = 7.0f;         // コアの締まり
			float edgeSoftness_ = 1.2f;          // 外側の落ち方

			//=========================================================
			// 分割
			//=========================================================

			uint32_t sliceCount_ = 64; // スライス数

			//=========================================================
			// ゆらぎ
			//=========================================================

			float noiseScale_ = 1.0f; // ノイズスケール
			float noiseSpeed_ = 1.0f; // ノイズ速度

			//=========================================================
			// 状態
			//=========================================================

			bool active_ = false;    // 有効フラグ
			bool telegraph_ = false; // 予告状態
		};

		//=============================================================
		// 初期化・更新・描画
		//=============================================================

		/// <summary>
		/// LaserBeam3Dを初期化します。
		/// </summary>
		/// <param name="dx">DirectX共通管理</param>
		void Initialize(DirectXCommon* dx);

		/// <summary>
		/// LaserBeam3Dを更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		void Update(float dt);

		/// <summary>
		/// LaserBeam3Dを描画します。
		/// </summary>
		/// <param name="viewProj">ビュー射影行列</param>
		/// <param name="camRightWS">カメラ右方向ベクトル</param>
		/// <param name="camUpWS">カメラ上方向ベクトル</param>
		/// <param name="camFwdWS">カメラ前方向ベクトル</param>
		void Draw(const Matrix4x4& viewProj,
			const Vector3& camRightWS,
			const Vector3& camUpWS,
			const Vector3& camFwdWS);

		/// <summary>
		/// ImGuiデバッグ表示を行います。
		/// </summary>
		void ImGuiDebug();

		//=============================================================
		// Getter
		//=============================================================

		/// <summary>
		/// アクティブかを取得します。
		/// </summary>
		/// <returns>有効ならtrue</returns>
		bool IsActive() const { return desc_.active_; }

		/// <summary>
		/// 設定情報を取得します。
		/// </summary>
		/// <returns>現在の設定情報</returns>
		const Desc& GetDesc() const { return desc_; }

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// アクティブ状態を設定します。
		/// </summary>
		/// <param name="a">有効状態</param>
		void SetActive(bool a) { desc_.active_ = a; }

		/// <summary>
		/// 設定情報を設定します。
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

		Desc desc_{};       // 設定情報
		float time_ = 0.0f; // 経過時間
	};
}