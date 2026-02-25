#pragma once
#include "BaseScene.h"
#include "DirectXCommon.h"
#include "AbstractSceneFactory.h"

//=============================================================
// SceneManagerクラス
// シーンの管理を行うクラス。
//=============================================================
namespace TKM {
	class SceneManager {
	public://メンバ関数
		/// <summary>
		/// </span class="code-inline">SceneManager</span>のコンストラクタ
		/// </summary>
		void Update();
		/// <summary>
		/// </span class="code-inline">SceneManager</span>の描画
		/// </summary>
		void Draw();
		/// <summary>
		/// 3Dオブジェクトの描画
		/// </summary>
		void Draw3D();
		/// <summary>
		/// Spriteの描画
		/// </summary>
		void DrawSprite();

		/// <summary>
		/// </span class="code-inline">SceneManager</span>のデストラクタ
		/// </summary>
		~SceneManager();

		/// <summary>
		/// アプリ終了を要求します
		/// </summary>
		void RequestQuit() { quitRequested_ = true; }
		/// <summary>
		/// 終了要求が出ているか
		/// </summary>
		bool IsQuitRequested() const { return quitRequested_; }

		// Setter========================================
		/// <summary>
		/// </span class="code-inline">SceneManager</span>のDirectXCommonセット
		/// </summary>
		/// <param name="sceneFactory"></param>
		void SetSceneFactory(TKM::AbstractSceneFactory* sceneFactory) {
			sceneFactory_ = sceneFactory;
		}
		/// <summary>
		/// </span class="code-inline">SceneManager</span>のコンストラクタ
		/// </summary>
		/// <param name="nextScene"></param>
		void SetNextScene(TKM::BaseScene* nextScene) {
			nextScene_ = nextScene;
		}
		// ==============================================

	private:
		//今のシーン( 実行中 )
		TKM::BaseScene* scene_ = nullptr;

		//次のシーン( 次フレームから実行 )
		TKM::BaseScene* nextScene_ = nullptr;

		TKM::DirectXCommon* dxCommon_ = nullptr;

		//シーンファクトリー
		TKM::AbstractSceneFactory* sceneFactory_ = nullptr;

		bool quitRequested_ = false; // アプリ終了要求フラグ
	};
}