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
#include "engine/3d/camera/Camera.h"

class DirectXCommon;

//=============================================================
// Skyboxクラス
// 背景のキューブマップ（スカイボックス）を描画するクラス。
//=============================================================
class Skybox {
public:
	/// <summary>スカイボックスを初期化します。</summary>
	/// <param name="dxCommon">DirectX共通。</param>
	/// <param name="srvManager">SRVマネージャ。</param>
	/// <param name="texturePath">使用するキューブマップのパス。</param>
	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, const std::string& texturePath);
	/// <summary>スカイボックスを描画します。</summary>
	void Draw();

	/// <summary>スカイボックスの回転を更新します。</summary>
	void UpdateRotation();

	/// <summary>ImGuiでスカイボックスのパラメータを更新します。</summary>
	void ImGuiUpdate();

	///<summary>スカイボックスの位置、回転、スケールを設定します。</summary>
	///<param name="position">位置ベクトル。</param>
	void SetScale(const Vector3& scale) { scale_ = scale; }
	///<param name="position">位置ベクトル。</param>
	void SetCamera(Camera* camera) { camera_ = camera; }
	///<param name="position">位置ベクトル。</param>
	void SetRotation(const Vector3& rot) { rotation_ = rot; }

	// 定数バッファ用構造体
	struct TransformationMatrix {
		Matrix4x4 viewProjection;
		Matrix4x4 world;
	};
	// マテリアル用構造体
	struct Material {
		Vector4 color;
		uint32_t enableLighting;
		Matrix4x4 uvTransform;
		float shininess;
	};
	// GPU用カメラ構造体
	struct CameraForGPU {
		Vector3 worldPosition;//カメラの位置
		float padding;//16byte境界に合わせるためのパディング
	};

private:
	// 頂点構造体
	struct Vertex {
		Vector3 position;
	};

	DirectXCommon* dxCommon_ = nullptr;
	Camera* camera_ = nullptr;

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

	Vector3 scale_ = { 1000.0f, 1000.0f, 1000.0f }; // デフォルトスケール
	Vector3 rotation_ = { 0.0f, 0.0f, 0.0f }; // デフォルト回転
	Vector3 translation_ = { 0.0f, 0.0f, 0.0f }; // デフォルト位置
	
	//カメラ用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource;
	//データを書き込む
	CameraForGPU* cameraData = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;

	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource_;

	float rotationSpeedX_ = 0.002f; // デフォルト値

	/// <summary>
	/// 生成関数
	/// </summary>
	/// <param name="dxCommon">DirectX共通。</param>
	void CreateVertexBuffer();// 頂点バッファ生成
	/// <param name="srvManager">SRVマネージャ。</param>
	void CreateRootSignature();// RootSignature生成
	///<param name="texturePath">使用するキューブマップのパス。</param>
	void CreatePipelineState();// PSO生成
	/// <summary>
	/// カメラ用リソースを作成します。
	/// </summary>
	/// <param name="dxCommon"></param>
	void CameraResource(DirectXCommon* dxCommon);
};
