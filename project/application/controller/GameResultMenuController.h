#pragma once
#include <memory>
#include <array>
#include <string>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "Input.h"
#include "MyMath.h"

class GameResultMenuController {
public:
	enum class Command {
		None,
		Restart,
		ReturnToTitle,
	};

	struct Desc {
		std::string panelTex = "./resources/gradationLine.png";
		std::array<std::string, 2> itemTex = {
			"./resources/restart_pause.png",       // リスタート
			"./resources/title_pause.png",     // タイトルに戻る
		};
		std::string cursorTex = "./resources/circle2.png";
	};

	void Initialize(
		TKM::SpriteCommon* spriteCommon,
		TKM::DirectXCommon* dxCommon,
		TKM::BaseScene* parentScene,
		float screenW,
		float screenH,
		const Desc& desc = Desc()
	);

	Command Update(float dt);
	void Draw();
	void UpdateLayout(float screenW, float screenH);

private:
	enum class Item {
		Restart = 0,
		ReturnToTitle,
		Count
	};

	bool TriggerPadUp_();
	bool TriggerPadDown_();
	bool TriggerA_();
	void MoveIndex_(int delta);

	static float Clamp01_(float v);

	Desc desc_{};

	TKM::SpriteCommon* spriteCommon_ = nullptr;
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::BaseScene* parentScene_ = nullptr;

	float screenW_ = 0.0f;
	float screenH_ = 0.0f;

	int index_ = 0;

	bool prevUp_ = false;
	bool prevDown_ = false;
	bool prevA_ = false;

	std::unique_ptr<TKM::Sprite> panel_;
	std::array<std::unique_ptr<TKM::Sprite>, (int)Item::Count> items_;
	std::unique_ptr<TKM::Sprite> cursor_;

	Vector2 panelPos_{};
	Vector2 panelSize_{};
	Vector2 baseItemPos_{};
	float itemSpacingY_ = 64.0f;

	// ポーズ画面と同じ「脈動」
	float pulseTime_ = 0.0f;
};