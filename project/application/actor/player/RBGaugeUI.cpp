#define NOMINMAX
#include "RBGaugeUI.h"
#include <algorithm>

namespace TKM {
	void RBGaugeUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		desc_ = desc;

		// frame（枠）
		frame_ = std::make_unique<Sprite>();
		frame_->Initialize(spriteCommon_, dxCommon_, desc_.frameTex_);
		frame_->SetParentScene(parentScene_);
		frame_->SetAnchorPoint({ 0.0f, 0.0f });
		frame_->SetAutoAdjustTextureSize(false);

		// lag（後ろ）
		// 左右で別スプライトにして、左右両端から中央に向かって伸びるようにする。
		lagL_ = std::make_unique<Sprite>();
		lagL_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
		lagL_->SetParentScene(parentScene_);
		lagL_->SetAnchorPoint({ 1.0f, 0.0f });
		lagL_->SetColor(desc_.lagColor_);
		lagL_->SetAutoAdjustTextureSize(false);

		lagR_ = std::make_unique<Sprite>();
		lagR_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
		lagR_->SetParentScene(parentScene_);
		lagR_->SetAnchorPoint({ 0.0f, 0.0f });
		lagR_->SetColor(desc_.lagColor_);
		lagR_->SetAutoAdjustTextureSize(false);

		// fill（前）
		// 左右で別スプライトにして、左右両端から中央に向かって伸びるようにする。
		fillL_ = std::make_unique<Sprite>();
		fillL_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
		fillL_->SetParentScene(parentScene_);
		fillL_->SetAnchorPoint({ 1.0f, 0.0f });
		fillL_->SetColor(desc_.baseColor_);
		fillL_->SetAutoAdjustTextureSize(false);
		
		fillR_ = std::make_unique<Sprite>();
		fillR_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
		fillR_->SetParentScene(parentScene_);
		fillR_->SetAnchorPoint({ 0.0f, 0.0f });
		fillR_->SetColor(desc_.baseColor_);
		fillR_->SetAutoAdjustTextureSize(false);

		// 初期状態
		lastAmmo_ = -1;
		lagAmmo_ = 0.0f;
		shakeTimer_ = 0.0f;
		drainTimer_ = 0.0f;
		prevAmmo_ = -1;
		punchTimer_ = 0.0f;
		prevHalfW_ = 0.0f;

		// 破片プール
		chips_.clear();
		chips_.resize(kChipPool_);

		// 破片スプライトの初期化
		for (auto& c : chips_) {
			c.sp_ = std::make_unique<Sprite>();
			c.sp_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_); // fillTex流用
			c.sp_->SetParentScene(parentScene_);
			c.sp_->SetAnchorPoint({ 0.5f, 0.5f });
			c.sp_->SetAutoAdjustTextureSize(false);
			c.active_ = false;
		}
	}

	void RBGaugeUI::Update(float dt, int ammo, int maxAmmo, bool refilling, bool blink) {
		if (!visible_) { return; }
		if (!frame_ || !fillL_ || !fillR_ || !lagL_ || !lagR_) { return; }

		maxAmmo = std::max(1, maxAmmo);
		ammo = std::clamp(ammo, 0, maxAmmo);

		// prevAmmo 初期化（初回だけ）
		if (prevAmmo_ < 0 || prevAmmo_ > maxAmmo) {
			prevAmmo_ = ammo;
		}

		// 消費検出（oldAmmo保持が重要）
		int oldAmmo = prevAmmo_;
		bool consumed = (ammo < oldAmmo);
		if (consumed) {
			shakeTimer_ = desc_.shakeTime_;
			drainTimer_ = kDrainFlashSec_;
			punchTimer_ = kPunchSec_;
		}

		// rate
		float rate = std::clamp(float(ammo) / float(maxAmmo), 0.0f, 1.0f);

		// lag（後ろ）追従
		if (lagAmmo_ > float(ammo)) {
			lagAmmo_ = std::max(float(ammo), lagAmmo_ - desc_.lagSpeed_ * dt);
		} else {
			lagAmmo_ = float(ammo);
		}
		float lagRate = std::clamp(lagAmmo_ / float(maxAmmo), 0.0f, 1.0f);

		// サイズ計算
		float halfW = desc_.size_.x * 0.5f;
		// rate をかけて、左右両端から中央に向かって伸びるようにする。
		float curHalf = halfW * rate;
		// lagRate も同様に
		float lagHalf = halfW * lagRate;
		// 位置計算（中心基準）
		float cx = desc_.center_.x;
		// y は固定
		float y = desc_.center_.y;

		// 枠
		Vector2 framePos = { cx - halfW, y };
		frame_->SetSize({ desc_.size_.x + 10.0f, desc_.size_.y + 10.0f });

		// 破片：減った分だけ外側が砕ける（左右同時）
		if (consumed) {
			float oldRate = std::clamp(float(oldAmmo) / float(maxAmmo), 0.0f, 1.0f);
			float oldHalf = halfW * oldRate;
			float newHalf = curHalf;
			SpawnChips_(cx, y, oldHalf, newHalf);
		}

		// パンチ（高さだけ）
		float punch = 1.0f;
		if (punchTimer_ > 0.0f) {
			float t = std::clamp(punchTimer_ / kPunchSec_, 0.0f, 1.0f);
			float k = 1.0f - t;
			punch = 1.0f - 0.10f * (1.0f - k);
		}
		float h = desc_.size_.y * punch;

		lagL_->SetSize({ lagHalf, h });
		lagR_->SetSize({ lagHalf, h });
		fillL_->SetSize({ curHalf, h });
		fillR_->SetSize({ curHalf, h });

		// シェイク
		Vector2 off{ 0.0f, 0.0f };
		if (shakeTimer_ > 0.0f) {
			off.x = RandRange_(-desc_.shakePower_, desc_.shakePower_);
			off.y = RandRange_(-desc_.shakePower_, desc_.shakePower_);
		}

		frame_->SetPosition({ (framePos.x - 5.0f) + off.x, (framePos.y - 5.0f) + off.y });

		// 位置（中心固定：アンカーで左右へ伸びる）
		fillL_->SetPosition({ cx + off.x, y + off.y });
		lagL_->SetPosition({ cx + off.x, y + off.y });
		fillR_->SetPosition({ cx + off.x, y + off.y });
		lagR_->SetPosition({ cx + off.x, y + off.y });

		// 色（フラッシュ）
		Vector4 col = desc_.baseColor_;
		if (refilling) {
			col = desc_.refillColor_;
		} else if (drainTimer_ > 0.0f) {
			col = desc_.drainColor_;
		}

		// RBゲージ点滅
		if (blink) {
			blinkT_ += dt;
			const int phase = int(blinkT_ / blinkInterval_) % 2; // 0/1で交互
			const float mul = (phase == 0) ? 1.0f : blinkLowMul_; // OFF側を暗く

			// alphaじゃなくRGBを暗くする（ブレンド無しでも見える）
			col.x *= mul;
			col.y *= mul;
			col.z *= mul;
		} else {
			blinkT_ = 0.0f; // 点滅してないときはタイマーリセット
		}
		fillL_->SetColor(col);
		fillR_->SetColor(col);

		lagL_->SetColor(desc_.lagColor_);
		lagR_->SetColor(desc_.lagColor_);

		// タイマー更新
		if (drainTimer_ > 0.0f) { drainTimer_ = std::max(0.0f, drainTimer_ - dt); }
		if (shakeTimer_ > 0.0f) { shakeTimer_ = std::max(0.0f, shakeTimer_ - dt); }
		if (punchTimer_ > 0.0f) { punchTimer_ = std::max(0.0f, punchTimer_ - dt); }

		// 破片更新
		UpdateChips_(dt);

		// Update
		frame_->Update();
		lagL_->Update();
		lagR_->Update();
		fillL_->Update();
		fillR_->Update();

		// prevAmmo 更新（最後）
		prevAmmo_ = ammo;
	}

	void RBGaugeUI::Draw() {
		if (!visible_) { return; }

		if (frame_) frame_->Draw();
		if (lagL_)  lagL_->Draw();
		if (lagR_)  lagR_->Draw();
		if (fillL_) fillL_->Draw();
		if (fillR_) fillR_->Draw();

		// 破片はゲージの上に
		DrawChips_();
	}

	void RBGaugeUI::SetVisible(bool v) {
		visible_ = v; // visible_ は単純に描画するかどうかのフラグ。true のとき描画する、false のとき描画しない。
	}

	void RBGaugeUI::SetDesc(const Desc& desc) {
		desc_ = desc; // desc_ は RBGaugeUI の設定情報を保持するメンバ変数。SetDesc1 関数は外部から新しい設定情報を受け取って desc_ に保存するための関数。
	}

	float RBGaugeUI::RandRange_(float a, float b) {
		return a + (b - a) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)); // RandRange_11 関数は、a から b の範囲でランダムな浮動小数点数を生成するための関数。std::rand() は 0 から RAND_MAX までの整数を返すため、それを 0.0f から 1.0f の範囲に正規化し、さらに a と b の範囲にスケーリングして返します。
	}

	// ----------------------------
	// 破片
	// ----------------------------
	void RBGaugeUI::SpawnChips_(float cx, float y, float oldHalf, float newHalf) {
		float removed = oldHalf - newHalf;
		if (removed <= 0.0f) { return; }

		// 削れ量に応じて破片数
		int count = (int)std::clamp(removed / 6.0f, 2.0f, 10.0f);

		// 破片は左右両端から出るので、左端と右端の位置を計算しておく
		float leftEdge = cx - oldHalf;
		float rightEdge = cx + oldHalf;

		// 破片生成
		for (int side = 0; side < 2; side++) {

			// 破片プールから未使用のものを探す
			for (int i = 0; i < count; i++) {

				Chip* c = nullptr; // 破片プールから未使用のものを探すためのポインタ。これを見つけたら、そこに新しい破片の情報をセットしていく。もし見つからなかったら、これ以上破片を生成できないので関数からリターンする。

				// 破片プールから未使用のものを探す。active_ が false のものが未使用なので、それを見つけたら c にセットしてループを抜ける。もし見つからなかったら、これ以上破片を生成できないので関数からリターンする。
				for (auto& it : chips_) {

					// active_ が false のものが未使用なので、それを見つけたら c にセットしてループを抜ける。もし見つからなかったら、これ以上破片を生成できないので関数からリターンする。
					if (!it.active_) { c = &it; break; }
				}

				// もし見つからなかったら、これ以上破片を生成できないので関数からリターンする。
				if (!c) { return; }

				// 見つかった c に新しい破片の情報をセットしていく。active_ を true にして、寿命をランダムにして、位置を左右端から少しランダムにして、速度をランダムにして、サイズをランダムにして、スプライトのサイズと位置をセットして、色を減少色にする。
				c->active_ = true;
				c->maxLife_ = kChipLife_;
				c->life_ = kChipLife_;

				// 位置は左右端から少しランダムにして、y はゲージの高さを中心に少しランダムにする。side が 0 のとき左端、1 のとき右端になるようにする。
				float px = (side == 0) ? leftEdge : rightEdge;
				px += RandRange_(-4.0f, 4.0f); // さらに左右に少しランダムにする
				float py = y + RandRange_(-desc_.size_.y * 0.35f, desc_.size_.y * 0.35f); // y はゲージの高さを中心に少しランダムにする
				// これで、破片は左右端から出て、y はゲージの高さを中心に少しランダムに出るようになる。
				c->pos_ = { px, py };
				// 速度は、x は左右にランダムにして、y は上方向を基本にランダムにする。side が 0 のとき左向き、1 のとき右向きになるようにする。
				float dir = (side == 0) ? -1.0f : 1.0f;
				float vx = dir * (kChipSpeed_ + RandRange_(-kChipSpread_, kChipSpread_));
				float vy = RandRange_(-120.0f, 40.0f);
				// これで、破片は左右にランダムな速度で飛び出し、y は上方向を基本にランダムな速度で飛び出すようになる。
				c->vel_ = { vx, vy };
				c->size_ = RandRange_(kChipSizeMin_, kChipSizeMax_);
				c->sp_->SetSize({ c->size_, c->size_ });
				c->sp_->SetPosition(c->pos_);
				// 砕け色（減少色）
				c->sp_->SetColor(desc_.drainColor_);
			}
		}
	}

	void RBGaugeUI::UpdateChips_(float dt) {

		// active_ な破片だけ更新する。寿命を減らして、0 以下になったら active_ を false にして消す。物理っぽく、速度を加算して位置を更新する。フェードさせるために、寿命の割合に応じて色の alpha を変える。最後にスプライトの位置と色をセットして Update する。
		for (auto& c : chips_) {
			if (!c.active_) { continue; } // active_ な破片だけ更新するためのチェック。これが false のときはこの破片は未使用なので、更新処理をスキップして次の破片に移る。
			// 寿命を減らして、0 以下になったら active_ を false にして消す。これで、寿命が尽きた破片は非アクティブになって更新・描画されなくなる。

			c.life_ -= dt;

			// 寿命が尽きた破片は非アクティブになって更新・描画されなくなる。
			if (c.life_ <= 0.0f) {
				c.active_ = false;
				continue;
			}

			// 物理っぽく
			c.vel_.y += kChipGravity_ * dt;
			c.pos_.x += c.vel_.x * dt;
			c.pos_.y += c.vel_.y * dt;

			// フェード
			float a = std::clamp(c.life_ / c.maxLife_, 0.0f, 1.0f);

			Vector4 col = desc_.drainColor_;
			col.w *= a;

			c.sp_->SetColor(col);
			c.sp_->SetPosition(c.pos_);
			c.sp_->Update();
		}
	}

	void RBGaugeUI::DrawChips_() {

		// active_ な破片だけ描画する。これも Update と同様に、active_ が true のものだけ描画するようにする。
		for (auto& c : chips_) {
			if (!c.active_) { continue; }
			c.sp_->Draw(); // active_ な破片だけ描画するためのチェック。これが false のときはこの破片は未使用なので、描画処理をスキップして次の破片に移る。
		}
	}
} // namespace TKM