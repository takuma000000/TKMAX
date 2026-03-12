#include "TitleFlowStates.h"
#include "TitleScene.h"
#include "WaterRippleEffect.h"
#include "GameScene.h"
#include <Windows.h>

static TitleScene& AsTitle_(TKM::IStateContext& ctx) {
	return static_cast<TitleScene&>(ctx); // 状態コンテキストをタイトルシーンにキャスト
}

void TitleFlowIntroIrisOpenState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx); // 状態コンテキストをタイトルシーンにキャスト

	// 敵更新（アイリス中でも動かす）
	for (auto& u : s.titleEnemies_) {
		if (!u.alive_ || !u.enemy_) { continue; } // 念のため生存とnullptrチェック
		u.enemy_->Update(dt); // 敵の更新（移動やアニメーションなど）を行う
	}

	// 開幕アイリス（開く）更新
	if (s.irisOpening_) {
		s.irisScale_ = UpdateIrisScale(s.iris_.get(), s.irisTween_, dt); // アイリスのサイズを更新

		if (s.irisTween_.Finished()) { // アイリスの開きが完了したら
			s.irisOpening_ = false; // アイリス開幕フラグを下ろす
			s.irisScale_ = 0.0f; // 念のためサイズを完全に0にしておく
			s.iris_->SetSize({ s.irisScale_, s.irisScale_ }); // アイリスのサイズを反映

			s.seqTimer_ = 0.0f; // シーケンス全体の経過時間初期化
			s.flowSM_.Change(std::make_unique<TitleFlowIdleState>()); // 次の状態（Idle）へ遷移
		}
	}
}

void TitleFlowIdleState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx); // 状態コンテキストをタイトルシーンにキャスト
	s.seqTimer_ = 0.0f; // シーケンス全体の経過時間初期化
}

void TitleFlowIdleState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx); // 状態コンテキストをタイトルシーンにキャスト

	// 敵うようよ更新
	for (auto& u : s.titleEnemies_) {
		if (!u.alive_ || !u.enemy_) { continue; } // 念のため生存とnullptrチェック
		u.enemy_->Update(dt); // 敵の更新（移動やアニメーションなど）を行う
	}

	// ① メニューが出てる時：メニュー操作
	if (s.showUi_) {
		const auto cmd = s.titleMenu_->Update(dt); // メニューの更新（入力処理など）を行い、発行されたコマンドを取得
		s.UpdateShowdownActors_(dt); // メニューが出てるときの演出更新（敵の動きやエフェクトなど）
		// コマンドに応じた処理
		if (cmd == TitleMenuController::Command::Start) {
			s.showMenuAfterVanish_ = false; // Vanishing後にメニューを出さない（直接波紋へ）
			s.flowSM_.Change(std::make_unique<TitleFlowRippleState>()); // 直接波紋へ遷移
			return;
		}
		// とじるコマンドが出たらアプリ終了
		if (cmd == TitleMenuController::Command::Exit) {
			s.sceneManager_->RequestQuit(); // アプリ終了要求
			PostQuitMessage(0); // Windowsアプリケーションの終了要求
			s.earlyExitUpdate_ = true; // Updateの早期終了フラグを立てる（念のため）
			return;
		}
		return;
	}

	// ② メニューが出てない時：待ち→自動で消滅へ
	s.seqTimer_ += dt;
	const bool autoGo = (s.seqTimer_ >= 1.4f); // 1.4秒待って自動で消滅へ
	// Aボタンが押されたら即座に消滅へ
	if (autoGo) {
		s.showUi_ = false; // UI非表示
		s.titleMenu_->SetVisible(false); // メニュー非表示

		s.showMenuAfterVanish_ = true; // Vanishing後にメニューを出す（波紋は飛ばしてメニューへ遷移）
		s.ScheduleVanish_(); // タイトル敵の消滅をスケジュール（遅延時間をランダムにセット）
		// 消滅シーケンスへ遷移
		s.flowSM_.Change(std::make_unique<TitleFlowVanishingState>());
	}
}

void TitleFlowVanishingState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx); // 状態コンテキストをタイトルシーンにキャスト
	s.vanishTimer_ = 0.0f; // 消滅シーケンスの経過時間初期化
}

void TitleFlowVanishingState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx); // 状態コンテキストをタイトルシーンにキャスト
	// 消滅シーケンスの経過時間を更新
	s.vanishTimer_ += dt;

	// 消えるまで敵は動いてOK
	for (auto& u : s.titleEnemies_) {
		if (!u.alive_ || !u.enemy_) { continue; } // 念のため生存とnullptrチェック
		u.enemy_->Update(dt); // 敵の更新（移動やアニメーションなど）を行う
	}

	// ランダム時差で消す
	for (auto& u : s.titleEnemies_) {
		if (!u.alive_ || !u.enemy_) { continue; } //

		if (s.vanishTimer_ >= u.vanishDelay_) { // 消滅遅延時間を過ぎたら消す
			s.EmitTitleExplode_(u.enemy_->GetWorldPosition()); // 敵の位置で爆発エフェクトを出す
			u.alive_ = false; // 生存フラグを下ろす（消滅開始）
		}
	}

	// 全滅したら「メニューへ」 or 「波紋へ」
	if (s.AllEnemiesGone_()) {
		if (s.showMenuAfterVanish_) { // Vanishing後にメニューを出す場合
			s.showUi_ = true; // UI表示
			s.titleMenu_->SetVisible(true); // メニュー表示
			s.seqTimer_ = 0.0f; // シーケンス全体の経過時間初期化
			s.flowSM_.Change(std::make_unique<TitleFlowIdleState>()); // Idle状態へ遷移
			return;
		}
		// Vanishing後にメニューを出さない場合は直接波紋へ遷移
		s.flowSM_.Change(std::make_unique<TitleFlowRippleState>());
	}
}

void TitleFlowRippleState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx); // 状態コンテキストをタイトルシーンにキャスト
	s.rippleTimer_ = 0.0f; // 波紋エフェクトの経過時間初期化

	// 波紋を出す（元のコードそのまま移植）
	TKM::WaterRippleEffect::RippleDesc d{};
	d.duration_ = 1.0f;
	d.radiusMax_ = 0.857f;
	d.amplitude_ = 0.1f;
	d.frequency_ = 80.0f;
	d.width_ = 10.0f;
	d.color_ = { 1.0f, 1.0f, 1.0f };
	d.colorIntensity_ = 0.0f;
	s.rippleEffect_->Trigger({ 0.5f, 0.5f }, d);
}

void TitleFlowRippleState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx); // 状態コンテキストをタイトルシーンにキャスト

	s.rippleTimer_ += dt; // 波紋エフェクトの経過時間を更新

	if (s.rippleTimer_ >= TitleScene::kRippleWaitSec_) { // 波紋エフェクト発生から一定時間経ったら次の状態へ遷移
		s.flowSM_.Change(std::make_unique<TitleFlowIrisCloseState>()); // アイリス（閉じる）状態へ遷移
	}
}

void TitleFlowIrisCloseState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx); // 状態コンテキストをタイトルシーンにキャスト

	s.irisClosing_ = true; // アイリス（閉じる）フラグを立てる

	s.irisTween_.Reset( // アイリスのサイズを0から最大まで変化させるTweenをセット
		0.0f,
		s.irisMax_,
		TitleScene::kIrisDurationSec_,
		Ease::Type::InBack
	);
}

void TitleFlowIrisCloseState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx); // 状態コンテキストをタイトルシーンにキャスト

	if (!s.irisClosing_) { return; } // 念のためアイリス（閉じる）フラグチェック

	s.irisScale_ = UpdateIrisScale(s.iris_.get(), s.irisTween_, dt); // アイリスのサイズを更新
	// アイリスのサイズを反映
	if (s.irisTween_.Finished()) {
		s.sceneManager_->SetNextScene(std::make_unique<GameScene>(s.dxCommon_, s.srvManager_)); // 次のシーンをゲームシーンにセット
		s.earlyExitUpdate_ = true; // Updateの早期終了フラグを立てる（念のため）
	}
}