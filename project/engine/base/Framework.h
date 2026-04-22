#pragma once
#include <memory>
#include <ImGuiManager.h>

#include "SceneManager.h"
#include "AbstractSceneFactory.h"

// 前方宣言
class WindowsAPI;
namespace TKM {
	class DirectXCommon;
	class SrvManager;
	class Input;
	class ImGuiManager;
}

namespace TKM {

	//=============================================================
	// Frameworkクラス
	// ゲーム全体の処理を統括する基盤クラス
	//=============================================================
	class Framework {
	public:
		//=============================================================
		// 生成・破棄
		//=============================================================

		virtual ~Framework() = default;

		//=============================================================
		// 初期化・終了
		//=============================================================

		/// <summary>
		/// 初期化します。
		/// </summary>
		virtual void Initialize();

		/// <summary>
		/// 終了します。
		/// </summary>
		virtual void Finalize();

		//=============================================================
		// 更新・描画・実行
		//=============================================================

		/// <summary>
		/// 毎フレーム更新します。
		/// </summary>
		virtual void Update();

		/// <summary>
		/// 毎フレーム描画します。
		/// </summary>
		virtual void Draw();

		/// <summary>
		/// フレームワークを実行します。
		/// </summary>
		void Run();

		//=============================================================
		// 状態取得
		//=============================================================

		/// <summary>
		/// 終了要求中かを返します。
		/// </summary>
		virtual bool IsEndRequest() { return endRequest_; }

		//=============================================================
		// Getter

		/// <summary>
		/// WindowsAPIを取得します。
		/// </summary>
		WindowsAPI* GetWindowsAPI() const { return windowsAPI_.get(); }

		/// <summary>
		/// DirectX共通管理を取得します。
		/// </summary>
		TKM::DirectXCommon* GetDirectXCommon() const { return dxCommon_.get(); }

		/// <summary>
		/// SRV管理を取得します。
		/// </summary>
		SrvManager* GetSrvManager() const { return srvManager_.get(); }

		//=============================================================

		//=============================================================
		// Setter

		/// <summary>
		/// 終了要求フラグを設定します。
		/// </summary>
		void SetEndRequest(bool endRequest) { endRequest_ = endRequest; }

		//=============================================================

	protected:
		//=============================================================
		// 状態
		//=============================================================

		bool endRequest_ = false; // 終了要求フラグ

		//=============================================================
		// 共通管理
		//=============================================================

		std::unique_ptr<WindowsAPI> windowsAPI_;                 // Windows管理
		std::unique_ptr<TKM::DirectXCommon> dxCommon_;          // DirectX共通管理
		std::unique_ptr<SrvManager> srvManager_;                // SRV管理
		std::unique_ptr<TKM::ImGuiManager> imguiManager_ = nullptr; // ImGui管理

		//=============================================================
		// シーン生成
		//=============================================================

		std::unique_ptr<AbstractSceneFactory> sceneFactory_ = nullptr; // シーンファクトリー

	private:
		//=============================================================
		// シーン管理
		//=============================================================

		std::unique_ptr<TKM::SceneManager> sceneManager_ = nullptr; // シーン管理
	};
}