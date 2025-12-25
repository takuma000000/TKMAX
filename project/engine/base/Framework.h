#pragma once
#include <memory>
#include <ImGuiManager.h>

#include "SceneManager.h"
#include "AbstractSceneFactory.h"

//前方宣言
class WindowsAPI;
namespace TKM {
	class DirectXCommon;
}
class SrvManager;
class Input;
class ImGuiManager;

//=============================================================
// Frameworkクラス
// ゲーム全体の初期化・更新・描画・終了処理を統括する基盤クラス。
//=============================================================
namespace TKM {
	class Framework {
	public:
		virtual ~Framework() = default; // 仮想デストラクタ

		/// <summary>
		/// 初期化を行う関数。
		/// </summary>
		virtual void Initialize();   // 初期化
		/// <summary>
		/// 終了処理を行う関数。
		/// </summary>
		virtual void Finalize();     // 終了
		/// <summary>
		/// 毎フレーム更新を行う関数。
		/// </summary>
		virtual void Update();       // 毎フレーム更新
		/// <summary>
		/// 毎フレーム描画を行う関数。
		/// </summary>
		virtual void Draw();         // 描画
		/// <summary>
		/// 終了フラグの取得を行う関数。
		/// </summary>
		/// <returns></returns>
		virtual bool IsEndRequest() { return endRequest_; }
		/// <summary>
		/// フレームワークの実行を行う関数。
		/// </summary>
		void Run();

		// Getter========================================
		/// <summary>
		/// WindowsAPIのゲッター。
		/// </summary>
		/// <returns></returns>
		WindowsAPI* GetWindowsAPI() const { return windowsAPI.get(); }
		/// <summary>
		/// DirectXCommonのゲッター。
		/// </summary>
		/// <returns></returns>
		TKM::DirectXCommon* GetDirectXCommon() const { return dxCommon.get(); }
		/// <summary>
		/// SrvManagerのゲッター。
		/// </summary>
		/// <returns></returns>
		SrvManager* GetSrvManager() const { return srvManager.get(); }
		// ==============================================
		// Setter========================================
		/// <summary>
		/// 終了フラグの設定を行う関数。
		/// </summary>
		/// <param name="endRequest"></param>
		void SetEndRequest(bool endRequest) { endRequest_ = endRequest; } // 終了フラグを設定する
		// ==============================================
	protected:
		bool endRequest_ = false;    // 終了フラグ

		// 汎用メンバ変数
		std::unique_ptr<WindowsAPI> windowsAPI;
		std::unique_ptr<TKM::DirectXCommon> dxCommon;
		std::unique_ptr<SrvManager> srvManager;

		//ポインタ...ImGuiManager
		std::unique_ptr<ImGuiManager>  imguiManager = nullptr;

		//シーンファクトリー
		std::unique_ptr<AbstractSceneFactory> sceneFactory_ = nullptr;

	private:
		std::unique_ptr<SceneManager> sceneManager_ = nullptr;

	};
}