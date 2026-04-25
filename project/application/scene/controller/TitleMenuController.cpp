#include "TitleMenuController.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "AudioManager.h"

void TitleMenuController::Initialize(TKM::SpriteCommon* spriteCommon, TKM::DirectXCommon* dxCommon, TKM::BaseScene* parentScene, float screenW, float screenH, const Desc& desc) {
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

	// タイトルメニュー設定を保持する
	desc_ = desc;

	//=========================================================
	// 項目スプライト生成
	//=========================================================
	for (int i = 0; i < (int)Item::Count; ++i) {
		// メニュー項目スプライトを生成する
		items_[i] = std::make_unique<TKM::Sprite>();

		// 項目ごとのテクスチャで初期化する
		items_[i]->Initialize(spriteCommon_, dxCommon_, desc_.itemTex[i]);

		// サイズはコード側で指定するため自動調整を切る
		items_[i]->SetAutoAdjustTextureSize(false);

		// 中心基準で配置・拡縮できるようにする
		items_[i]->SetAnchorPoint({ 0.5f, 0.5f });

		{
			// テクスチャのメタ情報を取得する
			const auto& md = TKM::TextureManager::GetInstance()->GetMetadata(desc_.itemTex[i]);

			// テクスチャ左上を原点にする
			items_[i]->SetTextureLeftTop({ 0.0f, 0.0f });

			// テクスチャ全体を使用する
			items_[i]->SetTextureSize({ (float)md.width, (float)md.height });
		}

		// 初期状態では未選択として少し薄く表示する
		items_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.65f });
	}

	//=========================================================
	// 内部状態初期化
	//=========================================================
	for (int i = 0; i < (int)Item::Count; ++i) {
		// 各項目の初期スケールを等倍にする
		itemScale_[i] = 1.0f;
	}

	// 画面サイズに合わせて初期レイアウトを反映する
	UpdateLayout(screenW_, screenH_);
}

TitleMenuController::Command TitleMenuController::Update(float dt) {
	// 非表示中は入力も見た目も更新しない
	if (!visible_) { return Command::None; }

	//=========================================================
	// 入力更新
	//=========================================================

	// 上入力で選択項目を上へ移動する
	if (TriggerPadUp_()) {
		MoveIndex_(-1);
	}

	// 下入力で選択項目を下へ移動する
	if (TriggerPadDown_()) {
		MoveIndex_(+1);
	}

	//=========================================================
	// 見た目更新
	//=========================================================

	// 選択項目の脈動時間を進める
	pulseTime_ += dt;

	// 選択項目に使う脈動倍率を作る
	float pulse = 1.0f + 0.06f * std::sin(pulseTime_ * 6.0f);

	// 各項目を更新する
	for (int i = 0; i < (int)Item::Count; ++i) {
		// 項目スプライトが無ければ飛ばす
		if (!items_[i]) { continue; }

		// この項目が現在選択中かどうか
		bool selected = (i == index_);

		// 項目位置を計算する
		Vector2 pos = {
			baseItemPos_.x,
			baseItemPos_.y + itemSpacingY_ * (float)i
		};

		// 項目位置を反映する
		items_[i]->SetPosition(pos);

		// 選択中は濃く、未選択は薄く表示する
		Vector4 col = selected ? Vector4{ 1.0f,1.0f,1.0f,0.92f } : Vector4{ 1.0f,1.0f,1.0f,0.62f };

		// 項目色を反映する
		items_[i]->SetColor(col);

		// テクスチャのメタ情報を取得する
		const auto& md = TKM::TextureManager::GetInstance()->GetMetadata(desc_.itemTex[i]);

		// 選択中は少し大きく、未選択は小さめの高さにする
		float baseHeight = selected ? 72.0f : 64.0f;

		// 選択中だけ脈動倍率をかける
		if (selected) {
			baseHeight *= pulse;
		}

		// テクスチャのアスペクト比を初期化する
		float aspect = 1.0f;

		// 高さが有効ならアスペクト比を計算する
		if (md.height > 0) {
			aspect = (float)md.width / (float)md.height;
		}

		// アスペクト比を保ったサイズを作る
		Vector2 size = {
			baseHeight * aspect,
			baseHeight
		};

		// 項目サイズを反映する
		items_[i]->SetSize(size);

		// 項目スプライトを更新する
		items_[i]->Update();
	}

	//=========================================================
	// 決定入力
	//=========================================================

	// 決定入力があれば現在の選択に応じたコマンドを返す
	if (TriggerA_()) {
		// 決定SEを再生する
		TKM::AudioManager::GetInstance()->PlaySound("decision", 0.1f);

		// Start項目ならStart、それ以外ならExitを返す
		return (index_ == (int)Item::Start) ? Command::Start : Command::Exit;
	}

	// 何も決定されていなければNoneを返す
	return Command::None;
}

void TitleMenuController::Draw() {
	// 非表示中は描画しない
	if (!visible_) { return; }

	// 各項目を描画する
	for (int i = 0; i < (int)Item::Count; ++i) {
		items_[i]->Draw();
	}
}

void TitleMenuController::UpdateLayout(float screenW, float screenH) {
	// 画面幅を更新する
	screenW_ = screenW;

	// 画面高さを更新する
	screenH_ = screenH;

	// パネルサイズを設定する
	panelSize_ = { 340.0f, 200.0f };

	// パネル位置を画面中央下あたりに設定する
	panelPos_ = { 470.0f, 500.0f };

	// 項目群の基準位置を設定する
	baseItemPos_ = { panelPos_.x + panelSize_.x * 0.5f, panelPos_.y + 72.0f };

	// 項目同士の縦間隔を設定する
	itemSpacingY_ = 70.0f;

	// 各項目の位置をレイアウトに合わせて更新する
	for (int i = 0; i < (int)Item::Count; ++i) {
		// 項目位置を計算する
		Vector2 p = { baseItemPos_.x, baseItemPos_.y + itemSpacingY_ * (float)i };

		// 項目位置を反映する
		items_[i]->SetPosition(p);
	}
}

void TitleMenuController::SetVisible(bool v) {
	// 表示フラグを設定する
	visible_ = v;
}

bool TitleMenuController::TriggerPadUp_() {
	// 入力管理を取得する
	auto* in = TKM::Input::GetInstance();

	// 上方向入力が入っているか判定する
	const bool cur =
		in->TriggerKey(DIK_UP) ||
		in->TriggerKey(DIK_W) ||
		in->TriggerButton(XINPUT_GAMEPAD_DPAD_UP) ||
		(in->GetLeftStickY() > 16000);

	// 前フレームでは押されておらず、今回押された場合だけトリガーにする
	const bool trig = (cur && !prevUp_);

	// 今回の入力状態を保存する
	prevUp_ = cur;

	// トリガー結果を返す
	return trig;
}

bool TitleMenuController::TriggerPadDown_() {
	// 入力管理を取得する
	auto* in = TKM::Input::GetInstance();

	// 下方向入力が入っているか判定する
	const bool cur =
		in->TriggerKey(DIK_DOWN) ||
		in->TriggerKey(DIK_S) ||
		in->TriggerButton(XINPUT_GAMEPAD_DPAD_DOWN) ||
		(in->GetLeftStickY() < -16000);

	// 前フレームでは押されておらず、今回押された場合だけトリガーにする
	const bool trig = (cur && !prevDown_);

	// 今回の入力状態を保存する
	prevDown_ = cur;

	// トリガー結果を返す
	return trig;
}

bool TitleMenuController::TriggerA_() {
	// 入力管理を取得する
	auto* in = TKM::Input::GetInstance();

	// 決定入力が入っているか判定する
	const bool cur =
		in->TriggerKey(DIK_SPACE) ||
		in->TriggerKey(DIK_RETURN) ||
		in->TriggerButton(XINPUT_GAMEPAD_A);

	// 前フレームでは押されておらず、今回押された場合だけトリガーにする
	const bool trig = (cur && !prevA_);

	// 今回の入力状態を保存する
	prevA_ = cur;

	// トリガー結果を返す
	return trig;
}

void TitleMenuController::MoveIndex_(int delta) {
	// 項目数を取得する
	const int count = (int)Item::Count;

	// 範囲外に出たらループするように選択番号を更新する
	index_ = (index_ + delta + count) % count;

	// カーソル移動SEを再生する
	TKM::AudioManager::GetInstance()->PlaySound("cursor", 0.2f);
}