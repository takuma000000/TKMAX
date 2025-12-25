#include "BaseScene.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

using TKM::Sprite;

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

void BaseScene::UpdateMemory() {
#ifdef USE_IMGUI
	PROCESS_MEMORY_COUNTERS pmc{};
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		float memoryUsageMB = static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
		memoryHistory_[memoryHistoryIndex_] = memoryUsageMB;
		memoryHistoryIndex_ = (memoryHistoryIndex_ + 1) % kMemoryHistorySize;
	}
#endif
}

void BaseScene::ResetDrawCallCount() {
	drawCallCount_ = 0; // DrawCall数リセット
}

void BaseScene::ImGuiDebugInfo() {
#ifdef USE_IMGUI
	ImGui::Begin("情報");
	ImGui::Text("FPS : %.2f", fps_);
	ImGui::Separator();
	ImGui::Text("フレーム時間 : %.2f ms", frameTimeMs_);
	ImGui::Separator();
	ImGui::Text("DrawCall 回数 : %d", drawCallCount_);
	ImGui::Separator();

	// メモリ使用量（KB/MB表記）
	PROCESS_MEMORY_COUNTERS pmc{};
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		size_t memoryUsageKB = pmc.WorkingSetSize / 1024;      // KB
		size_t memoryUsageMB = memoryUsageKB / 1024;           // MB
		ImGui::Text("メモリ使用量 : %zu KB / %zu MB", memoryUsageKB, memoryUsageMB);
	}

	ImGui::Text("MB");
	ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤色
	ImGui::PlotLines(
		"メモリ推移",
		memoryHistory_.data(),
		kMemoryHistorySize,
		memoryHistoryIndex_,
		nullptr,
		0.0f,
		500.0f,
		ImVec2(0, 150)
	);
	ImGui::PopStyleColor();
	ImGui::Separator();

	ImGui::Text("アクティブ Sprite 数 : %d", Sprite::GetActiveCount());
	ImGui::Text("アクティブ Object3D 数 : %d", TKM::Object3d::GetActiveCount());
	ImGui::Separator();

	int totalParticles = 0;
	for (const auto& pair : TKM::ParticleManager::GetInstance()->GetParticleGroups()) {
		totalParticles += static_cast<int>(pair.second.particles.size());
	}
	ImGui::Text("アクティブ Particles: %d", totalParticles);
	ImGui::Text("パーティクルグループ数: %d", TKM::ParticleManager::GetInstance()->GetParticleGroups().size());
	ImGui::End();
#endif
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