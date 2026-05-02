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
// シーンの基本機能を定義する抽象基底クラス
//=============================================================
namespace TKM {
	class BaseScene {
	public:
		//=============================================================
		// 生成・破棄
		//=============================================================

		virtual ~BaseScene();

		//=============================================================
		// 初期化・終了・更新・描画
		//=============================================================

		/// <summary>
		/// シーンを初期化します。
		/// </summary>
		virtual void Initialize() = 0;

		/// <summary>
		/// シーンを終了します。
		/// </summary>
		virtual void Finalize() = 0;

		/// <summary>
		/// シーンを更新します。
		/// </summary>
		virtual void Update() = 0;

		/// <summary>
		/// シーンを描画します。
		/// </summary>
		virtual void Draw() = 0;

		/// <summary>
		/// 3Dオブジェクトを描画します。
		/// </summary>
		virtual void Draw3D();

		/// <summary>
		/// Spriteを描画します。
		/// </summary>
		virtual void DrawSprite();

		/// <summary>
		/// 背景Spriteを描画します。
		/// </summary>
		virtual void DrawBack();

		/// <summary>
		/// シーン開始処理の共通フローを実行します。
		/// Template Methodとして、開始前処理 → 各シーン初期化 → 開始後処理の順番を固定します。
		/// </summary>
		void Start();

		//=============================================================
		// ImGui
		//=============================================================

		/// <summary>
		/// ImGuiでシーン遷移UIを表示します。
		/// </summary>
		void ImGuiSceneChanger();

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// シーンマネージャを設定します。
		/// </summary>
		/// <param name="sceneManager">シーンマネージャ</param>
		virtual void SetSceneManager(TKM::SceneManager* sceneManager) {
			sceneManager_ = sceneManager;
		}

	protected:
		//=============================================================
		// シーン管理
		//=============================================================

		TKM::SceneManager* sceneManager_ = nullptr; // シーンマネージャ

		//=============================================================
		// フレーム情報
		//=============================================================

		float fps_ = 0.0f;         // フレームレート
		float timeCount_ = 0.0f;   // 経過時間
		int frameCount_ = 0;       // フレーム数
		float frameTimeMs_ = 0.0f; // フレーム時間(ms)

		//=============================================================
		// メモリ履歴
		//=============================================================

		static constexpr int kMemoryHistorySize_ = 100;                  // 履歴サイズ
		std::array<float, kMemoryHistorySize_> memoryHistory_{};         // メモリ使用履歴（MB）
		int memoryHistoryIndex_ = 0;                                     // 履歴インデックス

		//=============================================================
		// CPU / GPU履歴
		//=============================================================

		static constexpr int kUsageHistorySize_ = 100;                   // 履歴サイズ
		std::array<float, kUsageHistorySize_> cpuHistory_{};             // CPU使用率履歴（%）
		std::array<float, kUsageHistorySize_> gpuHistory_{};             // GPU使用率履歴（%）
		int usageHistoryIndex_ = 0;                                      // 履歴インデックス

		//=============================================================
		// 使用率情報
		//=============================================================

		float cpuUsagePercent_ = 0.0f;       // CPU使用率（%）
		float gpuUsagePercent_ = 0.0f;       // GPU使用率（%）
		bool gpuCounterAvailable_ = false;   // GPUカウンタ使用可否

		float minFps_ = 9999.0f;             // 最低FPS
		float maxFrameTimeMs_ = 0.0f;        // 最大フレーム時間
		float maxGpuUsagePercent_ = 0.0f;    // 最大GPU使用率
		int maxParticles_ = 0;               // 最大パーティクル数

		//=============================================================
		// CPU計測
		//=============================================================

		ULONGLONG lastCpuCheckTime100ns_ = 0; // 前回CPU計測時刻
		ULONGLONG lastCpuKernel100ns_ = 0;    // 前回CPUカーネル時間
		ULONGLONG lastCpuUser100ns_ = 0;      // 前回CPUユーザー時間

		//=============================================================
		// GPU計測
		//=============================================================

		PDH_HQUERY gpuQuery_ = nullptr;                 // GPU使用率取得用クエリ
		std::vector<PDH_HCOUNTER> gpuCounters_;        // GPUエンジンカウンタ一覧

		//=============================================================
		// パフォーマンス計測・表示
		//=============================================================

		/// <summary>
		/// パフォーマンス情報を更新します。
		/// </summary>
		void UpdatePerformanceInfo();

		/// <summary>
		/// ImGuiでゲームパッド情報を表示します。
		/// </summary>
		void ImGuiDebugGamepad();

		/// <summary>
		/// 音量調節をImGuiで表示します。
		/// </summary>
		void ImGuiAudioControl();

		/// <summary>
		/// メモリ使用量を計測して履歴化します。
		/// </summary>
		void UpdateMemory();

		/// <summary>
		/// ImGuiでデバッグ情報を表示します。
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

		//=============================================================
		// Template Method用フック
		//=============================================================

		/// <summary>
		/// Initialize前に行う共通処理です。
		/// 必要なシーンだけオーバーライドします。
		/// </summary>
		virtual void PreInitialize();

		/// <summary>
		/// Initialize後に行う共通処理です。
		/// 必要なシーンだけオーバーライドします。
		/// </summary>
		virtual void PostInitialize();
	};
}