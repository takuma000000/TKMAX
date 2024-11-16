#pragma once
#include "MyMath.h"
#include <string>
#include <vector>
#include "DirectXCommon.h"

class Object3dCommon;
class Model;
class Camera;

struct Vector2 {
	float x;
	float y;
};

struct Vector4 {
	float x;
	float y;
	float z;
	float w;
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
};

//座標変換行列データ
struct TransformationMatrix {
	Matrix4x4 wvp;
	Matrix4x4 World;
};

struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

class Object3d
{

public://メンバ関数
	void Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon);
	void Update();
	void Draw(DirectXCommon* dxCommon);
	void SetModel(const std::string& filePath);

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
	DirectionalLight* directionalLightData = nullptr;

	//VertexResource関数
	void VertexResource(DirectXCommon* dxCommon);
	//materialResource関数
	void MaterialResource(DirectXCommon* dxCommon);
	//wvpResource関数
	void WVPResource(DirectXCommon* dxCommon);
	//Light関数
	void Light(DirectXCommon* dxCommon);

	Transform transform;
	Transform cameraTransform;

	//SRV切り替え
	bool useMonsterBall = true;
};

