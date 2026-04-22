#pragma once
#include "Framework.h"
#include "Input.h"
#include "Object3dCommon.h"
#include "Camera.h"
#include "ImGuiManager.h"
#include "SceneFactory.h"
#include "AbstractSceneFactory.h"
#include "SceneManager.h"
#include <memory>

//=============================================================
// MyGameクラス
// ゲーム全体を管理するクラス
//=============================================================
namespace TKM {
	class MyGame : public TKM::Framework {
	public:
		//=============================================================
		// 初期化・終了
		//=============================================================

		/// <summary>
		/// ゲームを初期化します。
		/// </summary>
		void Initialize() override;

		/// <summary>
		/// ゲームを終了します。
		/// </summary>
		void Finalize() override;

		//=============================================================
		// 更新・描画
		//=============================================================

		/// <summary>
		/// 毎フレーム更新します。
		/// </summary>
		void Update() override;

		/// <summary>
		/// 毎フレーム描画します。
		/// </summary>
		void Draw() override;

	private:
		//=============================================================
		// 描画設定
		//=============================================================

		D3D12_VIEWPORT viewport_{};   // ビューポート
		D3D12_RECT scissorRect_{};    // シザー矩形

		//=============================================================
		// 状態・管理
		//=============================================================

		bool endRequest_ = false;                                   // 終了フラグ
		std::unique_ptr<TKM::SceneManager> sceneManager_ = nullptr; // シーンマネージャ
	};
}