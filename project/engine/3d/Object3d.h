#pragma once
#include "MyMath.h"
#include <string>
#include <vector>
#include "DirectXCommon.h"
#include "Vector3.h"
#include "Vector4.h"

class Object3dCommon;
class Model;
class Camera;
class BaseScene;

struct Vector2 {
	float x;
	float y;
};

struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

//頂点データ
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct MaterialData {
	std::string textureFilePath;
	//テクスチャ番号
	uint32_t textureIndex = 0;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};

//マテリアルデータ
struct Material {
	Vector4	color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
	float shininess;//明るさ
};

//座標変換行列データ
struct TransformationMatrix {
	Matrix4x4 wvp;
	Matrix4x4 World;
	Matrix4x4 WorldInverseTranspose;
};

struct DirectionalLightEX {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

struct PointLightEX {
	Vector4 color;
	Vector3 position;
	float intensity;
	float radius;
	float decay;
	float padding[2];
};

struct SpotLightEX {
	Vector4 color;
	Vector3 position;
	float intensity;
	Vector3 direction;
	float distance;
	float decay;
	float cosAngle;
	float cosFalloffStart;
	float padding[2];
};

struct CameraForGPU{
	Vector3 worldPosition;//カメラの位置
	float padding;//16byte境界に合わせるためのパディング
};

class Object3d
{

public://メンバ関数
	void Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon);
	void Update();
	void Draw(DirectXCommon* dxCommon);
	void SetModel(const std::string& filePath);

	void TransferMatrix();

public:
	//getter
	const Vector3& GetScale() const { return transform.scale; }
	const Vector3& GetRotate() const { return transform.rotate; }
	const Vector3& GetTranslate() const { return transform.translate; }
	//setter
	void SetScale(const Vector3& scale) { this->transform.scale = scale; }
	void SetRotate(const Vector3& rotate) { this->transform.rotate = rotate; }
	void SetTranslate(const Vector3& translate) { this->transform.translate = translate; }
	void SetModel(Model* model) { this->model_ = model; }
	void SetCamera(Camera* camera) { this->camera = camera; }
	void SetParentScene(BaseScene* parentScene);

private:
	Object3dCommon* object3dCommon = nullptr;
	DirectXCommon* dxCommon_;
	Model* model_ = nullptr;
	Camera* camera = nullptr;

	//Objファイルのデータ
	ModelData modelData;

	//.mtlファイル読み込み
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	//.objファイル読み込み
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

	//頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	//頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	//頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	//マテリアル用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	//マテリアルにデータを書き込む
	Material* materialData = nullptr;

	//WVP用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
	//データを書き込む
	TransformationMatrix* wvpData = nullptr;

	//Light用のマテリアルリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceLight;
	//データを書き込む
	DirectionalLightEX* directionalLightData = nullptr;

	//PointLight用のマテリアルリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource;
	//データを書き込む
	PointLightEX* pointLightData = nullptr;

	//SpotLight用のマテリアルリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource;
	//データを書き込む
	SpotLightEX* spotLightData = nullptr;

	//カメラ用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource;
	//データを書き込む
	CameraForGPU* cameraData = nullptr;

	//VertexResource関数
	void VertexResource(DirectXCommon* dxCommon);
	//materialResource関数
	void MaterialResource(DirectXCommon* dxCommon);
	//wvpResource関数
	void WVPResource(DirectXCommon* dxCommon);
	//cameraResource関数
	void CameraResource(DirectXCommon* dxCommon);
	//Light関数
	void Light(DirectXCommon* dxCommon);
	//PointLight関数
	void PointLight(DirectXCommon* dxCommon);
	//SpotLight関数
	void SpotLight(DirectXCommon* dxCommon);

	Transform transform;
	Transform cameraTransform;

	//SRV切り替え
	bool useMonsterBall = true;

	BaseScene* parentScene_ = nullptr;
};

