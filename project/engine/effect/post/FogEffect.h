#pragma once
#include "BaseEffect.h"
#include "MyMath.h"

namespace TKM {

	//=============================================================
	// FogEffectクラス
	// 霧エフェクトの管理を行うクラス
	//=============================================================
	class FogEffect : public TKM::BaseEffect {
	public:
		//=============================================================
		// 初期化・更新・描画
		//=============================================================

		/// <summary>
		/// 霧エフェクトを初期化します。
		/// </summary>
		/// <param name="dx">DirectX共通管理</param>
		void Initialize(TKM::DirectXCommon* dx) override {
			TKM::BaseEffect::Initialize(dx);
		}

		/// <summary>
		/// 霧エフェクトを更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		void Update(float dt) override;

		/// <summary>
		/// 霧エフェクトを描画します。
		/// </summary>
		void Draw() override {}  // 描画は DirectXCommon 側で行う

		/// <summary>
		/// ImGuiデバッグ表示を行います。
		/// </summary>
		void ImGuiDebug();

		//=============================================================
		// Getter
		//=============================================================

		/// <summary>
		/// 霧が有効かを取得します。
		/// </summary>
		/// <returns>有効ならtrue</returns>
		bool IsActive() const { return active_; }

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// 霧の基準ワールド座標を設定します。
		/// </summary>
		/// <param name="pos">基準座標</param>
		void SetWorldPos(const Vector3& pos) { worldPos_ = pos; }

		/// <summary>
		/// ワールドスケールを設定します。
		/// </summary>
		/// <param name="s">ワールドスケール</param>
		void SetWorldScale(float s) { worldScale_ = s; }

		/// <summary>
		/// 霧の有効状態を設定します。
		/// </summary>
		/// <param name="a">有効状態</param>
		void SetActive(bool a) { active_ = a; }

	private:
		//=============================================================
		// 基本パラメータ
		//=============================================================

		bool   active_ = true;          // 有効フラグ
		float  density_ = 0.495f;       // 濃さ
		float  start_ = 0.17f;          // 開始距離
		float  end_ = 0.376f;           // 終了距離
		float  noiseScale_ = 10.0f;     // ノイズスケール
		float  noiseStrength_ = 0.817f; // ノイズ強度

		//=============================================================
		// 時間制御
		//=============================================================

		float time_ = 0.0f;             // 経過時間
		float timeScale_ = 1.0f;        // 時間スケール
		bool  freezeTime_ = false;      // 時間停止フラグ

		//=============================================================
		// 移動
		//=============================================================

		Vector2 driftSpeedXZ_ = { 0.0f, 0.0f };  // 自動移動速度
		Vector2 driftOffsetXZ_ = { 0.0f, 0.0f }; // 蓄積オフセット

		//=============================================================
		// 色・基準座標
		//=============================================================

		Vector3 color_ = { 0.9f, 0.9f, 1.0f };      // 色
		Vector3 worldPos_ = { 0.0f, 0.0f, 0.0f };   // 基準ワールド座標
		float   worldScale_ = 0.02f;                // ワールドスケール
	};
}