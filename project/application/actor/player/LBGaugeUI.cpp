#define NOMINMAX
#include "LBGaugeUI.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace TKM {
	void LBGaugeUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		desc_ = desc;

		// フレーム
		frame_ = std::make_unique<Sprite>();
		frame_->Initialize(spriteCommon_, dxCommon_, desc_.frameTex_);
		frame_->SetParentScene(parentScene_); // 描画順の関係で親シーンは同じにしておく
		frame_->SetAnchorPoint({ 0.0f, 0.0f }); // 左上基準
		frame_->SetAutoAdjustTextureSize(false); // サイズは自分で指定する

		// セグメント（塗りつぶし）
		for (int i = 0; i < 5; i++) {
			// 5個作るけど、使うのはdesc_.segments_個だけ。残りは画面外に退避させる。
			seg_[i] = std::make_unique<Sprite>();
			seg_[i]->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
			seg_[i]->SetParentScene(parentScene_); // 描画順の関係で親シーンは同じにしておく
			seg_[i]->SetAnchorPoint({ 0.0f, 0.0f }); // 左上基準
			seg_[i]->SetAutoAdjustTextureSize(false); // サイズは自分で指定する
			seg_[i]->SetColor(desc_.baseColor_); // 初期は通常色
		}
		// 初期値
		currentAmmo_ = 0; // 表示上の残弾数（小数点以下も扱う）
		lastAmmo_ = -1; // 前フレームの残弾数（変化を検出するため）
		drainTimer_ = 0.0f; // 消費パルス用タイマー
		refillTimer_ = 0.0f; // 回復パルス用タイマー
		shownAmmo_ = 0; // プレイヤーに見えている残弾数（整数、アニメーションで追従させる）
		refillTarget_ = 0; // 回復アニメーションの目標値
		refillAnimating_ = false; // 回復アニメーション中かどうか
		refillStepTimer_ = 0.0f; // 回復アニメーションのステップタイマー

		// アニメ用の初期値
		ApplyLayout_();
	}

	void LBGaugeUI::SetDesc(const Desc& desc) {
		desc_ = desc;
		ApplyLayout_();
	}

	void LBGaugeUI::ApplyLayout_() {
		if (!frame_) { return; }

		// frame：中心→左上に変換して配置
		const float left = desc_.center_.x - desc_.size_.x * 0.5f;
		const float top = desc_.center_.y - desc_.size_.y * 0.5f;
		// フレームの位置とサイズを設定
		frame_->SetPosition({ left, top });
		frame_->SetSize(desc_.size_);

		baseFramePos_ = { left, top }; // アニメ用の基準位置
		baseFrameSize_ = desc_.size_; // アニメ用の基準サイズ

		// 5分割
		int segCount = std::clamp(desc_.segments_, 1, 5); // 1〜5の範囲でセグメント数を制限
		// 内側余白とセグメント間の隙間を考慮して、セグメントのサイズと位置を計算
		const float pad = std::max(0.0f, desc_.pad_);
		const float gap = std::max(0.0f, desc_.gap_);
		// 内側余白を引いた残りのスペースを、セグメントと隙間で分割していく
		const float innerW = std::max(1.0f, desc_.size_.x - pad * 2.0f);
		const float innerH = std::max(1.0f, desc_.size_.y - pad * 2.0f);
		// セグメント間の隙間の合計幅
		const float totalGap = gap * float(segCount - 1);
		const float segW = std::max(1.0f, (innerW - totalGap) / float(segCount));
		const float segH = innerH;

		// セグメントの位置とサイズを設定。使わない分は画面外に退避。
		for (int i = 0; i < 5; i++) {

			// セグメントがnullptrの可能性はないけど、一応安全に。
			if (!seg_[i]) continue;

			// 使う分は配置
			if (i < segCount) {
				Vector2 p{
					left + pad + (segW + gap) * float(i), // 左端 + 内側余白 + (セグメント幅 + 隙間) * インデックス
					top + pad // 上端 + 内側余白
				};
				seg_[i]->SetPosition(p); // 位置を設定
				seg_[i]->SetSize({ segW, segH }); // サイズを設定

				baseSegPos_[i] = p; // アニメ用の基準位置
				baseSegSize_[i] = { segW, segH }; // アニメ用の基準サイズ
			} else {
				// 使わない分は画面外に退避（Drawでも描かないけど保険）
				seg_[i]->SetPosition({ -10000.0f, -10000.0f }); // 十分遠い位置に移動
				seg_[i]->SetSize({ 1.0f, 1.0f }); // サイズも最小限にしておく
			}
		}
	}

	void LBGaugeUI::ApplyAnim_(float dt, bool pressed) {
		if (!frame_) { return; }

		//----------------------------
		// 押下パルス（ふわふわ）
		//----------------------------
		const float targetAmp = pressed ? 1.0f : 0.0f; // 押されているときは1.0f、そうでないときは0.0fが目標値
		// 追従を少し速めに（0.0 -> 1.0）
		pressPulseAmp_ += (targetAmp - pressPulseAmp_) * std::min(1.0f, dt * 12.0f);
		pressPulseT_ += dt * 10.0f; // パルスの速さ（10.0fは適当に選んだ値で、速くしたり遅くしたりして調整する）

		float pulse = 1.0f + std::sinf(pressPulseT_) * (0.03f * pressPulseAmp_); // 3%程度

		//----------------------------
		// パンチ（消費/回復）
		//----------------------------
		float punch = 1.0f; // パンチの基本は1.0f（変化なし）

		// 消費パンチ
		if (punchTimer_ > 0.0f) {
			punchTimer_ -= dt;
			// 0.0fから始まって、kPunchSec_の間に徐々に0.0fへ減衰していく値を計算
			float t = std::clamp(punchTimer_ / kPunchSec_, 0.0f, 1.0f); // 1 -> 0
			punch += t * punchAmp_; // tが1のとき最大パンチ、0のときパンチなし
		}

		// 回復パンチ
		if (refillPunchTimer_ > 0.0f) {
			refillPunchTimer_ -= dt;
			// 0.0fから始まって、kRefillPunchSec_の間に徐々に0.0fへ減衰していく値を計算
			float t = std::clamp(refillPunchTimer_ / kRefillPunchSec_, 0.0f, 1.0f); // 1 -> 0
			punch += t * refillPunchAmp_; // tが1のとき最大パンチ、0のときパンチなし
		}

		//----------------------------
		// シェイク（消費時にだけ）
		//----------------------------
		Vector2 shake{ 0.0f, 0.0f }; // シェイクのオフセット量

		// 消費時のパンチと同じタイミングでシェイクも開始する想定なので、shakeTimer_を使って時間経過を管理する
		if (shakeTimer_ > 0.0f) {
			shakeTimer_ -= dt;
			float k = std::clamp(shakeTimer_ / kShakeSec_, 0.0f, 1.0f); // 1 -> 0（時間経過に伴ってシェイクが減衰していく）

			// 乱数生成（-1.0f .. 1.0fの範囲でランダムな値を生成するラムダ関数）
			auto rand01 = []() {
				return float(std::rand()) / float(RAND_MAX); // 0.0fから1.0fの範囲で乱数を生成
				};
			// ランダムな方向に揺れるように、-1.0fから1.0fの範囲で乱数を生成して、それにシェイクの振幅と減衰係数を掛ける
			float rx = (rand01() * 2.0f - 1.0f);
			float ry = (rand01() * 2.0f - 1.0f);
			// シェイクのオフセット量を計算
			shake.x = rx * shakeAmpPx_ * k;
			shake.y = ry * shakeAmpPx_ * k;
		}

		//----------------------------
		// スケールを「中心拡縮」っぽく見せるための位置補正
		// anchorは(0,0)なので、拡縮分の半分だけ左上に戻す
		//----------------------------
		float scale = pulse * punch; // 最終的な拡縮は、押下パルスとパンチの両方を掛け合わせたもの
		auto apply = [&](Sprite* sp, const Vector2& basePos, const Vector2& baseSize) {
			if (!sp) { return; } // スケールを掛けたサイズを計算
			
			Vector2 size{ baseSize.x * scale, baseSize.y * scale }; // スケールを掛けたサイズを計算
			Vector2 pos{
				basePos.x - (size.x - baseSize.x) * 0.5f + shake.x, // 拡縮分の半分だけ左上に戻す + シェイク
				basePos.y - (size.y - baseSize.y) * 0.5f + shake.y // 拡縮分の半分だけ左上に戻す + シェイク
			};

			sp->SetPosition(pos); // 位置を更新
			sp->SetSize(size); // サイズを更新
			};

		apply(frame_.get(), baseFramePos_, baseFrameSize_); // フレームに適用

		// セグメントに適用
		for (int i = 0; i < 5; i++) {
			apply(seg_[i].get(), baseSegPos_[i], baseSegSize_[i]); 
		}
	}

	void LBGaugeUI::Update(float dt, int ammo, int maxAmmo, bool blink) {
		if (!visible_) { return; } // 表示してないなら更新も処理しない
		if (!frame_) { return; } // フレームがないなら更新できない（安全策）

		maxAmmo = std::max(1, maxAmmo);
		ammo = std::clamp(ammo, 0, maxAmmo);
		currentAmmo_ = ammo;

		//==================================================
		// 初回：0→ammo へ「パパパ」で立ち上げ
		//==================================================

		// lastAmmo_が-1のときは初回（初期化後最初のUpdate呼び出し）とみなす。-1はありえない値なので。
		if (lastAmmo_ < 0) {
			shownAmmo_ = 0;                         // 初期は0から見せる
			refillAnimating_ = true;                // 回復アニメ開始
			refillTarget_ = std::clamp(ammo, 0, 5); // 目標
			refillStepTimer_ = 0.0f;                // タイマー初期化

			lastAmmo_ = ammo; // 初期化
		} else { // 2回目以降は変化を検出してアニメーション開始

			//============================
			// 変化検出
			//============================

			// ammoが前フレームより減っていたら消費アニメ、増えていたら回復アニメを開始する
			if (ammo < lastAmmo_) {
				// 消費したときは、減るのが即座に反映されて、パルスとシェイクが発生する感じ
				drainTimer_ = kFlashSec_;
				punchTimer_ = kPunchSec_;
				shakeTimer_ = kShakeSec_;

				// 減少は即反映（残像が残らないように）
				shownAmmo_ = std::clamp(ammo, 0, 5);
				refillAnimating_ = false; // 回復アニメはもし動いてたら止める（消費の方が優先される感じで）

			} else if (ammo > lastAmmo_) { // 増えたときは、増えるのが少し遅れて反映されて、回復のパルスが発生する感じ
				// 回復したときは、増えるのが少し遅れて反映される感じで、回復のパルスが発生する感じ
				refillTimer_ = kFlashSec_;
				refillPunchTimer_ = kRefillPunchSec_;
				// 増加は少し遅れて反映（残像が残る感じで）
				shownAmmo_ = std::clamp(shownAmmo_, 0, 5);

				// いま表示してる数 → 目標(ammo)までパパパ
				refillAnimating_ = true;
				refillTarget_ = std::clamp(ammo, 0, 5);
				refillStepTimer_ = 0.0f;
			}
			// 変化がなければ、アニメーションはそのまま継続（パルスや回復の段階は時間経過で更新されていく）
			lastAmmo_ = ammo;
		}

		//============================
		// タイマー
		//============================

		// タイマーを減算していく。0未満にならないようにクランプする。
		if (drainTimer_ > 0.0f) drainTimer_ -= dt;

		// 回復のタイマーも同様に減算
		if (refillTimer_ > 0.0f) refillTimer_ -= dt;

		//============================
		// 回復段階更新
		//============================

		// 回復アニメーション中は、refillStepTimer_を加算していって、一定時間ごとにshownAmmo_を1ずつ増やしていく。shownAmmo_がrefillTarget_に追いついたらアニメーション終了。
		if (refillAnimating_) {
			refillStepTimer_ += dt;

			// 一定時間ごとにshownAmmo_を1ずつ増やす
			while (refillStepTimer_ >= kRefillStepSec_) {
				refillStepTimer_ -= kRefillStepSec_; // タイマーをリセット（次のステップに向けて）

				// 1個増えるごとに、shownAmmo_がrefillTarget_に追いついてないかチェック。追いついてなければ1増やす。追いついたらアニメーション終了。
				if (shownAmmo_ < refillTarget_) {
					shownAmmo_++;
					// 1個増えるたびに軽くパンチ（気持ちよさ）
					refillPunchTimer_ = kRefillPunchSec_;
				} else { // 追いついたらアニメーション終了
					refillAnimating_ = false;
					break;
				}
			}
		}

		//============================
		// 色（押下中はちょい明るく）
		//============================
		Vector4 c = desc_.baseColor_; // 基本は通常色

		// 消費・回復のタイマーが動いているときは、それぞれの色を優先して表示する。さらに、押下中は全体的に明るくする感じで。
		if (drainTimer_ > 0.0f) { c = desc_.drainColor_; }

		// 回復の方は、消費のタイマーが動いているときより優先度を下げる（両方動いてるときは消費色を優先する）感じで。
		if (refillTimer_ > 0.0f) { c = desc_.refillColor_; }

		// 押下中は全体的に明るくする感じで。点滅も兼ねる。
		if (blink) { c.w = MyMath::Clamp01(c.w + 0.25f); }

		//============================
		// スプライト更新
		//============================
		frame_->Update();

		// セグメントは「左から ammo 個」表示なので、i < ammo のときは表示する。色も更新する。
		for (int i = 0; i < 5; i++) {
			if (!seg_[i]) continue; // 安全策
			seg_[i]->Update(); // 更新は毎フレーム必要（アニメーションのため）
			seg_[i]->SetColor(c); // 色を更新
		}

		// 動きアニメは1回だけ（ループ外）
		ApplyAnim_(dt, blink);
	}

	void LBGaugeUI::Draw() {
		if (!visible_) { return; } // 表示してないなら描画しない
		if (!frame_) { return; } // フレームがないなら描画できない（安全策）

		frame_->Draw(); // フレームは常に描画

		// 「左から ammo 個」表示（= 右から消える）
		int drawCount = std::clamp(shownAmmo_, 0, 5);

		// 描画は「左から ammo 個」なので、i < ammo のときは描画する。残りは描画しない。
		for (int i = 0; i < drawCount; i++) {
			if (seg_[i]) { seg_[i]->Draw(); }
		}
	}
}