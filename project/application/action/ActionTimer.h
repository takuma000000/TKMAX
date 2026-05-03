#pragma once
#include <array>
#include <memory>
#include <vector>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"
#include <string>

//=============================================================
// ActionTimer
// 制限時間のカウントと数字Sprite表示を管理するクラス
//=============================================================
class ActionTimer {
public:
	void Initialize(TKM::DirectXCommon* dxCommon, int limitSeconds);
	void Update();
	void Draw();

	bool IsTimeUp() const { return remainingTime_ <= 0.0f; }
	int GetDisplaySeconds() const;

private:
	void LoadNumberTextures_();
	void CreateDigitSprites_(TKM::DirectXCommon* dxCommon);
	void UpdateDigitSprites_();

	static constexpr int kDigitCount_ = 10;
	static constexpr float kFrameTime_ = 1.0f / 60.0f;

	static constexpr float kDigitWidth_ = 32.0f;
	static constexpr float kDigitHeight_ = 48.0f;
	static constexpr float kDigitSpacing_ = 4.0f;

	Vector2 basePosition_ = { 24.0f, 24.0f };

	float remainingTime_ = 0.0f;

	std::array<std::string, kDigitCount_> numberTexturePaths_ = {};
	std::vector<std::array<std::unique_ptr<TKM::Sprite>, kDigitCount_>> digitSprites_;
};