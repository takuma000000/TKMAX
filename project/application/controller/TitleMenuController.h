#pragma once
#include <memory>
#include <array>
#include <string>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "Input.h"
#include "MyMath.h"

class DirectXCommon;
class BaseScene;

class TitleMenuController {
public:
	enum class Command {
		None,
		Start,
		Exit,
	};

	struct Desc {
		std::string panelTex = "./resources/gradationLine.png";
		std::array<std::string, 2> itemTex = {
			"./resources/uvChecker.png", // はじめる
			"./resources/uvChecker.png",  // とじる
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
		Start = 0,
		Exit,
		Count
	};

	bool TriggerPadUp_();
	bool TriggerPadDown_();
	bool TriggerA_();

	void MoveIndex_(int delta);

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

	std::array<float, (int)Item::Count> itemScale_{};
	Vector2 itemSizeNormal_{ 260.0f, 48.0f };
	float selectScale_ = 1.18f;   // 選択中倍率
	float scaleSpeed_ = 14.0f;    // 追従速度（大きいほどキビキビ）

	float pulseTime_ = 0.0f; // 選択中の脈動
};