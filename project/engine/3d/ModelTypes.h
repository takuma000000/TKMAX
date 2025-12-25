#pragma once
#include "MyMath.h"
#include <string>
#include <vector>

// 座標変換情報
struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

// 頂点データ
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

// マテリアルデータ（読み込んだテクスチャパスなど）
struct MaterialData {
	std::string textureFilePath;
	// テクスチャ番号
	uint32_t textureIndex = 0;
};

// モデルデータ
struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};

// マテリアル（GPU に送る用）
struct Material {
	Vector4	color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
	float shininess;//明るさ
};