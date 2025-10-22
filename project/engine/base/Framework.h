#pragma once
#include <memory>
#include <ImGuiManager.h>

#include "SceneManager.h"
#include "AbstractSceneFactory.h"


//前方宣言
class WindowsAPI;
class DirectXCommon;
class SrvManager;
class Input;
class ImGuiManager;

//=============================================================
// Frameworkクラス
// ゲーム全体の初期化・更新・描画・終了処理を統括する基盤クラス。
//=============================================================
class Framework
{
public:
	// 仮想関数（派生クラスで必ず実装する必要がある）
	/// <summary>初期化を行う関数。</summary>
	virtual void Initialize();   // 初期化
	/// <summary>終了処理を行う関数。</summary>
	virtual void Finalize();     // 終了
	/// <summary>毎フレーム更新を行う関数。</summary>
	virtual void Update();       // 毎フレーム更新
	/// <summary>毎フレーム描画を行う関数。</summary>
	virtual void Draw();         // 描画

	// ゲーム終了のチェック
	/// <summary>終了要求があるかを取得する関数。</summary>
	virtual bool IsEndRequest() { return endRequest_; }

	///<summary>デストラクタ。</summary>
	virtual ~Framework() = default;

	//実行
	///<summary>フレームワークの実行を行う関数。</summary>
	void Run();

	// 初期化した共通機能を派生クラスで使えるようにするためのアクセサ
	//// <summary>WindowsAPIのゲッター。</summary>
	WindowsAPI* GetWindowsAPI() const { return windowsAPI.get(); }
	/// <summary>DirectXCommonのゲッター。</summary>
	DirectXCommon* GetDirectXCommon() const { return dxCommon.get(); }
	/// <summary>SrvManagerのゲッター。</summary>
	SrvManager* GetSrvManager() const { return srvManager.get(); }

public:
	///<summary>終了フラグを設定する関数。</summary>
	void SetEndRequest(bool endRequest) { endRequest_ = endRequest; } // 終了フラグを設定する
protected:
	bool endRequest_ = false;    // 終了フラグ

	// 汎用メンバ変数
	std::unique_ptr<WindowsAPI> windowsAPI;
	std::unique_ptr<DirectXCommon> dxCommon;
	std::unique_ptr<SrvManager> srvManager;

	//ポインタ...ImGuiManager
	std::unique_ptr<ImGuiManager>  imguiManager = nullptr;

	//シーンファクトリー
	std::unique_ptr<AbstractSceneFactory> sceneFactory_ = nullptr;

private:
	std::unique_ptr<SceneManager> sceneManager_ = nullptr;

};

