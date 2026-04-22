#pragma once
#include "BaseScene.h"
#include "DirectXCommon.h"
#include "AbstractSceneFactory.h"
#include <memory>
#include <string>

//=============================================================
// SceneManagerクラス
// シーンの管理を行うクラス
//=============================================================
namespace TKM {
	class SceneManager {
	public:
		//=============================================================
		// 生成・破棄
		//=============================================================

		/// <summary>
		/// SceneManagerを破棄します。
		/// </summary>
		~SceneManager();

		//=============================================================
		// 更新・描画
		//=============================================================

		/// <summary>
		/// SceneManagerを更新します。
		/// </summary>
		void Update();

		/// <summary>
		/// SceneManagerを描画します。
		/// </summary>
		void Draw();

		/// <summary>
		/// 3Dオブジェクトを描画します。
		/// </summary>
		void Draw3D();

		/// <summary>
		/// Spriteを描画します。
		/// </summary>
		void DrawSprite();

		//=============================================================
		// シーン制御
		//=============================================================

		/// <summary>
		/// アプリ終了を要求します。
		/// </summary>
		void RequestQuit() { quitRequested_ = true; }

		/// <summary>
		/// 終了要求中かを取得します。
		/// </summary>
		/// <returns>終了要求中ならtrue</returns>
		bool IsQuitRequested() const { return quitRequested_; }

		/// <summary>
		/// シーン名を指定して次のシーンへ切り替えます。
		/// </summary>
		/// <param name="sceneName">切り替え先のシーン名</param>
		void ChangeScene(const std::string& sceneName);

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// シーンファクトリーを設定します。
		/// </summary>
		/// <param name="sceneFactory">シーンファクトリー</param>
		void SetSceneFactory(TKM::AbstractSceneFactory* sceneFactory) {
			sceneFactory_ = sceneFactory;
		}

		/// <summary>
		/// 次のシーンを設定します。
		/// </summary>
		/// <param name="nextScene">次に切り替えるシーン</param>
		void SetNextScene(std::unique_ptr<TKM::BaseScene> nextScene) {
			nextScene_ = std::move(nextScene);
		}

	private:
		//=============================================================
		// シーン
		//=============================================================

		std::unique_ptr<TKM::BaseScene> scene_ = nullptr;     // 現在のシーン
		std::unique_ptr<TKM::BaseScene> nextScene_ = nullptr; // 次のシーン

		//=============================================================
		// 共通参照
		//=============================================================

		TKM::DirectXCommon* dxCommon_ = nullptr;          // DirectX共通管理
		TKM::AbstractSceneFactory* sceneFactory_ = nullptr; // シーンファクトリー

		//=============================================================
		// 状態
		//=============================================================

		bool quitRequested_ = false; // アプリ終了要求フラグ
	};
}