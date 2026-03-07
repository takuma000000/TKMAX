#define NOMINMAX
#include "PauseMenuController.h"
#include "TextureManager.h"
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

		// 状態
		state_ = State::Closed;
		index_ = 0;
		fadeT_ = 0.0f;
		pulseTime_ = 0.0f;
		curtainAlpha_ = 0.0f;
		// 入力状態
		prevStart_ = false;
		prevUp_ = false;
		prevDown_ = false;
		prevA_ = false;
		prevB_ = false;

		// 暗幕
		curtain_ = std::make_unique<Sprite>();
		curtain_->Initialize(spriteCommon_, dxCommon_, desc_.curtainTex);
		curtain_->SetParentScene(parentScene_);
		curtain_->SetAnchorPoint({ 0.0f, 0.0f }); // 左上を基準にする
		curtain_->SetAutoAdjustTextureSize(false);
		curtain_->SetPosition({ 0.0f, 0.0f }); // 画面全体を覆うようにする
		curtain_->SetSize({ screenW_, screenH_ }); // 画面全体を覆うようにする
		curtain_->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f }); // 最初は透明にする

		// パネル
		panel_ = std::make_unique<Sprite>();
		panel_->Initialize(spriteCommon_, dxCommon_, desc_.panelTex);
		panel_->SetParentScene(parentScene_);
		panel_->SetAnchorPoint({ 0.0f, 0.0f }); // 左上を基準にする
		panel_->SetAutoAdjustTextureSize(false);

		// 項目
		for (int i = 0; i < (int)Item::Count; ++i) {
			items_[i] = std::make_unique<Sprite>();
			items_[i]->Initialize(spriteCommon_, dxCommon_, desc_.itemTex[i]);
			items_[i]->SetParentScene(parentScene_);
			items_[i]->SetAnchorPoint({ 0.5f, 0.5f }); // 中心を基準にする
			items_[i]->SetAutoAdjustTextureSize(false);
			items_[i]->SetSize({ 220.0f, 100.0f }); // サイズ設定
			{
				const auto& md = TextureManager::GetInstance()->GetMetadata(desc_.itemTex[i]);
				items_[i]->SetTextureLeftTop({ 0.0f,0.0f }); // テクスチャ全体を使う
				items_[i]->SetTextureSize({ (float)md.width, (float)md.height });
			}
			items_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.65f }); // 最初は少し透明にする
		}

		// カーソル
		cursor_ = std::make_unique<Sprite>();
		cursor_->Initialize(spriteCommon_, dxCommon_, desc_.cursorTex);
		cursor_->SetParentScene(parentScene_);
		cursor_->SetAnchorPoint({ 0.0f, 0.0f });
		cursor_->SetAutoAdjustTextureSize(false);
		cursor_->SetSize({ 28.0f, 28.0f }); // サイズ設定
		cursor_->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f }); // 最初は少し透明にする

		UpdateLayout(screenW_, screenH_); // レイアウト初期化
	}

	void PauseMenuController::UpdateLayout(float screenW, float screenH) {
		screenW_ = screenW;
		screenH_ = screenH;

		panelSize_ = { 340.0f, 280.0f }; // サイズ設定
		panelPos_ = { screenW_ - panelSize_.x - 40.0f, screenH_ - panelSize_.y - 40.0f }; // 画面右下に配置
	
		baseItemPos_ = { panelPos_.x + panelSize_.x * 0.5f, panelPos_.y + 94.0f }; // 項目の基準位置（パネル内の、最初の項目が来る位置）
		itemSpacingY_ = 70.0f; // 項目間の垂直スペース

		if (curtain_) {
			curtain_->SetSize({ screenW_, screenH_ }); // 画面全体を覆うようにする
		}
		if (panel_) {
			panel_->SetPosition(panelPos_); // 位置設定
			panel_->SetSize(panelSize_); // サイズ設定
			panel_->SetColor({ 0.08f, 0.08f, 0.10f, 0.75f }); // 半透明の暗い色にする
		}

		// 項目の位置を更新
		for (int i = 0; i < (int)Item::Count; ++i) {
			if (items_[i]) {
				items_[i]->SetPosition({ baseItemPos_.x, baseItemPos_.y + itemSpacingY_ * (float)i }); // 位置設定
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
		fadeT_ = 0.0f;
		pulseTime_ = 0.0f;
		index_ = 0;
	}

	void PauseMenuController::Close_() {
		state_ = State::Resuming;
		fadeT_ = 0.0f;
	}

	void PauseMenuController::MoveIndex_(int delta) {
		int count = (int)Item::Count;
		index_ = (index_ + delta + count) % count;
	}

	PauseMenuController::Command PauseMenuController::Update(float dt, bool allowOpen) {
		// Startで開く（static は使わず、インスタンスのメンバでエッジ検出）
		Input* in = Input::GetInstance();
		bool startNow = in->PushButton(XINPUT_GAMEPAD_START);
		bool trigStart = (startNow && !prevStart_);
		prevStart_ = startNow;

		auto clamp01 = [](float v) {
			return std::max(0.0f, std::min(v, 1.0f));
			};
		auto smoothStep01 = [&](float t) {
			t = clamp01(t);
			return t * t * (3.0f - 2.0f * t); // SmoothStep
			};

		constexpr float kTargetCurtainAlpha = 0.55f;
		constexpr float kOpenSpeed = 8.0f;
		constexpr float kCloseSpeed = 10.0f;

		if (state_ == State::Closed) {
			if (allowOpen && trigStart) {
				Open_();
			}
			return Command::None;
		}

		// フェード（収束 Lerp ではなく、0→1 の進行で「キレ」を出す）
		if (state_ == State::Pausing) {
			fadeT_ = clamp01(fadeT_ + dt * kOpenSpeed);
			float e = smoothStep01(fadeT_);
			curtainAlpha_ = kTargetCurtainAlpha * e;
			if (fadeT_ >= 1.0f) {
				state_ = State::Paused;
				curtainAlpha_ = kTargetCurtainAlpha;
			}
		} else if (state_ == State::Resuming) { // 開いている途中から閉じる途中へ（閉じる途中はフェードアウト）
			fadeT_ = clamp01(fadeT_ + dt * kCloseSpeed);
			float e = smoothStep01(fadeT_);
			curtainAlpha_ = kTargetCurtainAlpha * (1.0f - e);
			// 完全に閉じたら状態を Closed にして、透明にする
			if (fadeT_ >= 1.0f) {
				state_ = State::Closed;
				curtainAlpha_ = 0.0f;
				return Command::None;
			}
		} else { // Paused
			curtainAlpha_ = kTargetCurtainAlpha;
		}

		// 操作
		if (state_ == State::Paused) {
			// 上下で選択変更（ループ）
			if (TriggerPadUp_()) { MoveIndex_(-1); }
			// 下入力もループ
			if (TriggerPadDown_()) { MoveIndex_(+1); }
			// B で閉じる（Resume と同じ扱いで、項目選択は無し）
			if (TriggerB_()) {
				Close_();
				return Command::None;
			}
			// A で決定
			if (TriggerA_()) {
				// どの項目が選ばれているかでコマンドを返す
				if (index_ == (int)Item::Resume) {
					Close_();
					return Command::Resume;
				}
				// Restart は、選択して A 押した瞬間に即コマンドを返す（長押し無しで即確定）
				if (index_ == (int)Item::Restart) {
					Close_();
					return Command::Restart;
				}
				// ReturnToTitle は、選択して A 押した瞬間に即コマンドを返す（長押し無しで即確定）
				if (index_ == (int)Item::ReturnToTitle) {
					Close_();
					return Command::ReturnToTitle; // 長押し無しで即確定
				}
			}
		}

		// 見た目更新（開く瞬間の気持ちよさ：下からスッ + ちょいポン + 項目は順番に出す）
		pulseTime_ += dt;

		// UI の出現率（0:非表示 ～ 1:表示）
		float uiOpen = 0.0f;
		if (state_ == State::Pausing) {
			uiOpen = smoothStep01(fadeT_);
		} else if (state_ == State::Paused) {
			uiOpen = 1.0f;
		} else if (state_ == State::Resuming) {
			uiOpen = 1.0f - smoothStep01(fadeT_);
		}

		// 暗幕
		if (curtain_) {
			curtain_->SetColor({ 0.0f, 0.0f, 0.0f, curtainAlpha_ });
			curtain_->Update();
		}

		// パネル：下からスッ + 少しポン（サイズで表現）
		if (panel_) {
			const float t = smoothStep01(uiOpen);
			const float slideY = (1.0f - t) * 18.0f;

			// 0.92 -> 1.02 -> 1.00 くらいの小さなポン（やりすぎない）
			const float pi = 3.14159265f;
			float pop = std::sin(t * pi);                  // 0->1->0
			float scale = 0.92f + 0.08f * t + 0.02f * pop; // 0.92 -> 1.02 -> 1.00 付近

			Vector2 baseSize = panelSize_;
			Vector2 newSize = { baseSize.x * scale, baseSize.y * scale };

			// 中心固定（アンカー0,0なので位置を補正）
			Vector2 baseCenter = { panelPos_.x + baseSize.x * 0.5f, panelPos_.y + baseSize.y * 0.5f };
			Vector2 newPos = { baseCenter.x - newSize.x * 0.5f, baseCenter.y - newSize.y * 0.5f + slideY };

			panel_->SetPosition(newPos);
			panel_->SetSize(newSize);
			panel_->SetColor({ 0.08f, 0.08f, 0.10f, 0.75f * t });
			panel_->Update();
		}

		// 選択の脈動（開き中は控えめ、開き切ったら通常）
		float pulseBlend = (uiOpen >= 0.95f) ? 1.0f : uiOpen;
		float pulse = 1.0f + (0.06f * pulseBlend) * std::sin(pulseTime_ * 6.0f);

		// 項目：順番にフェードイン + 少し下からスッ
		for (int i = 0; i < (int)Item::Count; ++i) {
			if (!items_[i]) { continue; }

			// スタッガー（上から順に少し遅れて出る）
			const float delay = 0.08f * (float)i;
			float itemT = clamp01((uiOpen - delay) / 0.70f);
			itemT = smoothStep01(itemT);

			bool selected = (i == index_);

			// 位置：基準位置から、出現率に応じて少し下からスッと上がるようにする
			Vector2 itemPos = {
				baseItemPos_.x,
				baseItemPos_.y + itemSpacingY_ * (float)i + (1.0f - itemT) * 10.0f
			};
			items_[i]->SetPosition(itemPos);
			// 色：選択中は少し明るく、非選択は少し暗く（さらに出現率に応じて透明から不透明へ）
			Vector4 col = selected ? Vector4{ 1.0f, 1.0f, 1.0f, 0.92f } : Vector4{ 1.0f, 1.0f, 1.0f, 0.62f };
			col.w *= itemT;
			items_[i]->SetColor(col);

			Vector2 baseSize = selected ? Vector2{ 240.0f, 48.0f } : Vector2{ 220.0f, 44.0f };
			Vector2 size = selected ? Vector2{ baseSize.x * pulse, baseSize.y * pulse } : baseSize;
			// 出現中は少しだけ小さめ（ポン）にして完成感
			float s = 0.96f + 0.04f * itemT;
			size = { size.x * s, size.y * s };

			// サイズを設定
			items_[i]->SetSize(size);
			items_[i]->Update();
		}

		// カーソル：選択項目の出現率に追従してフェード
		if (cursor_) {
			const float delay = 0.08f * (float)index_;
			float itemT = clamp01((uiOpen - delay) / 0.70f);
			itemT = smoothStep01(itemT);

			// 位置：選択項目の左側に配置。選択項目と同じく、出現率に応じて少し下からスッと上がるようにする
			Vector2 pos = {
				baseItemPos_.x - 150.0f,
				baseItemPos_.y + itemSpacingY_ * (float)index_ + (1.0f - itemT) * 10.0f
			};
			// 色：選択項目と同じアルファで表示
			cursor_->SetPosition(pos);
			cursor_->SetSize({ 28.0f, 28.0f });
			cursor_->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f * itemT });
			cursor_->Update();
		}

		return Command::None; // 開いている間は特にコマンド無し
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