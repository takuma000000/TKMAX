#pragma once
#include <memory>
#include <string>
#include <algorithm>
#include <cstdlib>
#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "WindowsAPI.h"
#include <vector>

namespace TKM {

	//=============================================================
	// RBGaugeUIクラス
	// プレイヤーの弾数を表示するUIの管理クラス。
	//=============================================================
	class RBGaugeUI {
	public:
		struct Desc {
			Vector2 center_ = { WindowsAPI::GetClientWidth() * 0.5f, WindowsAPI::GetClientHeight() - 60.0f }; // 中央基準位置
			Vector2 size_ = { 520.0f, 18.0f }; // 全体サイズ

			float shakeTime_ = 0.12f; // 揺れ時間
			float shakePower_ = 4.0f; // 揺れ強さ

			float lagSpeed_ = 900.0f; // 遅延バーの追従速度

			std::string frameTex_ = "./resources/texture/gray.jpg"; // フレーム
			std::string fillTex_ = "./resources/texture/blue.dds";  // 塗り

			Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 0.3f }; // 通常色
			Vector4 drainColor_ = { 0.25f, 0.95f, 1.0f, 1.0f }; // 減少時
			Vector4 refillColor_ = { 0.55f, 1.0f, 0.55f, 1.0f }; // 回復時
			Vector4 lagColor_ = { 0.65f, 0.65f, 0.65f, 1.0f }; // 遅延バー
		};

	public:

		/// <summary>
		/// 弾数UIを初期化します。
		/// </summary>
		/// <param name="spriteCommon">スプライト共通管理クラス</param>
		/// <param name="dxCommon">DirectX共通管理クラス</param>
		/// <param name="parentScene">所属シーン</param>
		/// <param name="desc">UI設定情報</param>
		void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc);

		/// <summary>
		/// 弾数UIを更新します。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="ammo">現在弾数</param>
		/// <param name="maxAmmo">最大弾数</param>
		/// <param name="refilling">回復中かどうか</param>
		/// <param name="blink">点滅を行うかどうか</param>
		void Update(float dt, int ammo, int maxAmmo, bool refilling, bool blink = false);

		/// <summary>
		/// 弾数UIを描画します。
		/// </summary>
		void Draw();

		/// <summary>
		/// 表示状態を取得します。
		/// </summary>
		/// <returns>trueなら表示中、falseなら非表示</returns>
		bool IsVisible() const { return visible_; }

		// Getter=====================================
		/// <summary>
		/// 設定情報を取得します。
		/// </summary>
		/// <returns>現在のUI設定情報</returns>
		const Desc& GetDesc() const { return desc_; }
		// ===========================================

		// Setter=====================================
		/// <summary>
		/// 表示状態を設定します。
		/// </summary>
		/// <param name="v">trueで表示、falseで非表示</param>
		void SetVisible(bool v);

		/// <summary>
		/// 設定情報を設定します。
		/// </summary>
		/// <param name="desc">設定するUI情報</param>
		void SetDesc(const Desc& desc);
		// ===========================================

	private:
		/// <summary>
		/// 指定範囲のランダム値を返します。
		/// </summary>
		/// <param name="a">最小値</param>
		/// <param name="b">最大値</param>
		/// <returns>a～b の範囲のランダム値</returns>
		float RandRange_(float a, float b);

		//======================================================================
		// 参照 / 設定
		//======================================================================
		SpriteCommon* spriteCommon_ = nullptr; // スプライト共通管理クラス
		DirectXCommon* dxCommon_ = nullptr; // DirectX共通管理クラス
		BaseScene* parentScene_ = nullptr; // 所属シーン
		Desc desc_{};     // UI設定
		bool visible_ = true; // 表示状態

		//======================================================================
		// 状態
		//======================================================================
		int lastAmmo_ = -1; // 前フレーム弾数
		int prevAmmo_ = -1; // さらに前の弾数
		float lagAmmo_ = 0.0f; // 遅延バー用弾数
		float prevHalfW_ = 0.0f; // 前フレーム幅

		//======================================================================
		// 揺れ
		//======================================================================
		float shakeTimer_ = 0.0f; // 揺れ残り時間

		//======================================================================
		// 色変化（発射直後）
		//======================================================================
		float drainTimer_ = 0.0f; // 色変化タイマー
		static constexpr float kDrainFlashSec_ = 0.10f; // 発射直後の色変化時間

		//======================================================================
		// パンチ演出
		//======================================================================
		float punchTimer_ = 0.0f; // パンチ残り時間
		static constexpr float kPunchSec_ = 0.08f; // パンチ時間

		//======================================================================
		// スプライト
		//======================================================================
		std::unique_ptr<Sprite> frame_; // フレーム
		std::unique_ptr<Sprite> fillL_; // 塗り左
		std::unique_ptr<Sprite> fillR_; // 塗り右
		std::unique_ptr<Sprite> lagL_; // 遅延バー左
		std::unique_ptr<Sprite> lagR_; // 遅延バー右

		//======================================================================
		// 破片
		//======================================================================
		struct Chip {
			std::unique_ptr<Sprite> sp_; // 破片スプライト
			Vector2 pos_{}; // 破片位置
			Vector2 vel_{}; // 破片速度
			float life_ = 0.0f; // 残り寿命
			float maxLife_ = 0.0f; // 最大寿命
			float size_ = 6.0f; // サイズ
			bool active_ = false; // 有効状態
		};

		std::vector<Chip> chips_; // 破片プール

		static constexpr int   kChipPool_ = 64; // 破片プール数
		static constexpr float kChipLife_ = 0.22f; // 破片寿命
		static constexpr float kChipSpeed_ = 140.0f; // 破片初速
		static constexpr float kChipSpread_ = 90.0f; // 拡散角度
		static constexpr float kChipGravity_ = 520.0f; // 重力加速度
		static constexpr float kChipSizeMin_ = 4.0f; // 破片最小サイズ
		static constexpr float kChipSizeMax_ = 10.0f; // 破片最大サイズ

		//======================================================================
		// 点滅
		//======================================================================
		float blinkT_ = 0.0f; // 点滅タイマー
		float blinkInterval_ = 0.10f; // 点滅間隔
		float blinkLowMul_ = 0.25f; // 点滅時の暗さ倍率

		/// <summary>
		/// 破片を生成します。
		/// </summary>
		/// <param name="cx">生成中心X座標</param>
		/// <param name="y">生成位置Y座標</param>
		/// <param name="oldHalf">生成前の半幅</param>
		/// <param name="newHalf">生成後の半幅</param>
		void SpawnChips_(float cx, float y, float oldHalf, float newHalf);

		/// <summary>
		/// 破片を更新します。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		void UpdateChips_(float dt);

		/// <summary>
		/// 破片を描画します。
		/// </summary>
		void DrawChips_();
	};
}