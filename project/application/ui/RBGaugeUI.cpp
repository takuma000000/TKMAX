#define NOMINMAX
#include "RBGaugeUI.h"
#include <algorithm>

namespace TKM {
	void RBGaugeUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc) {
		// スプライト共通情報を保持する
		spriteCommon_ = spriteCommon;

		// DirectX共通情報を保持する
		dxCommon_ = dxCommon;

		// 親シーンを保持する
		parentScene_ = parentScene;

		// ゲージの配置・色・挙動設定を保持する
		desc_ = desc;

		//=========================================================
		// フレーム生成
		//=========================================================

		// ゲージ枠用スプライトを生成する
		frame_ = std::make_unique<Sprite>();

		// 枠テクスチャで初期化する
		frame_->Initialize(spriteCommon_, dxCommon_, desc_.frameTex_);

		// 描画順をそろえるため親シーンを設定する
		frame_->SetParentScene(parentScene_);

		// 左上基準で扱う
		frame_->SetAnchorPoint({ 0.0f, 0.0f });

		// サイズはコード側で指定するので自動調整を切る
		frame_->SetAutoAdjustTextureSize(false);

		//=========================================================
		// 遅延バー生成
		//=========================================================

		// 左側の遅延バーを生成する
		lagL_ = std::make_unique<Sprite>();

		// 塗りつぶしテクスチャで初期化する
		lagL_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);

		// 描画順をそろえるため親シーンを設定する
		lagL_->SetParentScene(parentScene_);

		// 左側バーは中心から左へ伸びるので右上基準にする
		lagL_->SetAnchorPoint({ 1.0f, 0.0f });

		// 遅延バー色を設定する
		lagL_->SetColor(desc_.lagColor_);

		// サイズはコード側で指定するので自動調整を切る
		lagL_->SetAutoAdjustTextureSize(false);

		// 右側の遅延バーを生成する
		lagR_ = std::make_unique<Sprite>();

		// 塗りつぶしテクスチャで初期化する
		lagR_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);

		// 描画順をそろえるため親シーンを設定する
		lagR_->SetParentScene(parentScene_);

		// 右側バーは中心から右へ伸びるので左上基準にする
		lagR_->SetAnchorPoint({ 0.0f, 0.0f });

		// 遅延バー色を設定する
		lagR_->SetColor(desc_.lagColor_);

		// サイズはコード側で指定するので自動調整を切る
		lagR_->SetAutoAdjustTextureSize(false);

		//=========================================================
		// 前面バー生成
		//=========================================================

		// 左側の前面バーを生成する
		fillL_ = std::make_unique<Sprite>();

		// 塗りつぶしテクスチャで初期化する
		fillL_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);

		// 描画順をそろえるため親シーンを設定する
		fillL_->SetParentScene(parentScene_);

		// 左側バーは中心から左へ伸びるので右上基準にする
		fillL_->SetAnchorPoint({ 1.0f, 0.0f });

		// 通常色を設定する
		fillL_->SetColor(desc_.baseColor_);

		// サイズはコード側で指定するので自動調整を切る
		fillL_->SetAutoAdjustTextureSize(false);

		// 右側の前面バーを生成する
		fillR_ = std::make_unique<Sprite>();

		// 塗りつぶしテクスチャで初期化する
		fillR_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);

		// 描画順をそろえるため親シーンを設定する
		fillR_->SetParentScene(parentScene_);

		// 右側バーは中心から右へ伸びるので左上基準にする
		fillR_->SetAnchorPoint({ 0.0f, 0.0f });

		// 通常色を設定する
		fillR_->SetColor(desc_.baseColor_);

		// サイズはコード側で指定するので自動調整を切る
		fillR_->SetAutoAdjustTextureSize(false);

		//=========================================================
		// 内部状態初期化
		//=========================================================

		// 前回弾数を未初期化値にする
		lastAmmo_ = -1;

		// 遅延バー用の弾数値を初期化する
		lagAmmo_ = 0.0f;

		// シェイクタイマーを初期化する
		shakeTimer_ = 0.0f;

		// 消費フラッシュタイマーを初期化する
		drainTimer_ = 0.0f;

		// 前回弾数を未初期化値にする
		prevAmmo_ = -1;

		// パンチタイマーを初期化する
		punchTimer_ = 0.0f;

		// 前回の半幅を初期化する
		prevHalfW_ = 0.0f;

		//=========================================================
		// 破片プール生成
		//=========================================================

		// 破片リストを空にする
		chips_.clear();

		// 固定数の破片をあらかじめ確保する
		chips_.resize(kChipPool_);

		// 破片スプライトを初期化する
		for (auto& c : chips_) {
			// 破片用スプライトを生成する
			c.sp_ = std::make_unique<Sprite>();

			// 塗りつぶしテクスチャを破片にも流用する
			c.sp_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);

			// 描画順をそろえるため親シーンを設定する
			c.sp_->SetParentScene(parentScene_);

			// 破片は中心基準で扱う
			c.sp_->SetAnchorPoint({ 0.5f, 0.5f });

			// サイズはコード側で指定するので自動調整を切る
			c.sp_->SetAutoAdjustTextureSize(false);

			// 初期状態では未使用にしておく
			c.active_ = false;
		}
	}

	void RBGaugeUI::Update(float dt, int ammo, int maxAmmo, bool refilling, bool blink) {
		// 非表示なら更新しない
		if (!visible_) { return; }

		// 必要なスプライトが無ければ更新できない
		if (!frame_ || !fillL_ || !fillR_ || !lagL_ || !lagR_) { return; }

		// 最大弾数は最低1に補正する
		maxAmmo = std::max(1, maxAmmo);

		// 現在弾数を0～最大弾数に収める
		ammo = std::clamp(ammo, 0, maxAmmo);

		// 初回だけ前回弾数を現在弾数で初期化する
		if (prevAmmo_ < 0 || prevAmmo_ > maxAmmo) {
			prevAmmo_ = ammo;
		}

		// 前回弾数を退避する
		int oldAmmo = prevAmmo_;

		// 弾数が減ったかどうかを判定する
		bool consumed = (ammo < oldAmmo);

		// 弾数が減った場合は消費演出を開始する
		if (consumed) {
			shakeTimer_ = desc_.shakeTime_;
			drainTimer_ = kDrainFlashSec_;
			punchTimer_ = kPunchSec_;
		}

		// 現在弾数の割合を計算する
		float rate = std::clamp(float(ammo) / float(maxAmmo), 0.0f, 1.0f);

		//=========================================================
		// 遅延バー追従
		//=========================================================

		// 弾数が減った時は遅延バーをゆっくり追従させる
		if (lagAmmo_ > float(ammo)) {
			lagAmmo_ = std::max(float(ammo), lagAmmo_ - desc_.lagSpeed_ * dt);
		} else {
			// 弾数が増えた時は即座に追従させる
			lagAmmo_ = float(ammo);
		}

		// 遅延バー用の割合を計算する
		float lagRate = std::clamp(lagAmmo_ / float(maxAmmo), 0.0f, 1.0f);

		//=========================================================
		// サイズ・位置計算
		//=========================================================

		// ゲージ全体の半分の幅
		float halfW = desc_.size_.x * 0.5f;

		// 現在弾数に応じた前面バーの半幅
		float curHalf = halfW * rate;

		// 遅延バーの半幅
		float lagHalf = halfW * lagRate;

		// 中心X座標
		float cx = desc_.center_.x;

		// 中心Y座標
		float y = desc_.center_.y;

		// フレーム左上位置を計算する
		Vector2 framePos = { cx - halfW, y };

		// フレームサイズを少し大きめに設定する
		frame_->SetSize({ desc_.size_.x + 10.0f, desc_.size_.y + 10.0f });

		// 弾数が減った場合、減った部分から破片を出す
		if (consumed) {
			float oldRate = std::clamp(float(oldAmmo) / float(maxAmmo), 0.0f, 1.0f);
			float oldHalf = halfW * oldRate;
			float newHalf = curHalf;
			SpawnChips_(cx, y, oldHalf, newHalf);
		}

		//=========================================================
		// パンチ演出
		//=========================================================

		// 基本倍率
		float punch = 1.0f;

		// パンチタイマーが動いている間だけ高さを縮める
		if (punchTimer_ > 0.0f) {
			float t = std::clamp(punchTimer_ / kPunchSec_, 0.0f, 1.0f);
			float k = 1.0f - t;
			punch = 1.0f - 0.10f * (1.0f - k);
		}

		// パンチ反映後の高さ
		float h = desc_.size_.y * punch;

		// 遅延バーと前面バーのサイズを更新する
		lagL_->SetSize({ lagHalf, h });
		lagR_->SetSize({ lagHalf, h });
		fillL_->SetSize({ curHalf, h });
		fillR_->SetSize({ curHalf, h });

		//=========================================================
		// シェイク演出
		//=========================================================

		// シェイクオフセット初期値
		Vector2 off{ 0.0f, 0.0f };

		// シェイク中だけランダムに位置をずらす
		if (shakeTimer_ > 0.0f) {
			off.x = RandRange_(-desc_.shakePower_, desc_.shakePower_);
			off.y = RandRange_(-desc_.shakePower_, desc_.shakePower_);
		}

		// フレーム位置を反映する
		frame_->SetPosition({ (framePos.x - 5.0f) + off.x, (framePos.y - 5.0f) + off.y });

		// 中心基準で左右のバー位置を反映する
		fillL_->SetPosition({ cx + off.x, y + off.y });
		lagL_->SetPosition({ cx + off.x, y + off.y });
		fillR_->SetPosition({ cx + off.x, y + off.y });
		lagR_->SetPosition({ cx + off.x, y + off.y });

		//=========================================================
		// 色更新
		//=========================================================

		// 通常色を基本にする
		Vector4 col = desc_.baseColor_;

		// 回復中なら回復色にする
		if (refilling) {
			col = desc_.refillColor_;
		} else if (drainTimer_ > 0.0f) {
			// 消費フラッシュ中なら消費色にする
			col = desc_.drainColor_;
		}

		// RBゲージ点滅
		if (blink) {
			// 点滅タイマーを進める
			blinkT_ += dt;

			// 0/1を交互に切り替える
			const int phase = int(blinkT_ / blinkInterval_) % 2;

			// 暗い側の倍率を決める
			const float mul = (phase == 0) ? 1.0f : blinkLowMul_;

			// RGBを暗くして点滅表現する
			col.x *= mul;
			col.y *= mul;
			col.z *= mul;
		} else {
			// 点滅していない間はタイマーをリセットする
			blinkT_ = 0.0f;
		}

		// 前面バーへ色を反映する
		fillL_->SetColor(col);
		fillR_->SetColor(col);

		// 遅延バーへ色を反映する
		lagL_->SetColor(desc_.lagColor_);
		lagR_->SetColor(desc_.lagColor_);

		//=========================================================
		// タイマー更新
		//=========================================================

		// 消費フラッシュタイマーを減らす
		if (drainTimer_ > 0.0f) { drainTimer_ = std::max(0.0f, drainTimer_ - dt); }

		// シェイクタイマーを減らす
		if (shakeTimer_ > 0.0f) { shakeTimer_ = std::max(0.0f, shakeTimer_ - dt); }

		// パンチタイマーを減らす
		if (punchTimer_ > 0.0f) { punchTimer_ = std::max(0.0f, punchTimer_ - dt); }

		// 破片を更新する
		UpdateChips_(dt);

		//=========================================================
		// スプライト更新
		//=========================================================

		// フレームを更新する
		frame_->Update();

		// 遅延バーを更新する
		lagL_->Update();
		lagR_->Update();

		// 前面バーを更新する
		fillL_->Update();
		fillR_->Update();

		// 今回の弾数を次フレーム用に保存する
		prevAmmo_ = ammo;
	}

	void RBGaugeUI::Draw() {
		// 非表示なら描画しない
		if (!visible_) { return; }

		// フレームを描画する
		if (frame_) frame_->Draw();

		// 遅延バーを描画する
		if (lagL_)  lagL_->Draw();
		if (lagR_)  lagR_->Draw();

		// 前面バーを描画する
		if (fillL_) fillL_->Draw();
		if (fillR_) fillR_->Draw();

		// 破片はゲージの上に描画する
		DrawChips_();
	}

	void RBGaugeUI::SetVisible(bool v) {
		// 表示フラグを設定する
		visible_ = v;
	}

	void RBGaugeUI::SetDesc(const Desc& desc) {
		// 設定値を更新する
		desc_ = desc;
	}

	float RBGaugeUI::RandRange_(float a, float b) {
		// 0.0～1.0の乱数を作り、a～bの範囲へ変換して返す
		return a + (b - a) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
	}

	//=========================================================
	// 破片生成
	//=========================================================
	void RBGaugeUI::SpawnChips_(float cx, float y, float oldHalf, float newHalf) {
		// 減った幅を計算する
		float removed = oldHalf - newHalf;

		// 減っていないなら破片を出さない
		if (removed <= 0.0f) { return; }

		// 削れ量に応じて破片数を決める
		int count = (int)std::clamp(removed / 6.0f, 2.0f, 10.0f);

		// 左端の発生位置
		float leftEdge = cx - oldHalf;

		// 右端の発生位置
		float rightEdge = cx + oldHalf;

		// 左右両端から破片を出す
		for (int side = 0; side < 2; side++) {

			// 指定数だけ破片を作る
			for (int i = 0; i < count; i++) {

				// 使用する破片を入れるポインタ
				Chip* c = nullptr;

				// 未使用の破片をプールから探す
				for (auto& it : chips_) {
					if (!it.active_) { c = &it; break; }
				}

				// 空きが無ければ生成をやめる
				if (!c) { return; }

				// 破片を有効化する
				c->active_ = true;

				// 破片の最大寿命を設定する
				c->maxLife_ = kChipLife_;

				// 破片の現在寿命を設定する
				c->life_ = kChipLife_;

				// 左右どちらの端から出すか決める
				float px = (side == 0) ? leftEdge : rightEdge;

				// X位置に少しランダムさを加える
				px += RandRange_(-4.0f, 4.0f);

				// Y位置に少しランダムさを加える
				float py = y + RandRange_(-desc_.size_.y * 0.35f, desc_.size_.y * 0.35f);

				// 破片の初期位置を設定する
				c->pos_ = { px, py };

				// 左右方向を決める
				float dir = (side == 0) ? -1.0f : 1.0f;

				// 横方向の初速を決める
				float vx = dir * (kChipSpeed_ + RandRange_(-kChipSpread_, kChipSpread_));

				// 縦方向の初速を決める
				float vy = RandRange_(-120.0f, 40.0f);

				// 破片の速度を設定する
				c->vel_ = { vx, vy };

				// 破片サイズをランダムに決める
				c->size_ = RandRange_(kChipSizeMin_, kChipSizeMax_);

				// 破片スプライトサイズを設定する
				c->sp_->SetSize({ c->size_, c->size_ });

				// 破片スプライト位置を設定する
				c->sp_->SetPosition(c->pos_);

				// 破片の色を消費色にする
				c->sp_->SetColor(desc_.drainColor_);
			}
		}
	}

	void RBGaugeUI::UpdateChips_(float dt) {
		// 全破片を順番に更新する
		for (auto& c : chips_) {
			// 未使用破片は更新しない
			if (!c.active_) { continue; }

			// 寿命を減らす
			c.life_ -= dt;

			// 寿命が尽きたら未使用に戻す
			if (c.life_ <= 0.0f) {
				c.active_ = false;
				continue;
			}

			// 重力を加える
			c.vel_.y += kChipGravity_ * dt;

			// 速度で位置を進める
			c.pos_.x += c.vel_.x * dt;
			c.pos_.y += c.vel_.y * dt;

			// 残り寿命から透明度を計算する
			float a = std::clamp(c.life_ / c.maxLife_, 0.0f, 1.0f);

			// 消費色をベースにフェードさせる
			Vector4 col = desc_.drainColor_;
			col.w *= a;

			// 破片の色を反映する
			c.sp_->SetColor(col);

			// 破片の位置を反映する
			c.sp_->SetPosition(c.pos_);

			// 破片スプライトを更新する
			c.sp_->Update();
		}
	}

	void RBGaugeUI::DrawChips_() {
		// 有効な破片だけ描画する
		for (auto& c : chips_) {
			if (!c.active_) { continue; }

			// 破片を描画する
			c.sp_->Draw();
		}
	}
} // namespace TKM