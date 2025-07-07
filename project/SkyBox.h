#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "MyMath.h"

#include "TextureManager.h"
#include "DirectXCommon.h"

class DirectXCommon;

class Skybox {
public:
	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, const std::string& texturePath);
	void Draw(const Matrix4x4& view, const Matrix4x4& projection);

	//ImGui
	void ImGuiUpdate();

	void SetScale(const Vector3& scale) { scale_ = scale; }

	// 定数バッファ用構造体
	struct TransformationMatrix {
		Matrix4x4 viewProjection;
		Matrix4x4 world;
	};
	struct Material {
		Vector4 color;
		uint32_t enableLighting;
		Matrix4x4 uvTransform;
		float shininess;
	};
	struct CameraForGPU {
		Vector3 worldPosition;//カメラの位置
		float padding;//16byte境界に合わせるためのパディング
	};

private:
	struct Vertex {
		Vector3 position;
	};

	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	UINT vertexCount_ = 0;

	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_{};

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
	TransformationMatrix* mappedData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialBuffer_;
	Material* mappedMaterial_ = nullptr;

	Vector3 scale_ = { 5.0f, 5.0f, 5.0f }; // デフォルトスケール
	Vector3 rotation_ = { 0.0f, 0.0f, 0.0f }; // デフォルト回転
	Vector3 translation_ = { 0.0f, 0.0f, 0.0f }; // デフォルト位置

	//カメラ用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource;
	//データを書き込む
	CameraForGPU* cameraData = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;

	
	/// <summary>
	/// 生成関数
	/// </summary>
	void CreateVertexBuffer();// 頂点バッファ生成
	void CreateRootSignature();// RootSignature生成
	void CreatePipelineState();// PSO生成

	//cameraResource関数
	void CameraResource(DirectXCommon* dxCommon);

};
