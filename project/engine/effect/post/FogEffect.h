#pragma once
#include "BaseEffect.h"
#include "MyMath.h"

namespace TKM {

	//=============================================================
	// FogEffectクラス
	// 霧エフェクトの管理を行うクラス。
	//=============================================================
	class FogEffect : public TKM::BaseEffect {
	public:
		/// <summary>
		/// ポストエフェクトの初期化
		/// </summary>
		/// <param name="dx"></param>
		void Initialize(TKM::DirectXCommon* dx) override {
			TKM::BaseEffect::Initialize(dx);
		}
		/// <summary>
		/// ポストエフェクトの更新
		/// </summary>
		/// <param name="dt"></param>
		void Update(float dt) override;
		/// <summary>
		/// ポストエフェクトの描画
		/// </summary>
		void Draw() override {}  // 描画は DirectXCommon 側のチェーンでやる
		/// <summary>
		/// ImGuiデバッグ表示
		/// </summary>
		void ImGuiDebug();

		/// <summary>
		/// 霧の有効・無効
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return active_; }

		// Setter========================================
		/// <summary>
		/// 霧の基準となるワールド座標の設定
		/// </summary>
		/// <param name="pos"></param>
		void SetWorldPos(const Vector3& pos) { worldPos_ = pos; }
		/// <summary>
		/// 霧パターンの「世界空間スケール」の設定
		/// </summary>
		/// <param name="s"></param>
		void SetWorldScale(float s) { worldScale_ = s; }
		/// <summary>
		/// 霧の有効・無効設定
		/// </summary>
		/// <param name="a"></param>
		void SetActive(bool a) { active_ = a; }
		// ==============================================

	private:
		bool   active_ = true;             // 霧は最初から有効でOK
		float  density_ = 0.495f;            // 画面全体の濃さ
		float  start_ = 0.17f;              // 全画面に霧をかけたいので 0〜1 のまま
		float  end_ = 0.376f;
		float  noiseScale_ = 10.0f;         // 塊の大きさ）
		float  noiseStrength_ = 0.817f;      // ムラの強さ
		float  time_ = 0.0f; // 時間経過用
		float timeScale_ = 1.0f;      // 霧アニメ速度（Time倍率）
		Vector2 driftSpeedXZ_ = { 0.0f, 0.0f };   // 霧の自動移動速度（ワールド単位/秒）
		Vector2 driftOffsetXZ_ = { 0.0f, 0.0f };  // 蓄積オフセット
		bool   freezeTime_ = false;    // 時間停止（形だけ止めたい時）
		Vector3 color_ = { 0.9f, 0.9f, 1.0f }; // 霧の色

		Vector3 worldPos_ = { 0.0f, 0.0f, 0.0f }; // 霧の基準となるワールド座標
		float   worldScale_ = 0.02f;                // どれくらい動きに反応するか
	};
}