#include "BaseScene.h"
#include <Input.h>
#include <Xinput.h>

#ifdef USE_IMGUI
#include <externals/imgui/imgui.h>
#endif

void BaseScene::Initialize() {
}

void BaseScene::Finalize() {
}

void BaseScene::Update() {
}

void BaseScene::Draw() {
}

void BaseScene::UpdatePerformanceInfo() {
#ifdef USE_IMGUI

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

#endif
}

void BaseScene::ResetDrawCallCount() {
	drawCallCount_ = 0; // DrawCall数リセット
}

void BaseScene::ImGuiDebugGamepad() {
#ifdef USE_IMGUI
	ImGui::Begin("ゲームパッド");

	auto DrawButtonBar = [](const char* label, bool isPressed, const ImVec4& color) {
		float value = isPressed ? 1.0f : 0.0f;
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
		ImGui::ProgressBar(value, ImVec2(200, 0), label);
		ImGui::PopStyleColor();
		};

	XINPUT_STATE state{};
	DWORD result = XInputGetState(0, &state);
	if (result == ERROR_SUCCESS) {
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "接続中！！！");
	} else {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "接続されていません");
	}

	ImGui::Separator();
	DrawButtonBar("A", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_A), ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
	DrawButtonBar("B", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_B), ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
	DrawButtonBar("X", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_X), ImVec4(0.0f, 0.4f, 1.0f, 1.0f));
	DrawButtonBar("Y", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_Y), ImVec4(1.0f, 0.4f, 0.7f, 1.0f));
	DrawButtonBar("Start", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_START), ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	DrawButtonBar("Back", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_BACK), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	DrawButtonBar("LB", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER), ImVec4(0.6f, 0.2f, 0.8f, 1.0f));
	DrawButtonBar("RB", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER), ImVec4(1.0f, 0.6f, 0.0f, 1.0f));

	// RT
	float rtValue = static_cast<float>(Input::GetInstance()->GetRightTrigger()) / 255.0f;
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
	ImGui::ProgressBar(rtValue, ImVec2(200, 0), "RT");
	ImGui::PopStyleColor();

	// LT
	float ltValue = static_cast<float>(Input::GetInstance()->GetLeftTrigger()) / 255.0f;
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
	ImGui::ProgressBar(ltValue, ImVec2(200, 0), "LT");
	ImGui::PopStyleColor();

	ImGui::End();
#endif
}