#pragma once
#include <memory>
#include <string>
#include <array>
#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "WindowsAPI.h"

namespace TKM {

	class LBGaugeUI {
	public:
		struct Desc {
			// RBと同じ思想：中心座標で扱う
			Vector2 center_ = { WindowsAPI::kClientWidth_ * 0.5f, WindowsAPI::kClientHeight_ - 40.0f };
			Vector2 size_ = { 520.0f, 18.0f };

			// 5分割
			int segments_ = 5;
			float pad_ = 2.2f;     // 内側余白
			float gap_ = 7.7f;     // セグメント間の隙間

			// テクスチャ
			std::string frameTex_ = "./resources/texture/gray.jpg";
			std::string fillTex_ = "./resources/texture/gauge_green.jpg";

			// 色
			Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };   // 通常
			Vector4 drainColor_ = { 0.25f, 0.95f, 1.0f, 1.0f }; // 消費直後
			Vector4 refillColor_ = { 0.55f, 1.0f, 0.55f, 1.0f };// 回復直後
		};

		void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc);
		void Update(float dt, int ammo, int maxAmmo, bool blink = false);
		void Draw();

		bool IsVisible() const { return visible_; }
		const Desc& GetDesc() const { return desc_; }
		void SetVisible(bool v) { visible_ = v; }
		void SetDesc(const Desc& desc) { desc_ = desc; ApplyLayout_(); }

	private:
		void ApplyLayout_();
		void ApplyAnim_(float dt, bool pressed);

		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;

		Desc desc_{};
		bool visible_ = true;

		int currentAmmo_ = 0;
		int lastAmmo_ = -1;

		float drainTimer_ = 0.0f;
		float refillTimer_ = 0.0f;
		static constexpr float kFlashSec_ = 0.12f;

		std::unique_ptr<Sprite> frame_;
		std::array<std::unique_ptr<Sprite>, 5> seg_{};

		//======================================================================
		// レイアウト基準（ApplyLayout_で保存して、Updateで動かす）
		//======================================================================
		Vector2 baseFramePos_{};
		Vector2 baseFrameSize_{};
		std::array<Vector2, 5> baseSegPos_{};
		std::array<Vector2, 5> baseSegSize_{};

		//======================================================================
		// アニメ（押下パルス / 消費パンチ / 回復パンチ / シェイク）
		//======================================================================
		float pressPulseT_ = 0.0f;      // 押下中の波
		float pressPulseAmp_ = 0.0f;    // 0..1（押してない時は0へ戻す）

		float shakeTimer_ = 0.0f;
		static constexpr float kShakeSec_ = 0.12f;
		float shakeAmpPx_ = 2.0f;       // 揺れ幅（小さめが良い）

		float punchTimer_ = 0.0f;       // 消費パンチ
		static constexpr float kPunchSec_ = 0.10f;
		float punchAmp_ = 0.10f;        // 拡縮量（10%）

		float refillPunchTimer_ = 0.0f; // 回復パンチ
		static constexpr float kRefillPunchSec_ = 0.10f;
		float refillPunchAmp_ = 0.07f;  // 回復は少し弱め
		//======================================================================
		// 回復時の段階表示
		//======================================================================
		int shownAmmo_ = 0;          // 今 “見せてる” 個数
		int refillTarget_ = 0;       // 最終的に見せたい個数
		bool refillAnimating_ = false; // 回復アニメ中かどうか
		float refillStepTimer_ = 0.0f; // 次の個数を見せるまでのタイマー
		static constexpr float kRefillStepSec_ = 0.045f; // パパパ速度（好みで）
	};

}