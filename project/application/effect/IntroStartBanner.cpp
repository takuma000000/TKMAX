#include "IntroStartBanner.h"

namespace TKM {

	void IntroStartBanner::Initialize(DirectXCommon* dxCommon) {
		sprite_ = std::make_unique<Sprite>();
		sprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon, "./resources/texture/start.png");
		sprite_->SetAnchorPoint({ 0.5f, 0.5f });
		sprite_->SetPosition({ startPos_.x, startPos_.y });
		sprite_->SetSize({ 100, 100 });
		sprite_->SetColor({ 1,1,1,1 });

		Reset();
	}

	void IntroStartBanner::Reset() {
		slideIn_ = false;
		visible_ = false;
		started_ = false;
		fadeOut_ = false;
		finished_ = false;
		holdElapsed_ = 0.0f;
		alpha_ = 1.0f;

		if (sprite_) {
			sprite_->SetPosition({ startPos_.x, startPos_.y });
			sprite_->SetColor({ 1,1,1,1 });
		}
	}

	void IntroStartBanner::Start() {
		if (started_) { return; }

		started_ = true;
		visible_ = true;
		slideIn_ = true;
		fadeOut_ = false;
		finished_ = false;
		holdElapsed_ = 0.0f;
		alpha_ = 1.0f;

		sprite_->SetColor({ 1,1,1,alpha_ });
		sprite_->SetPosition({ startPos_.x, endPos_.y });
		tween_.Reset(0.0f, 1.0f, duration_, Ease::Type::OutBack);
	}

	void IntroStartBanner::Update(float dt) {
		if (!visible_ || !sprite_) { return; }

		if (slideIn_) {
			float t = tween_.Update(dt);

			if (glowOn_) {
				float glow = 1.0f + glowAmp_ * std::sin(t * MyMath::GetPI());
				sprite_->SetColor({ glow, glow, glow, alpha_ });
			} else {
				sprite_->SetColor({ 1,1,1,alpha_ });
			}

			float x = MyMath::Lerp(startPos_.x, endPos_.x, t);
			sprite_->SetPosition({ x, endPos_.y });
			sprite_->Update();

			if (tween_.Finished()) {
				slideIn_ = false;
				holdElapsed_ = 0.0f;
			}
			return;
		}

		if (!fadeOut_ && glowOn_) {
			float t01 = (holdSec_ > 0.0f) ? std::min(holdElapsed_ / holdSec_, 1.0f) : 1.0f;
			float decay = 1.0f - 0.7f * t01;
			float glow = 1.0f + decay * 0.20f * std::sin(holdElapsed_ * glowSpeed_);
			sprite_->SetColor({ glow, glow, glow, alpha_ });
		}

		if (!fadeOut_) {
			holdElapsed_ += dt;
			if (holdElapsed_ >= holdSec_) {
				fadeOut_ = true;
			}
		}

		if (fadeOut_) {
			alpha_ -= dt / fadeSec_;
			if (alpha_ <= 0.0f) {
				alpha_ = 0.0f;
				visible_ = false;
				finished_ = true;
			}
			sprite_->SetColor({ 1,1,1,alpha_ });
		}

		sprite_->Update();
	}

	void IntroStartBanner::Draw() const {
		if (visible_ && sprite_) {
			sprite_->Draw();
		}
	}

}