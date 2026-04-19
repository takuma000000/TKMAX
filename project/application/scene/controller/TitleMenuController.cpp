#include "TitleMenuController.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "AudioManager.h"

void TitleMenuController::Initialize(TKM::SpriteCommon* spriteCommon, TKM::DirectXCommon* dxCommon, TKM::BaseScene* parentScene, float screenW, float screenH, const Desc& desc) {
	spriteCommon_ = spriteCommon;
	dxCommon_ = dxCommon;
	parentScene_ = parentScene;
	screenW_ = screenW;
	screenH_ = screenH;
	desc_ = desc;

	// 項目スプライト生成
	for (int i = 0; i < (int)Item::Count; ++i) {
		items_[i] = std::make_unique<TKM::Sprite>();
		items_[i]->Initialize(spriteCommon_, dxCommon_, desc_.itemTex[i]);
		items_[i]->SetAutoAdjustTextureSize(false);
		items_[i]->SetAnchorPoint({ 0.5f, 0.5f }); // 中心
		// テクスチャサイズ設定
		{
			const auto& md = TKM::TextureManager::GetInstance()->GetMetadata(desc_.itemTex[i]);
			items_[i]->SetTextureLeftTop({ 0.0f, 0.0f }); // テクスチャ全体を使う
			items_[i]->SetTextureSize({ (float)md.width, (float)md.height }); // テクスチャサイズをスプライトのサイズにする
		}
		// 色は少し薄めにしておく（Updateで選択中は濃く、非選択は薄くする）
		items_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.65f });
	}

	// 初期値
	for (int i = 0; i < (int)Item::Count; ++i) {
		itemScale_[i] = 1.0f; // 初期スケールは1.0（等倍）
	}

	// レイアウト更新
	UpdateLayout(screenW_, screenH_);
}

TitleMenuController::Command TitleMenuController::Update(float dt) {
	if (!visible_) { return Command::None; } // 非表示のときは入力も見た目も更新しない

	// ==========================
	// 入力
	// ==========================
	if (TriggerPadUp_()) {
		MoveIndex_(-1);
	}
	if (TriggerPadDown_()) {
		MoveIndex_(+1);
	}

	// ==========================
	// 見た目：ポーズ画面と同じ脈動
	// ==========================
	pulseTime_ += dt;
	float pulse = 1.0f + 0.06f * std::sin(pulseTime_ * 6.0f);

	// 項目：選択中は大きく + 脈動、非選択は小さめ
	for (int i = 0; i < (int)Item::Count; ++i) {
		if (!items_[i]) { continue; }

		bool selected = (i == index_);

		// 位置：選択中は少し右へ（ポーズ画面と同じ）
		Vector2 pos = {
			baseItemPos_.x,
			baseItemPos_.y + itemSpacingY_ * (float)i
		};
		items_[i]->SetPosition(pos);

		// 色：選択中は濃く、非選択は薄く
		Vector4 col = selected ? Vector4{ 1.0f,1.0f,1.0f,0.92f } : Vector4{ 1.0f,1.0f,1.0f,0.62f };
		items_[i]->SetColor(col);

		const auto& md = TKM::TextureManager::GetInstance()->GetMetadata(desc_.itemTex[i]);
		// 基準の高さ（選択中は大きく、非選択は小さめ）
		float baseHeight = selected ? 72.0f : 64.0f;
		// 脈動
		if (selected) {
			baseHeight *= pulse;
		}
		// アスペクト比を保って幅を決める
		float aspect = 1.0f;
		if (md.height > 0) {
			aspect = (float)md.width / (float)md.height;
		}
		// サイズ
		Vector2 size = {
			baseHeight * aspect,
			baseHeight
		};
		// スプライトにサイズを設定
		items_[i]->SetSize(size);
		items_[i]->Update();
	}

	// 決定
	if (TriggerA_()) {
		TKM::AudioManager::GetInstance()->PlaySound("decision", 0.1f);
		return (index_ == (int)Item::Start) ? Command::Start : Command::Exit;
	}

	return Command::None;
}

void TitleMenuController::Draw() {
	if (!visible_) { return; } // 非表示のときは入力も見た目も更新しない

	for (int i = 0; i < (int)Item::Count; ++i) {
		items_[i]->Draw();
	}
}

void TitleMenuController::UpdateLayout(float screenW, float screenH) {
	screenW_ = screenW;
	screenH_ = screenH;

	// サイズ: 項目が収まる程度に
	panelSize_ = { 340.0f, 200.0f };
	// 位置: 真ん中下あたり
	panelPos_ = { 470.0f, 500.0f };
	// 項目の基準位置: パネルの内側で、上から少し下がったあたり
	baseItemPos_ = { panelPos_.x + panelSize_.x * 0.5f, panelPos_.y + 72.0f };
	// 項目間隔
	itemSpacingY_ = 70.0f;

	for (int i = 0; i < (int)Item::Count; ++i) {
		Vector2 p = { baseItemPos_.x, baseItemPos_.y + itemSpacingY_ * (float)i };
		items_[i]->SetPosition(p);
	}
}

void TitleMenuController::SetVisible(bool v) {
	visible_ = v; // コマンド自体は常に更新するが、描画は見た目の状態に合わせる
}

bool TitleMenuController::TriggerPadUp_() {
	auto* in = TKM::Input::GetInstance();
	const bool cur =
		in->TriggerKey(DIK_UP) || // キーボード上
		in->TriggerKey(DIK_W) || // Wキー
		in->TriggerButton(XINPUT_GAMEPAD_DPAD_UP) || // 十字キー上
		(in->GetLeftStickY() > 16000);   // 左スティック上
	const bool trig = (cur && !prevUp_);
	prevUp_ = cur;
	return trig;
}

bool TitleMenuController::TriggerPadDown_() {
	auto* in = TKM::Input::GetInstance();
	const bool cur =
		in->TriggerKey(DIK_DOWN) || // キーボード下
		in->TriggerKey(DIK_S) || //　Sキー
		in->TriggerButton(XINPUT_GAMEPAD_DPAD_DOWN) || // 十字キー下
		(in->GetLeftStickY() < -16000);   // 左スティック下
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

	TKM::AudioManager::GetInstance()->PlaySound("cursor", 0.2f);
}