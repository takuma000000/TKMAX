#include "DodgeUI.h"
#include "Player.h"

namespace TKM {

	void DodgeUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon) {
		// ゲージ背景スプライトの初期化
		gaugeBack_ = std::make_unique<Sprite>();
		gaugeBack_->Initialize(spriteCommon, dxCommon, "./resources/texture/blue.dds"); // ゲージ背景用のテクスチャを使用
		gaugeBack_->SetAutoAdjustTextureSize(false);
		gaugeBack_->SetAnchorPoint({ 0.5f, 0.5f }); // 中心を基準に配置
		gaugeBack_->SetSize(gaugeBackSize_); // ゲージ背景のサイズを設定
		// ゲージ塗りつぶしスプライトの初期化
		gaugeFill_ = std::make_unique<Sprite>();
		gaugeFill_->Initialize(spriteCommon, dxCommon, "./resources/texture/gauge_green.jpg"); // ゲージ用のテクスチャを使用
		gaugeFill_->SetAutoAdjustTextureSize(false);
		gaugeFill_->SetAnchorPoint({ 0.0f, 0.5f }); // 左端を基準に配置
		gaugeFill_->SetSize({ 0.0f, gaugeSize_.y }); // 最初は幅0で初期化
	}

	void DodgeUI::Update(Player* player, float screenW, float screenH) {
		// プレイヤーオブジェクトが存在しない場合は非表示にする
		if (!player || !gaugeBack_ || !gaugeFill_) {
			visible_ = false;
			return;
		}

		// プレイヤーから回避クールダウンゲージの表示状態と溜まり具合を取得
		visible_ = player->IsDodgeCooldownGaugeVisible();

		// ゲージが表示されない場合は更新処理をスキップ
		if (!visible_) {
			return;
		}

		// クランプして0.0f～1.0fの範囲に収める
		rate_ = MyMath::Clamp01(player->GetDodgeCooldownGaugeRate());
		// プレイヤーから回避クールダウンゲージの画面上の中心位置を取得
		const Vector2 center = player->GetDodgeCooldownGaugeScreenPos(screenW, screenH);
		// ゲージ背景と塗りつぶしの位置とサイズを更新
		gaugeBack_->SetPosition(center);
		// 塗りつぶしは左端を基準に配置しているため、中心から半分のサイズを引いて位置を調整
		gaugeFill_->SetPosition({
			center.x - gaugeSize_.x * 0.5f,
			center.y
			});
		// ゲージの幅は溜まり具合に応じて変化させる
		gaugeFill_->SetSize({
			gaugeSize_.x * rate_,
			gaugeSize_.y
			});

		// スプライトの更新
		gaugeBack_->Update();
		gaugeFill_->Update();
	}

	void DodgeUI::Draw(float hudAlpha) {
		// ゲージが表示されない場合やスプライトが初期化されていない場合は描画処理をスキップ
		if (!visible_ || !gaugeBack_ || !gaugeFill_) {
			return;
		}

		// HUD全体のアルファ値をゲージの色に乗算して適用
		Vector4 back = backColor_;
		back.w *= hudAlpha;
		// 塗りつぶし色も同様にアルファ値を乗算
		Vector4 fill = fillColor_;
		fill.w *= hudAlpha;

		// ゲージ背景と塗りつぶしの色を設定して描画
		gaugeBack_->SetColor(back);
		gaugeBack_->Draw();
		// 塗りつぶしは溜まり具合に応じて幅が変化しているため、描画前にサイズを更新している
		gaugeFill_->SetColor(fill);
		gaugeFill_->Draw();
	}

}