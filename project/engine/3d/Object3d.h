#pragma once
#include "MyMath.h"
#include <string>
#include <vector>
#include "DirectXCommon.h"
#include "ModelTypes.h"

namespace TKM {
	class Camera;
	class Model;
	class Object3dCommon;
	class BaseScene;
}

//=============================================================
// GPU送信用構造体
//=============================================================

// 座標変換行列
struct TransformationMatrix {
	Matrix4x4 wvp_;
	Matrix4x4 World_;
	Matrix4x4 WorldInverseTranspose_;
};

// 平行光源
struct DirectionalLightEX {
	Vector4 color_;
	Vector3 direction_;
	float intensity_;
};

// 点光源
struct PointLightEX {
	Vector4 color_;
	Vector3 position_;
	float intensity_;
	float radius_;
	float decay_;
	float padding_[2];
};

// スポットライト
struct SpotLightEX {
	Vector4 color_;
	Vector3 position_;
	float intensity_;
	Vector3 direction_;
	float distance_;
	float decay_;
	float cosAngle_;
	float cosFalloffStart_;
	float padding_[2];
};

// カメラ情報
struct CameraForGPU {
	Vector3 worldPosition_; // カメラ位置
	float padding_;         // 16byte境界調整
};

// 環境マップ情報
struct EnvironmentEX {
	bool useEnvironment_ = false; // 環境マップ使用フラグ
	Vector3 padding_;
};

namespace TKM {

	//=============================================================
	// Object3dクラス
	// 3Dオブジェクトの描画を管理するクラス
	//=============================================================
	class Object3d {
	public:
		//=============================================================
		// 生成・破棄
		//=============================================================

		Object3d();
		~Object3d();

		//=============================================================
		// 初期化・更新・描画
		//=============================================================

		/// <summary>
		/// 3Dオブジェクトを初期化します。
		/// </summary>
		void Initialize(TKM::Object3dCommon* object3dCommon, TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// 3Dオブジェクトを更新します。
		/// </summary>
		void Update();

		/// <summary>
		/// 3Dオブジェクトを描画します。
		/// </summary>
		void Draw(TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// 親オブジェクトを解除します。
		/// </summary>
		void ClearParent() { parent_ = nullptr; }

		//=============================================================
		// Getter
		//=============================================================

		/// <summary>
		/// スケールを取得します。
		/// </summary>
		const Vector3& GetScale() const { return transform_.scale_; }

		/// <summary>
		/// 回転を取得します。
		/// </summary>
		const Vector3& GetRotate() const { return transform_.rotate_; }

		/// <summary>
		/// 平行移動を取得します。
		/// </summary>
		const Vector3& GetTranslate() const { return transform_.translate_; }

		/// <summary>
		/// アクティブ数を取得します。
		/// </summary>
		static int GetActiveCount() { return activeCount_; }

		/// <summary>
		/// 色を取得します。
		/// </summary>
		Vector4 GetColor() const { return materialData_ ? materialData_->color_ : Vector4{ 1,1,1,1 }; }

		/// <summary>
		/// ワールド行列を取得します。
		/// </summary>
		const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }

		/// <summary>
		/// モデルを取得します。
		/// </summary>
		TKM::Model* GetModel() const { return model_; }

		/// <summary>
		/// マテリアルのGPUアドレスを取得します。
		/// </summary>
		D3D12_GPU_VIRTUAL_ADDRESS GetMaterialGPUVirtualAddress() const {
			return materialResource_ ? materialResource_->GetGPUVirtualAddress() : 0;
		}

		/// <summary>
		/// WVPのGPUアドレスを取得します。
		/// </summary>
		D3D12_GPU_VIRTUAL_ADDRESS GetWVPGPUVirtualAddress() const {
			return wvpResource_ ? wvpResource_->GetGPUVirtualAddress() : 0;
		}

		/// <summary>
		/// 平行光源のGPUアドレスを取得します。
		/// </summary>
		D3D12_GPU_VIRTUAL_ADDRESS GetDirectionalLightGPUVirtualAddress() const {
			return materialResourceLight_ ? materialResourceLight_->GetGPUVirtualAddress() : 0;
		}

		/// <summary>
		/// カメラのGPUアドレスを取得します。
		/// </summary>
		D3D12_GPU_VIRTUAL_ADDRESS GetCameraGPUVirtualAddress() const {
			return cameraResource_ ? cameraResource_->GetGPUVirtualAddress() : 0;
		}

		/// <summary>
		/// 点光源のGPUアドレスを取得します。
		/// </summary>
		D3D12_GPU_VIRTUAL_ADDRESS GetPointLightGPUVirtualAddress() const {
			return pointLightResource_ ? pointLightResource_->GetGPUVirtualAddress() : 0;
		}

		/// <summary>
		/// スポットライトのGPUアドレスを取得します。
		/// </summary>
		D3D12_GPU_VIRTUAL_ADDRESS GetSpotLightGPUVirtualAddress() const {
			return spotLightResource_ ? spotLightResource_->GetGPUVirtualAddress() : 0;
		}

		/// <summary>
		/// 環境マップのGPUアドレスを取得します。
		/// </summary>
		D3D12_GPU_VIRTUAL_ADDRESS GetEnvironmentGPUVirtualAddress() const {
			return environment_ ? environment_->GetGPUVirtualAddress() : 0;
		}

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// スケールを設定します。
		/// </summary>
		void SetScale(const Vector3& scale) { this->transform_.scale_ = scale; }

		/// <summary>
		/// 回転を設定します。
		/// </summary>
		void SetRotate(const Vector3& rotate) { this->transform_.rotate_ = rotate; }

		/// <summary>
		/// 平行移動を設定します。
		/// </summary>
		void SetTranslate(const Vector3& translate) { this->transform_.translate_ = translate; }

		/// <summary>
		/// モデルを設定します。
		/// </summary>
		void SetModel(TKM::Model* model) { this->model_ = model; }

		/// <summary>
		/// カメラを設定します。
		/// </summary>
		void SetCamera(TKM::Camera* camera) { this->camera_ = camera; }

		/// <summary>
		/// 親シーンを設定します。
		/// </summary>
		void SetParentScene(TKM::BaseScene* parentScene);

		/// <summary>
		/// 環境マップを設定します。
		/// </summary>
		void SetEnvironment(const std::string& filename);

		/// <summary>
		/// 色を設定します。
		/// </summary>
		void SetColor(const Vector4& color) { if (materialData_) { materialData_->color_ = color; } }

		/// <summary>
		/// モデルをファイルパスから設定します。
		/// </summary>
		void SetModel(const std::string& filePath);

		/// <summary>
		/// 親オブジェクトを設定します。
		/// </summary>
		void SetParent(const TKM::Object3d* parent) { parent_ = parent; }

		/// <summary>
		/// Object3d側の色をモデル描画に使うか設定します。
		/// </summary>
		void SetUseObjectColor(bool use) { useObjectColor_ = use; }

	private:
		//=============================================================
		// 共通参照
		//=============================================================

		TKM::Object3dCommon* object3dCommon_ = nullptr; // 3Dオブジェクト共通管理
		TKM::DirectXCommon* dxCommon_ = nullptr;        // DirectX共通管理
		TKM::Model* model_ = nullptr;                   // 使用モデル
		TKM::Camera* camera_ = nullptr;                 // 使用カメラ
		TKM::BaseScene* parentScene_ = nullptr;         // 親シーン

		//=============================================================
		// モデルデータ
		//=============================================================

		ModelData modelData_; // 読み込み済みモデルデータ

		//=============================================================
		// 読み込み
		//=============================================================

		/// <summary>
		/// マテリアルテンプレートファイルを読み込みます。
		/// </summary>
		static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

		/// <summary>
		/// Objファイルを読み込みます。
		/// </summary>
		static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

		//=============================================================
		// GPUリソース
		//=============================================================

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;      // 頂点リソース
		VertexData* vertexData_ = nullptr;                           // 頂点データ書き込み先
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};                // 頂点バッファビュー

		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;    // マテリアルリソース
		Material* materialData_ = nullptr;                           // マテリアルデータ書き込み先

		Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;         // WVPリソース
		TransformationMatrix* wvpData_ = nullptr;                    // WVPデータ書き込み先

		Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceLight_; // 平行光源リソース
		DirectionalLightEX* directionalLightData_ = nullptr;           // 平行光源データ書き込み先

		Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;  // 点光源リソース
		PointLightEX* pointLightData_ = nullptr;                     // 点光源データ書き込み先

		Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;   // スポットライトリソース
		SpotLightEX* spotLightData_ = nullptr;                       // スポットライトデータ書き込み先

		Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;      // カメラリソース
		CameraForGPU* cameraData_ = nullptr;                         // カメラデータ書き込み先

		Microsoft::WRL::ComPtr<ID3D12Resource> environment_;         // 環境マップリソース
		EnvironmentEX* environmentData_ = nullptr;                   // 環境マップデータ書き込み先
		D3D12_GPU_DESCRIPTOR_HANDLE environmentSrvHandleGPU_{};      // 環境マップSRV

		//=============================================================
		// リソース生成
		//=============================================================

		/// <summary>
		/// 頂点リソースを生成します。
		/// </summary>
		void VertexResource(TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// マテリアルリソースを生成します。
		/// </summary>
		void MaterialResource(TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// WVPリソースを生成します。
		/// </summary>
		void WVPResource(TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// カメラリソースを生成します。
		/// </summary>
		void CameraResource(TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// 平行光源リソースを生成します。
		/// </summary>
		void Light(TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// 点光源リソースを生成します。
		/// </summary>
		void PointLight(DirectXCommon* dxCommon);

		/// <summary>
		/// スポットライトリソースを生成します。
		/// </summary>
		void SpotLight(TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// 環境マップリソースを生成します。
		/// </summary>
		void Environment(TKM::DirectXCommon* dxCommon);

		//=============================================================
		// Transform・状態
		//=============================================================

		Transform transform_;       // 自身のTransform
		Transform cameraTransform_; // カメラ用Transform

		bool useMonsterBall_ = true;          // SRV切り替えフラグ
		inline static int activeCount_ = 0;   // アクティブ数

		//=============================================================
		// 親子関係・行列
		//=============================================================

		const TKM::Object3d* parent_ = nullptr; // 親オブジェクト
		Matrix4x4 worldMatrix_{};               // ワールド行列

		//=============================================================
		// 色
		//=============================================================

		bool useObjectColor_ = false; // モデルの色の代わりにObject3d側の色を使うか
	};
}