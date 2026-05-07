#include "IntroStartBanner.h"

namespace TKM {

	void IntroStartBanner::Initialize(DirectXCommon* dxCommon) {
		// スタートバナー用スプライトを生成する
		sprite_ = std::make_unique<Sprite>();

		// start.pngを使ってスプライトを初期化する
		sprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon, "./resources/texture/start.png");

		// 中心基準で配置・拡縮できるようにする
		sprite_->SetAnchorPoint({ 0.5f, 0.5f });

		// 初期位置を設定する
		sprite_->SetPosition({ startPos_.x, startPos_.y });

		// 初期サイズを設定する
		sprite_->SetSize({ 100, 100 });

		// 初期色を白・不透明に設定する
		sprite_->SetColor({ 1,1,1,1 });

		// 内部状態を初期化する
		Reset();
	}

	void IntroStartBanner::Reset() {
		// スライドイン状態を解除する
		slideIn_ = false;

		// 非表示状態にする
		visible_ = false;

		// 開始済みフラグを解除する
		started_ = false;

		// フェードアウト状態を解除する
		fadeOut_ = false;

		// 完了フラグを解除する
		finished_ = false;

		// 表示維持時間をリセットする
		holdElapsed_ = 0.0f;

		// 透明度を初期値に戻す
		alpha_ = 1.0f;

		// スプライトがある場合だけ見た目も初期状態へ戻す
		if (sprite_) {
			// 開始位置へ戻す
			sprite_->SetPosition({ startPos_.x, startPos_.y });

			// 白・不透明へ戻す
			sprite_->SetColor({ 1,1,1,1 });
		}
	}

	void IntroStartBanner::Start() {
		// すでに開始済みなら二重開始しない
		if (started_) { return; }

		// 開始済み状態にする
		started_ = true;

		// 表示状態にする
		visible_ = true;

		// スライドインを開始する
		slideIn_ = true;

		// フェードアウトはまだ行わない
		fadeOut_ = false;

		// 完了フラグを解除する
		finished_ = false;

		// 表示維持時間をリセットする
		holdElapsed_ = 0.0f;

		// 透明度を初期化する
		alpha_ = 1.0f;

		// 初期表示色を反映する
		sprite_->SetColor({ 1,1,1,alpha_ });

		// Y位置は最終位置、X位置は開始位置にして横から入ってくる形にする
		sprite_->SetPosition({ startPos_.x, endPos_.y });

		// スライドイン用トゥイーンを開始する
		tween_.Reset(0.0f, 1.0f, duration_, Ease::Type::OutBack);
	}

	void IntroStartBanner::Update(float dt) {
		// 非表示、またはスプライト未生成なら更新しない
		if (!visible_ || !sprite_) { return; }

		//=========================================================
		// スライドイン中の更新
		//=========================================================
		if (slideIn_) {
			// トゥイーンを進めて進行率を取得する
			float t = tween_.Update(dt);

			// グローONなら登場時に一瞬明るくする
			if (glowOn_) {
				// 進行率に応じた発光倍率を作る
				float glow = 1.0f + glowAmp_ * std::sin(t * MyMath::GetPI());

				// RGBを明るくしつつ透明度を反映する
				sprite_->SetColor({ glow, glow, glow, alpha_ });
			} else {
				// グローOFFなら通常色のままにする
				sprite_->SetColor({ 1,1,1,alpha_ });
			}

			// X位置を開始位置から終了位置へ補間する
			float x = MyMath::Lerp(startPos_.x, endPos_.x, t);

			// 補間した位置を反映する
			sprite_->SetPosition({ x, endPos_.y });

			// スプライトを更新する
			sprite_->Update();

			// スライドインが終わったら待機フェーズへ移る
			if (tween_.Finished()) {
				slideIn_ = false;
				holdElapsed_ = 0.0f;
			}
			return;
		}

		//=========================================================
		// 表示維持中のグロー更新
		//=========================================================
		if (!fadeOut_ && glowOn_) {
			// 表示維持時間に対する進行率を求める
			float t01 = (holdSec_ > 0.0f) ? std::min(holdElapsed_ / holdSec_, 1.0f) : 1.0f;

			// 時間経過でグローを弱める
			float decay = 1.0f - 0.7f * t01;

			// ゆっくり明滅するグロー値を作る
			float glow = 1.0f + decay * 0.20f * std::sin(holdElapsed_ * glowSpeed_);

			// グロー色を反映する
			sprite_->SetColor({ glow, glow, glow, alpha_ });
		}

		//=========================================================
		// 表示維持時間更新
		//=========================================================
		if (!fadeOut_) {
			// 表示維持時間を進める
			holdElapsed_ += dt;

			// 指定時間を超えたらフェードアウトへ移る
			if (holdElapsed_ >= holdSec_) {
				fadeOut_ = true;
			}
		}

		//=========================================================
		// フェードアウト更新
		//=========================================================
		if (fadeOut_) {
			// フェード時間に応じて透明度を下げる
			alpha_ -= dt / fadeSec_;

			// 完全に透明になったら非表示・完了扱いにする
			if (alpha_ <= 0.0f) {
				alpha_ = 0.0f;
				visible_ = false;
				finished_ = true;
			}

			// 透明度をスプライトへ反映する
			sprite_->SetColor({ 1,1,1,alpha_ });
		}

		// スプライトを更新する
		sprite_->Update();
	}

	void IntroStartBanner::Draw() const {
		// 表示中かつスプライトがある場合だけ描画する
		if (visible_ && sprite_) {
			sprite_->Draw();
		}
	}

}