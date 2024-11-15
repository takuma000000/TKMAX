#include "Camera.h"
#include "Object3d.h"

void Camera::Update()
{
	//TransformからWorldMatrixを作る
	worldMatrix = MyMath::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	//cameraTransformからcameraMatrixを作る
	cameraMatrix = MyMath::MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
	//cameraMatrixからviewMatrixを作る
	viewMatrix = MyMath::Inverse4x4(cameraMatrix);
}
