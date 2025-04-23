#pragma once
#include "DirectXCommon.h"
#include "Matrix4x4.h"
#include "Vector3.h"
#include "Vector4.h"

class SpriteCommon;
class BaseScene;

class Sprite
{

public:
	struct Vector2 {
		float x;
		float y;
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
	const Vector2& GetAnchorPoint() const { return anchorPoint; }
	bool GetIsFlipX() const { return isFlipX_; }
	bool GetIsFlipY() const { return isFlipY_; }
	const Vector2& GetTextureLeftTop() const { return textureLeftTop; }
	const Vector2& GetTextureSize() const { return textureSize; }
	//setter
	void SetPosition(const Vector2& position) { this->position = position; }
	void SetTransform(const Transform& transform) { this->transform = transform; }
	void SetRotation(float rotation) { this->rotation = rotation; }
	void SetColor(const Vector4& color) { materialData->color = color; }
	void SetSize(const Vector2& size) { this->size = size; }
	void SetAnchorPoint(const Vector2& anchorPoint) { this->anchorPoint = anchorPoint; }
	void SetIsFlipX(bool isFlipX) { this->isFlipX_ = isFlipX; }
	void SetIsFlipY(bool isFlipY) { this->isFlipY_ = isFlipY; }
	void SetTextureLeftTop(const Vector2& textureLeftTop) { this->textureLeftTop = textureLeftTop; }
	void SetTextureSize(const Vector2& textureSize) { this->textureSize = textureSize; }
	void SetParentScene(BaseScene* parentScene);
	void SetTextureIndex(uint32_t index) { textureIndex = index; }

public://メンバ関数
	void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, std::string textureFilePath);
	void Update();
	void Draw();

	// ImGuiのデバッグ処理
	void ImGuiDebug();

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

	Vector2 position = { 100.0f,100.0f };

	Transform transformSprite;
	Transform cameraTransform;
	Transform transform;

	//回転
	float rotation = 0.0f;

	//サイズ
	Vector2 size = { 360.0f,360.0f };

	//テクスチャ番号
	uint32_t textureIndex = 0;

	//アンカーポイント
	Vector2 anchorPoint = { 0.0f,0.0f };
	//左右フリップ
	bool isFlipX_ = false;
	//上下フリップ
	bool isFlipY_ = false;

	//テクスチャ左上座標
	Vector2 textureLeftTop = { 0.0f,0.0f };
	//テクスチャ切り出しサイズ
	Vector2 textureSize = { 64.0f,64.0f };

	//テクスチャサイズをイメージに合わせる
	void AdjustTextureSize();

	//ファイルパスを保存するメンバー変数
	std::string textureFilePath;

	BaseScene* parentScene_ = nullptr;

};