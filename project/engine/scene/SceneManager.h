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
	public:
		//次シーン予約
		/// <summary>次のシーンを設定します。</summary>
		void SetNextScene(TKM::BaseScene* nextScene) {
			nextScene_ = nextScene;
		}

		//メンバ関数
		//シーンファクトリー
		/// <summary>シーンファクトリーを設定します。</summary>
		void SetSceneFactory(TKM::AbstractSceneFactory* sceneFactory) {
			sceneFactory_ = sceneFactory;
		}

	private:

		//今のシーン( 実行中 )
		TKM::BaseScene* scene_ = nullptr;

		//次のシーン( 次フレームから実行 )
		TKM::BaseScene* nextScene_ = nullptr;

		TKM::DirectXCommon* dxCommon = nullptr;

		//シーンファクトリー
		TKM::AbstractSceneFactory* sceneFactory_ = nullptr;

	public://メンバ関数
		/// <summary>
		/// </span class="code-inline">SceneManager</span>のコンストラクタ
		/// </summary>
		void Update();
		/// <summary>
		/// </span class="code-inline">SceneManager</span>の描画
		/// </summary>
		void Draw();

		//デストラクタ
		///<summary>
		/// </span class="code-inline">SceneManager</span>のデストラクタ
		/// </summary>
		~SceneManager();
	};
}