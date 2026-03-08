#pragma once
#include "BaseEffect.h"
#include "MyMath.h"

namespace TKM {

	//=============================================================
	// WaterRippleEffectクラス
	// 水面の波紋エフェクトの管理を行うクラス。
	//=============================================================
	class WaterRippleEffect : public TKM::BaseEffect {
	public:
		struct RippleDesc {
			float  duration_ = 0.6f;        // 継続秒
			float  radiusMax_ = 0.857f;     // 最大半径(UV)
			float  amplitude_ = 0.1f;       // ゆがみ量
			float  frequency_ = 80.0f;      // 細かさ
			float  width_ = 10.0f;          // 帯の幅（大きいほどシャープ）
			Vector3 color_ = { 1.0f,1.0f,1.0f };
			float  colorIntensity_ = 0.0f;  // 色の強さ
		};

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="dx"></param>
		void Initialize(TKM::DirectXCommon* dx) override;
		/// <summary>
		/// 毎フレーム更新
		/// </summary>
		/// <param name="dt"></param>
		void Update(float dt) override;
		/// <summary>
		/// 描画処理（何もしない）
		/// </summary>
		void Draw() override {}
		/// <summary>
		/// デバッグ用ImGui表示
		/// </summary>
		void ImGuiDebug();

		/// <summary>
		/// 波紋がアクティブか？
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return active_; }
		/// <summary>
		/// 波紋開始（詳細指定版）
		/// </summary>
		/// <param name="centerUV"></param>
		/// <param name="desc"></param>
		void Trigger(const Vector2& centerUV, const RippleDesc& desc);

	private:
		//==============================================
		// 状態
		//==============================================
		bool active_ = false; // エフェクト有効フラグ
		//==============================================
		// タイマー
		//==============================================
		float time_ = 0.0f; // 経過時間
		//==============================================
		// パラメータ
		//==============================================
		Vector2 centerUV_ = { 0.5f, 0.5f }; // 波紋中心 (UV)
		RippleDesc currentDesc_{}; // 現在の波紋設定
	};
}