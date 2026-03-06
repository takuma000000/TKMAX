#include "GameResultMenuController.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "TextureManager.h"

float GameResultMenuController::Clamp01_(float v) {
	if (v < 0.0f) return 0.0f;
	if (v > 1.0f) return 1.0f;
	return v;
}

void GameResultMenuController::Initialize(
	TKM::SpriteCommon* spriteCommon,
	TKM::DirectXCommon* dxCommon,
	TKM::BaseScene* parentScene,
	float screenW,
	float screenH,
	const Desc& desc
) {
	spriteCommon_ = spriteCommon;
	dxCommon_ = dxCommon;
	parentScene_ = parentScene;
	screenW_ = screenW;
	screenH_ = screenH;
	desc_ = desc;

	// パネル
	panel_ = std::make_unique<TKM::Sprite>();
	panel_->Initialize(spriteCommon_, dxCommon_, desc_.panelTex);
	panel_->SetParentScene(parentScene_);
	panel_->SetAnchorPoint({ 0.0f, 0.0f });
	panel_->SetAutoAdjustTextureSize(false);

	// 項目（Pauseと同じ：autoAdjust OFF なら UV を実サイズにする）
	for (int i = 0; i < (int)Item::Count; ++i) {
		items_[i] = std::make_unique<TKM::Sprite>();
		items_[i]->Initialize(spriteCommon_, dxCommon_, desc_.itemTex[i]);
		items_[i]->SetParentScene(parentScene_);
		items_[i]->SetAnchorPoint({ 0.5f, 0.5f });
		items_[i]->SetAutoAdjustTextureSize(false);

		{
			const auto& md = TKM::TextureManager::GetInstance()->GetMetadata(desc_.itemTex[i]);
			items_[i]->SetTextureLeftTop({ 0.0f, 0.0f });
			items_[i]->SetTextureSize({ (float)md.width, (float)md.height });
		}

		items_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.65f });
	}

	// カーソル
	cursor_ = std::make_unique<TKM::Sprite>();
	cursor_->Initialize(spriteCommon_, dxCommon_, desc_.cursorTex);
	cursor_->SetParentScene(parentScene_);
	cursor_->SetAnchorPoint({ 0.0f, 0.0f });
	cursor_->SetAutoAdjustTextureSize(false);
	cursor_->SetSize({ 28.0f, 28.0f });
	cursor_->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f });

	pulseTime_ = 0.0f;
	index_ = 0;

	UpdateLayout(screenW_, screenH_);
}

GameResultMenuController::Command GameResultMenuController::Update(float dt) {
	if (TriggerPadUp_()) {
		MoveIndex_(-1);
	}
	if (TriggerPadDown_()) {
		MoveIndex_(+1);
	}

	if (TriggerA_()) {
		if (index_ == (int)Item::Restart) {
			return Command::Restart;
		}
		return Command::ReturnToTitle;
	}

	// ---- 見た目（ポーズ画面と同じ）----
	pulseTime_ += dt;
	float pulse = 1.0f + 0.06f * std::sin(pulseTime_ * 6.0f);

	// パネル
	if (panel_) {
		panel_->SetPosition(panelPos_);
		panel_->SetSize(panelSize_);
		panel_->SetColor({ 0.08f, 0.08f, 0.10f, 0.75f });
		panel_->Update();
	}

	// 項目
	for (int i = 0; i < (int)Item::Count; ++i) {
		if (!items_[i]) { continue; }

		bool selected = (i == index_);

		Vector2 pos = {
			baseItemPos_.x,
			baseItemPos_.y + itemSpacingY_ * (float)i
		};

		Vector4 col = selected ? Vector4{ 1.0f, 1.0f, 1.0f, 0.92f } : Vector4{ 1.0f, 1.0f, 1.0f, 0.62f };
		items_[i]->SetColor(col);

		Vector2 baseSize = selected ? Vector2{ 240.0f, 48.0f } : Vector2{ 220.0f, 44.0f };
		Vector2 size = selected ? Vector2{ baseSize.x * pulse, baseSize.y * pulse } : baseSize;

		items_[i]->SetPosition(pos);
		items_[i]->SetSize(size);
		items_[i]->Update();
	}

	// カーソル
	if (cursor_) {
		Vector2 pos = { baseItemPos_.x - 150.0f, baseItemPos_.y + itemSpacingY_ * (float)index_ };
		cursor_->SetPosition(pos);
		cursor_->SetSize({ 28.0f, 28.0f });
		cursor_->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f });
		cursor_->Update();
	}

	return Command::None;
}

void GameResultMenuController::Draw() {
	if (panel_) panel_->Draw();
	for (int i = 0; i < (int)Item::Count; ++i) {
		if (items_[i]) items_[i]->Draw();
	}
	if (cursor_) cursor_->Draw();
}

void GameResultMenuController::UpdateLayout(float screenW, float screenH) {
	screenW_ = screenW;
	screenH_ = screenH;

	// タイトル/ポーズと同じ「右下・余白40」
	panelSize_ = { 340.0f, 200.0f };
	panelPos_ = { screenW_ - panelSize_.x - 40.0f, screenH_ - panelSize_.y - 40.0f };

	// タイトルで調整したのと同じ基準
	baseItemPos_ = { panelPos_.x + panelSize_.x * 0.5f, panelPos_.y + 60.0f + 22.0f };
	itemSpacingY_ = 64.0f;
}

bool GameResultMenuController::TriggerPadUp_() {
	auto* in = TKM::Input::GetInstance();
	const bool cur = in->TriggerKey(DIK_UP) || in->TriggerButton(XINPUT_GAMEPAD_DPAD_UP);
	const bool trig = (cur && !prevUp_);
	prevUp_ = cur;
	return trig;
}

bool GameResultMenuController::TriggerPadDown_() {
	auto* in = TKM::Input::GetInstance();
	const bool cur = in->TriggerKey(DIK_DOWN) || in->TriggerButton(XINPUT_GAMEPAD_DPAD_DOWN);
	const bool trig = (cur && !prevDown_);
	prevDown_ = cur;
	return trig;
}

bool GameResultMenuController::TriggerA_() {
	auto* in = TKM::Input::GetInstance();
	const bool cur = in->TriggerKey(DIK_SPACE) || in->TriggerKey(DIK_RETURN) || in->TriggerButton(XINPUT_GAMEPAD_A);
	const bool trig = (cur && !prevA_);
	prevA_ = cur;
	return trig;
}

void GameResultMenuController::MoveIndex_(int delta) {
	const int count = (int)Item::Count;
	index_ = (index_ + delta + count) % count;
}