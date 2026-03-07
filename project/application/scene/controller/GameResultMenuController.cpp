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
	panel_->SetAnchorPoint({ 0.0f, 0.0f }); // タイトル/ポーズと同じ基準
	panel_->SetAutoAdjustTextureSize(false);

	// 項目
	for (int i = 0; i < (int)Item::Count; ++i) {
		items_[i] = std::make_unique<TKM::Sprite>();
		items_[i]->Initialize(spriteCommon_, dxCommon_, desc_.itemTex[i]);
		items_[i]->SetParentScene(parentScene_);
		items_[i]->SetAnchorPoint({ 0.5f, 0.5f }); // タイトル/ポーズと同じ基準
		items_[i]->SetAutoAdjustTextureSize(false);

		{
			const auto& md = TKM::TextureManager::GetInstance()->GetMetadata(desc_.itemTex[i]);
			items_[i]->SetTextureLeftTop({ 0.0f, 0.0f }); // タイトル/ポーズと同じ基準
			items_[i]->SetTextureSize({ (float)md.width, (float)md.height }); // タイトル/ポーズと同じ基準
		}
		// タイトル/ポーズと同じ基準で、選択されていない状態の色にしておく
		items_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.65f });
	}

	// カーソル
	cursor_ = std::make_unique<TKM::Sprite>();
	cursor_->Initialize(spriteCommon_, dxCommon_, desc_.cursorTex);
	cursor_->SetParentScene(parentScene_);
	cursor_->SetAnchorPoint({ 0.0f, 0.0f }); // タイトル/ポーズと同じ基準で、項目の左側に表示する
	cursor_->SetAutoAdjustTextureSize(false); // タイトル/ポーズと同じ基準で、テクスチャサイズに関わらず 28x28 にしておく
	cursor_->SetSize({ 28.0f, 28.0f }); // タイトル/ポーズと同じ基準で、テクスチャサイズに関わらず 28x28 にしておく
	cursor_->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f }); // タイトル/ポーズと同じ基準で、選択されている状態の色にしておく

	// 状態初期化
	pulseTime_ = 0.0f;
	index_ = 0;

	// レイアウト更新
	UpdateLayout(screenW_, screenH_);
}

GameResultMenuController::Command GameResultMenuController::Update(float dt) {
	// 入力
	if (TriggerPadUp_()) {
		MoveIndex_(-1); // 上入力でインデックスを減らす
	}
	// 下入力でインデックスを増やす
	if (TriggerPadDown_()) {
		MoveIndex_(+1); // 下入力でインデックスを増やす
	}
	// A ボタンで決定
	if (TriggerA_()) {
		// 決定された項目に応じたコマンドを返す
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
		panel_->SetColor({ 0.08f, 0.08f, 0.10f, 0.75f }); // タイトル/ポーズと同じ基準で、選択されていない状態の色にしておく
		panel_->Update();
	}

	// 項目
	for (int i = 0; i < (int)Item::Count; ++i) {
		if (!items_[i]) { continue; }
		// 選択されている項目は明るく、サイズも少し大きくして強調する
		bool selected = (i == index_);
		// タイトル/ポーズと同じ基準で、選択されている項目は明るく、サイズも少し大きくして強調する
		Vector2 pos = {
			baseItemPos_.x,
			baseItemPos_.y + itemSpacingY_ * (float)i
		};
		// タイトル/ポーズと同じ基準で、選択されている項目は明るく、サイズも少し大きくして強調する
		Vector4 col = selected ? Vector4{ 1.0f, 1.0f, 1.0f, 0.92f } : Vector4{ 1.0f, 1.0f, 1.0f, 0.62f };
		items_[i]->SetColor(col); // タイトル/ポーズと同じ基準で、選択されている項目は明るく、サイズも少し大きくして強調する

		Vector2 baseSize = selected ? Vector2{ 240.0f, 48.0f } : Vector2{ 220.0f, 44.0f }; // タイトル/ポーズと同じ基準で、選択されている項目はサイズも少し大きくして強調する
		Vector2 size = selected ? Vector2{ baseSize.x * pulse, baseSize.y * pulse } : baseSize; // タイトル/ポーズと同じ基準で、選択されている項目はサイズも少し大きくして強調する
		// タイトル/ポーズと同じ基準で、選択されている項目は明るく、サイズも少し大きくして強調する
		items_[i]->SetPosition(pos);
		items_[i]->SetSize(size);
		items_[i]->Update();
	}

	// カーソル
	if (cursor_) {
		Vector2 pos = { baseItemPos_.x - 150.0f, baseItemPos_.y + itemSpacingY_ * (float)index_ }; // タイトル/ポーズと同じ基準で、項目の左側に表示する
		cursor_->SetPosition(pos);
		cursor_->SetSize({ 28.0f, 28.0f }); // タイトル/ポーズと同じ基準で、テクスチャサイズに関わらず 28x28 にしておく
		cursor_->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f }); // タイトル/ポーズと同じ基準で、選択されている状態の色にしておく
		cursor_->Update();
	}

	return Command::None; // 何もなければ None を返す
}

void GameResultMenuController::Draw() {
	if (panel_) panel_->Draw();

	// 項目
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

// 入力トリガー判定
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