#pragma once
#include "Framework.h"
#include "Input.h"
#include "Object3dCommon.h"
#include "camera/Camera.h"
#include "ImGuiManager.h"
#include "SceneFactory.h"
#include "AbstractSceneFactory.h"
#include "SceneManager.h"
#include <memory>

//=============================================================
// MyGameクラス
// ゲーム全体を管理するクラス。
//=============================================================
namespace TKM {
	class MyGame : public TKM::Framework {
	public://メンバ関数
		/// <summary>
		/// シーンを初期化します。
		/// </summary>
		void Initialize() override;
		/// <summary>
		/// シーンを終了します。
		/// </summary>
		void Finalize() override;
		/// <summary>
		/// 毎フレーム更新を行う関数。
		/// </summary>
		void Update() override;
		/// <summary>
		/// 毎フレーム描画を行う関数。
		/// </summary>
		void Draw() override;

	private:
		D3D12_VIEWPORT viewport_;
		D3D12_RECT scissorRect_;

		bool endRequest_ = false; // 終了フラグ
		std::unique_ptr<TKM::SceneManager> sceneManager_ = nullptr; // シーンマネージャー

	};
}