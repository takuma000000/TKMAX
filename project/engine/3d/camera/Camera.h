#pragma once
#include "MyMath.h"
#include "WindowsAPI.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

//=============================================================
// Cameraクラス
// ビュー行列と射影行列を扱うカメラクラス。
//=============================================================
namespace TKM {
	class Camera
	{

		struct Transform {
			Vector3 scale;
			Vector3 rotate;
			Vector3 translate;
		};

	private:
		Transform transform;
		Matrix4x4 worldMatrix;
		Matrix4x4 viewMatrix;

		Matrix4x4 projectionMatrix;
		float fovY;	//水平方向視野角
		float aspectRatio;	//アスペクト比
		float nearClip;	//ニアクリップ距離
		float farClip;	//ファークリップ距離

		Matrix4x4 viewProjectionMatrix;

	public://メンバ関数

		/// <summary>カメラを生成します。</summary>
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
		void SetRotate(const Vector3& rotate) { this->transform.rotate = rotate; }
		/// <summary>
		/// スケールの設定。
		/// </summary>
		/// <param name="scale"></param>
		void SetScale(const Vector3& scale) { this->transform.scale = scale; }
		/// <summary>
		/// 平行移動の設定。
		/// </summary>
		/// <param name="translate"></param>
		void SetTranslate(const Vector3& translate) { this->transform.translate = translate; }
		/// <summary>
		/// 垂直方向視野角の設定。
		/// </summary>
		/// <param name="horizontal"></param>
		void SetFovY(float horizontal) { this->fovY = horizontal; }
		/// <summary>
		/// アスペクト比の設定。
		/// </summary>
		/// <param name="aspect"></param>
		void SetAspectRatio(float aspect) { this->aspectRatio = aspect; }
		/// <summary>
		/// ニアクリップ距離の設定。
		/// </summary>
		/// <param name="nearClip"></param>
		void SetNearClip(float nearClip) { this->nearClip = nearClip; }
		/// <summary>
		/// ファークリップ距離の設定。
		/// </summary>
		/// <param name="farClip"></param>
		void SetFarClip(float farClip) { this->farClip = farClip; }
		// Getter=====================================
		/// <summary>
		/// ワールド行列の取得。
		/// </summary>
		/// <returns></returns>
		const Matrix4x4& GetWorldMatrix() const { return worldMatrix; }
		/// <summary>
		/// ビュー行列の取得。
		/// </summary>
		/// <returns></returns>
		const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
		/// <summary>
		/// 射影行列の取得。
		/// </summary>
		/// <returns></returns>
		const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }
		/// <summary>
		/// ビュー射影行列の取得。
		/// </summary>
		/// <returns></returns>
		const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
		/// <summary>
		/// 回転の取得。
		/// </summary>
		/// <returns></returns>
		const Vector3& GetRotate() const { return transform.rotate; }
		/// <summary>
		/// 平行移動の取得。
		/// </summary>
		/// <returns></returns>
		const Vector3& GetTranslate() const { return transform.translate; }
		/// <summary>
		/// スケールの取得。
		/// </summary>
		/// <returns></returns>
		const Vector3& GetScale() const { return transform.scale; }
		// ===========================================
	};
} // namespace TKM