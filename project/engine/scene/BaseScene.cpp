#include "BaseScene.h"

#ifdef USE_IMGUI
#include <externals/imgui/imgui.h>
#endif

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
	frameCount_++; // フレーム数カウント
	float deltaTime = ImGui::GetIO().DeltaTime; // 経過時間取得(秒)
	timeCount_ += deltaTime; // 経過時間加算

	// フレームタイム計算
	frameTimeMs_ = deltaTime * 1000.0f; // 秒 → ミリ秒

	// 1秒経過したらFPS計算
	if (timeCount_ >= 1.0f) {
		fps_ = static_cast<float>(frameCount_) / timeCount_; // FPS計算
		frameCount_ = 0; // フレーム数リセット
		timeCount_ = 0.0f; // 経過時間リセット
	}
}

void BaseScene::ResetDrawCallCount()
{
	drawCallCount_ = 0; // DrawCall数リセット
}
