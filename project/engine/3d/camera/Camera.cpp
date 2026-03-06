#include "Camera.h"

namespace TKM {
	Camera::Camera()
		//初期化
		:transform_({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} })
		, fovY_(0.45f)
		, aspectRatio_(float(WindowsAPI::kClientWidth_) / float(WindowsAPI::kClientHeight_))
		, nearClip_(0.1f)
		, farClip_(100.0f) // 描画距離の上限
		, worldMatrix_(MyMath::MakeAffineMatrix(transform_.scale_, transform_.rotate_, transform_.translate_))
		, viewMatrix_(MyMath::Inverse4x4(worldMatrix_))
		, projectionMatrix_(MyMath::MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_))
		, viewProjectionMatrix_(MyMath::Multiply(viewMatrix_, projectionMatrix_))
	{
	}

	void Camera::Update() {
		//cameraTransformからcameraMatrixを作る
		worldMatrix_ = MyMath::MakeAffineMatrix(transform_.scale_, transform_.rotate_, transform_.translate_);
		//cameraMatrixからviewMatrixを作る
		viewMatrix_ = MyMath::Inverse4x4(worldMatrix_);
		//projectionMatrixを作って投資投影行列を書き込む
		projectionMatrix_ = MyMath::MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_);
		//合成行列
		viewProjectionMatrix_ = MyMath::Multiply(viewMatrix_, projectionMatrix_);
	}

	void Camera::ImGuiDebug() {
#ifdef USE_IMGUI
		ImGui::Begin("カメラ");
		ImGui::DragFloat3("位置", &transform_.translate_.x, 0.01f);
		ImGui::DragFloat3("回転", &transform_.rotate_.x, 0.01f);
		ImGui::DragFloat3("拡縮", &transform_.scale_.x, 0.01f);
		ImGui::End();
#endif
	}
}