#define NOMINMAX
#include "LBGaugeUI.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace TKM {
	void LBGaugeUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc) {
		// スプライト共通情報を保持する
		spriteCommon_ = spriteCommon;

		// DirectX共通情報を保持する
		dxCommon_ = dxCommon;

		// 親シーンを保持する
		parentScene_ = parentScene;

		// レイアウトや色などの設定値を保持する
		desc_ = desc;

		//=========================================================
		// フレームスプライト生成
		//=========================================================

		// フレーム用スプライトを生成する
		frame_ = std::make_unique<Sprite>();

		// テクスチャを使って初期化する
		frame_->Initialize(spriteCommon_, dxCommon_, desc_.frameTex_);

		// 描画順をそろえるため親シーンを設定する
		frame_->SetParentScene(parentScene_);

		// 左上基準で扱う
		frame_->SetAnchorPoint({ 0.0f, 0.0f });

		// サイズは自前で指定するので自動調整を切る
		frame_->SetAutoAdjustTextureSize(false);

		//=========================================================
		// セグメントスプライト生成
		//=========================================================

		for (int i = 0; i < 5; i++) {
			// 最大5本分のセグメントを生成する
			seg_[i] = std::make_unique<Sprite>();

			// 塗りつぶし用テクスチャで初期化する
			seg_[i]->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);

			// 描画順をそろえるため親シーンを設定する
			seg_[i]->SetParentScene(parentScene_);

			// 左上基準で扱う
			seg_[i]->SetAnchorPoint({ 0.0f, 0.0f });

			// サイズは自前で指定するので自動調整を切る
			seg_[i]->SetAutoAdjustTextureSize(false);

			// 初期色として通常色を設定する
			seg_[i]->SetColor(desc_.baseColor_);
		}

		//=========================================================
		// 表示状態・アニメ状態の初期化
		//=========================================================

		// 現在の残弾表示値を初期化する
		currentAmmo_ = 0;

		// 前回弾数は未初期化判定用に -1 にする
		lastAmmo_ = -1;

		// 消費フラッシュ用タイマーを初期化する
		drainTimer_ = 0.0f;

		// 回復フラッシュ用タイマーを初期化する
		refillTimer_ = 0.0f;

		// プレイヤーに見せる残弾数を初期化する
		shownAmmo_ = 0;

		// 回復アニメの目標値を初期化する
		refillTarget_ = 0;

		// 回復アニメ状態をオフにする
		refillAnimating_ = false;

		// 回復ステップタイマーを初期化する
		refillStepTimer_ = 0.0f;

		// 初期レイアウトを反映する
		ApplyLayout_();
	}

	void LBGaugeUI::SetDesc(const Desc& desc) {
		// 新しい設定値を保持する
		desc_ = desc;

		// レイアウトを再計算して反映する
		ApplyLayout_();
	}

	void LBGaugeUI::ApplyLayout_() {
		// フレーム未生成なら何もしない
		if (!frame_) { return; }

		//=========================================================
		// フレーム配置
		//=========================================================

		// 中心座標から左上座標へ変換する
		const float left = desc_.center_.x - desc_.size_.x * 0.5f;
		const float top = desc_.center_.y - desc_.size_.y * 0.5f;

		// フレームの位置を設定する
		frame_->SetPosition({ left, top });

		// フレームのサイズを設定する
		frame_->SetSize(desc_.size_);

		// アニメ用の基準位置を保存する
		baseFramePos_ = { left, top };

		// アニメ用の基準サイズを保存する
		baseFrameSize_ = desc_.size_;

		//=========================================================
		// セグメント分割計算
		//=========================================================

		// セグメント数は 1 ～ 5 に制限する
		int segCount = std::clamp(desc_.segments_, 1, 5);

		// 内側余白を 0 以上に制限する
		const float pad = std::max(0.0f, desc_.pad_);

		// セグメント間の隙間を 0 以上に制限する
		const float gap = std::max(0.0f, desc_.gap_);

		// 内側有効幅を計算する
		const float innerW = std::max(1.0f, desc_.size_.x - pad * 2.0f);

		// 内側有効高さを計算する
		const float innerH = std::max(1.0f, desc_.size_.y - pad * 2.0f);

		// 隙間の合計幅を計算する
		const float totalGap = gap * float(segCount - 1);

		// 1本あたりのセグメント幅を計算する
		const float segW = std::max(1.0f, (innerW - totalGap) / float(segCount));

		// セグメント高さは内側高さそのまま使う
		const float segH = innerH;

		//=========================================================
		// セグメント配置
		//=========================================================

		for (int i = 0; i < 5; i++) {
			// セグメント未生成なら飛ばす
			if (!seg_[i]) continue;

			// 使用するセグメントだけレイアウトする
			if (i < segCount) {
				Vector2 p{
					left + pad + (segW + gap) * float(i),
					top + pad
				};

				// セグメント位置を設定する
				seg_[i]->SetPosition(p);

				// セグメントサイズを設定する
				seg_[i]->SetSize({ segW, segH });

				// アニメ用の基準位置を保存する
				baseSegPos_[i] = p;

				// アニメ用の基準サイズを保存する
				baseSegSize_[i] = { segW, segH };
			} else {
				// 使わないセグメントは画面外へ逃がす
				seg_[i]->SetPosition({ -10000.0f, -10000.0f });

				// サイズも最小限にしておく
				seg_[i]->SetSize({ 1.0f, 1.0f });
			}
		}
	}

	void LBGaugeUI::ApplyAnim_(float dt, bool pressed) {
		// フレーム未生成なら何もしない
		if (!frame_) { return; }

		//=========================================================
		// 押下パルス
		//=========================================================

		// 押されていれば1.0、そうでなければ0.0を目標振幅にする
		const float targetAmp = pressed ? 1.0f : 0.0f;

		// 現在振幅を目標へなめらかに追従させる
		pressPulseAmp_ += (targetAmp - pressPulseAmp_) * std::min(1.0f, dt * 12.0f);

		// パルス位相を進める
		pressPulseT_ += dt * 10.0f;

		// サイン波で軽い膨らみを作る
		float pulse = 1.0f + std::sinf(pressPulseT_) * (0.03f * pressPulseAmp_);

		//=========================================================
		// パンチアニメ
		//=========================================================

		// 基本倍率は 1.0
		float punch = 1.0f;

		// 消費時パンチ
		if (punchTimer_ > 0.0f) {
			// タイマーを減らす
			punchTimer_ -= dt;

			// 残り割合を求める
			float t = std::clamp(punchTimer_ / kPunchSec_, 0.0f, 1.0f);

			// 残り割合に応じて拡大倍率を加算する
			punch += t * punchAmp_;
		}

		// 回復時パンチ
		if (refillPunchTimer_ > 0.0f) {
			// タイマーを減らす
			refillPunchTimer_ -= dt;

			// 残り割合を求める
			float t = std::clamp(refillPunchTimer_ / kRefillPunchSec_, 0.0f, 1.0f);

			// 残り割合に応じて拡大倍率を加算する
			punch += t * refillPunchAmp_;
		}

		//=========================================================
		// シェイク
		//=========================================================

		// シェイク量の初期値
		Vector2 shake{ 0.0f, 0.0f };

		if (shakeTimer_ > 0.0f) {
			// シェイクタイマーを減らす
			shakeTimer_ -= dt;

			// 時間経過に応じた減衰率を求める
			float k = std::clamp(shakeTimer_ / kShakeSec_, 0.0f, 1.0f);

			// 0.0～1.0 の乱数を返すラムダ
			auto rand01 = []() {
				return float(std::rand()) / float(RAND_MAX);
				};

			// X方向ランダム
			float rx = (rand01() * 2.0f - 1.0f);

			// Y方向ランダム
			float ry = (rand01() * 2.0f - 1.0f);

			// 減衰込みのシェイク量を計算する
			shake.x = rx * shakeAmpPx_ * k;
			shake.y = ry * shakeAmpPx_ * k;
		}

		//=========================================================
		// 拡縮＋シェイク反映
		//=========================================================

		// 最終的な拡縮率を決める
		float scale = pulse * punch;

		// 1枚分のスプライトへ拡縮とシェイクを適用する共通ラムダ
		auto apply = [&](Sprite* sp, const Vector2& basePos, const Vector2& baseSize) {
			if (!sp) { return; }

			// 拡縮後サイズを計算する
			Vector2 size{ baseSize.x * scale, baseSize.y * scale };

			// 左上基準のまま中心拡縮っぽく見せるため位置補正する
			Vector2 pos{
				basePos.x - (size.x - baseSize.x) * 0.5f + shake.x,
				basePos.y - (size.y - baseSize.y) * 0.5f + shake.y
			};

			// 位置を反映する
			sp->SetPosition(pos);

			// サイズを反映する
			sp->SetSize(size);
			};

		// フレームへ反映する
		apply(frame_.get(), baseFramePos_, baseFrameSize_);

		// 全セグメントへ反映する
		for (int i = 0; i < 5; i++) {
			apply(seg_[i].get(), baseSegPos_[i], baseSegSize_[i]);
		}
	}

	void LBGaugeUI::Update(float dt, int ammo, int maxAmmo, bool blink) {
		// 非表示なら更新しない
		if (!visible_) { return; }

		// フレーム未生成なら更新しない
		if (!frame_) { return; }

		// 最大弾数は最低1に補正する
		maxAmmo = std::max(1, maxAmmo);

		// 現在弾数は 0 ～ maxAmmo に収める
		ammo = std::clamp(ammo, 0, maxAmmo);

		// 内部の現在弾数を更新する
		currentAmmo_ = ammo;

		//=========================================================
		// 初回立ち上げ処理
		//=========================================================

		if (lastAmmo_ < 0) {
			// 最初は0発から見せる
			shownAmmo_ = 0;

			// 回復アニメ扱いで立ち上げる
			refillAnimating_ = true;

			// 初回目標値を設定する
			refillTarget_ = std::clamp(ammo, 0, 5);

			// ステップタイマーを初期化する
			refillStepTimer_ = 0.0f;

			// 現在値を前回値として保存する
			lastAmmo_ = ammo;
		} else {

			//=====================================================
			// 弾数変化検出
			//=====================================================

			if (ammo < lastAmmo_) {
				// 消費フラッシュを開始する
				drainTimer_ = kFlashSec_;

				// 消費パンチを開始する
				punchTimer_ = kPunchSec_;

				// シェイクを開始する
				shakeTimer_ = kShakeSec_;

				// 減少は即反映する
				shownAmmo_ = std::clamp(ammo, 0, 5);

				// 回復アニメ中なら止める
				refillAnimating_ = false;

			} else if (ammo > lastAmmo_) {
				// 回復フラッシュを開始する
				refillTimer_ = kFlashSec_;

				// 回復パンチを開始する
				refillPunchTimer_ = kRefillPunchSec_;

				// 表示値はそのまま維持して段階回復させる
				shownAmmo_ = std::clamp(shownAmmo_, 0, 5);

				// 回復アニメを開始する
				refillAnimating_ = true;

				// 回復先の目標値を設定する
				refillTarget_ = std::clamp(ammo, 0, 5);

				// ステップタイマーを初期化する
				refillStepTimer_ = 0.0f;
			}

			// 今回の弾数を前回値として保存する
			lastAmmo_ = ammo;
		}

		//=========================================================
		// タイマー更新
		//=========================================================

		// 消費タイマーを減らす
		if (drainTimer_ > 0.0f) drainTimer_ -= dt;

		// 回復タイマーを減らす
		if (refillTimer_ > 0.0f) refillTimer_ -= dt;

		//=========================================================
		// 回復アニメ段階更新
		//=========================================================

		if (refillAnimating_) {
			// ステップタイマーを進める
			refillStepTimer_ += dt;

			// 一定時間ごとに1本ずつ増やす
			while (refillStepTimer_ >= kRefillStepSec_) {
				refillStepTimer_ -= kRefillStepSec_;

				if (shownAmmo_ < refillTarget_) {
					// 表示弾数を1つ増やす
					shownAmmo_++;

					// 増えるたびに軽いパンチを発生させる
					refillPunchTimer_ = kRefillPunchSec_;
				} else {
					// 目標に到達したら回復アニメ終了
					refillAnimating_ = false;
					break;
				}
			}
		}

		//=========================================================
		// 色決定
		//=========================================================

		// 基本色を通常色にする
		Vector4 c = desc_.baseColor_;

		// 消費中は消費色を使う
		if (drainTimer_ > 0.0f) { c = desc_.drainColor_; }

		// 回復中は回復色を使う
		if (refillTimer_ > 0.0f) { c = desc_.refillColor_; }

		// 押下中はアルファを少し上げて強調する
		if (blink) { c.w = MyMath::Clamp01(c.w + 0.25f); }

		//=========================================================
		// スプライト更新
		//=========================================================

		// フレームを更新する
		frame_->Update();

		// セグメントの状態を更新する
		for (int i = 0; i < 5; i++) {
			if (!seg_[i]) continue;

			// 毎フレーム更新する
			seg_[i]->Update();

			// 決定した色を反映する
			seg_[i]->SetColor(c);
		}

		// アニメーション反映は最後にまとめて行う
		ApplyAnim_(dt, blink);
	}

	void LBGaugeUI::Draw() {
		// 非表示なら描画しない
		if (!visible_) { return; }

		// フレーム未生成なら描画しない
		if (!frame_) { return; }

		// フレームは常に描画する
		frame_->Draw();

		// 表示するセグメント数を 0 ～ 5 に収める
		int drawCount = std::clamp(shownAmmo_, 0, 5);

		// 左から順に必要本数だけ描画する
		for (int i = 0; i < drawCount; i++) {
			if (seg_[i]) { seg_[i]->Draw(); }
		}
	}
}