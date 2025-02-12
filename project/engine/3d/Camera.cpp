#include "Camera.h"
#include "Object3d.h"

Camera::Camera()
	:transform({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} })
	, fovY(0.45f)
	, aspectRatio(float(WindowsAPI::kClientWidth) / float(WindowsAPI::kClientHeight))
	, nearClip(0.1f)
	, farClip(100.0f)
	, worldMatrix(MyMath::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate))
	, viewMatrix(MyMath::Inverse4x4(worldMatrix))
	, projectionMatrix(MyMath::MakePerspectiveFovMatrix(fovY, aspectRatio, nearClip, farClip))
	, viewProjectionMatrix(MyMath::Multiply(viewMatrix, projectionMatrix))
{}

void Camera::Update()
{
	//cameraTransformからcameraMatrixを作る
	worldMatrix = MyMath::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	//cameraMatrixからviewMatrixを作る
	viewMatrix = MyMath::Inverse4x4(worldMatrix);
	//projectionMatrixを作って投資投影行列を書き込む
	projectionMatrix = MyMath::MakePerspectiveFovMatrix(fovY, aspectRatio, nearClip, farClip);
	//合成行列
	viewProjectionMatrix = MyMath::Multiply(viewMatrix, projectionMatrix);
}

void Camera::ImGuiDebug() {

	ImGui::Begin("Camera");
	ImGui::DragFloat3("Translate", &transform.translate.x, 0.01f);
	ImGui::End();
}