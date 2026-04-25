#include "GameResultMenuController.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "TextureManager.h"
#include "AudioCatalog.h"

float GameResultMenuController::Clamp01_(float v) {
	// 0未満なら0に丸める
	if (v < 0.0f) return 0.0f;

	// 1より大きければ1に丸める
	if (v > 1.0f) return 1.0f;

	// 0〜1の範囲内ならそのまま返す
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
	// スプライト共通情報を保持する
	spriteCommon_ = spriteCommon;

	// DirectX共通情報を保持する
	dxCommon_ = dxCommon;

	// 親シーンを保持する
	parentScene_ = parentScene;

	// 画面幅を保持する
	screenW_ = screenW;

	// 画面高さを保持する
	screenH_ = screenH;

	// メニュー設定を保持する
	desc_ = desc;

	//=========================================================
	// メニュー項目生成
	//=========================================================
	for (int i = 0; i < (int)Item::Count; ++i) {
		// 項目用スプライトを生成する
		items_[i] = std::make_unique<TKM::Sprite>();

		// 項目ごとのテクスチャで初期化する
		items_[i]->Initialize(spriteCommon_, dxCommon_, desc_.itemTex[i]);

		// 親シーンを設定して描画順を合わせる
		items_[i]->SetParentScene(parentScene_);

		// 中心基準で配置・拡縮できるようにする
		items_[i]->SetAnchorPoint({ 0.5f, 0.5f });

		// サイズはコード側で指定するため自動調整を切る
		items_[i]->SetAutoAdjustTextureSize(false);

		{
			// テクスチャのメタ情報を取得する
			const auto& md = TKM::TextureManager::GetInstance()->GetMetadata(desc_.itemTex[i]);

			// テクスチャ左上を原点にする
			items_[i]->SetTextureLeftTop({ 0.0f, 0.0f });

			// テクスチャ全体を使用する
			items_[i]->SetTextureSize({ (float)md.width, (float)md.height });
		}

		// 初期状態では未選択色にする
		items_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.65f });
	}

	//=========================================================
	// カーソル生成
	//=========================================================

	// カーソル用スプライトを生成する
	cursor_ = std::make_unique<TKM::Sprite>();

	// カーソルテクスチャで初期化する
	cursor_->Initialize(spriteCommon_, dxCommon_, desc_.cursorTex);

	// 親シーンを設定して描画順を合わせる
	cursor_->SetParentScene(parentScene_);

	// 項目の左側に置くため左上基準で扱う
	cursor_->SetAnchorPoint({ 0.0f, 0.0f });

	// サイズはコード側で指定するため自動調整を切る
	cursor_->SetAutoAdjustTextureSize(false);

	// カーソルサイズを固定する
	cursor_->SetSize({ 28.0f, 28.0f });

	// カーソル色を設定する
	cursor_->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f });

	//=========================================================
	// 内部状態初期化
	//=========================================================

	// 選択項目のパルス用時間を初期化する
	pulseTime_ = 0.0f;

	// 初期選択を先頭にする
	index_ = 0;

	// 画面サイズに合わせて配置を更新する
	UpdateLayout(screenW_, screenH_);
}

GameResultMenuController::Command GameResultMenuController::Update(float dt) {
	// 上入力があれば選択を1つ上へ移動する
	if (TriggerPadUp_()) {
		MoveIndex_(-1);
	}

	// 下入力があれば選択を1つ下へ移動する
	if (TriggerPadDown_()) {
		MoveIndex_(+1);
	}

	// 決定入力があれば現在選択中のコマンドを返す
	if (TriggerA_()) {
		// 決定SEを再生する
		TKM::AudioManager::GetInstance()->PlaySound("decision", 0.2f);

		//=========================================================
		// 決定時の波紋演出
		//=========================================================
		if (dxCommon_) {
			// 水面波紋エフェクトを取得する
			auto ripple = dxCommon_->GetWaterRippleEffect();

			// 波紋エフェクトがあれば画面中央に発生させる
			if (ripple) {
				TKM::WaterRippleEffect::RippleDesc desc{};

				// 波紋の継続時間を設定する
				desc.duration_ = 0.6f;

				// 波紋の最大半径を設定する
				desc.radiusMax_ = 0.85f;

				// 波紋の振幅を設定する
				desc.amplitude_ = 0.1f;

				// 波紋の細かさを設定する
				desc.frequency_ = 80.0f;

				// 波紋の太さを設定する
				desc.width_ = 10.0f;

				// 波紋色を白に設定する
				desc.color_ = { 1.0f,1.0f,1.0f };

				// 画面中央に波紋を発生させる
				ripple->Trigger({ 0.5f, 0.5f }, desc);
			}
		}

		// Restart項目ならリスタートコマンドを返す
		if (index_ == (int)Item::Restart) {
			return Command::Restart;
		}

		// それ以外はタイトルへ戻るコマンドを返す
		return Command::ReturnToTitle;
	}

	//=========================================================
	// 見た目更新
	//=========================================================

	// パルス時間を進める
	pulseTime_ += dt;

	// 選択項目の拡縮に使うパルス倍率を作る
	float pulse = 1.0f + 0.06f * std::sin(pulseTime_ * 6.0f);

	//=========================================================
	// 項目スプライト更新
	//=========================================================
	for (int i = 0; i < (int)Item::Count; ++i) {
		// 項目スプライトが無ければ飛ばす
		if (!items_[i]) { continue; }

		// 現在選択中かどうか
		bool selected = (i == index_);

		// 項目の表示位置を計算する
		Vector2 pos = {
			baseItemPos_.x,
			baseItemPos_.y + itemSpacingY_ * (float)i
		};

		// 選択中なら明るく、未選択なら少し暗くする
		Vector4 col = selected ? Vector4{ 1.0f, 1.0f, 1.0f, 0.92f } : Vector4{ 1.0f, 1.0f, 1.0f, 0.62f };

		// 色を反映する
		items_[i]->SetColor(col);

		// 選択中なら少し大きい基準サイズにする
		Vector2 baseSize = selected ? Vector2{ 240.0f, 48.0f } : Vector2{ 220.0f, 44.0f };

		// 選択中ならパルス拡縮を反映する
		Vector2 size = selected ? Vector2{ baseSize.x * pulse, baseSize.y * pulse } : baseSize;

		// 位置を反映する
		items_[i]->SetPosition(pos);

		// サイズを反映する
		items_[i]->SetSize(size);

		// スプライトを更新する
		items_[i]->Update();
	}

	//=========================================================
	// カーソル更新
	//=========================================================
	if (cursor_) {
		// 選択項目の左側にカーソル位置を合わせる
		Vector2 pos = { baseItemPos_.x - 150.0f, baseItemPos_.y + itemSpacingY_ * (float)index_ };

		// カーソル位置を反映する
		cursor_->SetPosition(pos);

		// カーソルサイズを固定する
		cursor_->SetSize({ 28.0f, 28.0f });

		// カーソル色を反映する
		cursor_->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f });

		// カーソルを更新する
		cursor_->Update();
	}

	// 何も決定されていなければNoneを返す
	return Command::None;
}

void GameResultMenuController::Draw() {
	// メニュー項目を描画する
	for (int i = 0; i < (int)Item::Count; ++i) {
		if (items_[i]) items_[i]->Draw();
	}

	// カーソルを描画する
	if (cursor_) cursor_->Draw();
}

void GameResultMenuController::UpdateLayout(float screenW, float screenH) {
	// 画面幅を更新する
	screenW_ = screenW;

	// 画面高さを更新する
	screenH_ = screenH;

	// パネルサイズを設定する
	panelSize_ = { 340.0f, 200.0f };

	// 右下に余白40で配置する
	panelPos_ = { screenW_ - panelSize_.x - 40.0f, screenH_ - panelSize_.y - 40.0f };

	// 項目群の基準位置を設定する
	baseItemPos_ = { panelPos_.x + panelSize_.x * 0.5f, panelPos_.y + 60.0f + 22.0f };

	// 項目同士の縦間隔を設定する
	itemSpacingY_ = 64.0f;
}

bool GameResultMenuController::TriggerPadUp_() {
	// 入力管理を取得する
	auto* in = TKM::Input::GetInstance();

	// 上方向入力が入っているか判定する
	const bool cur =
		in->TriggerKey(DIK_W) ||
		in->TriggerKey(DIK_UP) ||
		in->TriggerButton(XINPUT_GAMEPAD_DPAD_UP) ||
		(in->GetLeftStickY() > 16000);

	// 前フレームでは押されておらず、今回押された場合だけトリガーにする
	const bool trig = (cur && !prevUp_);

	// 今回の入力状態を保存する
	prevUp_ = cur;

	// トリガー結果を返す
	return trig;
}

bool GameResultMenuController::TriggerPadDown_() {
	// 入力管理を取得する
	auto* in = TKM::Input::GetInstance();

	// 下方向入力が入っているか判定する
	const bool cur =
		in->TriggerKey(DIK_S) ||
		in->TriggerKey(DIK_DOWN) ||
		in->TriggerButton(XINPUT_GAMEPAD_DPAD_DOWN) ||
		(in->GetLeftStickY() < -16000);

	// 前フレームでは押されておらず、今回押された場合だけトリガーにする
	const bool trig = (cur && !prevDown_);

	// 今回の入力状態を保存する
	prevDown_ = cur;

	// トリガー結果を返す
	return trig;
}

bool GameResultMenuController::TriggerA_() {
	// 入力管理を取得する
	auto* in = TKM::Input::GetInstance();

	// 決定入力が入っているか判定する
	const bool cur =
		in->PushButton(XINPUT_GAMEPAD_A) ||
		in->TriggerKey(DIK_SPACE) ||
		in->TriggerKey(DIK_RETURN);

	// 前フレームでは押されておらず、今回押された場合だけトリガーにする
	const bool trig = (cur && !prevA_);

	// 今回の入力状態を保存する
	prevA_ = cur;

	// トリガー結果を返す
	return trig;
}

void GameResultMenuController::MoveIndex_(int delta) {
	// 項目数を取得する
	const int count = (int)Item::Count;

	// 範囲外に出たらループするように選択番号を更新する
	index_ = (index_ + delta + count) % count;

	// カーソル移動SEを再生する
	TKM::AudioManager::GetInstance()->PlaySound("cursor", 0.3f);
}