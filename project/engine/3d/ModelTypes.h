#pragma once
#include "MyMath.h"
#include <string>
#include <vector>

//=============================================================
// ModelTypesクラス
// モデルデータ構造体定義
//=============================================================

// 座標変換情報
struct Transform {
	Vector3 scale_;
	Vector3 rotate_;
	Vector3 translate_;
};

// 頂点データ
struct VertexData {
	Vector4 position_;
	Vector2 texcoord_;
	Vector3 normal_;
};

// マテリアルデータ（読み込んだテクスチャパスなど）
struct MaterialData {
	std::string textureFilePath_;
	// テクスチャ番号
	uint32_t textureIndex_ = 0;
};

// サブメッシュ（マテリアルごとの頂点範囲）
struct SubMeshData {
	uint32_t startVertex_ = 0;
	uint32_t vertexCount_ = 0;
	MaterialData material_;
};

// モデルデータ
struct ModelData {
	// 座標変換情報
	std::vector<VertexData> vertices_;
	// 互換のため残す（単一マテリアル用、または先頭マテリアル）
	MaterialData material_;
	// マルチマテリアル用
	std::vector<SubMeshData> submeshes_;
};

// マテリアル（GPU に送る用）
struct Material {
	Vector4	color_;
	int32_t enableLighting_;
	float padding_[3];
	Matrix4x4 uvTransform_;
	float shininess_;//明るさ
};