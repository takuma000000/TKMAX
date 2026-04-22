#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include "MyMath.h"

#include "TextureManager.h"
#include "DirectXCommon.h"
#include "Camera.h"

class DirectXCommon;

namespace TKM {

	//=============================================================
	// Skyboxクラス
	// スカイボックスを描画するクラス
	//=============================================================
	class Skybox {
	public:
		//=============================================================
		// 初期化・更新・描画
		//=============================================================

		/// <summary>
		/// スカイボックスを初期化します。
		/// </summary>
		void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, const std::string& texturePath);

		/// <summary>
		/// スカイボックスを描画します。
		/// </summary>
		void Draw();

		/// <summary>
		/// 回転を更新します。
		/// </summary>
		void UpdateRotation();

		/// <summary>
		/// ImGui調整項目を更新します。
		/// </summary>
		void ImGuiUpdate();

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// スケールを設定します。
		/// </summary>
		void SetScale(const Vector3& scale) { scale_ = scale; }

		/// <summary>
		/// カメラを設定します。
		/// </summary>
		void SetCamera(TKM::Camera* camera) { camera_ = camera; }

		/// <summary>
		/// 回転を設定します。
		/// </summary>
		void SetRotation(const Vector3& rot) { rotation_ = rot; }

		//=============================================================
		// 色設定
		//=============================================================

		/// <summary>
		/// 色を設定します。
		/// </summary>
		void SetColor(const Vector4& color);

		/// <summary>
		/// 色を取得します。
		/// </summary>
		Vector4 GetColor() const;

		//=============================================================
		// GPU送信用構造体
		//=============================================================

		// 座標変換行列
		struct TransformationMatrix {
			Matrix4x4 viewProjection_;
			Matrix4x4 world_;
		};

		// マテリアル
		struct Material {
			Vector4 color_;
			uint32_t enableLighting_;
			Matrix4x4 uvTransform_;
			float shininess_;
		};

		// カメラ情報
		struct CameraForGPU {
			Vector3 worldPosition_; // カメラ位置
			float padding_;         // 16byte境界調整
		};

	private:
		//=============================================================
		// 頂点構造体
		//=============================================================

		struct Vertex {
			Vector3 position_;
		};

		//=============================================================
		// 共通参照
		//=============================================================

		DirectXCommon* dxCommon_ = nullptr; // DirectX共通管理
		TKM::Camera* camera_ = nullptr;     // 使用カメラ

		//=============================================================
		// 描画リソース
		//=============================================================

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_; // 頂点バッファ
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};         // 頂点バッファビュー
		UINT vertexCount_ = 0;                                // 頂点数

		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_{};          // テクスチャSRV

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_; // ルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_; // パイプラインステート

		Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_; // 定数バッファ
		TransformationMatrix* mappedData_ = nullptr;           // 行列データ書き込み先

		Microsoft::WRL::ComPtr<ID3D12Resource> materialBuffer_; // マテリアルバッファ
		Material* mappedMaterial_ = nullptr;                    // マテリアルデータ書き込み先

		//=============================================================
		// Transform
		//=============================================================

		Vector3 scale_ = { 1000.0f, 1000.0f, 1000.0f };      // スケール
		Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };            // 回転
		Vector3 translation_ = { 0.0f, 0.0f, 0.0f };         // 位置

		//=============================================================
		// カメラ・テクスチャ
		//=============================================================

		Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;       // カメラリソース
		CameraForGPU* cameraData_ = nullptr;                          // カメラデータ書き込み先

		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;      // テクスチャリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource_; // 中間リソース

		//=============================================================
		// パラメータ
		//=============================================================

		float rotationSpeedX_ = 0.002f; // X回転速度

		//=============================================================
		// 内部生成処理
		//=============================================================

		/// <summary>
		/// 頂点バッファを生成します。
		/// </summary>
		void CreateVertexBuffer();

		/// <summary>
		/// ルートシグネチャを生成します。
		/// </summary>
		void CreateRootSignature();

		/// <summary>
		/// パイプラインステートを生成します。
		/// </summary>
		void CreatePipelineState();

		/// <summary>
		/// カメラリソースを生成します。
		/// </summary>
		void CameraResource(DirectXCommon* dxCommon);
	};
}