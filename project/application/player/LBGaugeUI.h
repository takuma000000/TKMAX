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
			std::string fillTex_ = "./resources/texture/gold.jpeg";

			// 色
			Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };   // 通常
			Vector4 drainColor_ = { 0.25f, 0.95f, 1.0f, 1.0f }; // 消費直後
			Vector4 refillColor_ = { 0.55f, 1.0f, 0.55f, 1.0f };// 回復直後
		};

	public:
		void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc);
		void Update(float dt, int ammo, int maxAmmo, bool blink = false);
		void Draw();

		bool IsVisible() const { return visible_; }
		const Desc& GetDesc() const { return desc_; }
		void SetVisible(bool v) { visible_ = v; }
		void SetDesc(const Desc& desc) { desc_ = desc; ApplyLayout_(); }

	private:
		void ApplyLayout_();

	private:
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
	};

}