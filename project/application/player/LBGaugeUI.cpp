#define NOMINMAX
#include "LBGaugeUI.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>

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
		// 初期値
		currentAmmo_ = 0;
		lastAmmo_ = -1;
		drainTimer_ = 0.0f;
		refillTimer_ = 0.0f;
		shownAmmo_ = 0;
		refillTarget_ = 0;
		refillAnimating_ = false;
		refillStepTimer_ = 0.0f;

		ApplyLayout_();
	}

	void LBGaugeUI::ApplyLayout_() {
		if (!frame_) { return; }

		// frame：中心→左上に変換して配置
		const float left = desc_.center_.x - desc_.size_.x * 0.5f;
		const float top = desc_.center_.y - desc_.size_.y * 0.5f;

		frame_->SetPosition({ left, top });
		frame_->SetSize(desc_.size_);

		baseFramePos_ = { left, top }; // アニメ用の基準位置
		baseFrameSize_ = desc_.size_; // アニメ用の基準サイズ

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

				baseSegPos_[i] = p; // アニメ用の基準位置
				baseSegSize_[i] = { segW, segH }; // アニメ用の基準サイズ
			} else {
				// 使わない分は画面外に退避（Drawでも描かないけど保険）
				seg_[i]->SetPosition({ -10000.0f, -10000.0f });
				seg_[i]->SetSize({ 1.0f, 1.0f });
			}
		}
	}

	void LBGaugeUI::ApplyAnim_(float dt, bool pressed) {
		if (!frame_) { return; }

		//----------------------------
		// 押下パルス（ふわふわ）
		//----------------------------
		const float targetAmp = pressed ? 1.0f : 0.0f;
		// 追従を少し速めに（0.0 -> 1.0）
		pressPulseAmp_ += (targetAmp - pressPulseAmp_) * std::min(1.0f, dt * 12.0f);
		pressPulseT_ += dt * 10.0f;

		float pulse = 1.0f + std::sinf(pressPulseT_) * (0.03f * pressPulseAmp_); // 3%程度

		//----------------------------
		// パンチ（消費/回復）
		//----------------------------
		float punch = 1.0f;

		if (punchTimer_ > 0.0f) {
			punchTimer_ -= dt;
			float t = std::clamp(punchTimer_ / kPunchSec_, 0.0f, 1.0f); // 1 -> 0
			punch += t * punchAmp_;
		}
		if (refillPunchTimer_ > 0.0f) {
			refillPunchTimer_ -= dt;
			float t = std::clamp(refillPunchTimer_ / kRefillPunchSec_, 0.0f, 1.0f);
			punch += t * refillPunchAmp_;
		}

		float scale = pulse * punch;

		//----------------------------
		// シェイク（消費時にだけ）
		//----------------------------
		Vector2 shake{ 0.0f, 0.0f };
		if (shakeTimer_ > 0.0f) {
			shakeTimer_ -= dt;
			float k = std::clamp(shakeTimer_ / kShakeSec_, 0.0f, 1.0f);

			auto rand01 = []() {
				return float(std::rand()) / float(RAND_MAX);
				};

			float rx = (rand01() * 2.0f - 1.0f);
			float ry = (rand01() * 2.0f - 1.0f);

			shake.x = rx * shakeAmpPx_ * k;
			shake.y = ry * shakeAmpPx_ * k;
		}

		//----------------------------
		// スケールを「中心拡縮」っぽく見せるための位置補正
		// anchorは(0,0)なので、拡縮分の半分だけ左上に戻す
		//----------------------------
		auto apply = [&](Sprite* sp, const Vector2& basePos, const Vector2& baseSize) {
			if (!sp) { return; }

			Vector2 size{ baseSize.x * scale, baseSize.y * scale };
			Vector2 pos{
				basePos.x - (size.x - baseSize.x) * 0.5f + shake.x,
				basePos.y - (size.y - baseSize.y) * 0.5f + shake.y
			};

			sp->SetPosition(pos);
			sp->SetSize(size);
			};

		apply(frame_.get(), baseFramePos_, baseFrameSize_);
		for (int i = 0; i < 5; i++) {
			apply(seg_[i].get(), baseSegPos_[i], baseSegSize_[i]);
		}
	}

	void LBGaugeUI::Update(float dt, int ammo, int maxAmmo, bool blink) {
		if (!visible_) { return; }
		if (!frame_) { return; }

		maxAmmo = std::max(1, maxAmmo);
		ammo = std::clamp(ammo, 0, maxAmmo);
		currentAmmo_ = ammo;

		//==================================================
		// 初回：0→ammo へ「パパパ」で立ち上げ
		//==================================================
		if (lastAmmo_ < 0) {
			shownAmmo_ = 0;                         // 初期は0から見せる
			refillAnimating_ = true;                // 回復アニメ開始
			refillTarget_ = std::clamp(ammo, 0, 5); // 目標
			refillStepTimer_ = 0.0f;

			lastAmmo_ = ammo; // 初期化
		} else {

			//============================
			// 変化検出
			//============================
			if (ammo < lastAmmo_) {
				drainTimer_ = kFlashSec_;
				punchTimer_ = kPunchSec_;
				shakeTimer_ = kShakeSec_;

				// 減少は即反映（残像が残らないように）
				shownAmmo_ = std::clamp(ammo, 0, 5);
				refillAnimating_ = false;

			} else if (ammo > lastAmmo_) {
				refillTimer_ = kFlashSec_;
				refillPunchTimer_ = kRefillPunchSec_;

				shownAmmo_ = std::clamp(shownAmmo_, 0, 5);

				// いま表示してる数 → 目標(ammo)までパパパ
				refillAnimating_ = true;
				refillTarget_ = std::clamp(ammo, 0, 5);
				refillStepTimer_ = 0.0f;
			}

			lastAmmo_ = ammo;
		}

		//============================
		// タイマー
		//============================
		if (drainTimer_ > 0.0f) drainTimer_ -= dt;
		if (refillTimer_ > 0.0f) refillTimer_ -= dt;

		//============================
		// 回復段階更新
		//============================
		if (refillAnimating_) {
			refillStepTimer_ += dt;

			while (refillStepTimer_ >= kRefillStepSec_) {
				refillStepTimer_ -= kRefillStepSec_;

				if (shownAmmo_ < refillTarget_) {
					shownAmmo_++;
					// 1個増えるたびに軽くパンチ（気持ちよさ）
					refillPunchTimer_ = kRefillPunchSec_;
				} else {
					refillAnimating_ = false;
					break;
				}
			}
		}

		//============================
		// 色（押下中はちょい明るく）
		//============================
		Vector4 c = desc_.baseColor_;
		if (drainTimer_ > 0.0f) { c = desc_.drainColor_; }
		if (refillTimer_ > 0.0f) { c = desc_.refillColor_; }
		if (blink) { c.w = Clamp01_(c.w + 0.25f); }

		//============================
		// スプライト更新
		//============================
		frame_->Update();

		for (int i = 0; i < 5; i++) {
			if (!seg_[i]) continue;
			seg_[i]->Update();
			seg_[i]->SetColor(c);
		}

		// 動きアニメは1回だけ（ループ外）
		ApplyAnim_(dt, blink);
	}

	void LBGaugeUI::Draw() {
		if (!visible_) { return; }
		if (!frame_) { return; }

		frame_->Draw();

		// 「左から ammo 個」表示（= 右から消える）
		int drawCount = std::clamp(shownAmmo_, 0, 5);
		for (int i = 0; i < drawCount; i++) {
			if (seg_[i]) { seg_[i]->Draw(); }
		}
	}

}