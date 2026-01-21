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
		lagR_->SetAnchorPoint({ 0.0f, 0.0f });
		lagR_->SetColor(desc_.lagColor);
		lagR_->SetAutoAdjustTextureSize(false);

		// fill（前）
		fillL_ = std::make_unique<Sprite>();
		fillL_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex);
		fillL_->SetParentScene(parentScene_);
		fillL_->SetAnchorPoint({ 1.0f, 0.0f });
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

		prevAmmo_ = -1;
		punchTimer_ = 0.0f;
		prevHalfW_ = 0.0f;

		// 破片プール
		chips_.clear();
		chips_.resize(kChipPool_);
		for (auto& c : chips_) {
			c.sp = std::make_unique<Sprite>();
			c.sp->Initialize(spriteCommon_, dxCommon_, desc_.fillTex); // fillTex流用
			c.sp->SetParentScene(parentScene_);
			c.sp->SetAnchorPoint({ 0.5f, 0.5f });
			c.sp->SetAutoAdjustTextureSize(false);
			c.active = false;
		}
	}

	void RBGaugeUI::Update(float dt, int ammo, int maxAmmo, bool refilling) {
		if (!visible_) { return; }
		if (!frame_ || !fillL_ || !fillR_ || !lagL_ || !lagR_) { return; }

		maxAmmo = std::max(1, maxAmmo);
		ammo = std::clamp(ammo, 0, maxAmmo);

		// prevAmmo 初期化（初回だけ）
		if (prevAmmo_ < 0 || prevAmmo_ > maxAmmo) {
			prevAmmo_ = ammo;
		}

		// 消費検出（oldAmmo保持が重要）
		int oldAmmo = prevAmmo_;
		bool consumed = (ammo < oldAmmo);
		if (consumed) {
			shakeTimer_ = desc_.shakeTime;
			drainTimer_ = kDrainFlashSec_;
			punchTimer_ = kPunchSec_;
		}

		// rate
		float rate = std::clamp(float(ammo) / float(maxAmmo), 0.0f, 1.0f);

		// lag（後ろ）追従
		if (lagAmmo_ > float(ammo)) {
			lagAmmo_ = std::max(float(ammo), lagAmmo_ - desc_.lagSpeed * dt);
		} else {
			lagAmmo_ = float(ammo);
		}
		float lagRate = std::clamp(lagAmmo_ / float(maxAmmo), 0.0f, 1.0f);

		// サイズ計算
		float halfW = desc_.size.x * 0.5f;

		float curHalf = halfW * rate;
		float lagHalf = halfW * lagRate;

		float cx = desc_.center.x;
		float y = desc_.center.y;

		// 枠
		Vector2 framePos = { cx - halfW, y };
		frame_->SetSize({ desc_.size.x + 10.0f, desc_.size.y + 10.0f });

		// 破片：減った分だけ外側が砕ける（左右同時）
		if (consumed) {
			float oldRate = std::clamp(float(oldAmmo) / float(maxAmmo), 0.0f, 1.0f);
			float oldHalf = halfW * oldRate;
			float newHalf = curHalf;
			SpawnChips_(cx, y, oldHalf, newHalf);
		}

		// パンチ（高さだけ）
		float punch = 1.0f;
		if (punchTimer_ > 0.0f) {
			float t = std::clamp(punchTimer_ / kPunchSec_, 0.0f, 1.0f);
			float k = 1.0f - t;
			punch = 1.0f - 0.10f * (1.0f - k);
		}
		float h = desc_.size.y * punch;

		lagL_->SetSize({ lagHalf, h });
		lagR_->SetSize({ lagHalf, h });
		fillL_->SetSize({ curHalf, h });
		fillR_->SetSize({ curHalf, h });

		// シェイク
		Vector2 off{ 0.0f, 0.0f };
		if (shakeTimer_ > 0.0f) {
			off.x = RandRange_(-desc_.shakePower, desc_.shakePower);
			off.y = RandRange_(-desc_.shakePower, desc_.shakePower);
		}

		frame_->SetPosition({ (framePos.x - 5.0f) + off.x, (framePos.y - 5.0f) + off.y });

		// 位置（中心固定：アンカーで左右へ伸びる）
		fillL_->SetPosition({ cx + off.x, y + off.y });
		lagL_->SetPosition({ cx + off.x, y + off.y });
		fillR_->SetPosition({ cx + off.x, y + off.y });
		lagR_->SetPosition({ cx + off.x, y + off.y });

		// 色（フラッシュ）
		Vector4 col = desc_.baseColor;
		if (refilling) {
			col = desc_.refillColor;
		} else if (drainTimer_ > 0.0f) {
			col = desc_.drainColor;
		}
		fillL_->SetColor(col);
		fillR_->SetColor(col);

		lagL_->SetColor(desc_.lagColor);
		lagR_->SetColor(desc_.lagColor);

		// タイマー更新
		if (drainTimer_ > 0.0f) { drainTimer_ = std::max(0.0f, drainTimer_ - dt); }
		if (shakeTimer_ > 0.0f) { shakeTimer_ = std::max(0.0f, shakeTimer_ - dt); }
		if (punchTimer_ > 0.0f) { punchTimer_ = std::max(0.0f, punchTimer_ - dt); }

		// 破片更新
		UpdateChips_(dt);

		// Update
		frame_->Update();
		lagL_->Update();
		lagR_->Update();
		fillL_->Update();
		fillR_->Update();

		// prevAmmo 更新（最後）
		prevAmmo_ = ammo;
	}

	void RBGaugeUI::Draw() {
		if (!visible_) { return; }

		if (frame_) frame_->Draw();
		if (lagL_)  lagL_->Draw();
		if (lagR_)  lagR_->Draw();
		if (fillL_) fillL_->Draw();
		if (fillR_) fillR_->Draw();

		// 破片はゲージの上に
		DrawChips_();
	}

	// ----------------------------
	// 破片
	// ----------------------------
	void RBGaugeUI::SpawnChips_(float cx, float y, float oldHalf, float newHalf) {
		float removed = oldHalf - newHalf;
		if (removed <= 0.0f) { return; }

		// 削れ量に応じて破片数
		int count = (int)std::clamp(removed / 6.0f, 2.0f, 10.0f);

		float leftEdge = cx - oldHalf;
		float rightEdge = cx + oldHalf;

		for (int side = 0; side < 2; side++) {
			for (int i = 0; i < count; i++) {

				Chip* c = nullptr;
				for (auto& it : chips_) {
					if (!it.active) { c = &it; break; }
				}
				if (!c) { return; }

				c->active = true;
				c->maxLife = kChipLife_;
				c->life = kChipLife_;

				float px = (side == 0) ? leftEdge : rightEdge;
				px += RandRange_(-4.0f, 4.0f);
				float py = y + RandRange_(-desc_.size.y * 0.35f, desc_.size.y * 0.35f);

				c->pos = { px, py };

				float dir = (side == 0) ? -1.0f : 1.0f;
				float vx = dir * (kChipSpeed_ + RandRange_(-kChipSpread_, kChipSpread_));
				float vy = RandRange_(-120.0f, 40.0f);
				c->vel = { vx, vy };

				c->size = RandRange_(kChipSizeMin_, kChipSizeMax_);

				c->sp->SetSize({ c->size, c->size });
				c->sp->SetPosition(c->pos);

				// 砕け色（減少色）
				c->sp->SetColor(desc_.drainColor);
			}
		}
	}

	void RBGaugeUI::UpdateChips_(float dt) {
		for (auto& c : chips_) {
			if (!c.active) { continue; }

			c.life -= dt;
			if (c.life <= 0.0f) {
				c.active = false;
				continue;
			}

			// 物理っぽく
			c.vel.y += kChipGravity_ * dt;
			c.pos.x += c.vel.x * dt;
			c.pos.y += c.vel.y * dt;

			// フェード
			float a = std::clamp(c.life / c.maxLife, 0.0f, 1.0f);

			Vector4 col = desc_.drainColor;
			col.w *= a;

			c.sp->SetColor(col);
			c.sp->SetPosition(c.pos);
			c.sp->Update();
		}
	}

	void RBGaugeUI::DrawChips_() {
		for (auto& c : chips_) {
			if (!c.active) { continue; }
			c.sp->Draw();
		}
	}

} // namespace TKM