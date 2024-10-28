#pragma once
#include "DirectXCommon.h"

class SpriteCommon;

class Sprite
{

	struct Vector2 {
		float x;
		float y;
	};

	struct Vector3 {
		float x;
		float y;
		float z;
	};

	struct Vector4 {
		float x;
		float y;
		float z;
		float w;
	};

	//頂点データ
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

	struct MaterialData {
		std::string textureFilePath;
	};

	struct ModelData {
		std::vector<VertexData> vertices;
		MaterialData material;
	};

public://メンバ関数
	void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon);

private:
	SpriteCommon* spriteCommon = nullptr;
	DirectXCommon* dxCommon_;

	//バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;
	//バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;
	//バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};

};