#include "TitleMenuController.h"
#include "DirectXCommon.h"
#include "BaseScene.h"

void TitleMenuController::Initialize(TKM::SpriteCommon* spriteCommon, TKM::DirectXCommon* dxCommon, TKM::BaseScene* parentScene, float screenW, float screenH, const Desc& desc) {
	spriteCommon_ = spriteCommon;
	dxCommon_ = dxCommon;
	parentScene_ = parentScene;
	screenW_ = screenW;
	screenH_ = screenH;
	desc_ = desc;

	panel_ = std::make_unique<TKM::Sprite>();
	panel_->Initialize(spriteCommon_, dxCommon_, desc_.panelTex);

	panel_->SetAutoAdjustTextureSize(false);
	for (int i = 0; i < (int)Item::Count; ++i) {
		items_[i] = std::make_unique<TKM::Sprite>();
		items_[i]->Initialize(spriteCommon_, dxCommon_, desc_.itemTex[i]);
		items_[i]->SetAutoAdjustTextureSize(false);
	}

	cursor_ = std::make_unique<TKM::Sprite>();
	cursor_->Initialize(spriteCommon_, dxCommon_, desc_.cursorTex);
	cursor_->SetAutoAdjustTextureSize(false);

	for (int i = 0; i < (int)Item::Count; ++i) {
		itemScale_[i] = 1.0f;
	}

	UpdateLayout(screenW_, screenH_);
}

TitleMenuController::Command TitleMenuController::Update(float dt) {
	// 入力
	if (TriggerPadUp_()) { MoveIndex_(-1); }
	if (TriggerPadDown_()) { MoveIndex_(+1); }

	// ==========================
	// 見た目：ポーズ画面と同じ脈動
	// ==========================
	pulseTime_ += dt;
	float pulse = 1.0f + 0.06f * std::sin(pulseTime_ * 6.0f);

	// パネル（必要なら薄く）
	if (panel_) {
		panel_->SetColor({ 0.08f, 0.08f, 0.10f, 0.75f });
		panel_->Update();
	}

	// 項目：選択中は大きく + 脈動、非選択は小さめ
	for (int i = 0; i < (int)Item::Count; ++i) {
		if (!items_[i]) { continue; }

		bool selected = (i == index_);

		// 位置：選択中は少し右へ（ポーズ画面と同じ）
		Vector2 pos = {
			baseItemPos_.x + (selected ? 12.0f : 0.0f),
			baseItemPos_.y + itemSpacingY_ * (float)i
		};
		items_[i]->SetPosition(pos);

		// 色：選択中は濃く、非選択は薄く
		Vector4 col = selected ? Vector4{ 1.0f,1.0f,1.0f,0.92f } : Vector4{ 1.0f,1.0f,1.0f,0.62f };
		items_[i]->SetColor(col);

		// サイズ：ポーズ画面準拠
		Vector2 baseSize = selected ? Vector2{ 240.0f, 48.0f } : Vector2{ 220.0f, 44.0f };
		Vector2 size = selected ? Vector2{ baseSize.x * pulse, baseSize.y * pulse } : baseSize;
		items_[i]->SetSize(size);

		items_[i]->Update();
	}

	// カーソル：選択項目に追従
	if (cursor_) {
		Vector2 pos = { baseItemPos_.x - 42.0f, baseItemPos_.y + itemSpacingY_ * (float)index_ + 8.0f };
		cursor_->SetPosition(pos);
		cursor_->SetSize({ 28.0f, 28.0f });
		cursor_->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f });
		cursor_->Update();
	}

	// 決定
	if (TriggerA_()) {
		return (index_ == (int)Item::Start) ? Command::Start : Command::Exit;
	}

	return Command::None;
}

void TitleMenuController::Draw() {
	if (panel_) panel_->Draw();

	for (int i = 0; i < (int)Item::Count; ++i) {
		if (items_[i]) items_[i]->Draw();
	}

	if (cursor_) cursor_->Draw();
}

void TitleMenuController::UpdateLayout(float screenW, float screenH) {
	screenW_ = screenW;
	screenH_ = screenH;

	panelSize_ = { 340.0f, 200.0f }; // パネルサイズ
	// 画面右下に寄せる
	panelPos_ = { screenW_ - panelSize_.x - 40.0f, screenH_ - panelSize_.y - 40.0f };

	baseItemPos_ = { panelPos_.x + 60.0f, panelPos_.y + 60.0f }; // 項目基準位置
	itemSpacingY_ = 64.0f; // 項目間隔

	if (panel_) {
		panel_->SetPosition(panelPos_);
		panel_->SetSize(panelSize_);
		panel_->SetColor({ 1,1,1,0.6f });
	}

	for (int i = 0; i < (int)Item::Count; ++i) {
		if (items_[i]) {
			Vector2 p = { baseItemPos_.x, baseItemPos_.y + itemSpacingY_ * (float)i };
			items_[i]->SetPosition(p);
		}
	}

	if (cursor_) {
		Vector2 cp = { baseItemPos_.x - 70.0f, baseItemPos_.y + itemSpacingY_ * (float)index_ + 10.0f };
		cursor_->SetPosition(cp);
		cursor_->SetSize({ 32.0f, 32.0f });
	}
}

bool TitleMenuController::TriggerPadUp_() {
	auto* in = TKM::Input::GetInstance();
	const bool cur = in->TriggerKey(DIK_UP) || in->TriggerButton(XINPUT_GAMEPAD_DPAD_UP);
	const bool trig = (cur && !prevUp_);
	prevUp_ = cur;
	return trig;
}

bool TitleMenuController::TriggerPadDown_() {
	auto* in = TKM::Input::GetInstance();
	const bool cur = in->TriggerKey(DIK_DOWN) || in->TriggerButton(XINPUT_GAMEPAD_DPAD_DOWN);
	const bool trig = (cur && !prevDown_);
	prevDown_ = cur;
	return trig;
}

bool TitleMenuController::TriggerA_() {
	auto* in = TKM::Input::GetInstance();
	const bool cur = in->TriggerKey(DIK_SPACE) || in->TriggerKey(DIK_RETURN) || in->TriggerButton(XINPUT_GAMEPAD_A);
	const bool trig = (cur && !prevA_);
	prevA_ = cur;
	return trig;
}

void TitleMenuController::MoveIndex_(int delta) {
	const int count = (int)Item::Count;
	index_ = (index_ + delta + count) % count;

	if (cursor_) {
		Vector2 cp = { baseItemPos_.x - 70.0f, baseItemPos_.y + itemSpacingY_ * (float)index_ + 10.0f };
		cursor_->SetPosition(cp);
	}
}