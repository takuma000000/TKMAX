#pragma once
#include <array>
#include <string>
#include <vector>
#include <Windows.h>
#include <Psapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include "Sprite.h"
#include "Object3d.h"
#include "ParticleManager.h"
#include <Input.h>
#include <Xinput.h>

namespace TKM {
	class SceneManager;
}

//=============================================================
// BaseSceneクラス
// シーンの基本機能を定義する抽象基底クラス。
//=============================================================
namespace TKM {
	class BaseScene {
	public:
		virtual ~BaseScene();
		/// <summary>
		/// </summary>シーンを初期化します。
		/// </summary>
		virtual void Initialize() = 0;
		/// <summary>
		/// </summary>シーンを終了処理します。
		/// </summary>
		virtual void Finalize() = 0;
		/// <summary>
		/// </summary>シーンを更新します。
		/// </summary>
		virtual void Update() = 0;
		/// <summary>
		/// </summary>シーンを描画します。
		/// </summary>
		virtual void Draw() = 0;
		/// <summary>
		/// </summary>3Dオブジェクトを描画します。
		/// </summary>
		virtual void Draw3D();
		/// <summary>
		/// </summary>Spriteを描画します。
		/// </summary>
		virtual void DrawSprite();
		/// <summary>
		/// </summary>背景Spriteを描画します（3Dより先に描かれる）。</summary>
		/// </summary>
		virtual void DrawBack();
		/// <summary>
		/// </summary>DrawCall数を加算します。</summary>
		/// </summary>
		void AddDrawCallCount() { drawCallCount_++; }

		// Setter========================================
		/// <summary>
		/// </summary>シーンマネージャを設定します。
		/// </summary>
		/// <param name="sceneManager"></param>
		virtual void SetSceneManager(TKM::SceneManager* sceneManager) {
			sceneManager_ = sceneManager;
		}
		// ==============================================
	protected:
		// シーンマネージャへのポインタ
		TKM::SceneManager* sceneManager_ = nullptr;

		float fps_ = 0.0f;          // フレームレート
		float timeCount_ = 0.0f;    // 経過時間
		int frameCount_ = 0;        // フレーム数
		float frameTimeMs_ = 0.0f;  // フレームタイム(ms)

		int drawCallCount_ = 0;  // DrawCall数カウント用

		// メモリ履歴
		static constexpr int kMemoryHistorySize_ = 100; // 履歴サイズ
		std::array<float, kMemoryHistorySize_> memoryHistory_{}; // 過去のメモリ使用履歴（MB）
		int memoryHistoryIndex_ = 0; // 履歴インデックス

		// CPU / GPU履歴
		static constexpr int kUsageHistorySize_ = 100; // CPU/GPU履歴サイズ
		std::array<float, kUsageHistorySize_> cpuHistory_{}; // CPU使用率履歴（%）
		std::array<float, kUsageHistorySize_> gpuHistory_{}; // GPU使用率履歴（%）
		int usageHistoryIndex_ = 0; // CPU/GPU履歴インデックス

		float cpuUsagePercent_ = 0.0f; // このプロセスのCPU使用率（%）
		float gpuUsagePercent_ = 0.0f; // このプロセスのGPU使用率（%）
		bool gpuCounterAvailable_ = false; // GPUカウンタが使えるか

		ULONGLONG lastCpuCheckTime100ns_ = 0; // 前回CPU計測時刻（100ns）
		ULONGLONG lastCpuKernel100ns_ = 0;    // 前回CPUカーネル時間（100ns）
		ULONGLONG lastCpuUser100ns_ = 0;      // 前回CPUユーザー時間（100ns）

		PDH_HQUERY gpuQuery_ = nullptr; // GPU使用率取得用クエリ
		std::vector<PDH_HCOUNTER> gpuCounters_; // 対象プロセスのGPUエンジンカウンタ一覧

		/// <summary>
		/// </summary>パフォーマンス情報を更新します。</summary>
		/// </summary>
		void UpdatePerformanceInfo();// TKMAXパフォーマンス可視化
		/// <summary>
		/// </summary>DrawCall数をリセットします。</summary>
		/// </summary>
		void ResetDrawCallCount(); // カウントリセット
		/// <summary>
		/// </summary>ImGuiでゲームパッド情報を表示します。</summary>
		/// </summary>
		void ImGuiDebugGamepad();
		/// <summary>
		/// </summary>メモリ使用量を計測・履歴化します。</summary>
		/// </summary>
		void UpdateMemory();
		/// <summary>
		/// </summary>ImGuiでデバッグ情報を表示します。</summary>
		/// </summary>
		void ImGuiDebugInfo();
		/// <summary>
		/// CPU使用率を更新します。
		/// </summary>
		void UpdateCpuUsage_();
		/// <summary>
		/// GPU使用率を更新します。
		/// </summary>
		void UpdateGpuUsage_();
		/// <summary>
		/// GPUカウンタを初期化します。
		/// </summary>
		void InitializeGpuCounters_();
		/// <summary>
		/// GPUカウンタを解放します。
		/// </summary>
		void FinalizeGpuCounters_();
	};
}