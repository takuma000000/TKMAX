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

namespace TKM {

	class RBGaugeUI {
	public:
		struct Desc {
			// 位置は「中央基準」で扱う（左右端→中央の計算が楽）
			Vector2 center = { WindowsAPI::kClientWidth * 0.5f, WindowsAPI::kClientHeight - 60.0f };
			Vector2 size = { 520.0f, 18.0f }; // 全体幅/高さ

			// shake
			float shakeTime = 0.12f;
			float shakePower = 4.0f;

			// lag（遅延バー）
			float lagSpeed = 900.0f; // 大きいほど速く追従（弾なので速めが気持ちいい）

			// テクスチャ
			std::string frameTex = "./resources/circle.png";      // 仮（差し替えOK）
			std::string fillTex = "./resources/gradationLine.png";  // 仮（差し替えOK）

			// 色（単純回避用）
			Vector4 baseColor = { 0.25f, 0.95f, 1.0f, 1.0f };   // 通常
			Vector4 drainColor = { 1.0f, 0.75f, 0.15f, 1.0f };   // 減ってる最中（撃った直後）
			Vector4 refillColor = { 0.55f, 1.0f, 0.55f, 1.0f };   // 回復中
			Vector4 lagColor = { 0.65f, 0.65f, 0.65f, 1.0f };  // 遅延バー
		};

	public:

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="spriteCommon"></param>
		/// <param name="dxCommon"></param>
		/// <param name="parentScene"></param>
		/// <param name="desc"></param>
		void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc);
		/// <summary>
		/// 更新
		/// </summary>
		/// <param name="dt"></param>
		/// <param name="ammo"></param>
		/// <param name="maxAmmo"></param>
		/// <param name="refilling"></param>
		void Update(float dt, int ammo, int maxAmmo, bool refilling);
		/// <summary>
		/// 描画
		/// </summary>
		void Draw();

		/// <summary>
		/// 可視状態の取得。
		/// </summary>
		/// <returns></returns>
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
		/// 可視状態の設定。
		/// </summary>
		/// <param name="v"></param>
		void SetVisible(bool v) { visible_ = v; }
		// ===========================================

	private:
		/// <summary>
		/// a〜bの範囲でランダムな浮動小数点数を返す。
		/// </summary>
		/// <param name="a"></param>
		/// <param name="b"></param>
		/// <returns></returns>
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
	};
} // namespace TKM