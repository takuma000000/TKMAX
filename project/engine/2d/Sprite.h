#pragma once
#include "DirectXCommon.h"
#include "MyMath.h"

class SpriteCommon;
class BaseScene;

//=============================================================
// Spriteクラス
// 2Dスプライト描画を行うクラス。
//=============================================================
class Sprite
{

public:

	Sprite();
	~Sprite();

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
	/// <summary>各種ゲッター。</summary>
	/// <returns>各種値。</returns>
	const Vector2& GetPosition() const { return position; }
	///<summary>変換情報の取得。</summary>
	///<returns>変換情報。</returns>
	const Transform& GetTransform() const { return transform; }
	///<summary>回転角の取得。</summary>
	///<returns>回転角。</returns>
	float GetRotation() const { return rotation; }
	///<summary>色の取得。</summary>
	///<returns>色。</returns>
	const Vector4& GetColor() const { return materialData->color; }
	///<summary>サイズの取得。</summary>
	///<returns>サイズ。</returns>
	const Vector2& GetSize() const { return size; }
	///<summary>アンカーポイントの取得。</summary>
	///<returns>アンカーポイント。</returns>
	const Vector2& GetAnchorPoint() const { return anchorPoint; }
	///<summary>左右フリップの取得。</summary>
	///<returns>左右フリップ。</returns>
	bool GetIsFlipX() const { return isFlipX_; }
	///<summary>上下フリップの取得。</summary>
	///<returns>上下フリップ。</returns>
	bool GetIsFlipY() const { return isFlipY_; }
	///<summary>テクスチャ左上座標の取得。</summary>
	///<returns>テクスチャ左上座標。</returns>
	const Vector2& GetTextureLeftTop() const { return textureLeftTop; }
	///<summary>テクスチャ切り出しサイズの取得。</summary>
	///<returns>テクスチャ切り出しサイズ。</returns>
	const Vector2& GetTextureSize() const { return textureSize; }
	///<summary>アクティブスプライト数の取得。</summary>
	///<returns>アクティブスプライト数。</returns>
	static int GetActiveCount() { return activeCount_; }
	//setter

	//
	/// <summary>各種セッター。</summary>
	/// <param name="value">各種値。</param>
	void SetPosition(const Vector2& position) { this->position = position; }
	///<summary>変換情報の設定。</summary>
	///<param name="transform">変換情報。</param>
	void SetTransform(const Transform& transform) { this->transform = transform; }
	///<summary>回転角の設定。</summary>
	///<param name="rotation">回転角。</param>
	void SetRotation(float rotation) { this->rotation = rotation; }
	///<summary>色の設定。</summary>
	///<param name="color">色。</param>
	void SetColor(const Vector4& color) { materialData->color = color; }
	///<summary>サイズの設定。</summary>
	///<param name="size">サイズ。</param>
	void SetSize(const Vector2& size) { this->size = size; }
	///<summary>アンカーポイントの設定。</summary>
	///<param name="anchorPoint">アンカーポイント。</param>
	void SetAnchorPoint(const Vector2& anchorPoint) { this->anchorPoint = anchorPoint; }
	///<summary>左右フリップの設定。</summary>
	///<param name="isFlipX">左右フリップ。</param>
	void SetIsFlipX(bool isFlipX) { this->isFlipX_ = isFlipX; }
	///<summary>上下フリップの設定。</summary>
	///<param name="isFlipY">上下フリップ。</param>
	void SetIsFlipY(bool isFlipY) { this->isFlipY_ = isFlipY; }
	///<summary>テクスチャ左上座標の設定。</summary>
	///<param name="textureLeftTop">テクスチャ左上座標。</param>
	void SetTextureLeftTop(const Vector2& textureLeftTop) { this->textureLeftTop = textureLeftTop; }
	///<summary>テクスチャ切り出しサイズの設定。</summary>
	///<param name="textureSize">テクスチャ切り出しサイズ。</param>
	void SetTextureSize(const Vector2& textureSize) { this->textureSize = textureSize; }
	///<summary>親シーンの設定。</summary>
	///<param name="parentScene">親シーン。</param>
	void SetParentScene(BaseScene* parentScene);

public://メンバ関数
	/// <summary>スプライトを初期化します。</summary>
	/// <param name="spriteCommon">スプライト共通設定。</param>
	/// <param name="dxCommon">DirectX共通。</param>
	/// <param name="textureFilePath">使用するテクスチャのファイルパス。</param>
	void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, const std::string textureFilePath);
	/// <summary>スプライトを終了します。</summary>
	void Update();
	/// <summary>スプライトを描画します。</summary>
	void Draw();

	// ImGuiのデバッグ処理
	/// <summary>デバッグ用ImGui表示。</summary>
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
	/// <summary>テクスチャサイズをイメージに合わせる。</summary>
	void AdjustTextureSize();

	//ファイルパスを保存するメンバー変数
	std::string textureFilePath;

	BaseScene* parentScene_ = nullptr;

	inline static int activeCount_ = 0;

};