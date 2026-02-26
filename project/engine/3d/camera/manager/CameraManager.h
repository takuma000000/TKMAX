#pragma once
#include <memory>
#include "Camera.h"
#include "DebugCamera.h"

namespace TKM {

	//=============================================================
	// CameraManagerクラス
	// カメラの管理を行うクラス。
	//=============================================================
	class CameraManager {
	public:
		static CameraManager* GetInstance();

		/// <summary>
		/// カメラマネージャーを初期化します。
		/// </summary>
		/// <param name="mainRotate">通常カメラの初期回転（オイラー角）</param>
		/// <param name="mainTranslate">通常カメラの初期平行移動</param>
		/// <param name="debugTarget">デバッグカメラの注視点</param>
		void Initialize(
			const Vector3& mainRotate,
			const Vector3& mainTranslate,
			const Vector3& debugTarget);
		/// <summary>
		/// カメラマネージャーを更新します。
		/// </summary>
		/// <returns></returns>
		Camera* Update();

		/// <summary>
		/// デバッグカメラを使用しているかどうかを取得します。
		/// </summary>
		/// <returns>デバッグカメラを使用している場合 true、それ以外は false</returns>
		bool IsUsingDebugCamera() const;

		// Setter========================================
		/// <summary>
		/// デバッグカメラの使用を切り替えます。
		/// </summary>
		/// <param name="enable">デバッグカメラを使用する場合 true、それ以外は false</param>
		void SetUseDebugCamera(bool enable);
		// ==============================================
		// Getter========================================
		/// <summary>
		/// 現在アクティブなカメラを取得します。
		/// </summary>
		/// <returns>現在アクティブなカメラ</returns>
		Camera* GetActiveCamera() const;
		/// <summary>
		/// 通常カメラを取得します。
		/// </summary>
		/// <returns>通常カメラ</returns>
		Camera* GetMainCamera() const;
		// ==============================================

	private:
		CameraManager() = default;
		~CameraManager() = default;

		//======================================================================
		// 内部データ
		//======================================================================
		std::unique_ptr<Camera> main_ = nullptr;
		std::unique_ptr<DebugCamera> debug_ = nullptr;
		//======================================================================
		// 状態
		//======================================================================
		bool useDebug_ = false; // デバッグカメラ使用フラグ
		//======================================================================
		// キャッシュ
		//======================================================================
		Camera* active_ = nullptr; // Updateのたびに条件分岐してどちらかを返すのは微妙なので、キャッシュしておく
	};
}