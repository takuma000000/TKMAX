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

//座標変換行列データ
struct TransformationMatrix {
	Matrix4x4 wvp_;
	Matrix4x4 World_;
	Matrix4x4 WorldInverseTranspose_;
};

//ライト構造体
struct DirectionalLightEX {
	Vector4 color_;
	Vector3 direction_;
	float intensity_;
};

//PointLight構造体
struct PointLightEX {
	Vector4 color_;
	Vector3 position_;
	float intensity_;
	float radius_;
	float decay_;
	float padding_[2];
};

//SpotLight構造体
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

//カメラ構造体
struct CameraForGPU {
	Vector3 worldPosition_;//カメラの位置
	float padding_;//16byte境界に合わせるためのパディング
};

// 環境マップ構造体
struct EnvironmentEX {
	bool useEnvironment_ = false; // 環境マップを使用するかどうか
	Vector3 padding_;
};

//=============================================================
// Object3dクラス
// 3Dオブジェクトの描画・変換・ライト設定を行うクラス。
//=============================================================
namespace TKM {
	class Object3d {

	public://メンバ関数

		Object3d(); // コンストラクタ
		~Object3d(); // デストラクタ

		/// <summary>
		/// 3Dオブジェクトを初期化します。
		/// </summary>
		/// <param name="object3dCommon"></param>
		/// <param name="dxCommon"></param>
		void Initialize(TKM::Object3dCommon* object3dCommon, TKM::DirectXCommon* dxCommon);
		/// <summary>
		/// 3Dオブジェクトを終了します。
		/// </summary>
		void Update();
		/// <summary>
		/// 3Dオブジェクトを描画します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void Draw(TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// 親オブジェクトのクリア。
		/// </summary>
		void ClearParent() { parent_ = nullptr; }

		// Getter===================================
		/// <summary>スケール、回転、平行移動の取得。</summary>
		const Vector3& GetScale() const { return transform_.scale_; }
		/// <summary>回転の取得。</summary>
		const Vector3& GetRotate() const { return transform_.rotate_; }
		/// <summary>平行移動の取得。</summary>
		const Vector3& GetTranslate() const { return transform_.translate_; }
		/// <summary>アクティブオブジェクト数の取得。</summary>
		static int GetActiveCount() { return activeCount_; }
		/// <summary>モデルの取得。</summary>
		Vector4 GetColor() const { return materialData_ ? materialData_->color_ : Vector4{ 1,1,1,1 }; }
		/// <summary>
		/// ワールド行列の取得。
		/// </summary>
		/// <returns></returns>
		const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
		/// <summary>
		/// モデルの取得。
		/// </summary>
		/// <returns></returns>
		TKM::Model* GetModel() const { return model_; }
		/// <summary>
		/// カメラの取得。
		/// </summary>
		/// <returns></returns>
		D3D12_GPU_VIRTUAL_ADDRESS GetMaterialGPUVirtualAddress() const {
			return materialResource_ ? materialResource_->GetGPUVirtualAddress() : 0;
		}
		/// <summary>
		/// ワールドビュー射影行列の取得。
		/// </summary>
		/// <returns></returns>
		D3D12_GPU_VIRTUAL_ADDRESS GetWVPGPUVirtualAddress() const {
			return wvpResource_ ? wvpResource_->GetGPUVirtualAddress() : 0;
		}
		/// <summary>
		/// ライトの取得。
		/// </summary>
		/// <returns></returns>
		D3D12_GPU_VIRTUAL_ADDRESS GetDirectionalLightGPUVirtualAddress() const {
			return materialResourceLight_ ? materialResourceLight_->GetGPUVirtualAddress() : 0;
		}
		/// <summary>
		/// カメラの取得。
		/// </summary>
		/// <returns></returns>
		D3D12_GPU_VIRTUAL_ADDRESS GetCameraGPUVirtualAddress() const {
			return cameraResource_ ? cameraResource_->GetGPUVirtualAddress() : 0;
		}
		/// <summary>
		/// ポイントライトの取得。
		/// </summary>
		/// <returns></returns>
		D3D12_GPU_VIRTUAL_ADDRESS GetPointLightGPUVirtualAddress() const {
			return pointLightResource_ ? pointLightResource_->GetGPUVirtualAddress() : 0;
		}
		/// <summary>
		/// スポットライトの取得。
		/// </summary>
		/// <returns></returns>
		D3D12_GPU_VIRTUAL_ADDRESS GetSpotLightGPUVirtualAddress() const {
			return spotLightResource_ ? spotLightResource_->GetGPUVirtualAddress() : 0;
		}
		/// <summary>
		/// 環境マップの取得。
		/// </summary>
		/// <returns></returns>
		D3D12_GPU_VIRTUAL_ADDRESS GetEnvironmentGPUVirtualAddress() const {
			return environment_ ? environment_->GetGPUVirtualAddress() : 0;
		}
		// =========================================
		// Setter===================================
		/// <summary>
		/// スケール設定。
		/// </summary>
		/// <param name="scale"></param>
		void SetScale(const Vector3& scale) { this->transform_.scale_ = scale; }
		/// <summary>
		/// 回転の設定。
		/// </summary>
		/// <param name="rotate"></param>
		void SetRotate(const Vector3& rotate) { this->transform_.rotate_ = rotate; }
		/// <summary>
		/// 平行移動の設定。
		/// </summary>
		/// <param name="translate"></param>
		void SetTranslate(const Vector3& translate) { this->transform_.translate_ = translate; }
		/// <summary>
		/// モデルの設定。
		/// </summary>
		/// <param name="model"></param>
		void SetModel(TKM::Model* model) { this->model_ = model; }
		/// <summary>
		/// カメラの設定。
		/// </summary>
		/// <param name="camera"></param>
		void SetCamera(TKM::Camera* camera) { this->camera_ = camera; }
		/// <summary>
		/// 親シーンの設定。
		/// </summary>
		/// <param name="parentScene"></param>
		void SetParentScene(TKM::BaseScene* parentScene);
		/// <summary>
		/// 環境マップの設定。
		/// </summary>
		/// <param name="filename"></param>
		void SetEnvironment(const std::string& filename);
		/// <summary>
		/// 色の設定。
		/// </summary>
		/// <param name="color"></param>
		void SetColor(const Vector4& color) { if (materialData_) { materialData_->color_ = color; } }
		/// <summary>
		/// デバッグ用ImGui表示。
		/// </summary>
		/// <param name="filePath"></param>
		void SetModel(const std::string& filePath);
		/// <summary>
		/// 親オブジェクトの設定。
		/// </summary>
		/// <param name="parent"></param>
		void SetParent(const TKM::Object3d* parent) { parent_ = parent; }
		// =========================================
	private:
		TKM::Object3dCommon* object3dCommon_ = nullptr;
		TKM::DirectXCommon* dxCommon_;
		TKM::Model* model_ = nullptr;
		TKM::Camera* camera_ = nullptr;

		//Objファイルのデータ
		ModelData modelData_;

		/// <summary>
		/// マテリアルテンプレートファイルを読み込みます。
		/// </summary>
		/// <param name="directoryPath"></param>
		/// <param name="filename"></param>
		/// <returns></returns>
		static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
		/// <summary>
		/// Objファイルを読み込みます。
		/// </summary>
		/// <param name="directoryPath"></param>
		/// <param name="filename"></param>
		/// <returns></returns>
		static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

		//頂点リソースを作る
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
		//頂点リソースにデータを書き込む
		VertexData* vertexData_ = nullptr;
		//頂点バッファビューを作成する
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

		//マテリアル用のリソースを作る
		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
		//マテリアルにデータを書き込む
		Material* materialData_ = nullptr;

		//WVP用のリソースを作る
		Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
		//データを書き込む
		TransformationMatrix* wvpData_ = nullptr;

		//Light用のマテリアルリソースを作る
		Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceLight_;
		//データを書き込む
		DirectionalLightEX* directionalLightData_ = nullptr;

		//PointLight用のマテリアルリソースを作る
		Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
		//データを書き込む
		PointLightEX* pointLightData_ = nullptr;

		//SpotLight用のマテリアルリソースを作る
		Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
		//データを書き込む
		SpotLightEX* spotLightData_ = nullptr;

		//カメラ用のリソースを作る
		Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
		//データを書き込む
		CameraForGPU* cameraData_ = nullptr;

		// 映り込み
		Microsoft::WRL::ComPtr<ID3D12Resource> environment_;
		// 環境マップデータ
		EnvironmentEX* environmentData_ = nullptr;
		// 環境マップ...GPUハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE environmentSrvHandleGPU_;

		/// <summary>
		/// 頂点リソースを作成します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void VertexResource(TKM::DirectXCommon* dxCommon);
		/// <summary>
		/// マテリアルリソースを作成します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void MaterialResource(TKM::DirectXCommon* dxCommon);
		/// <summary>
		/// WVPリソースを作成します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void WVPResource(TKM::DirectXCommon* dxCommon);
		/// <summary>
		/// カメラリソースを作成します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void CameraResource(TKM::DirectXCommon* dxCommon);
		/// <summary>
		/// Lightリソースを作成します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void Light(TKM::DirectXCommon* dxCommon);
		/// <summary>
		/// PointLightリソースを作成します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void PointLight(DirectXCommon* dxCommon);
		/// <summary>
		/// SpotLightリソースを作成します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void SpotLight(TKM::DirectXCommon* dxCommon);
		/// <summary>
		/// 環境マップリソースを作成します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void Environment(TKM::DirectXCommon* dxCommon);

		Transform transform_;
		Transform cameraTransform_;

		//SRV切り替え
		bool useMonsterBall_ = true;

		TKM::BaseScene* parentScene_ = nullptr;

		inline static int activeCount_ = 0; // 静的メンバ変数

		// 親オブジェクトへのポインタ
		const TKM::Object3d* parent_ = nullptr;
		Matrix4x4 worldMatrix_{}; // ワールド行列
	};
}