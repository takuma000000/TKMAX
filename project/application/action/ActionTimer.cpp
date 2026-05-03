#include "ActionTimer.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include <algorithm>
#include <cmath>

void ActionTimer::Initialize(TKM::DirectXCommon* dxCommon, int limitSeconds) {
	remainingTime_ = static_cast<float>(limitSeconds);

	LoadNumberTextures_();
	CreateDigitSprites_(dxCommon);
	UpdateDigitSprites_();
}

void ActionTimer::Update() {
	if (remainingTime_ > 0.0f) {
		remainingTime_ -= kFrameTime_;

		if (remainingTime_ < 0.0f) {
			remainingTime_ = 0.0f;
		}
	}

	UpdateDigitSprites_();

	for (auto& digitSet : digitSprites_) {
		for (auto& sprite : digitSet) {
			if (sprite) {
				sprite->Update();
			}
		}
	}
}

void ActionTimer::Draw() {
	for (auto& digitSet : digitSprites_) {
		for (auto& sprite : digitSet) {
			if (sprite) {
				sprite->Draw();
			}
		}
	}
}

int ActionTimer::GetDisplaySeconds() const {
	return static_cast<int>(std::ceil(remainingTime_));
}

void ActionTimer::LoadNumberTextures_() {
	for (int i = 0; i < kDigitCount_; ++i) {
		numberTexturePaths_[i] = "./resources/texture/number/" + std::to_string(i) + ".png";
		TKM::TextureManager::GetInstance()->LoadTexture(numberTexturePaths_[i]);
	}
}

void ActionTimer::CreateDigitSprites_(TKM::DirectXCommon* dxCommon) {
	constexpr int kMaxDisplayDigits = 3;

	digitSprites_.clear();
	digitSprites_.resize(kMaxDisplayDigits);

	for (int digitIndex = 0; digitIndex < kMaxDisplayDigits; ++digitIndex) {
		for (int number = 0; number < kDigitCount_; ++number) {
			auto sprite = std::make_unique<TKM::Sprite>();

			sprite->Initialize(
				TKM::SpriteCommon::GetInstance(),
				dxCommon,
				numberTexturePaths_[number]
			);

			sprite->SetAutoAdjustTextureSize(false);
			sprite->SetTextureLeftTop({ 0.0f, 0.0f });

			const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata(numberTexturePaths_[number]);

			Vector2 texSize = {
				static_cast<float>(meta.width),
				static_cast<float>(meta.height)
			};

			sprite->SetTextureSize(texSize);
			sprite->SetSize({ kDigitWidth_, kDigitHeight_ });
			sprite->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

			digitSprites_[digitIndex][number] = std::move(sprite);
		}
	}
}

void ActionTimer::UpdateDigitSprites_() {
	int displaySeconds = GetDisplaySeconds();

	if (displaySeconds < 0) {
		displaySeconds = 0;
	}

	std::string timeText = std::to_string(displaySeconds);

	for (size_t digitIndex = 0; digitIndex < digitSprites_.size(); ++digitIndex) {
		Vector2 position = {
			basePosition_.x + static_cast<float>(digitIndex) * (kDigitWidth_ + kDigitSpacing_),
			basePosition_.y
		};

		for (int number = 0; number < kDigitCount_; ++number) {
			auto& sprite = digitSprites_[digitIndex][number];

			if (!sprite) {
				continue;
			}

			sprite->SetPosition(position);
			sprite->SetSize({ kDigitWidth_, kDigitHeight_ });

			if (digitIndex < timeText.size() && number == timeText[digitIndex] - '0') {
				sprite->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
			} else {
				sprite->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
			}
		}
	}
}