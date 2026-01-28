#define NOMINMAX
#include "PauseMenuController.h"
#include <algorithm>
#include <cmath>

namespace TKM {
	void PauseMenuController::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH, const Desc& desc) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		screenW_ = screenW;
		screenH_ = screenH;
		desc_ = desc;

		state_ = State::Closed;
		index_ = 0;
		animT_ = 0.0f;
		curtainAlpha_ = 0.0f;

		// 暗幕
		curtain_ = std::make_unique<Sprite>();
		curtain_->Initialize(spriteCommon_, dxCommon_, desc_.curtainTex);
		curtain_->SetParentScene(parentScene_);
		curtain_->SetAnchorPoint({ 0.0f, 0.0f });
		curtain_->SetAutoAdjustTextureSize(false);
		curtain_->SetPosition({ 0.0f, 0.0f });
		curtain_->SetSize({ screenW_, screenH_ });
		curtain_->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });

		// パネル
		panel_ = std::make_unique<Sprite>();
		panel_->Initialize(spriteCommon_, dxCommon_, desc_.panelTex);
		panel_->SetParentScene(parentScene_);
		panel_->SetAnchorPoint({ 0.0f, 0.0f });
		panel_->SetAutoAdjustTextureSize(false);

		// 項目
		for (int i = 0; i < (int)Item::Count; ++i) {
			items_[i] = std::make_unique<Sprite>();
			items_[i]->Initialize(spriteCommon_, dxCommon_, desc_.itemTex[i]);
			items_[i]->SetParentScene(parentScene_);
			items_[i]->SetAnchorPoint({ 0.0f, 0.0f });
			items_[i]->SetAutoAdjustTextureSize(false);
			items_[i]->SetSize({ 220.0f, 100.0f });
			{
				const auto& md = TextureManager::GetInstance()->GetMetadata(desc_.itemTex[i]);
				items_[i]->SetTextureLeftTop({ 0.0f,0.0f });
				items_[i]->SetTextureSize({ (float)md.width, (float)md.height });
			}
			items_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.65f });
		}

		// カーソル
		cursor_ = std::make_unique<Sprite>();
		cursor_->Initialize(spriteCommon_, dxCommon_, desc_.cursorTex);
		cursor_->SetParentScene(parentScene_);
		cursor_->SetAnchorPoint({ 0.0f, 0.0f });
		cursor_->SetAutoAdjustTextureSize(false);
		cursor_->SetSize({ 28.0f, 28.0f });
		cursor_->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f });

		UpdateLayout(screenW_, screenH_);
	}

	void PauseMenuController::UpdateLayout(float screenW, float screenH) {
		screenW_ = screenW;
		screenH_ = screenH;

		panelSize_ = { 340.0f, 280.0f };
		panelPos_ = { screenW_ - panelSize_.x - 40.0f, screenH_ - panelSize_.y - 40.0f };

		baseItemPos_ = { panelPos_.x + 70.0f, panelPos_.y + 70.0f };
		itemSpacingY_ = 70.0f;

		if (curtain_) {
			curtain_->SetSize({ screenW_, screenH_ });
		}
		if (panel_) {
			panel_->SetPosition(panelPos_);
			panel_->SetSize(panelSize_);
			panel_->SetColor({ 0.08f, 0.08f, 0.10f, 0.75f });
		}

		for (int i = 0; i < (int)Item::Count; ++i) {
			if (items_[i]) {
				items_[i]->SetPosition({ baseItemPos_.x, baseItemPos_.y + itemSpacingY_ * (float)i });
			}
		}
	}

	bool PauseMenuController::TriggerPadUp_() {
		Input* in = Input::GetInstance();
		bool now = in->PushButton(XINPUT_GAMEPAD_DPAD_UP);
		bool trig = (now && !prevUp_);
		prevUp_ = now;
		return trig;
	}

	bool PauseMenuController::TriggerPadDown_() {
		Input* in = Input::GetInstance();
		bool now = in->PushButton(XINPUT_GAMEPAD_DPAD_DOWN);
		bool trig = (now && !prevDown_);
		prevDown_ = now;
		return trig;
	}

	bool PauseMenuController::TriggerA_() {
		Input* in = Input::GetInstance();
		bool now = in->PushButton(XINPUT_GAMEPAD_A);
		bool trig = (now && !prevA_);
		prevA_ = now;
		return trig;
	}

	bool PauseMenuController::TriggerB_() {
		Input* in = Input::GetInstance();
		bool now = in->PushButton(XINPUT_GAMEPAD_B);
		bool trig = (now && !prevB_);
		prevB_ = now;
		return trig;
	}

	void PauseMenuController::Open_() {
		state_ = State::Pausing;
		animT_ = 0.0f;
		index_ = 0;
	}

	void PauseMenuController::Close_() {
		state_ = State::Resuming;
		animT_ = 0.0f;
	}

	void PauseMenuController::MoveIndex_(int delta) {
		int count = (int)Item::Count;
		index_ = (index_ + delta + count) % count;
	}

	PauseMenuController::Command PauseMenuController::Update(float dt, bool allowOpen) {
		// Startで開く
		Input* in = Input::GetInstance();
		bool startNow = in->PushButton(XINPUT_GAMEPAD_START);
		static bool prevStart = false;
		bool trigStart = (startNow && !prevStart);
		prevStart = startNow;

		if (state_ == State::Closed) {
			if (allowOpen && trigStart) {
				Open_();
			}
			return Command::None;
		}

		// フェード
		if (state_ == State::Pausing) {
			animT_ = std::min(animT_ + dt * 6.0f, 1.0f);
			curtainAlpha_ = MyMath::Lerp(curtainAlpha_, 0.55f, 0.25f);
			if (animT_ >= 1.0f) {
				state_ = State::Paused;
			}
		} else if (state_ == State::Resuming) {
			animT_ = std::min(animT_ + dt * 6.0f, 1.0f);
			curtainAlpha_ = MyMath::Lerp(curtainAlpha_, 0.0f, 0.25f);
			if (animT_ >= 1.0f) {
				state_ = State::Closed;
			}
			return Command::None;
		} else {
			curtainAlpha_ = MyMath::Lerp(curtainAlpha_, 0.55f, 0.2f);
		}

		// 操作
		if (state_ == State::Paused) {
			if (TriggerPadUp_()) { MoveIndex_(-1); }
			if (TriggerPadDown_()) { MoveIndex_(+1); }

			if (TriggerB_()) {
				Close_();
				return Command::None;
			}

			if (TriggerA_()) {
				if (index_ == (int)Item::Resume) {
					Close_();
					return Command::Resume;
				}
				if (index_ == (int)Item::Restart) {
					Close_();
					return Command::Restart;
				}
				if (index_ == (int)Item::ReturnToTitle) {
					Close_();
					return Command::ReturnToTitle; // ★長押し無しで即確定
				}
			}
		}

		// 見た目更新（選択が分かる：カーソル + 拡大 + 右ズレ）
		animT_ += dt;

		if (curtain_) {
			curtain_->SetColor({ 0.0f, 0.0f, 0.0f, curtainAlpha_ });
			curtain_->Update();
		}
		if (panel_) {
			panel_->Update();
		}

		float pulse = 1.0f + 0.06f * std::sin(animT_ * 6.0f);

		for (int i = 0; i < (int)Item::Count; ++i) {
			if (!items_[i]) { continue; }

			bool selected = (i == index_);

			Vector2 itemPos = { baseItemPos_.x + (selected ? 12.0f : 0.0f), baseItemPos_.y + itemSpacingY_ * (float)i };
			items_[i]->SetPosition(itemPos);

			Vector4 col = selected ? Vector4{ 1.0f, 1.0f, 1.0f, 0.92f } : Vector4{ 1.0f, 1.0f, 1.0f, 0.62f };
			items_[i]->SetColor(col);

			Vector2 size = selected ? Vector2{ 240.0f * pulse, 48.0f * pulse } : Vector2{ 220.0f, 44.0f };
			items_[i]->SetSize(size);

			items_[i]->Update();
		}

		if (cursor_) {
			Vector2 pos = { baseItemPos_.x - 42.0f, baseItemPos_.y + itemSpacingY_ * (float)index_ + 8.0f };
			cursor_->SetPosition(pos);
			cursor_->SetSize({ 28.0f, 28.0f });
			cursor_->Update();
		}

		return Command::None;
	}

	void PauseMenuController::Draw() {
		if (state_ == State::Closed) { return; }

		if (curtain_) curtain_->Draw();
		if (panel_) panel_->Draw();
		for (int i = 0; i < (int)Item::Count; ++i) {
			if (items_[i]) items_[i]->Draw();
		}
		if (cursor_) cursor_->Draw();
	}
} // namespace TKM