#pragma once
#include "DirectXCommon.h"
#include "MyMath.h"
#include <string>
#include <vector>

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

class ModelCommon;

class Model
{
private:
	ModelCommon* modelCommon_ = nullptr;
	DirectXCommon* dxCommon_;

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

	//VertexResource関数
	void VertexResource(DirectXCommon* dxCommon);
	//materialResource関数
	void MaterialResource(DirectXCommon* dxCommon);

public://メンバ関数
	void Initialize(ModelCommon* modelCommon, DirectXCommon* dxCommon);
	void Draw();

public:


};

