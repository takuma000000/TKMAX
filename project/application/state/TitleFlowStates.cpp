#include "TitleFlowStates.h"
#include "TitleScene.h"
#include "WaterRippleEffect.h"
#include "GameScene.h"
#include <Windows.h>

static TitleScene& AsTitle_(TKM::IStateContext& ctx) {
	return static_cast<TitleScene&>(ctx);
}

void TitleFlowIntroIrisOpenState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx);

	// 敵更新（アイリス中でも動かす）
	for (auto& u : s.titleEnemies_) {
		if (!u.alive_ || !u.enemy_) { continue; }
		u.enemy_->Update(dt);
	}

	// 開幕アイリス（開く）更新
	if (s.irisOpening_) {
		s.irisScale_ = UpdateIrisScale(s.iris_.get(), s.irisTween_, dt);

		if (s.irisTween_.Finished()) {
			s.irisOpening_ = false;
			s.irisScale_ = 0.0f;
			s.iris_->SetSize({ s.irisScale_, s.irisScale_ });

			s.seqTimer_ = 0.0f;
			s.flowSM_.Change(std::make_unique<TitleFlowIdleState>());
		}
	}
}

void TitleFlowIdleState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx);
	s.seqTimer_ = 0.0f;
}

void TitleFlowIdleState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx);

	// 敵うようよ更新
	for (auto& u : s.titleEnemies_) {
		if (!u.alive_ || !u.enemy_) { continue; }
		u.enemy_->Update(dt);
	}

	// ① メニューが出てる時：メニュー操作
	if (s.showUi_) {
		const auto cmd = s.titleMenu_->Update(dt);
		s.UpdateShowdownActors_(dt);

		if (cmd == TitleMenuController::Command::Start) {
			s.showMenuAfterVanish_ = false;
			s.flowSM_.Change(std::make_unique<TitleFlowRippleState>());
			return;
		}
		if (cmd == TitleMenuController::Command::Exit) {
			s.sceneManager_->RequestQuit();
			PostQuitMessage(0);
			s.earlyExitUpdate_ = true;
			return;
		}
		return;
	}

	// ② メニューが出てない時：待ち→自動で消滅へ
	s.seqTimer_ += dt;
	const bool autoGo = (s.seqTimer_ >= 1.4f);

	if (autoGo) {
		s.showUi_ = false;
		s.titleMenu_->SetVisible(false);

		s.showMenuAfterVanish_ = true;
		s.ScheduleVanish_();

		s.flowSM_.Change(std::make_unique<TitleFlowVanishingState>());
	}
}

void TitleFlowVanishingState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx);
	s.vanishTimer_ = 0.0f;
}

void TitleFlowVanishingState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx);

	s.vanishTimer_ += dt;

	// 消えるまで敵は動いてOK
	for (auto& u : s.titleEnemies_) {
		if (!u.alive_ || !u.enemy_) { continue; }
		u.enemy_->Update(dt);
	}

	// ランダム時差で消す
	for (auto& u : s.titleEnemies_) {
		if (!u.alive_ || !u.enemy_) { continue; }

		if (s.vanishTimer_ >= u.vanishDelay_) {
			s.EmitTitleExplode_(u.enemy_->GetWorldPosition());
			u.alive_ = false;
		}
	}

	// 全滅したら「メニューへ」 or 「波紋へ」
	if (s.AllEnemiesGone_()) {
		if (s.showMenuAfterVanish_) {
			s.showUi_ = true;
			s.titleMenu_->SetVisible(true);
			s.seqTimer_ = 0.0f;
			s.flowSM_.Change(std::make_unique<TitleFlowIdleState>());
			return;
		}

		s.flowSM_.Change(std::make_unique<TitleFlowRippleState>());
	}
}

void TitleFlowRippleState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx);
	s.rippleTimer_ = 0.0f;

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
	auto& s = AsTitle_(ctx);

	s.rippleTimer_ += dt;

	if (s.rippleTimer_ >= TitleScene::kRippleWaitSec_) {
		s.flowSM_.Change(std::make_unique<TitleFlowIrisCloseState>());
	}
}

void TitleFlowIrisCloseState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsTitle_(ctx);

	s.irisClosing_ = true;

	s.irisTween_.Reset(
		0.0f,
		s.irisMax_,
		TitleScene::kIrisDurationSec_,
		Ease::Type::InBack
	);
}

void TitleFlowIrisCloseState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsTitle_(ctx);

	if (!s.irisClosing_) { return; }

	s.irisScale_ = UpdateIrisScale(s.iris_.get(), s.irisTween_, dt);

	if (s.irisTween_.Finished()) {
		s.sceneManager_->SetNextScene(new GameScene(s.dxCommon_, s.srvManager_));
		s.earlyExitUpdate_ = true;
	}
}