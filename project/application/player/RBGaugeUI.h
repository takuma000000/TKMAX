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

	class RBGaugeUI {
	public:
		struct Desc {
			// 位置は「中央基準」で扱う（左右端→中央の計算が楽）
			Vector2 center_ = { WindowsAPI::kClientWidth_ * 0.5f, WindowsAPI::kClientHeight_ - 60.0f };
			Vector2 size_ = { 520.0f, 18.0f }; // 全体幅/高さ

			// shake
			float shakeTime_ = 0.12f;
			float shakePower_ = 4.0f;

			// lag（遅延バー）
			float lagSpeed_ = 900.0f; // 大きいほど速く追従（弾なので速めが気持ちいい）

			// テクスチャ
			std::string frameTex_ = "./resources/circle.png";      // 仮（差し替えOK）
			std::string fillTex_ = "./resources/gradationLine.png";  // 仮（差し替えOK）

			// 色（単純回避用）
			Vector4 baseColor_ = { 0.25f, 0.95f, 1.0f, 1.0f };   // 通常
			Vector4 drainColor_ = { 1.0f, 0.75f, 0.15f, 1.0f };   // 減ってる最中（撃った直後）
			Vector4 refillColor_ = { 0.55f, 1.0f, 0.55f, 1.0f };   // 回復中
			Vector4 lagColor_ = { 0.65f, 0.65f, 0.65f, 1.0f };  // 遅延バー
		};

	public:

		/// <summary>
		/// 弾数 UI を初期化します。
		/// </summary>
		/// <param name="spriteCommon">スプライト共通管理クラス</param>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="parentScene">所属する親シーン</param>
		/// <param name="desc">弾数 UI の設定情報</param>
		void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc);
		/// <summary>
		/// 弾数 UI の更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="ammo">現在の弾数</param>
		/// <param name="maxAmmo">最大弾数</param>
		/// <param name="refilling">リロード（補充）中の場合 true</param>
		void Update(float dt, int ammo, int maxAmmo, bool refilling);
		/// <summary>
		/// 弾数 UI を描画します。
		/// </summary>
		void Draw();

		/// <summary>
		/// UI が表示状態かどうかを取得します。
		/// </summary>
		/// <returns>表示中の場合 true、それ以外は false</returns>
		bool IsVisible() const { return visible_; }

		// Getter=====================================
		/// <summary>
		/// 設定の取得。
		/// </summary>
		/// <returns></returns>
		Desc& GetDesc() { return desc_; }
		// ===========================================
		// Setter=====================================
		/// <summary>
		/// 可視状態を設定します。
		/// </summary>
		/// <param name="v">表示する場合 true、それ以外は false</param>
		void SetVisible(bool v) { visible_ = v; }
		// ===========================================

	private:
		/// <summary>
		/// a〜b の範囲でランダムな浮動小数点数を返します。
		/// </summary>
		/// <param name="a">最小値</param>
		/// <param name="b">最大値</param>
		/// <returns>a〜b の範囲内のランダムな浮動小数点数</returns>
		float RandRange_(float a, float b) {
			return a + (b - a) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
		}

		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;
		Desc desc_{};

		bool visible_ = true;

		// 状態
		int lastAmmo_ = -1;
		float lagAmmo_ = 0.0f;

		// 揺れ
		float shakeTimer_ = 0.0f;

		// 直後だけ色を変える（撃った直後＝短時間）
		float drainTimer_ = 0.0f;
		static constexpr float kDrainFlashSec_ = 0.10f;

		// スプライト
		std::unique_ptr<Sprite> frame_;
		std::unique_ptr<Sprite> fillL_;
		std::unique_ptr<Sprite> fillR_;
		std::unique_ptr<Sprite> lagL_;
		std::unique_ptr<Sprite> lagR_;

		// パンチ（減った瞬間だけ縮んで戻る）
		float punchTimer_ = 0.0f;
		static constexpr float kPunchSec_ = 0.08f; // 好みで

		int prevAmmo_ = -1;

		struct Chip {
			std::unique_ptr<Sprite> sp_;
			Vector2 pos_{};
			Vector2 vel_{};
			float life_ = 0.0f;
			float maxLife_ = 0.0f;
			float size_ = 6.0f;
			bool active_ = false;
		};

		std::vector<Chip> chips_;

		// 破片パラメータ（好みで調整）
		static constexpr int   kChipPool_ = 64;
		static constexpr float kChipLife_ = 0.22f;
		static constexpr float kChipSpeed_ = 140.0f;
		static constexpr float kChipSpread_ = 90.0f;
		static constexpr float kChipGravity_ = 520.0f;
		static constexpr float kChipSizeMin_ = 4.0f;
		static constexpr float kChipSizeMax_ = 10.0f;

		// 前フレームの「片側幅」を保持（削れた量から破片数を決める）
		float prevHalfW_ = 0.0f;

		/// <summary>
		/// 破片を発生させます。
		/// </summary>
		/// <param name="cx">発生中心の X 座標（ワールド座標）</param>
		/// <param name="y">発生位置の Y 座標（ワールド座標）</param>
		/// <param name="oldHalf">分割前の半径（または半幅）</param>
		/// <param name="newHalf">分割後の半径（または半幅）</param>
		void SpawnChips_(float cx, float y, float oldHalf, float newHalf);
		/// <summary>
		/// 破片の更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		void UpdateChips_(float dt);
		/// <summary>
		/// 破片を描画する。
		/// </summary>
		void DrawChips_();
	};
} // namespace TKM