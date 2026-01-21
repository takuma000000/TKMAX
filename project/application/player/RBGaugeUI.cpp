#define NOMINMAX
#include "RBGaugeUI.h"
#include <algorithm>

namespace TKM {

	void RBGaugeUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		desc_ = desc;

		// frame（枠）
		frame_ = std::make_unique<Sprite>();
		frame_->Initialize(spriteCommon_, dxCommon_, desc_.frameTex);
		frame_->SetParentScene(parentScene_);
		frame_->SetAnchorPoint({ 0.0f, 0.0f });
		frame_->SetAutoAdjustTextureSize(false);

		// lag（後ろ）
		lagL_ = std::make_unique<Sprite>();
		lagL_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex);
		lagL_->SetParentScene(parentScene_);
		lagL_->SetAnchorPoint({ 1.0f, 0.0f });
		lagL_->SetColor(desc_.lagColor);
		lagL_->SetAutoAdjustTextureSize(false);

		lagR_ = std::make_unique<Sprite>();
		lagR_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex);
		lagR_->SetParentScene(parentScene_);
		lagR_->SetAnchorPoint({ 1.0f, 0.0f });
		lagR_->SetColor(desc_.lagColor);
		lagR_->SetAutoAdjustTextureSize(false);

		// fill（前）
		fillL_ = std::make_unique<Sprite>();
		fillL_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex);
		fillL_->SetParentScene(parentScene_);
		fillL_->SetAnchorPoint({ 0.0f, 0.0f });
		fillL_->SetColor(desc_.baseColor);
		fillL_->SetAutoAdjustTextureSize(false);

		fillR_ = std::make_unique<Sprite>();
		fillR_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex);
		fillR_->SetParentScene(parentScene_);
		fillR_->SetAnchorPoint({ 0.0f, 0.0f });
		fillR_->SetColor(desc_.baseColor);
		fillR_->SetAutoAdjustTextureSize(false);

		// 初期状態
		lastAmmo_ = -1;
		lagAmmo_ = 0.0f;
		shakeTimer_ = 0.0f;
		drainTimer_ = 0.0f;
	}

	void RBGaugeUI::Update(float dt, int ammo, int maxAmmo, bool refilling) {
		if (!visible_) { return; }
		if (!frame_ || !fillL_ || !fillR_ || !lagL_ || !lagR_) { return; }

		maxAmmo = std::max(1, maxAmmo);
		ammo = std::clamp(ammo, 0, maxAmmo);

		// 初回
		if (lastAmmo_ < 0 || lastAmmo_ > maxAmmo) {
			lastAmmo_ = ammo;
			lagAmmo_ = float(ammo);
		}

		// 消費検出：弾が減った瞬間だけ演出
		if (ammo < lastAmmo_) {
			shakeTimer_ = desc_.shakeTime;
			drainTimer_ = kDrainFlashSec_;
			lastAmmo_ = ammo;
		}

		// drainTimer 更新
		if (drainTimer_ > 0.0f) {
			drainTimer_ -= dt;
			if (drainTimer_ < 0.0f) drainTimer_ = 0.0f;
		}

		// rate
		float rate = float(ammo) / float(maxAmmo);
		rate = std::clamp(rate, 0.0f, 1.0f);

		// lag（後ろ）追従
		if (lagAmmo_ > float(ammo)) {
			lagAmmo_ = std::max(float(ammo), lagAmmo_ - desc_.lagSpeed * dt);
		} else {
			lagAmmo_ = float(ammo);
		}

		float lagRate = lagAmmo_ / float(maxAmmo);
		lagRate = std::clamp(lagRate, 0.0f, 1.0f);

		// 両端→中央：ゲージが減るほど「中央だけ残る」方式
		float halfW = desc_.size.x * 0.5f;

		float curHalf = halfW * rate;      // 片側の残り幅
		float lagHalf = halfW * lagRate;

		float cx = desc_.center.x;
		float y = desc_.center.y;

		// 枠（全体）
		Vector2 framePos = { cx - halfW, y };
		frame_->SetPosition({ framePos.x - 5.0f, framePos.y - 5.0f });
		frame_->SetSize({ desc_.size.x + 10.0f, desc_.size.y + 10.0f });

		// ---- ここがキモ ----
		// 左：中心を右端固定（anchorX=1）で左に伸びる
		// 右：中心を左端固定（anchorX=0）で右に伸びる
		// → 幅が小さくなるほど「中心に向かって縮む」= 両端から中央へ減る

		// サイズ反映
		lagL_->SetSize({ lagHalf, desc_.size.y });
		lagR_->SetSize({ lagHalf, desc_.size.y });

		fillL_->SetSize({ curHalf, desc_.size.y });
		fillR_->SetSize({ curHalf, desc_.size.y });

		// 位置（中心固定）
		Vector2 leftPos = { cx, y };
		Vector2 rightPos = { cx, y };

		// シェイク
		if (shakeTimer_ > 0.0f) {
			shakeTimer_ -= dt;

			Vector2 off{
				RandRange_(-desc_.shakePower, desc_.shakePower),
				RandRange_(-desc_.shakePower, desc_.shakePower)
			};

			frame_->SetPosition({ (framePos.x - 5.0f) + off.x, (framePos.y - 5.0f) + off.y });

			lagL_->SetPosition({ leftPos.x + off.x,  leftPos.y + off.y });
			lagR_->SetPosition({ rightPos.x + off.x, rightPos.y + off.y });

			fillL_->SetPosition({ leftPos.x + off.x,  leftPos.y + off.y });
			fillR_->SetPosition({ rightPos.x + off.x, rightPos.y + off.y });
		} else {
			frame_->SetPosition({ framePos.x - 5.0f, framePos.y - 5.0f });

			lagL_->SetPosition(leftPos);
			lagR_->SetPosition(rightPos);

			fillL_->SetPosition(leftPos);
			fillR_->SetPosition(rightPos);
		}

		// Update
		frame_->Update();
		lagL_->Update();
		lagR_->Update();
		fillL_->Update();
		fillR_->Update();
	}

	void RBGaugeUI::Draw() {
		if (!visible_) { return; }
		if (frame_) frame_->Draw();
		if (lagL_)  lagL_->Draw();
		if (lagR_)  lagR_->Draw();
		if (fillL_) fillL_->Draw();
		if (fillR_) fillR_->Draw();
	}

} // namespace TKM