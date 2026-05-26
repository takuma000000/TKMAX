#define NOMINMAX
#include "PauseMenuController.h"
#include "AudioManager.h"
#include "TextureManager.h"
#include <algorithm>
#include <cmath>

namespace TKM {
	void PauseMenuController::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH, const Desc& desc) {
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

		// ポーズメニュー設定を保持する
		desc_ = desc;

		//=========================================================
		// 状態初期化
		//=========================================================

		// 初期状態は閉じた状態にする
		state_ = State::Closed;

		// 初期選択項目を先頭にする
		index_ = 0;

		// フェード進行率を初期化する
		fadeT_ = 0.0f;

		// 選択項目の脈動用タイマーを初期化する
		pulseTime_ = 0.0f;

		// 暗幕の透明度を初期化する
		curtainAlpha_ = 0.0f;

		//=========================================================
		// 入力状態初期化
		//=========================================================

		// START入力の前回状態を初期化する
		prevStart_ = false;

		// 上入力の前回状態を初期化する
		prevUp_ = false;

		// 下入力の前回状態を初期化する
		prevDown_ = false;

		// 決定入力の前回状態を初期化する
		prevA_ = false;

		// キャンセル入力の前回状態を初期化する
		prevB_ = false;

		//=========================================================
		// 暗幕生成
		//=========================================================

		// 背景を暗くする暗幕スプライトを生成する
		curtain_ = std::make_unique<Sprite>();

		// 暗幕テクスチャで初期化する
		curtain_->Initialize(spriteCommon_, dxCommon_, desc_.curtainTex);

		// 親シーンを設定して描画順を合わせる
		curtain_->SetParentScene(parentScene_);

		// 左上基準で画面全体に広げる
		curtain_->SetAnchorPoint({ 0.0f, 0.0f });

		// サイズはコード側で画面サイズに合わせるため自動調整を切る
		curtain_->SetAutoAdjustTextureSize(false);

		// 画面左上に配置する
		curtain_->SetPosition({ 0.0f, 0.0f });

		// 画面全体を覆うサイズにする
		curtain_->SetSize({ 1280.0f, 720.0f });

		{
			// テクスチャのメタ情報を取得する
			const auto& md = TextureManager::GetInstance()->GetMetadata(desc_.curtainTex);

			// テクスチャ左上を原点にする
			curtain_->SetTextureLeftTop({ 0.0f, 0.0f });

			// テクスチャ全体を使用する
			curtain_->SetTextureSize({ (float)md.width, (float)md.height });
		}

		// 初期状態では透明にする
		curtain_->SetColor({ 0.8f, 0.8f, 1.0f, 1.0f });

		//=========================================================
		// 項目生成
		//=========================================================

		for (int i = 0; i < (int)Item::Count; ++i) {
			// メニュー項目スプライトを生成する
			items_[i] = std::make_unique<Sprite>();

			// 項目ごとのテクスチャで初期化する
			items_[i]->Initialize(spriteCommon_, dxCommon_, desc_.itemTex[i]);

			// 親シーンを設定して描画順を合わせる
			items_[i]->SetParentScene(parentScene_);

			// 中心基準で選択時の拡縮をしやすくする
			items_[i]->SetAnchorPoint({ 0.5f, 0.5f });

			// サイズはコード側で指定するため自動調整を切る
			items_[i]->SetAutoAdjustTextureSize(false);

			// 初期サイズを設定する
			items_[i]->SetSize({ 220.0f, 85.0f });

			{
				// テクスチャのメタ情報を取得する
				const auto& md = TextureManager::GetInstance()->GetMetadata(desc_.itemTex[i]);

				// テクスチャ左上を原点にする
				items_[i]->SetTextureLeftTop({ 0.0f,0.0f });

				// テクスチャ全体を使用する
				items_[i]->SetTextureSize({ (float)md.width, (float)md.height });
			}

			// 初期状態では未選択として少し透明にする
			items_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.65f });
		}

		//=========================================================
		// 初期レイアウト反映
		//=========================================================

		// 画面サイズに合わせて初期レイアウトを反映する
		UpdateLayout(screenW_, screenH_);
	}

	void PauseMenuController::UpdateLayout(float screenW, float screenH) {
		// 画面幅を更新する
		screenW_ = screenW;

		// 画面高さを更新する
		screenH_ = screenH;

		// パネルサイズを設定する
		panelSize_ = { 340.0f, 280.0f };

		// パネルを画面右下に余白40で配置する
		panelPos_ = { screenW_ - panelSize_.x - 40.0f, screenH_ - panelSize_.y - 40.0f };

		// 項目群の基準位置をパネル内に設定する
		baseItemPos_ = { panelPos_.x + panelSize_.x * 0.5f, panelPos_.y + 94.0f };

		// 項目同士の縦間隔を設定する
		itemSpacingY_ = 82.0f;

		// 暗幕がある場合は画面全体を覆うサイズへ更新する
		if (curtain_) {
			curtain_->SetSize({ 1280.0f, 720.0f });
		}

		// 各項目を基準位置から縦に並べる
		for (int i = 0; i < (int)Item::Count; ++i) {
			if (items_[i]) {
				items_[i]->SetPosition({ baseItemPos_.x, baseItemPos_.y + itemSpacingY_ * (float)i });
			}
		}
	}

	bool PauseMenuController::TriggerPadUp_() {
		// 入力管理を取得する
		Input* in = Input::GetInstance();

		// 上方向入力が入っているか判定する
		bool now =
			in->PushButton(XINPUT_GAMEPAD_DPAD_UP) ||
			in->TriggerKey(DIK_W) ||
			in->TriggerKey(DIK_UP) ||
			(in->GetLeftStickY() > 16000);

		// 前フレームでは押されておらず、今回押された場合だけトリガーにする
		bool trig = (now && !prevUp_);

		// 今回の入力状態を保存する
		prevUp_ = now;

		// トリガー結果を返す
		return trig;
	}

	bool PauseMenuController::TriggerPadDown_() {
		// 入力管理を取得する
		Input* in = Input::GetInstance();

		// 下方向入力が入っているか判定する
		bool now =
			in->PushButton(XINPUT_GAMEPAD_DPAD_DOWN) ||
			in->TriggerKey(DIK_S) ||
			in->TriggerKey(DIK_DOWN) ||
			(in->GetLeftStickY() < -16000);

		// 前フレームでは押されておらず、今回押された場合だけトリガーにする
		bool trig = (now && !prevDown_);

		// 今回の入力状態を保存する
		prevDown_ = now;

		// トリガー結果を返す
		return trig;
	}

	bool PauseMenuController::TriggerA_() {
		// 入力管理を取得する
		Input* in = Input::GetInstance();

		// 決定入力が入っているか判定する
		bool now = in->PushButton(XINPUT_GAMEPAD_A) || in->TriggerKey(DIK_SPACE) || in->TriggerKey(DIK_RETURN);

		// 前フレームでは押されておらず、今回押された場合だけトリガーにする
		bool trig = (now && !prevA_);

		// 今回の入力状態を保存する
		prevA_ = now;

		// トリガー結果を返す
		return trig;
	}

	bool PauseMenuController::TriggerB_() {
		// 入力管理を取得する
		Input* in = Input::GetInstance();

		// キャンセル入力が入っているか判定する
		bool now = in->PushButton(XINPUT_GAMEPAD_B) || in->PushButton(XINPUT_GAMEPAD_START) || in->TriggerKey(DIK_TAB) || in->TriggerKey(DIK_ESCAPE);

		// 前フレームでは押されておらず、今回押された場合だけトリガーにする
		bool trig = (now && !prevB_);

		// 今回の入力状態を保存する
		prevB_ = now;

		// トリガー結果を返す
		return trig;
	}

	void PauseMenuController::Open_() {
		// ポーズ開始状態へ切り替える
		state_ = State::Pausing;

		// フェード進行率をリセットする
		fadeT_ = 0.0f;

		// 選択項目の脈動タイマーをリセットする
		pulseTime_ = 0.0f;

		// 初期選択を先頭に戻す
		index_ = 0;
	}

	void PauseMenuController::Close_() {
		// 再開中状態へ切り替える
		state_ = State::Resuming;

		// 閉じるフェード進行率をリセットする
		fadeT_ = 0.0f;
	}

	void PauseMenuController::MoveIndex_(int delta) {
		// 項目数を取得する
		int count = (int)Item::Count;

		// 範囲外に出たらループするように選択番号を更新する
		index_ = (index_ + delta + count) % count;

		// カーソル移動SEを再生する
		TKM::AudioManager::GetInstance()->PlaySound("cursor", 0.3f);
	}

	PauseMenuController::Command PauseMenuController::Update(float dt, bool allowOpen) {
		//=========================================================
		// ポーズ開閉入力
		//=========================================================

		// 入力管理を取得する
		Input* in = Input::GetInstance();

		// START/TAB/ESC のポーズ入力を取得する
		bool startNow = in->PushButton(XINPUT_GAMEPAD_START) || in->TriggerKey(DIK_TAB) || in->TriggerKey(DIK_ESCAPE);

		// 前フレームでは押されておらず、今回押された場合だけ開始トリガーにする
		bool trigStart = (startNow && !prevStart_);

		// 今回のSTART入力状態を保存する
		prevStart_ = startNow;

		// 0.0〜1.0へ丸めるラムダ
		auto clamp01 = [](float v) {
			return std::max(0.0f, std::min(v, 1.0f));
			};

		// SmoothStepで出現・消失の動きをなめらかにするラムダ
		auto smoothStep01 = [&](float t) {
			t = clamp01(t);
			return t * t * (3.0f - 2.0f * t);
			};

		// 暗幕の最大透明度
		constexpr float kTargetCurtainAlpha = 0.7f;

		// 開く速度
		constexpr float kOpenSpeed = 8.0f;

		// 閉じる速度
		constexpr float kCloseSpeed = 10.0f;

		//=========================================================
		// 閉じている状態
		//=========================================================
		if (state_ == State::Closed) {
			// 開ける状態で開始入力があればポーズを開く
			if (allowOpen && trigStart) {
				Open_();
				TKM::AudioManager::GetInstance()->PlaySound("pause", 0.4f);
			}
			return Command::None;
		}

		//=========================================================
		// フェード状態更新
		//=========================================================

		if (state_ == State::Pausing) {
			// 開く方向へフェード進行率を進める
			fadeT_ = clamp01(fadeT_ + dt * kOpenSpeed);

			// なめらかな進行率に変換する
			float e = smoothStep01(fadeT_);

			// 暗幕透明度を上げる
			curtainAlpha_ = kTargetCurtainAlpha * e;

			// 開き切ったらPaused状態にする
			if (fadeT_ >= 1.0f) {
				state_ = State::Paused;
				curtainAlpha_ = kTargetCurtainAlpha;
			}
		} else if (state_ == State::Resuming) {
			// 閉じる方向へフェード進行率を進める
			fadeT_ = clamp01(fadeT_ + dt * kCloseSpeed);

			// なめらかな進行率に変換する
			float e = smoothStep01(fadeT_);

			// 暗幕透明度を下げる
			curtainAlpha_ = kTargetCurtainAlpha * (1.0f - e);

			// 閉じ切ったらClosed状態に戻す
			if (fadeT_ >= 1.0f) {
				state_ = State::Closed;
				curtainAlpha_ = 0.0f;
				return Command::None;
			}
		} else {
			// 完全にポーズ中なら暗幕透明度を固定する
			curtainAlpha_ = kTargetCurtainAlpha;
		}

		//=========================================================
		// ポーズ中の操作
		//=========================================================

		if (state_ == State::Paused) {
			// 上入力で選択項目を上へ移動する
			if (TriggerPadUp_()) { MoveIndex_(-1); }

			// 下入力で選択項目を下へ移動する
			if (TriggerPadDown_()) { MoveIndex_(+1); }

			// キャンセル入力でポーズを閉じる
			if (TriggerB_()) {
				TKM::AudioManager::GetInstance()->PlaySound("pause", 0.4f);

				Close_();
				return Command::None;
			}

			// 決定入力で選択中の項目を実行する
			if (TriggerA_()) {
				// 決定SEを再生する
				TKM::AudioManager::GetInstance()->PlaySound("decision", 0.2f);

				//=================================================
				// 決定時の波紋演出
				//=================================================
				if (dxCommon_) {
					// 水面波紋エフェクトを取得する
					auto* ripple = dxCommon_->GetWaterRippleEffect();

					// 波紋エフェクトが有効なら画面中央に発生させる
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

						// 波紋の幅を設定する
						desc.width_ = 10.0f;

						// 波紋色を白にする
						desc.color_ = { 1.0f,1.0f,1.0f };

						// 画面中央に波紋を出す
						ripple->Trigger({ 0.5f, 0.5f }, desc);
					}
				}

				// Resumeが選ばれていたらポーズを閉じて再開する
				if (index_ == (int)Item::Resume) {
					Close_();
					return Command::Resume;
				}

				// Restartが選ばれていたらリスタート要求を返す
				if (index_ == (int)Item::Restart) {
					Close_();
					return Command::Restart;
				}

				// ReturnToTitleが選ばれていたらタイトル遷移要求を返す
				if (index_ == (int)Item::ReturnToTitle) {
					Close_();
					return Command::ReturnToTitle;
				}
			}
		}

		//=========================================================
		// 見た目更新
		//=========================================================

		// 選択項目の脈動時間を進める
		pulseTime_ += dt;

		// UIの開き具合を0.0〜1.0で作る
		float uiOpen = 0.0f;
		if (state_ == State::Pausing) {
			uiOpen = smoothStep01(fadeT_);
		} else if (state_ == State::Paused) {
			uiOpen = 1.0f;
		} else if (state_ == State::Resuming) {
			uiOpen = 1.0f - smoothStep01(fadeT_);
		}

		//=========================================================
		// 暗幕更新
		//=========================================================
		if (curtain_) {
			// 現在の暗幕透明度を反映する
			curtain_->SetColor({ 1.0f, 1.0f, 1.0f, curtainAlpha_ });

			// 暗幕を更新する
			curtain_->Update();
		}

		//=========================================================
		// 項目更新
		//=========================================================

		// 開き中は脈動を控えめにする
		float pulseBlend = (uiOpen >= 0.95f) ? 1.0f : uiOpen;

		// 選択項目用の脈動倍率を作る
		float pulse = 1.0f + (0.06f * pulseBlend) * std::sin(pulseTime_ * 6.0f);

		// 各項目を順番にフェードインさせる
		for (int i = 0; i < (int)Item::Count; ++i) {
			// 項目スプライトが無ければ飛ばす
			if (!items_[i]) { continue; }

			// 項目ごとの出現遅延
			const float delay = 0.08f * (float)i;

			// 項目の出現率を計算する
			float itemT = clamp01((uiOpen - delay) / 0.70f);

			// 出現率をなめらかにする
			itemT = smoothStep01(itemT);

			// この項目が選択中かどうか
			bool selected = (i == index_);

			// 少し下から上がってくる位置を計算する
			Vector2 itemPos = {
				baseItemPos_.x,
				baseItemPos_.y + itemSpacingY_ * (float)i + (1.0f - itemT) * 10.0f
			};

			// 項目位置を反映する
			items_[i]->SetPosition(itemPos);

			// 選択中は明るく、未選択は少し暗くする
			Vector4 col = selected ? Vector4{ 1.0f, 1.0f, 1.0f, 0.92f } : Vector4{ 1.0f, 1.0f, 1.0f, 0.62f };

			// 出現率に応じて透明度を変える
			col.w *= itemT;

			// 項目色を反映する
			items_[i]->SetColor(col);

			// 選択中は大きめ、未選択は少し小さめの基本サイズを作る
			Vector2 baseSize = selected ?
				Vector2{ 220.0f, 85.0f } :
				Vector2{ 200.0f, 77.0f };

			// 選択中なら脈動倍率を反映する
			Vector2 size = selected ? Vector2{ baseSize.x * pulse, baseSize.y * pulse } : baseSize;

			// 出現中は少し小さめから完成サイズへ近づける
			float s = 0.96f + 0.04f * itemT;
			size = { size.x * s, size.y * s };

			// 項目サイズを反映する
			items_[i]->SetSize(size);

			// 項目を更新する
			items_[i]->Update();
		}

		// このフレームではコマンドなし
		return Command::None;
	}

	void PauseMenuController::Draw() {
		// 閉じている場合は描画しない
		if (state_ == State::Closed) { return; }

		// 暗幕を描画する
		if (curtain_) curtain_->Draw();

		// 各項目を描画する
		for (int i = 0; i < (int)Item::Count; ++i) {
			if (items_[i]) items_[i]->Draw();
		}
	}
} // namespace TKM