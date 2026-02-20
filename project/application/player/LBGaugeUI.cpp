#define NOMINMAX
#include "LBGaugeUI.h"
#include <algorithm>

namespace TKM {

	static float Clamp01_(float a) {
		if (a < 0.0f) return 0.0f;
		if (a > 1.0f) return 1.0f;
		return a;
	}

	void LBGaugeUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		desc_ = desc;

		frame_ = std::make_unique<Sprite>();
		frame_->Initialize(spriteCommon_, dxCommon_, desc_.frameTex_);
		frame_->SetParentScene(parentScene_);
		frame_->SetAnchorPoint({ 0.0f, 0.0f });
		frame_->SetAutoAdjustTextureSize(false);

		for (int i = 0; i < 5; i++) {
			seg_[i] = std::make_unique<Sprite>();
			seg_[i]->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
			seg_[i]->SetParentScene(parentScene_);
			seg_[i]->SetAnchorPoint({ 0.0f, 0.0f });
			seg_[i]->SetAutoAdjustTextureSize(false);
			seg_[i]->SetColor(desc_.baseColor_);
		}

		currentAmmo_ = 0;
		lastAmmo_ = -1;
		drainTimer_ = 0.0f;
		refillTimer_ = 0.0f;

		ApplyLayout_();
	}

	void LBGaugeUI::ApplyLayout_() {
		if (!frame_) { return; }

		// frame：中心→左上に変換して配置
		const float left = desc_.center_.x - desc_.size_.x * 0.5f;
		const float top = desc_.center_.y - desc_.size_.y * 0.5f;

		frame_->SetPosition({ left, top });
		frame_->SetSize(desc_.size_);

		// 5分割
		int segCount = std::clamp(desc_.segments_, 1, 5);
		const float pad = std::max(0.0f, desc_.pad_);
		const float gap = std::max(0.0f, desc_.gap_);

		const float innerW = std::max(1.0f, desc_.size_.x - pad * 2.0f);
		const float innerH = std::max(1.0f, desc_.size_.y - pad * 2.0f);

		const float totalGap = gap * float(segCount - 1);
		const float segW = std::max(1.0f, (innerW - totalGap) / float(segCount));
		const float segH = innerH;

		for (int i = 0; i < 5; i++) {
			if (!seg_[i]) continue;

			if (i < segCount) {
				Vector2 p{
					left + pad + (segW + gap) * float(i),
					top + pad
				};
				seg_[i]->SetPosition(p);
				seg_[i]->SetSize({ segW, segH });
			} else {
				// 使わない分は画面外に退避（Drawでも描かないけど保険）
				seg_[i]->SetPosition({ -10000.0f, -10000.0f });
				seg_[i]->SetSize({ 1.0f, 1.0f });
			}
		}
	}

	void LBGaugeUI::Update(float dt, int ammo, int maxAmmo, bool blink) {
		if (!visible_) { return; }
		if (!frame_) { return; }

		maxAmmo = std::max(1, maxAmmo);
		ammo = std::clamp(ammo, 0, maxAmmo);
		currentAmmo_ = ammo;

		// 変化検出
		if (lastAmmo_ >= 0) {
			if (ammo < lastAmmo_) {
				drainTimer_ = kFlashSec_;
			} else if (ammo > lastAmmo_) {
				refillTimer_ = kFlashSec_;
			}
		}
		lastAmmo_ = ammo;

		if (drainTimer_ > 0.0f) drainTimer_ -= dt;
		if (refillTimer_ > 0.0f) refillTimer_ -= dt;

		// 色（RBと同じ：押してる最中はちょい明るくして存在感）
		Vector4 c = desc_.baseColor_;
		if (drainTimer_ > 0.0f) {
			c = desc_.drainColor_;
		}
		if (refillTimer_ > 0.0f) {
			c = desc_.refillColor_;
		}
		if (blink) {
			c.w = Clamp01_(c.w + 0.25f);
		}

		for (int i = 0; i < 5; i++) {
			if (seg_[i]) seg_[i]->SetColor(c);
		}
		// 更新
		if (frame_) { frame_->Update(); }
		for (int i = 0; i < 5; i++) {
			if (seg_[i]) { seg_[i]->Update(); }
		}
	}

	void LBGaugeUI::Draw() {
		if (!visible_) { return; }
		if (!frame_) { return; }

		frame_->Draw();

		// 「左から ammo 個」表示（= 右から消える）
		int drawCount = std::clamp(currentAmmo_, 0, 5);
		for (int i = 0; i < drawCount; i++) {
			if (seg_[i]) { seg_[i]->Draw(); }
		}
	}

}