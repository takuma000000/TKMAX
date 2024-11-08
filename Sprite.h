#pragma once
#include "DirectXCommon.h"
#include "Matrix4x4.h"
#include "Vector3.h"

class SpriteCommon;

class Sprite
{

public:
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

	/*struct Matrix4x4 {
		float m[4][4];
	};*/

	//頂点データ
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

	//マテリアルデータ
	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

	//座標変換行列データ.
	struct TransformationMatrix {
		Matrix4x4 wvp;
		Matrix4x4 World;
	};

	//getter
	const Vector2& GetPosition() const { return position; }
	const Transform& GetTransform() const { return transform; }
	float GetRotation() const { return rotation; }
	const Vector4& GetColor() const { return materialData->color; }
	const Vector2& GetSize() const { return size; }
	//setter
	void SetPosition(const Vector2& position) { this->position = position; }
	void SetTransform(const Transform& transform) { this->transform = transform; }
	void SetRotation(float rotation) { this->rotation = rotation; }
	void SetColor(const Vector4& color) { materialData->color = color; }
	void SetSize(const Vector2& size) { this->size = size; }

public://メンバ関数
	void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon);
	void Update();
	void Draw(D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU);

private:
	SpriteCommon* spriteCommon = nullptr;
	DirectXCommon* dxCommon_;

	//バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource = nullptr;
	//バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;
	Material* materialData = nullptr;
	TransformationMatrix* transformationMatrixData = nullptr;
	//バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};

	Vector2 position = { 0.0f,0.0f };

	Transform transformSprite;
	Transform cameraTransform;
	Transform transform;

	//回転
	float rotation = 0.0f;

	//サイズ
	Vector2 size = { 640.0f,360.0f };

};