#include <string>
#include <format>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <cstdint>
#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>
#include <cmath>
#include <fstream>
#include <sstream>
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/DirectXTex/DirectXTex.h"
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")

#include "MyMath.h"
#include "Vector3.h"

// GE3クラス化(MyClass)
#include "WindowsAPI.h"
#include "Input.h"
#include "DirectXCommon.h"
#include "SpriteCommon.h"
#include "engine/2d/Sprite.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "Camera.h"

//struct Vector2 {
//	float x;
//	float y;
//};

//struct Vector3 {
//	float x;
//	float y;
//	float z;
//};

//struct Vector4 {
//	float x;
//	float y;
//	float z;
//	float w;
//};

//struct Matrix3x3 {
//	float m[3][3];
//};

//struct Matrix4x4 {
//	float m[4][4];
//};

//struct Transform {
//	Vector3 scale;
//	Vector3 rotate;
//	Vector3 translate;
//};
//
//struct VertexData {
//	Vector4	position;
//	Vector2 texcoord;
//	Vector3 normal;
//};
//
//struct Material {
//	Vector4 color;
//	int32_t enableLighting;
//	float padding[3];
//	Matrix4x4 uvTransform;
//};
//
//struct TransformationMatrix {
//	Matrix4x4 wvp;
//	Matrix4x4 World;
//};
//
//struct DirectionalLight {
//	Vector4 color;
//	Vector3 direction;
//	float intensity;
//};
//
//struct MaterialData {
//	std::string textureFilePath;
//};
//
//struct ModelData {
//	std::vector<VertexData> vertices;
//	MaterialData material;
//};

Matrix4x4 Inverse(const Matrix4x4& m) {
	Matrix4x4 result;

	// 行列の余因子行列を計算
	result.m[0][0] = m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1] + m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][3] * m.m[3][2] - m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[1][3] * m.m[2][2] * m.m[3][1];
	result.m[0][1] = m.m[0][1] * m.m[2][3] * m.m[3][2] + m.m[0][2] * m.m[2][1] * m.m[3][3] + m.m[0][3] * m.m[2][2] * m.m[3][1] - m.m[0][1] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[2][3] * m.m[3][1] - m.m[0][3] * m.m[2][1] * m.m[3][2];
	result.m[0][2] = m.m[0][1] * m.m[1][2] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][3] * m.m[3][2] - m.m[0][2] * m.m[1][1] * m.m[3][3] - m.m[0][3] * m.m[1][2] * m.m[3][1];
	result.m[0][3] = m.m[0][1] * m.m[1][3] * m.m[2][2] + m.m[0][2] * m.m[1][1] * m.m[2][3] + m.m[0][3] * m.m[1][2] * m.m[2][1] - m.m[0][1] * m.m[1][2] * m.m[2][3] - m.m[0][2] * m.m[1][3] * m.m[2][1] - m.m[0][3] * m.m[1][1] * m.m[2][2];

	result.m[1][0] = m.m[1][0] * m.m[2][3] * m.m[3][2] + m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[1][3] * m.m[2][2] * m.m[3][0] - m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[1][3] * m.m[2][0] * m.m[3][2];
	result.m[1][1] = m.m[0][0] * m.m[2][2] * m.m[3][3] + m.m[0][2] * m.m[2][3] * m.m[3][0] + m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][0] * m.m[2][3] * m.m[3][2] - m.m[0][2] * m.m[2][0] * m.m[3][3] - m.m[0][3] * m.m[2][2] * m.m[3][0];
	result.m[1][2] = m.m[0][0] * m.m[1][3] * m.m[3][2] + m.m[0][2] * m.m[1][0] * m.m[3][3] + m.m[0][3] * m.m[1][2] * m.m[3][0] - m.m[0][0] * m.m[1][2] * m.m[3][3] - m.m[0][2] * m.m[1][3] * m.m[3][0] - m.m[0][3] * m.m[1][0] * m.m[3][2];
	result.m[1][3] = m.m[0][0] * m.m[1][2] * m.m[2][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] + m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][3] * m.m[2][2] - m.m[0][2] * m.m[1][0] * m.m[2][3] - m.m[0][3] * m.m[1][2] * m.m[2][0];

	result.m[2][0] = m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0] + m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][0] * m.m[2][3] * m.m[3][1] - m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[1][3] * m.m[2][1] * m.m[3][0];
	result.m[2][1] = m.m[0][0] * m.m[2][3] * m.m[3][1] + m.m[0][1] * m.m[2][0] * m.m[3][3] + m.m[0][3] * m.m[2][1] * m.m[3][0] - m.m[0][0] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[2][0] * m.m[3][1];
	result.m[2][2] = m.m[0][0] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][3] * m.m[3][0] + m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][0] * m.m[1][3] * m.m[3][1] - m.m[0][1] * m.m[1][0] * m.m[3][3] - m.m[0][3] * m.m[1][1] * m.m[3][0];
	result.m[2][3] = m.m[0][0] * m.m[1][3] * m.m[2][1] + m.m[0][1] * m.m[1][0] * m.m[2][3] + m.m[0][3] * m.m[1][1] * m.m[2][0] - m.m[0][0] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] - m.m[0][3] * m.m[1][0] * m.m[2][1];

	result.m[3][0] = m.m[1][0] * m.m[2][2] * m.m[3][1] + m.m[1][1] * m.m[2][0] * m.m[3][2] + m.m[1][2] * m.m[2][1] * m.m[3][0] - m.m[1][0] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][2] * m.m[3][0] - m.m[1][2] * m.m[2][0] * m.m[3][1];
	result.m[3][1] = m.m[0][0] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][2] * m.m[3][0] + m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][0] * m.m[2][2] * m.m[3][1] - m.m[0][1] * m.m[2][0] * m.m[3][2] - m.m[0][2] * m.m[2][1] * m.m[3][0];
	result.m[3][2] = m.m[0][0] * m.m[1][2] * m.m[3][1] + m.m[0][1] * m.m[1][0] * m.m[3][2] + m.m[0][2] * m.m[1][1] * m.m[3][0] - m.m[0][0] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[3][0] - m.m[0][2] * m.m[1][0] * m.m[3][1];
	result.m[3][3] = m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0] + m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][0] * m.m[1][2] * m.m[2][1] - m.m[0][1] * m.m[1][0] * m.m[2][2] - m.m[0][2] * m.m[1][1] * m.m[2][0];

	// 行列式を計算
	float determinant = m.m[0][0] * result.m[0][0] + m.m[0][1] * result.m[1][0] + m.m[0][2] * result.m[2][0] + m.m[0][3] * result.m[3][0];

	// 行列式が0の場合、逆行列は存在しない
	if (determinant == 0) {
		// エラーハンドリングや適切な処理を追加する
		// 適当なデフォルトの行列を返す場合なども考えられます
		return result; // ゼロ行列を返すことでエラーを示す
	}

	// 行列の逆行列を計算
	float inverseFactor = 1.0f / determinant;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] *= inverseFactor;
		}
	}

	return result;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result;

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = 0;
			for (int k = 0; k < 4; ++k) {
				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}

	return result;
}

Matrix4x4 MakeRotateXMatrix(float radian) {
	float c = std::cos(radian);
	float s = std::sin(radian);

	Matrix4x4 rotationMatrix;

	rotationMatrix.m[0][0] = 1.0f;
	rotationMatrix.m[0][1] = 0.0f;
	rotationMatrix.m[0][2] = 0.0f;
	rotationMatrix.m[0][3] = 0.0f;
	rotationMatrix.m[1][0] = 0.0f;
	rotationMatrix.m[1][1] = c;
	rotationMatrix.m[1][2] = s;
	rotationMatrix.m[1][3] = 0.0f;
	rotationMatrix.m[2][0] = 0.0f;
	rotationMatrix.m[2][1] = -s;
	rotationMatrix.m[2][2] = c;
	rotationMatrix.m[2][3] = 0.0f;
	rotationMatrix.m[3][0] = 0.0f;
	rotationMatrix.m[3][1] = 0.0f;
	rotationMatrix.m[3][2] = 0.0f;
	rotationMatrix.m[3][3] = 1.0f;

	return rotationMatrix;
}

Matrix4x4 MakeRotateYMatrix(float radian) {
	float c = std::cos(radian);
	float s = std::sin(radian);

	Matrix4x4 rotationMatrix;

	rotationMatrix.m[0][0] = c;
	rotationMatrix.m[0][1] = 0.0f;
	rotationMatrix.m[0][2] = -s;
	rotationMatrix.m[0][3] = 0.0f;
	rotationMatrix.m[1][0] = 0.0f;
	rotationMatrix.m[1][1] = 1.0f;
	rotationMatrix.m[1][2] = 0.0f;
	rotationMatrix.m[1][3] = 0.0f;
	rotationMatrix.m[2][0] = s;
	rotationMatrix.m[2][1] = 0.0f;
	rotationMatrix.m[2][2] = c;
	rotationMatrix.m[2][3] = 0.0f;
	rotationMatrix.m[3][0] = 0.0f;
	rotationMatrix.m[3][1] = 0.0f;
	rotationMatrix.m[3][2] = 0.0f;
	rotationMatrix.m[3][3] = 1.0f;

	return rotationMatrix;
}

Matrix4x4 MakeRotateZMatrix(float radian) {
	float c = std::cos(radian);
	float s = std::sin(radian);

	Matrix4x4 rotationMatrix;

	rotationMatrix.m[0][0] = c;
	rotationMatrix.m[0][1] = s;
	rotationMatrix.m[0][2] = 0.0f;
	rotationMatrix.m[0][3] = 0.0f;
	rotationMatrix.m[1][0] = -s;
	rotationMatrix.m[1][1] = c;
	rotationMatrix.m[1][2] = 0.0f;
	rotationMatrix.m[1][3] = 0.0f;
	rotationMatrix.m[2][0] = 0.0f;
	rotationMatrix.m[2][1] = 0.0f;
	rotationMatrix.m[2][2] = 1.0f;
	rotationMatrix.m[2][3] = 0.0f;
	rotationMatrix.m[3][0] = 0.0f;
	rotationMatrix.m[3][1] = 0.0f;
	rotationMatrix.m[3][2] = 0.0f;
	rotationMatrix.m[3][3] = 1.0f;

	return rotationMatrix;
}

Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 result;

	// 単位行列に設定します
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (i == j) {
				result.m[i][j] = 1.0f; // iとjが等しい場合は対角成分を1に
			} else {
				result.m[i][j] = 0.0f; // その他の成分は0に
			}
		}
	}

	// 拡大成分を設定します
	result.m[0][0] = scale.x; // X軸のスケール
	result.m[1][1] = scale.y; // Y軸のスケール
	result.m[2][2] = scale.z; // Z軸のスケール

	return result;
}


Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result;

	// 単位行列に設定します
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (i == j) {
				result.m[i][j] = 1.0f;
			} else {
				result.m[i][j] = 0.0f;
			}
		}
	}

	// 移動成分を設定します
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;

	return result;
}

Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate) {
	Matrix4x4 affineMatrix;

	// 各変換行列を作成
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

	Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
	Matrix4x4 rotateXYZMatrix = Multiply(rotateXMatrix, Multiply(rotateYMatrix, rotateZMatrix));

	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

	// 各変換行列を合成してアフィン変換行列を作成
	affineMatrix = Multiply(scaleMatrix, Multiply(rotateXYZMatrix, translateMatrix));

	return affineMatrix;
}

//Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
//	float tanHalfFovY = tanf(fovY * 0.5f);
//	float scaleX = 1.0f / (aspectRatio * tanHalfFovY);
//	float scaleY = 1.0f / tanHalfFovY;
//	//float nearFarRange = farClip - nearClip;
//
//	Matrix4x4 result;
//
//	result.m[0][0] = scaleX;
//	result.m[0][1] = 0.0f;
//	result.m[0][2] = 0.0f;
//	result.m[0][3] = 0.0f;
//
//	result.m[1][0] = 0.0f;
//	result.m[1][1] = scaleY;
//	result.m[1][2] = 0.0f;
//	result.m[1][3] = 0.0f;
//
//	result.m[2][0] = 0.0f;
//	result.m[2][1] = 0.0f;
//	result.m[2][2] = farClip / (farClip - nearClip);
//	result.m[2][3] = 1.0f;
//
//	result.m[3][0] = 0.0f;
//	result.m[3][1] = 0.0f;
//	result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);
//	result.m[3][3] = 0.0f;
//
//	return result;
//}

//Matrix4x4 MakeIdentity4x4() {
//
//	Matrix4x4 identity;
//
//	// 単位行列を生成する
//	for (int i = 0; i < 4; ++i) {
//		for (int j = 0; j < 4; ++j) {
//			identity.m[i][j] = (i == j) ? 1.0f : 0.0f;
//		}
//	}
//
//	return identity;
//
//}

//Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
//	Matrix4x4 mat;
//
//	mat.m[0][0] = 2.0f / (right - left);
//	mat.m[0][1] = 0.0f;
//	mat.m[0][2] = 0.0f;
//	mat.m[0][3] = 0.0f;
//
//	mat.m[1][0] = 0.0f;
//	mat.m[1][1] = 2.0f / (top - bottom);
//	mat.m[1][2] = 0.0f;
//	mat.m[1][3] = 0.0f;
//
//	mat.m[2][0] = 0.0f;
//	mat.m[2][1] = 0.0f;
//	mat.m[2][2] = 1.0f / (farClip - nearClip);
//	mat.m[2][3] = 0.0f;
//
//	mat.m[3][0] = -(right + left) / (right - left);
//	mat.m[3][1] = -(top + bottom) / (top - bottom);
//	mat.m[3][2] = (nearClip) / (nearClip - farClip);
//	mat.m[3][3] = 1.0f;
//
//	return mat;
//}

std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

//OutputDebugStringA関数
void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}

//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
//	ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heaptype, UINT numDescriptors, bool shaderVisible) {
//
//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
//	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
//	descriptorHeapDesc.Type = heaptype;
//	descriptorHeapDesc.NumDescriptors = numDescriptors;
//	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
//	assert(SUCCEEDED(hr));
//	return descriptorHeap;
//}

//Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height) {
//	////生成するResourceの設定
//	//D3D12_RESOURCE_DESC resourceDesc{};
//	//resourceDesc.Width = width;//Textureの幅
//	//resourceDesc.Height = height;//Textureの高さ
//	//resourceDesc.MipLevels = 1;//MipMapの数
//	//resourceDesc.DepthOrArraySize = 1;//奥行き or 配列Textureの配列数
//	//resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//DepthStencilとして利用可能なフォーマット
//	//resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。1固定
//	//resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元
//	//resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//DepthStencilとして使う通知
//
//	////利用するHeapの設定
//	//D3D12_HEAP_PROPERTIES heapProperties{};
//	//heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;//VRAM上に作る
//
//	////深度値のクリア設定
//	//D3D12_CLEAR_VALUE depthClearValue{};
//	//depthClearValue.DepthStencil.Depth = 1.0f;//1.0f( 最大値 )でクリア
//	//depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//フォーマット。Resourceと合わせる。
//
//	////Resourceの生成
//	//Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
//	//HRESULT hr = device->CreateCommittedResource(
//	//	&heapProperties,//Heapの設定
//	//	D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。特になし
//	//	&resourceDesc,//Resourceの設定
//	//	D3D12_RESOURCE_STATE_DEPTH_WRITE,//深度地を書き込む状態にしておく
//	//	&depthClearValue,//Clear最適値
//	//	IID_PPV_ARGS(&resource)//作成するResourceポインタへのポインタ
//	//);
//	//assert(SUCCEEDED(hr));
//
//	return resource;
//}

//D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
//	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
//	handleCPU.ptr += (descriptorSize * index);
//	return handleCPU;
//}
//
//D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
//	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
//	handleGPU.ptr += (descriptorSize * index);
//	return handleGPU;
//}

//MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
//	//中で必要となる変数の宣言
//	MaterialData materialData;//構築するMaterialData
//	std::string line;//ファイルから読んだ1行を格納するもの
//	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
//	assert(file.is_open());//とりあえず開けなかったら止める
//	while (std::getline(file, line)) {
//		std::string identifier;
//		std::istringstream s(line);
//		s >> identifier;
//
//		//identifierに応じた処理
//		if (identifier == "map_Kd") {
//			std::string textureFilename;
//			s >> textureFilename;
//			//連結してファイルパスにする
//			materialData.textureFilePath = directoryPath + "/" + textureFilename;
//		}
//	}
//	return materialData;
//}
//
//ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
//	//必要となる変数の宣言
//	ModelData modelData;//構築するモデルデータ
//	std::vector<Vector4> positions;//位置
//	std::vector<Vector3> normals;//法線
//	std::vector<Vector2> texcoords;//テクスチャ座標
//	std::string line;//ファイルから読んだ一行を格納するもの
//	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
//	assert(file.is_open());//とりあえず開けなかったら止める
//	while (std::getline(file, line)) {
//		std::string identifier;
//		std::istringstream s(line);
//		s >> identifier;//先頭の識別子を読む
//
//		//identifierに応じた処理
//		if (identifier == "v") {
//			Vector4 position;
//			s >> position.x >> position.y >> position.z;
//			position.w = 1.0f;
//			//position.x *= -1;
//			positions.push_back(position);
//		} else if (identifier == "vt") {
//			Vector2 texcoord;
//			s >> texcoord.x >> texcoord.y;
//			texcoord.y = 1.0f - texcoord.y;
//			texcoords.push_back(texcoord);
//		} else if (identifier == "vn") {
//			Vector3 normal;
//			s >> normal.x >> normal.y >> normal.z;
//			normal.x *= -1;
//			normals.push_back(normal);
//		} else if (identifier == "f") {
//			VertexData triangle[3];
//			//面は三角形限定。その他は未対応
//			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
//				std::string vertexDefinition;
//				s >> vertexDefinition;
//				//頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分離してIndexを取得する
//				std::istringstream v(vertexDefinition);
//				uint32_t elementIndices[3];
//				for (int32_t element = 0; element < 3; ++element) {
//					std::string index;
//					std::getline(v, index, '/');//区切りでインデックスを読んでいく
//					elementIndices[element] = std::stoi(index);
//				}
//				//要素へのIndexから、実際の要素の値を取得して、頂点を構築する
//				Vector4 position = positions[elementIndices[0] - 1];
//				Vector2 texcoord = texcoords[elementIndices[1] - 1];
//				Vector3 normal = normals[elementIndices[2] - 1];
//				VertexData vertex = { position,texcoord,normal };
//				modelData.vertices.push_back(vertex);
//				triangle[faceVertex] = { position,texcoord,normal };
//
//			}
//			//頂点を逆順で登録することで、周り順を逆にする
//			modelData.vertices.push_back(triangle[2]);
//			modelData.vertices.push_back(triangle[1]);
//			modelData.vertices.push_back(triangle[0]);
//		} else if (identifier == "mtllib") {
//			//materialTemplateLibraryファイルの名前を取得する
//			std::string materialFilename;
//			s >> materialFilename;
//			//基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
//			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
//		}
//	}
//	return modelData;
//}

//Transform変数を作る
Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
//Transform cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,-50.0f} };
//Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
Transform uvTransformSprite{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
};

//SRV切り替え
bool useMonsterBall = true;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	//基盤システムの初期化*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

	//ポインタ...WindowsAPI
	std::unique_ptr<WindowsAPI> windowsAPI = nullptr;
	//WindowsAPIの初期化
	windowsAPI = std::make_unique<WindowsAPI>();
	windowsAPI->Initialize();

	//ポインタ...Input
	std::unique_ptr<Input> input = nullptr;
	//入力の初期化
	input = std::make_unique<Input>();
	input->Initialize(windowsAPI.get());

	//ポインタ...DirectXCommon
	std::unique_ptr<DirectXCommon> dxCommon = nullptr;
	//DirectXの初期化
	dxCommon = std::make_unique<DirectXCommon>();
	dxCommon->Initialize(windowsAPI.get());

	//テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(dxCommon.get());
	//ファイルパス
	TextureManager::GetInstance()->LoadTexture("./resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./resources/pokemon.png");

	//ポインタ...SpriteCommon
	std::unique_ptr<SpriteCommon> spriteCommon = nullptr;
	//スプライト共通部の初期化
	spriteCommon = std::make_unique<SpriteCommon>();
	spriteCommon->Initialize(dxCommon.get());

	//ポインタ...Sprite
	std::unique_ptr<Sprite> sprite = nullptr;
	sprite = std::make_unique<Sprite>();
	sprite->Initialize(spriteCommon.get(), dxCommon.get(), "./resources/uvChecker.png");

	//ポインタ...Object3dCommon
	std::unique_ptr<Object3dCommon> object3dCommon = nullptr;
	//Object3d共通部の初期化
	object3dCommon = std::make_unique<Object3dCommon>();
	object3dCommon->Initialize(dxCommon.get());

	////ポインタ...ModelCommon
	//std::unique_ptr<ModelCommon> modelCommon = nullptr;
	////Object3d共通部の初期化
	//modelCommon = std::make_unique<ModelCommon>();
	//modelCommon->Initialize(dxCommon.get());
	ModelManager::GetInstance()->Initialize(dxCommon.get());
	ModelManager::GetInstance()->LoadModel("axis.obj", dxCommon.get());

	//Modelの初期化
	/*Model* model = new Model();
	model->Initialize(modelCommon.get(), dxCommon.get());*/

	///--------------------------------------------

	//Object3dの初期化
	Object3d* object3d = new Object3d();
	object3d->Initialize(object3dCommon.get(), dxCommon.get());
	object3d->SetModel("axis.obj");

	//AnotherObject3d ( もう一つのObject3d )
	Object3d* anotherObject3d = new Object3d();
	anotherObject3d->Initialize(object3dCommon.get(), dxCommon.get());
	anotherObject3d->SetModel("plane.obj");

	//---------------------------------------------

	//ポインタ...Camera
	std::unique_ptr<Camera> camera = nullptr;
	//Object3d共通部の初期化
	camera = std::make_unique<Camera>();
	camera->SetRotate({ 0.0f,0.0f,0.0f });
	camera->SetTranslate({ 0.0f,0.0f,-30.0f });
	//object3dCommon->SetDefaultCamera(camera.get());
	object3d->SetCamera(camera.get());
	anotherObject3d->SetCamera(camera.get());
	//ImGui用のcamera設定
	Vector3 cameraPosition = camera->GetTranslate();
	Vector3 cameraRotation = camera->GetRotate();


	//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;//0から始まる
	descriptorRange[0].NumDescriptors = 1;//数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//Offsetを自動計算

	//RootParameter作成。PixelShaderのMaterialとVertexShaderのTransform
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0;	//レジスタ番号0とバインド
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;	//VertexShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0;	//レジスタ番号0とバインド
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;//Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);//Tableで利用する数
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//CBVを使う
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//PixelShaderで使う
	rootParameters[3].Descriptor.ShaderRegister = 1;//レジスタ番号1を使う
	descriptionRootSignature.pParameters = rootParameters;	//ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);	//配列の長さ

	//Samplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0～1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0～1の範囲外をリピート
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0～1の範囲外をリピート
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;//ありったけのMipMapを使う
	staticSamplers[0].ShaderRegister = 0;//レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);


	HRESULT hr;
	//シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlog = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlog = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlog, &errorBlog);
	if (FAILED(hr)) {
		Log(reinterpret_cast<char*>(errorBlog->GetBufferPointer()));
		assert(false);
	}


	//バイナリを元に生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0, signatureBlog->GetBufferPointer(), signatureBlog->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));


	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_RASTERIZER_DESC resterizerDesc{};
	resterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	resterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon->CompileShader(L"resources/shaders/Object3D.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon->CompileShader(L"resources/shaders/Object3D.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	//書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;


	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
	graphicPipelineStateDesc.pRootSignature = rootSignature.Get();
	graphicPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() };
	graphicPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() };
	graphicPipelineStateDesc.BlendState = blendDesc;
	graphicPipelineStateDesc.RasterizerState = resterizerDesc;
	graphicPipelineStateDesc.NumRenderTargets = 1;
	graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicPipelineStateDesc.SampleDesc.Count = 1;
	graphicPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//DepthStencilの設定
	graphicPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	//モデル読み込み
	//ModelData modelData = LoadObjFile("resources", "plane.obj");
	//頂点リソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	//頂点バッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	/*vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);*/
	//頂点リソースにデータを書き込む
	//VertexData* vertexData = nullptr;
	//書き込むためのアドレスを取得
	/*vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());*/


	////Sprite用の頂点リソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = dxCommon->CreateBufferResource(sizeof(VertexData) * 6);
	////頂点バッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	////Resourceの先頭のアドレスから使う
	//vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	////使用するResourceのサイズは頂点6つ分のサイズ
	//vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	////1頂点あたりのサイズ
	//vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	//VertexData* vertexDataSprite = nullptr;
	//vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	//1枚目の三角形
	////左下
	//vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };
	//vertexDataSprite[0].texcoord = { 0.0f,1.0f };
	//vertexDataSprite[0].normal = { 0.0f,0.0f,-1.0f };
	////左上
	//vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };
	//vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	//vertexDataSprite[1].normal = { 0.0f,0.0f,-1.0f };
	////右上
	//vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f };
	//vertexDataSprite[2].texcoord = { 1.0f,1.0f };
	//vertexDataSprite[2].normal = { 0.0f,0.0f,-1.0f };
	////2枚目の三角形
	////左上
	//vertexDataSprite[3].position = { 0.0f,0.0f,0.0f,1.0f };
	//vertexDataSprite[3].texcoord = { 0.0f,0.0f };
	//vertexDataSprite[3].normal = { 0.0f,0.0f,-1.0f };
	////右上
	//vertexDataSprite[4].position = { 640.0f,0.0f,0.0f,1.0f };
	//vertexDataSprite[4].texcoord = { 1.0f,0.0f };
	//vertexDataSprite[4].normal = { 0.0f,0.0f,-1.0f };
	////右下
	//vertexDataSprite[5].position = { 640.0f,360.0f,0.0f,1.0f };
	//vertexDataSprite[5].texcoord = { 1.0f,1.0f };
	//vertexDataSprite[5].normal = { 0.0f,0.0f,-1.0f };

	////Sprite用のTransformationMatrix用のResourceを作る。Matrix4x4 1つ分のサイズを用意する
	//Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	//データを書き込む
	//TransformationMatrix* transformationMatrixDataSprite = nullptr;
	////書き込むためのアドレスを取得
	//transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	////単位行列を書き込んでおく
	//transformationMatrixDataSprite->wvp = MakeIdentity4x4();


	//マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = dxCommon->CreateBufferResource(sizeof(Material));
	//マテリアルにデータを書き込む
	//Material* materialData = nullptr;
	//書き込むためのアドレスを取得
	//materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	////今回は白を書き込んでみる
	//materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//materialData->enableLighting = true;
	//materialData->uvTransform = MyMath::MakeIdentity4x4();


	//WVP用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	//Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	//データを書き込む
	//TransformationMatrix* wvpData = nullptr;
	//書き込むためのアドレスを取得
	//wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	//単位行列を書き込んでおく
	//wvpData->wvp = MyMath::MakeIdentity4x4();


	////Sprite用のマテリアルリソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = dxCommon->CreateBufferResource(sizeof(Material));
	////Spriteにデータを書き込む
	//Material* materialDataSprite = nullptr;
	////書き込むためのアドレスを取得
	//materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	////色は白に設定
	//materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	////SprightはLightingしないのでfalseを設定する
	//materialDataSprite->enableLighting = false;
	//materialDataSprite->uvTransform = MakeIdentity4x4();

	//Light用のマテリアルリソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceLight = dxCommon->CreateBufferResource(sizeof(DirectionalLight));
	//DirectionalLight* directionalLightData = nullptr;
	//materialResourceLight->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	/*directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;*/

	////Index用のリソース
	//Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = dxCommon->CreateBufferResource(sizeof(uint32_t) * 6);
	////IndexのViewを作る
	//D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	////リソースの先頭のアドレスから使う
	//indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	////使用するリソースのサイズはインデックス6つ分のサイズ
	//indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	////インデックスはuint32_tとする
	//indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;
	////インデックスリソースにデータを書き込む
	//uint32_t* indexDataSprite = nullptr;
	//indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	//indexDataSprite[0] = 0;
	//indexDataSprite[1] = 1;
	//indexDataSprite[2] = 2;
	//indexDataSprite[3] = 1;
	//indexDataSprite[4] = 3;
	//indexDataSprite[5] = 2;

	//2枚目のTextureを読んで転送する
	/*DirectX::ScratchImage mipImage2;
	textureManager->LoadTexture(modelData.material.textureFilePath);
	const DirectX::TexMetadata metadata2 = mipImage2.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = textureManager->CreateTextureResource(metadata2);
	dxCommon->UploadTextureData(textureResource2.Get(), mipImage2);*/

	//metaData2を基にSRVの設定( 2枚目 )
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	//srvDesc2.Format = metadata2.format;
	//srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	//srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	////Textureを読んで転送する
	//DirectX::ScratchImage mipImages; 
	//textureManager->LoadTexture("resources/uvChecker.png");
	//const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	//Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = textureManager->CreateTextureResource(metadata);
	//dxCommon->UploadTextureData(textureResource.Get(), mipImages);

	////metaDataを基にSRVの設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	//srvDesc.Format = metadata.format;
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	//srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	// SRVを作成するDescriptorHeapの場所を決める
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = dxCommon->GetsrvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = dxCommon->GetsrvDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	//// 先頭はImGuiが使っているのでその次を使う
	//textureSrvHandleCPU = dxCommon->GetCPUDescriptorHandle(dxCommon->GetsrvDescriptorHeap(), dxCommon->GetDescriptorSizeSRV(), 2);
	//textureSrvHandleGPU = dxCommon->GetGPUDescriptorHandle(dxCommon->GetsrvDescriptorHeap(), dxCommon->GetDescriptorSizeSRV(), 2);

	////// SRVの生成
	////dxCommon->GetDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);

	//// SRVを作成するDescriptorHeapの場所を決める(2枚目)
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = dxCommon->GetCPUDescriptorHandle(dxCommon->GetsrvDescriptorHeap(), dxCommon->GetDescriptorSizeSRV(), 3);
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = dxCommon->GetGPUDescriptorHandle(dxCommon->GetsrvDescriptorHeap(), dxCommon->GetDescriptorSizeSRV(), 3);

	// SRVの生成
	//dxCommon->GetDevice()->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);


	//出力ウィンドウへの文字出力ループを抜ける
	Log("Hello,DirectX!\n");

	MSG msg{};
	//ウィンドウの×ボタンが押されるまでループ
	while (true) {

		//Windowsのメッセージ処理
		if (windowsAPI->ProcessMessage()) {
			//ゲームループを抜ける
			break;
		} else {

			//ゲームの処理
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			//色を変えるImGuiの処理
			ImGui::Begin("Setting");

			ImGui::Text("Camera Settings");

			// カメラの位置を制御
			ImGui::SliderFloat("Camera TranslateX", &cameraPosition.x, -3.0f, 3.0f);
			ImGui::SliderFloat("Camera TranslateY", &cameraPosition.y, -3.0f, 3.0f);
			ImGui::SliderFloat("Camera TranslateZ", &cameraPosition.z, -30.0f, 1.0f);

			// カメラの回転を制御
			ImGui::SliderFloat("Camera RotateX", &cameraRotation.x, -50.0f, 1.0f);
			ImGui::SliderFloat("Camera RotateY", &cameraRotation.y, -50.0f, 1.0f);
			ImGui::SliderFloat("Camera RotateZ", &cameraRotation.z, -50.0f, 1.0f);

			// カメラに反映
			camera->SetTranslate(cameraPosition);
			camera->SetRotate(cameraRotation);

			ImGui::Text("Model Transform");
			ImGui::SliderFloat("Model RotateX", &transform.rotate.x, -3.14159f, 3.14159f);
			ImGui::SliderFloat("Model RotateY", &transform.rotate.y, -3.14159f, 3.14159f);
			ImGui::SliderFloat("Model RotateZ", &transform.rotate.z, -3.14159f, 3.14159f);
			ImGui::SliderFloat("Model ScaleX", &transform.scale.x, 0.1f, 10.0f);
			ImGui::SliderFloat("Model ScaleY", &transform.scale.y, 0.1f, 10.0f);
			ImGui::SliderFloat("Model ScaleZ", &transform.scale.z, 0.1f, 10.0f);
			ImGui::SliderFloat("Model TranslateX", &transform.translate.x, -10.0f, 10.0f);
			ImGui::SliderFloat("Model TranslateY", &transform.translate.y, -10.0f, 10.0f);
			ImGui::SliderFloat("Model TranslateZ", &transform.translate.z, -10.0f, 10.0f);

			ImGui::Text("useMonsterBall");
			ImGui::Checkbox("useMonsterBall", &useMonsterBall);

			ImGui::Text("Lighting");
			//ImGui::SliderInt("Light", &materialData->enableLighting, 0, 1);
			//ImGui::SliderFloat3("LightDirector", &directionalLightData->direction.x, -1.0f, 1.0f);

			ImGui::Text("UVchecker");
			ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
			ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

			/*Sprite::Transform transformSprite = sprite->GetTransform();
			ImGui::Text("Sprite Transfom");
			ImGui::SliderFloat("Sprite RotateX", &transformSprite.rotate.x, -3.14159f, 3.14159f);
			ImGui::SliderFloat("Sprite RotateY", &transformSprite.rotate.y, -3.14159f, 3.14159f);
			ImGui::SliderFloat("Sprite RotateZ", &transformSprite.rotate.z, -3.14159f, 3.14159f);
			ImGui::SliderFloat("Sprite ScaleX", &transformSprite.scale.x, 0.1f, 10.0f);
			ImGui::SliderFloat("Sprite ScaleY", &transformSprite.scale.y, 0.1f, 10.0f);
			ImGui::SliderFloat("Sprite ScaleZ", &transformSprite.scale.z, 0.1f, 10.0f);
			ImGui::SliderFloat("Sprite TranslateX", &transformSprite.translate.x, 0.1f, 1000.0f);
			ImGui::SliderFloat("Sprite TranslateY", &transformSprite.translate.y, 0.1f, 1000.0f);
			ImGui::SliderFloat("Sprite TranslateZ", &transformSprite.translate.z, 0.1f, 1000.0f);

			sprite->SetTransform(transformSprite);*/

			ImGui::End();
			//開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
			//ImGui::ShowDemoWindow();

			//ImGuiの内部コマンドを生成する
			ImGui::Render();

			D3D12_VIEWPORT viewport = dxCommon->GetViewport();
			D3D12_RECT scissorRect = dxCommon->GetRect();

			//Draw
			dxCommon->PreDraw();
			spriteCommon->DrawSetCommon();
			object3dCommon->DrawSetCommon();

			//---------------------------------------------------------

			////現在の座標を変数で受け取る
			//Sprite::Vector2 position = sprite->GetPosition();
			////座標を変更する
			//position.x += 0.1f;
			//position.y += 0.1f;
			////変更を反映する
			//sprite->SetPosition(position);

			////角度を変化させる
			//float rotation = sprite->GetRotation();
			//rotation += 0.01f;
			//sprite->SetRotation(rotation);

			////色を変化させる
			//Sprite::Vector4 color = sprite->GetColor();
			//color.x += 0.01f;
			//if (color.x > 1.0f) {
			//	color.x -= 1.0f;
			//}
			//sprite->SetColor(color);

			//サイズを変化させる
			/*Sprite::Vector2 size = sprite->GetSize();
			size.x += 0.1f;
			size.y += 0.1f;
			sprite->SetSize(size);*/

			//Sprite::Vector2 position = sprite->GetPosition();
			////座標を変更する
			//position.x += 0.1f;
			//position.y += 0.1f;
			////変更を反映する
			//sprite->SetPosition(position);

			//object3d*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

			//translate
			Vector3 translate = object3d->GetTranslate();
			translate = { 3.0f,-4.0f,0.0f };
			object3d->SetTranslate(translate);
			//rotate
			Vector3 rotate = object3d->GetRotate();
			rotate += { 0.0f, 0.0f, 0.1f };
			object3d->SetRotate(rotate);
			//scale
			Vector3 scale = object3d->GetScale();
			scale = { 1.0f, 1.0f, 1.0f };
			object3d->SetScale(scale);

			//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-


			//anotherObject3d-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
			//translate
			Vector3 anotherTranslate = anotherObject3d->GetTranslate();
			anotherTranslate = { 3.0f,4.0f,0.0f };
			anotherObject3d->SetTranslate(anotherTranslate);
			//ratate
			Vector3 anotherRotate = anotherObject3d->GetRotate();
			anotherRotate += { 0.1f, 0.0f, 0.0f };
			anotherObject3d->SetRotate(anotherRotate);
			//scale
			Vector3 anotherScale = anotherObject3d->GetScale();
			anotherScale = { 1.0f, 1.0f, 1.0f };
			anotherObject3d->SetScale(anotherScale);

			//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-


			//---------------------------------------------------------

			//描画
			dxCommon->GetCommandList()->RSSetViewports(1, &viewport);
			dxCommon->GetCommandList()->RSSetScissorRects(1, &scissorRect);
			//dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
			//dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
			//dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
			////dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


			//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			////wvp用のCBufferの場所を設定
			//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

			////SRVを切り替える
			//dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);

			//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, materialResourceLight->GetGPUVirtualAddress());

			////dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

			camera->Update();
			sprite->Update();
			object3d->Update();
			anotherObject3d->Update();

			sprite->Draw();  // textureSrvHandleGPU は必要に応じて設定
			object3d->Draw(dxCommon.get());
			anotherObject3d->Draw(dxCommon.get());


			//Spriteを常にuvCheckerにする
			//dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

			//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());


			////Spriteの描画。変更が必要なものだけ変更する
			//dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);//VBVを設定
			////TransformationMatrixCBufferの場所を設定
			//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
			////描画
			//dxCommon->GetCommandList()->DrawInstanced(6, 1, 0, 0);

			//IBVを設定
			//dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferViewSprite);
			//描画。6個のインデックスを使用し1つのインスタンスを描画。その他は当面0で良い
			//commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			//実際のcommandListのImGuiの描画コマンドを積む
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());

			/*barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			commandList->ResourceBarrier(1, &barrier);*/

			//コマンドリストの内容を確定させる。全てのコマンドを積んでからCloseすること
			//hr = dxCommon->GetCommandList()->Close();
			//assert(SUCCEEDED(hr));

			/*ID3D12CommandList* commandLists[] = { commandList.Get() };
			commandQueue->ExecuteCommandLists(1, commandLists);
			swapChain->Present(1, 0);*/

			/*fenceValue++;
			commandQueue->Signal(fence.Get(), fenceValue);

			if (fence->GetCompletedValue() < fenceValue) {
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				WaitForSingleObject(fenceEvent, INFINITE);
			}*/

			/*hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));
			hr = commandList->Reset(commandAllocator.Get(), nullptr);
			assert(SUCCEEDED(hr));*/

			//三角形を動かす処理
			//transform.rotate.y += 0.01f;
			//Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			//wvpData->World = worldMatrix;

			////3次元的にする
			//Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			//Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			//Matrix4x4 projectionMatrix = MyMath::MakePerspectiveFovMatrix(0.45f, float(WindowsAPI::kClientWidth) / float(WindowsAPI::kClientHeight), 0.1f, 100.0f);
			//WVPMatrixを作る
			/*Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
			wvpData->wvp = worldViewProjectionMatrix;*/

			////UVTransform用の行列
			//Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
			//uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
			//uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
			//materialDataSprite->uvTransform = uvTransformMatrix;

			// 行列の更新
			/*Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
			Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(WindowsAPI::kClientWidth), float(WindowsAPI::kClientHeight), 0.0f, 100.0f);*/
			//Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));

			//transformationMatrixDataSprite->wvp = worldViewProjectionMatrixSprite;

			dxCommon->PostDraw();

		}

	}//ゲームループ終わり




	//ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	////				解放
	////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

	//Object3dの解放
	delete object3d;

	//3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();

	//CloseHandle(fenceEvent);

	//テクスチャマネージャの終了
	TextureManager::GetInstance()->Finalize();

	//WindowsAPIの終了処理
	windowsAPI->Finalize();

	return 0;

}