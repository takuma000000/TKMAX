#include "BaseScene.h"
#include <externals/imgui/imgui.h>

void BaseScene::Initialize()
{
}

void BaseScene::Finalize()
{
}

void BaseScene::Update()
{
}

void BaseScene::Draw()
{
}

void BaseScene::UpdatePerformanceInfo()
{
	frameCount_++;
	float deltaTime = ImGui::GetIO().DeltaTime;
	timeCount_ += deltaTime;

	// フレームタイム計算
	frameTimeMs_ = deltaTime * 1000.0f; // 秒 → ミリ秒

	// 1秒経過したらFPS計算
	if (timeCount_ >= 1.0f) {
		fps_ = static_cast<float>(frameCount_) / timeCount_;
		frameCount_ = 0;
		timeCount_ = 0.0f;
	}
}

void BaseScene::ResetDrawCallCount()
{
	drawCallCount_ = 0;
}
