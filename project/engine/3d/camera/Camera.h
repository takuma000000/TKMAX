#pragma once
#include "MyMath.h"
#include "WindowsAPI.h"
#include "Transform.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

//=============================================================
// Cameraクラス
// ビュー行列と射影行列を扱うカメラクラス。
//=============================================================
namespace TKM {
	class Camera {
	public://メンバ関数

		/// <summary>
		/// Cameraを初期化します。
		/// </summary>
		Camera();

		/// <summary>
		/// カメラを更新します。
		/// </summary>
		void Update();
		/// <summary>
		/// ImGuiデバッグ表示。
		/// </summary>
		void ImGuiDebug();

		// Setter=====================================
		/// <summary>
		/// 回転の設定。
		/// </summary>
		/// <param name="rotate"></param>
		void SetRotate(const Vector3& rotate) { this->transform_.rotate_ = rotate; }
		/// <summary>
		/// スケールの設定。
		/// </summary>
		/// <param name="scale"></param>
		void SetScale(const Vector3& scale) { this->transform_.scale_ = scale; }
		/// <summary>
		/// 平行移動の設定。
		/// </summary>
		/// <param name="translate"></param>
		void SetTranslate(const Vector3& translate) { this->transform_.translate_ = translate; }
		/// <summary>
		/// 垂直方向視野角の設定。
		/// </summary>
		/// <param name="horizontal"></param>
		void SetFovY(float horizontal) { this->fovY_ = horizontal; }
		/// <summary>
		/// アスペクト比の設定。
		/// </summary>
		/// <param name="aspect"></param>
		void SetAspectRatio(float aspect) { this->aspectRatio_ = aspect; }
		/// <summary>
		/// ニアクリップ距離の設定。
		/// </summary>
		/// <param name="nearClip"></param>
		void SetNearClip(float nearClip) { this->nearClip_ = nearClip; }
		/// <summary>
		/// ファークリップ距離の設定。
		/// </summary>
		/// <param name="farClip"></param>
		void SetFarClip(float farClip) { this->farClip_ = farClip; }
		// Getter=====================================
		/// <summary>
		/// ワールド行列の取得。
		/// </summary>
		/// <returns></returns>
		const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
		/// <summary>
		/// ビュー行列の取得。
		/// </summary>
		/// <returns></returns>
		const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
		/// <summary>
		/// 射影行列の取得。
		/// </summary>
		/// <returns></returns>
		const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
		/// <summary>
		/// ビュー射影行列の取得。
		/// </summary>
		/// <returns></returns>
		const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }
		/// <summary>
		/// 回転の取得。
		/// </summary>
		/// <returns></returns>
		const Vector3& GetRotate() const { return transform_.rotate_; }
		/// <summary>
		/// 平行移動の取得。
		/// </summary>
		/// <returns></returns>
		const Vector3& GetTranslate() const { return transform_.translate_; }
		/// <summary>
		/// スケールの取得。
		/// </summary>
		/// <returns></returns>
		const Vector3& GetScale() const { return transform_.scale_; }
		// ===========================================

	private:
		//======================================================================
		// Transform（カメラの位置・回転）
		//======================================================================
		Transform transform_; // カメラの変換情報
		//======================================================================
		// World / View
		//======================================================================
		Matrix4x4 worldMatrix_; // ワールド行列（カメラの位置・回転を反映した行列）
		Matrix4x4 viewMatrix_; // ビュー行列
		//======================================================================
		// Projection（射影パラメータ）
		//======================================================================
		float fovY_;        //水平方向視野角
		float aspectRatio_; //アスペクト比
		float nearClip_;    //ニアクリップ距離
		float farClip_;     //ファークリップ距離
		Matrix4x4 projectionMatrix_; // 射影行列
		//======================================================================
		// ViewProjection（最終描画用）
		//======================================================================
		Matrix4x4 viewProjectionMatrix_; // ビュー射影行列
	};
} // namespace TKM