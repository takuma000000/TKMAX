#include "TitleFlowStates.h"
#include "TitleScene.h"
#include "WaterRippleEffect.h"
#include "GameScene.h"
#include <Windows.h>

/// <summary>
/// 共通のStateContextをTitleSceneとして扱えるように変換します。
/// </summary>
/// <param name="ctx">ステートマシンから渡される共通コンテキスト</param>
/// <returns>TitleScene参照</returns>
static TitleScene& AsTitle_(TKM::IStateContext& ctx) {
	return static_cast<TitleScene&>(ctx);
}

//=====================================================
// IntroIrisOpen
//=====================================================
void TitleFlowIntroIrisOpenState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx);

	// アイリス演出中でも、タイトル背景の敵は動かし続ける
	for (auto& u : s.titleEnemies_) {
		// 消滅済み、または実体がない敵は更新しない
		if (!u.alive_ || !u.enemy_) {
			continue;
		}

		// 敵の移動・アニメーションを更新する
		u.enemy_->Update(dt);
	}

	// 開幕アイリスを開く
	if (s.irisOpening_) {
		// Tweenに合わせてアイリスのサイズを更新する
		s.irisScale_ = UpdateIrisScale(s.iris_.get(), s.irisTween_, dt);

		// アイリスが開き切ったらIdle状態へ進む
		if (s.irisTween_.Finished()) {
			// アイリス開き演出を終了する
			s.irisOpening_ = false;

			// 念のためアイリスサイズを完全に0に固定する
			s.irisScale_ = 0.0f;
			s.iris_->SetSize({ s.irisScale_, s.irisScale_ });

			// 次の待機シーケンス用にタイマーを初期化する
			s.seqTimer_ = 0.0f;

			// タイトル待機状態へ遷移する
			s.flowSM_.Change(std::make_unique<TitleFlowIdleState>());
		}
	}
}

//=====================================================
// Idle
//=====================================================
void TitleFlowIdleState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx);

	// 待機状態用のタイマーを初期化する
	s.seqTimer_ = 0.0f;
}

void TitleFlowIdleState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx);

	// タイトル背景の敵をうようよ動かす
	for (auto& u : s.titleEnemies_) {
		// 消滅済み、または実体がない敵は更新しない
		if (!u.alive_ || !u.enemy_) {
			continue;
		}

		// 敵の移動・アニメーションを更新する
		u.enemy_->Update(dt);
	}

	// メニュー表示中は、メニュー操作を優先する
	if (s.showUi_) {
		// メニューの入力更新を行い、発行されたコマンドを取得する
		const auto cmd = s.titleMenu_->Update(dt);

		// UI表示中は見つめ合い演出も更新する
		s.titleShowdown_->Update(dt, true);

		// STARTが選ばれたら、メニューを経由せず波紋演出へ進む
		if (cmd == TitleMenuController::Command::Start) {
			s.showMenuAfterVanish_ = false;
			s.flowSM_.Change(std::make_unique<TitleFlowRippleState>());
			return;
		}

		// EXITが選ばれたら、アプリ終了を要求する
		if (cmd == TitleMenuController::Command::Exit) {
			s.sceneManager_->RequestQuit();
			PostQuitMessage(0);

			// このフレームの通常更新を止める
			s.earlyExitUpdate_ = true;
			return;
		}

		// メニュー操作中はここで終了する
		return;
	}

	// メニューがまだ出ていない間は、少し待ってから敵の消滅演出へ進む
	s.seqTimer_ += dt;

	// 一定時間待ったら自動で消滅演出へ進む
	const bool autoGo = (s.seqTimer_ >= 1.4f);

	if (autoGo) {
		// 消滅演出に入るため、一旦UIとメニューは非表示にする
		s.showUi_ = false;
		s.titleMenu_->SetVisible(false);

		// 消滅後はメニュー表示へ進む
		s.showMenuAfterVanish_ = true;

		// 敵ごとの消滅タイミングをランダムに設定する
		s.ScheduleVanish_();

		// 消滅シーケンスへ遷移する
		s.flowSM_.Change(std::make_unique<TitleFlowVanishingState>());
	}
}

//=====================================================
// Vanishing
//=====================================================
void TitleFlowVanishingState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx);

	// 消滅シーケンス用のタイマーを初期化する
	s.vanishTimer_ = 0.0f;
}

void TitleFlowVanishingState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx);

	// 消滅シーケンスの経過時間を進める
	s.vanishTimer_ += dt;

	// 消えるまでは敵を動かし続ける
	for (auto& u : s.titleEnemies_) {
		// 消滅済み、または実体がない敵は更新しない
		if (!u.alive_ || !u.enemy_) {
			continue;
		}

		// 敵の移動・アニメーションを更新する
		u.enemy_->Update(dt);
	}

	// ランダムに設定された時差で敵を消していく
	for (auto& u : s.titleEnemies_) {
		// 消滅済み、または実体がない敵は処理しない
		if (!u.alive_ || !u.enemy_) {
			continue;
		}

		// 敵ごとの消滅遅延時間を過ぎたら消滅させる
		if (s.vanishTimer_ >= u.vanishDelay_) {
			// 敵の位置で爆発エフェクトを出す
			s.EmitTitleExplode_(u.enemy_->GetWorldPosition());

			// 生存フラグを下ろして、以降の更新・描画対象から外す
			u.alive_ = false;
		}
	}

	// すべての敵が消えたら、次に進む
	if (s.AllEnemiesGone_()) {
		// 消滅後にメニューを出す場合
		if (s.showMenuAfterVanish_) {
			s.showUi_ = true;
			s.titleMenu_->SetVisible(true);

			// メニュー待機用のタイマーを初期化する
			s.seqTimer_ = 0.0f;

			// Idleへ戻して、メニュー操作待ちにする
			s.flowSM_.Change(std::make_unique<TitleFlowIdleState>());
			return;
		}

		// メニューを出さない場合は、そのままゲーム開始用の波紋演出へ進む
		s.flowSM_.Change(std::make_unique<TitleFlowRippleState>());
	}
}

//=====================================================
// Ripple
//=====================================================
void TitleFlowRippleState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx);

	// 波紋演出用のタイマーを初期化する
	s.rippleTimer_ = 0.0f;

	// 波紋の見た目設定を作る
	TKM::WaterRippleEffect::RippleDesc d{};
	d.duration_ = 1.0f;
	d.radiusMax_ = 0.857f;
	d.amplitude_ = 0.1f;
	d.frequency_ = 80.0f;
	d.width_ = 10.0f;
	d.color_ = { 1.0f, 1.0f, 1.0f };
	d.colorIntensity_ = 0.0f;

	// 画面中央から波紋を発生させる
	s.rippleEffect_->Trigger({ 0.5f, 0.5f }, d);
}

void TitleFlowRippleState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx);

	// 波紋演出の経過時間を進める
	s.rippleTimer_ += dt;

	// 波紋を少し見せたら、アイリス閉じへ進む
	if (s.rippleTimer_ >= TitleScene::kRippleWaitSec_) {
		s.flowSM_.Change(std::make_unique<TitleFlowIrisCloseState>());
	}
}

//=====================================================
// IrisClose
//=====================================================
void TitleFlowIrisCloseState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx);

	// アイリス閉じ演出を開始する
	s.irisClosing_ = true;

	// アイリスサイズを0から最大まで広げ、画面を閉じる
	s.irisTween_.Reset(
		0.0f,
		s.irisMax_,
		TitleScene::kIrisDurationSec_,
		Ease::Type::InBack
	);
}

void TitleFlowIrisCloseState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx);

	// 念のため、閉じ演出中でない場合は何もしない
	if (!s.irisClosing_) {
		return;
	}

	// Tweenに合わせてアイリスサイズを更新する
	s.irisScale_ = UpdateIrisScale(s.iris_.get(), s.irisTween_, dt);

	// アイリスが閉じ切ったらゲームシーンへ遷移する
	if (s.irisTween_.Finished()) {
		s.sceneManager_->SetNextScene(std::make_unique<GameScene>(s.dxCommon_, s.srvManager_));

		// このフレームの通常更新を止める
		s.earlyExitUpdate_ = true;
	}
}