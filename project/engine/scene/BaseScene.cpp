#include "BaseScene.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cwchar>

#pragma comment(lib, "pdh.lib")

#ifdef USE_IMGUI
#include <imgui.h>
#endif

using TKM::Sprite;

namespace TKM {
	BaseScene::~BaseScene() {
		FinalizeGpuCounters_();
	}

	void BaseScene::Initialize() {}

	void BaseScene::Finalize() {}

	void BaseScene::Update() {}

	void BaseScene::Draw() {}

	void BaseScene::Draw3D() {
		Draw(); // デフォルトではDraw3DはDrawを呼び出すだけ。必要に応じて派生クラスでオーバーライドして3D描画処理を実装。
	}

	void BaseScene::DrawSprite() {}

	void BaseScene::DrawBack() {}

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

		UpdateCpuUsage_();
		UpdateGpuUsage_();

		if (fps_ > 0.0f && fps_ < minFps_) {
			minFps_ = fps_;
		}
		if (frameTimeMs_ > maxFrameTimeMs_) {
			maxFrameTimeMs_ = frameTimeMs_;
		}
		if (gpuUsagePercent_ > maxGpuUsagePercent_) {
			maxGpuUsagePercent_ = gpuUsagePercent_;
		}

		int totalParticles = 0;
		for (const auto& pair : TKM::ParticleManager::GetInstance()->GetParticleGroups()) {
			totalParticles += static_cast<int>(pair.second.particles_.size());
		}
		if (totalParticles > maxParticles_) {
			maxParticles_ = totalParticles;
		}

		cpuHistory_[usageHistoryIndex_] = cpuUsagePercent_;
		gpuHistory_[usageHistoryIndex_] = gpuUsagePercent_;
		usageHistoryIndex_ = (usageHistoryIndex_ + 1) % kUsageHistorySize_;
#endif
	}

	void BaseScene::UpdateMemory() {
#ifdef USE_IMGUI
		PROCESS_MEMORY_COUNTERS pmc{};
		if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
			float memoryUsageMB = static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
			memoryHistory_[memoryHistoryIndex_] = memoryUsageMB;
			memoryHistoryIndex_ = (memoryHistoryIndex_ + 1) % kMemoryHistorySize_;
		}
#endif
	}

	void BaseScene::ImGuiDebugInfo() {
#ifdef USE_IMGUI
		ImGui::Begin("情報");

		// =========================================================
		// 現在の重要情報
		// =========================================================
		ImGui::SeparatorText("現在の状態");

		ImGui::Text("FPS : %.2f", fps_);

		if (frameTimeMs_ >= 25.0f) {
			ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "フレーム時間 : %.2f ms", frameTimeMs_);
		} else if (frameTimeMs_ >= 20.0f) {
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "フレーム時間 : %.2f ms", frameTimeMs_);
		} else {
			ImGui::Text("フレーム時間 : %.2f ms", frameTimeMs_);
		}

		ImGui::Text("CPU使用率 : %.2f %%", cpuUsagePercent_);

		if (gpuCounterAvailable_) {
			if (gpuUsagePercent_ >= 90.0f) {
				ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "GPU使用率 : %.2f %%", gpuUsagePercent_);
			} else if (gpuUsagePercent_ >= 80.0f) {
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "GPU使用率 : %.2f %%", gpuUsagePercent_);
			} else {
				ImGui::Text("GPU使用率 : %.2f %%", gpuUsagePercent_);
			}
		} else {
			ImGui::Text("GPU使用率 : 取得不可");
		}

		PROCESS_MEMORY_COUNTERS pmc{};
		if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
			size_t memoryUsageKB = pmc.WorkingSetSize / 1024;
			size_t memoryUsageMB = memoryUsageKB / 1024;
			ImGui::Text("メモリ使用量 : %zu KB / %zu MB", memoryUsageKB, memoryUsageMB);
		}

		// =========================================================
		// ピーク値
		// =========================================================
		ImGui::SeparatorText("ピーク値");

		ImGui::Text("最低FPS : %.2f", (minFps_ == 9999.0f) ? 0.0f : minFps_);
		ImGui::Text("最大フレーム時間 : %.2f ms", maxFrameTimeMs_);
		ImGui::Text("最大GPU使用率 : %.2f %%", maxGpuUsagePercent_);
		ImGui::Text("最大Particles : %d", maxParticles_);

		if (ImGui::Button("ピーク値リセット")) {
			minFps_ = 9999.0f;
			maxFrameTimeMs_ = 0.0f;
			maxGpuUsagePercent_ = 0.0f;
			maxParticles_ = 0;
		}

		// =========================================================
		// 履歴グラフ
		// =========================================================
		ImGui::SeparatorText("履歴");

		ImGui::Text("メモリ推移 (MB)");
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PlotLines(
			"##MemoryHistory",
			memoryHistory_.data(),
			kMemoryHistorySize_,
			memoryHistoryIndex_,
			nullptr,
			0.0f,
			500.0f,
			ImVec2(0, 100)
		);
		ImGui::PopStyleColor();

		ImGui::Text("CPU使用率推移 (%%)");
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
		ImGui::PlotLines(
			"##CpuHistory",
			cpuHistory_.data(),
			kUsageHistorySize_,
			usageHistoryIndex_,
			nullptr,
			0.0f,
			100.0f,
			ImVec2(0, 80)
		);
		ImGui::PopStyleColor();

		ImGui::Text("GPU使用率推移 (%%)");
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
		ImGui::PlotLines(
			"##GpuHistory",
			gpuHistory_.data(),
			kUsageHistorySize_,
			usageHistoryIndex_,
			nullptr,
			0.0f,
			100.0f,
			ImVec2(0, 80)
		);
		ImGui::PopStyleColor();

		// =========================================================
		// 現在数
		// =========================================================
		ImGui::SeparatorText("現在数");

		ImGui::Text("アクティブ Sprite 数 : %d", Sprite::GetActiveCount());
		ImGui::Text("アクティブ Object3D 数 : %d", TKM::Object3d::GetActiveCount());

		int totalParticles = 0;
		std::vector<std::pair<std::string, int>> particleCounts;
		particleCounts.reserve(TKM::ParticleManager::GetInstance()->GetParticleGroups().size());

		for (const auto& pair : TKM::ParticleManager::GetInstance()->GetParticleGroups()) {
			int count = static_cast<int>(pair.second.particles_.size());
			totalParticles += count;
			if (count > 0) {
				particleCounts.emplace_back(pair.first, count);
			}
		}

		if (totalParticles >= 800) {
			ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "アクティブ Particles : %d", totalParticles);
		} else if (totalParticles >= 600) {
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "アクティブ Particles : %d", totalParticles);
		} else {
			ImGui::Text("アクティブ Particles : %d", totalParticles);
		}

		ImGui::Text("パーティクルグループ数 : %d", static_cast<int>(TKM::ParticleManager::GetInstance()->GetParticleGroups().size()));

		// =========================================================
		// パーティクル内訳 上位
		// =========================================================
		ImGui::SeparatorText("Particles内訳 上位");

		std::sort(
			particleCounts.begin(),
			particleCounts.end(),
			[](const auto& a, const auto& b) {
				return a.second > b.second;
			}
		);

		if (particleCounts.empty()) {
			ImGui::Text("生きているパーティクルはありません");
		} else {
			const int maxShow = 8;
			int showCount = (std::min)(maxShow, static_cast<int>(particleCounts.size()));

			for (int i = 0; i < showCount; ++i) {
				ImGui::Text("%s : %d", particleCounts[i].first.c_str(), particleCounts[i].second);
			}
		}

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
		DrawButtonBar("A", TKM::Input::GetInstance()->PushButton(XINPUT_GAMEPAD_A), ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
		DrawButtonBar("B", TKM::Input::GetInstance()->PushButton(XINPUT_GAMEPAD_B), ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		DrawButtonBar("X", TKM::Input::GetInstance()->PushButton(XINPUT_GAMEPAD_X), ImVec4(0.0f, 0.4f, 1.0f, 1.0f));
		DrawButtonBar("Y", TKM::Input::GetInstance()->PushButton(XINPUT_GAMEPAD_Y), ImVec4(1.0f, 0.4f, 0.7f, 1.0f));
		DrawButtonBar("Start", TKM::Input::GetInstance()->PushButton(XINPUT_GAMEPAD_START), ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
		DrawButtonBar("Back", TKM::Input::GetInstance()->PushButton(XINPUT_GAMEPAD_BACK), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		DrawButtonBar("LB", TKM::Input::GetInstance()->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER), ImVec4(0.6f, 0.2f, 0.8f, 1.0f));
		DrawButtonBar("RB", TKM::Input::GetInstance()->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER), ImVec4(1.0f, 0.6f, 0.0f, 1.0f));

		// RT
		float rtValue = static_cast<float>(TKM::Input::GetInstance()->GetRightTrigger()) / 255.0f;
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
		ImGui::ProgressBar(rtValue, ImVec2(200, 0), "RT");
		ImGui::PopStyleColor();

		// LT
		float ltValue = static_cast<float>(TKM::Input::GetInstance()->GetLeftTrigger()) / 255.0f;
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
		ImGui::ProgressBar(ltValue, ImVec2(200, 0), "LT");
		ImGui::PopStyleColor();

		ImGui::End();
#endif
	}

	void BaseScene::UpdateCpuUsage_() {
#ifdef USE_IMGUI
		FILETIME createTime{};
		FILETIME exitTime{};
		FILETIME kernelTime{};
		FILETIME userTime{};

		if (!GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime)) {
			cpuUsagePercent_ = 0.0f;
			return;
		}

		FILETIME nowFileTime{};
		GetSystemTimeAsFileTime(&nowFileTime);

		ULARGE_INTEGER now{};
		now.LowPart = nowFileTime.dwLowDateTime;
		now.HighPart = nowFileTime.dwHighDateTime;

		ULARGE_INTEGER kernel{};
		kernel.LowPart = kernelTime.dwLowDateTime;
		kernel.HighPart = kernelTime.dwHighDateTime;

		ULARGE_INTEGER user{};
		user.LowPart = userTime.dwLowDateTime;
		user.HighPart = userTime.dwHighDateTime;

		if (lastCpuCheckTime100ns_ == 0) {
			lastCpuCheckTime100ns_ = now.QuadPart;
			lastCpuKernel100ns_ = kernel.QuadPart;
			lastCpuUser100ns_ = user.QuadPart;
			cpuUsagePercent_ = 0.0f;
			return;
		}

		const ULONGLONG elapsedTime = now.QuadPart - lastCpuCheckTime100ns_;
		const ULONGLONG elapsedKernel = kernel.QuadPart - lastCpuKernel100ns_;
		const ULONGLONG elapsedUser = user.QuadPart - lastCpuUser100ns_;
		const ULONGLONG elapsedCpu = elapsedKernel + elapsedUser;

		lastCpuCheckTime100ns_ = now.QuadPart;
		lastCpuKernel100ns_ = kernel.QuadPart;
		lastCpuUser100ns_ = user.QuadPart;

		if (elapsedTime == 0) {
			return;
		}

		SYSTEM_INFO sysInfo{};
		GetSystemInfo(&sysInfo);

		DWORD cpuCount = sysInfo.dwNumberOfProcessors;
		if (cpuCount == 0) {
			cpuCount = 1;
		}

		double usage = (static_cast<double>(elapsedCpu) / static_cast<double>(elapsedTime))
			/ static_cast<double>(cpuCount) * 100.0;

		if (usage < 0.0) {
			usage = 0.0;
		}
		if (usage > 100.0) {
			usage = 100.0;
		}

		cpuUsagePercent_ = static_cast<float>(usage);
#endif
	}

	void BaseScene::InitializeGpuCounters_() {
#ifdef USE_IMGUI
		FinalizeGpuCounters_();

		if (PdhOpenQuery(nullptr, 0, &gpuQuery_) != ERROR_SUCCESS) {
			gpuQuery_ = nullptr;
			gpuCounterAvailable_ = false;
			return;
		}

		const DWORD pid = GetCurrentProcessId();

		DWORD counterListSize = 0;
		DWORD instanceListSize = 0;
		PDH_STATUS status = PdhEnumObjectItemsW(
			nullptr,
			nullptr,
			L"GPU Engine",
			nullptr,
			&counterListSize,
			nullptr,
			&instanceListSize,
			PERF_DETAIL_WIZARD,
			0
		);

		if (instanceListSize == 0) {
			FinalizeGpuCounters_();
			return;
		}

		std::vector<wchar_t> counterList(counterListSize);
		std::vector<wchar_t> instanceList(instanceListSize);

		status = PdhEnumObjectItemsW(
			nullptr,
			nullptr,
			L"GPU Engine",
			counterList.data(),
			&counterListSize,
			instanceList.data(),
			&instanceListSize,
			PERF_DETAIL_WIZARD,
			0
		);

		if (status != ERROR_SUCCESS) {
			FinalizeGpuCounters_();
			return;
		}

		const std::wstring pidText = L"pid_" + std::to_wstring(pid);

		for (const wchar_t* instance = instanceList.data(); *instance != L'\0'; instance += std::wcslen(instance) + 1) {
			std::wstring instanceName = instance;

			if (instanceName.find(pidText) == std::wstring::npos) {
				continue;
			}

			std::wstring counterPath = L"\\GPU Engine(" + instanceName + L")\\Utilization Percentage";

			PDH_HCOUNTER counter = nullptr;
			if (PdhAddEnglishCounterW(gpuQuery_, counterPath.c_str(), 0, &counter) == ERROR_SUCCESS) {
				gpuCounters_.push_back(counter);
			}
		}

		if (gpuCounters_.empty()) {
			FinalizeGpuCounters_();
			return;
		}

		PdhCollectQueryData(gpuQuery_);
		gpuCounterAvailable_ = true;
#endif
	}

	void BaseScene::FinalizeGpuCounters_() {
#ifdef USE_IMGUI
		gpuCounters_.clear();

		if (gpuQuery_) {
			PdhCloseQuery(gpuQuery_);
			gpuQuery_ = nullptr;
		}

		gpuCounterAvailable_ = false;
		gpuUsagePercent_ = 0.0f;
#endif
	}

	void BaseScene::UpdateGpuUsage_() {
#ifdef USE_IMGUI
		if (!gpuQuery_ && !gpuCounterAvailable_) {
			InitializeGpuCounters_();
		}

		if (!gpuCounterAvailable_ || !gpuQuery_) {
			gpuUsagePercent_ = 0.0f;
			return;
		}

		if (PdhCollectQueryData(gpuQuery_) != ERROR_SUCCESS) {
			gpuUsagePercent_ = 0.0f;
			return;
		}

		double totalUsage = 0.0;

		for (PDH_HCOUNTER counter : gpuCounters_) {
			PDH_FMT_COUNTERVALUE value{};
			if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS) {
				if (value.CStatus == ERROR_SUCCESS) {
					totalUsage += value.doubleValue;
				}
			}
		}

		if (totalUsage < 0.0) {
			totalUsage = 0.0;
		}
		if (totalUsage > 100.0) {
			totalUsage = 100.0;
		}

		gpuUsagePercent_ = static_cast<float>(totalUsage);
#endif
	}
}