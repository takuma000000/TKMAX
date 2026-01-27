#pragma once
#include <memory>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "MyMath.h"
#include "RBGaugeUI.h"
#include "Player.h"
#include <Input.h>

namespace TKM {
	class UIController {
	public:
		void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH);
		void UpdateLayout(float screenW, float screenH);
		void Update(float dt, Player* player);
		void Draw();

	private:
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;

		std::unique_ptr<Sprite> uiLT_;
		std::unique_ptr<Sprite> uiLB_;
		std::unique_ptr<Sprite> uiRB_;
		std::unique_ptr<RBGaugeUI> rbGaugeUI_;
	};
}