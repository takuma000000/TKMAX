#pragma once
#include "DirectXCommon.h"
#include "MyMath.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "BaseScene.h"

class SpriteCommon;
class BaseScene;

//=============================================================
// Spriteクラス
// 2Dスプライト描画を行うクラス。
//=============================================================
namespace TKM {
	class Sprite {
	public:

		Sprite();
		~Sprite();
		// 変換情報
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

		// Gettet=====================================
		/// <summary>
		/// 位置の取得。
		/// </summary>
		/// <returns></returns>
		const Vector2& GetPosition() const { return position_; }
		/// <summary>
		/// 変換情報の取得。
		/// </summary>
		/// <returns></returns>
		const Transform& GetTransform() const { return transform_; }
		/// <summary>
		/// 回転角の取得。
		/// </summary>
		/// <returns></returns>
		float GetRotation() const { return rotation_; }
		/// <summary>
		/// 色の取得。
		/// </summary>
		/// <returns></returns>
		const Vector4& GetColor() const { return materialData_->color; }
		/// <summary>
		/// サイズの取得。
		/// </summary>
		/// <returns></returns>
		const Vector2& GetSize() const { return size_; }
		/// <summary>
		/// アンカーポイントの取得。
		/// </summary>
		/// <returns></returns>
		const Vector2& GetAnchorPoint() const { return anchorPoint_; }
		/// <summary>
		/// 左右フリップの取得。
		/// </summary>
		/// <returns></returns>
		bool GetIsFlipX() const { return isFlipX_; }
		/// <summary>
		/// 上下フリップの取得。
		/// </summary>
		/// <returns></returns>
		bool GetIsFlipY() const { return isFlipY_; }
		/// <summary>
		/// テクスチャ左上座標の取得。
		/// </summary>
		/// <returns></returns>
		const Vector2& GetTextureLeftTop() const { return textureLeftTop_; }
		/// <summary>
		/// テクスチャ切り出しサイズの取得。
		/// </summary>
		/// <returns></returns>
		const Vector2& GetTextureSize() const { return textureSize_; }
		/// <summary>
		/// アクティブスプライト数の取得。
		/// </summary>
		/// <returns></returns>
		static int GetActiveCount() { return activeCount_; }
		/// <summary>
		/// 親シーンの取得。
		/// </summary>
		/// <returns></returns>
		bool GetAutoAdjustTextureSize() const { return autoAdjustTextureSize_; }
		// ===========================================
		// Settet=====================================
		/// <summary>
		/// 位置の設定。
		/// </summary>
		/// <param name="position"></param>
		void SetPosition(const Vector2& position) { this->position_ = position; }
		/// <summary>
		/// 変換情報の設定。
		/// </summary>
		/// <param name="transform"></param>
		void SetTransform(const Transform& transform) { this->transform_ = transform; }
		/// <summary>
		/// 回転角の設定。
		/// </summary>
		/// <param name="rotation"></param>
		void SetRotation(float rotation) { this->rotation_ = rotation; }
		/// <summary>
		/// 色の設定。
		/// </summary>
		/// <param name="color"></param>
		void SetColor(const Vector4& color) { materialData_->color = color; }
		/// <summary>
		/// サイズの設定。
		/// </summary>
		/// <param name="size"></param>
		void SetSize(const Vector2& size) { this->size_ = size; }
		/// <summary>
		/// アンカーポイントの設定。
		/// </summary>
		/// <param name="anchorPoint"></param>
		void SetAnchorPoint(const Vector2& anchorPoint) { this->anchorPoint_ = anchorPoint; }
		/// <summary>
		/// 左右フリップの設定。
		/// </summary>
		/// <param name="isFlipX"></param>
		void SetIsFlipX(bool isFlipX) { this->isFlipX_ = isFlipX; }
		/// <summary>
		/// 上下フリップの設定。
		/// </summary>
		/// <param name="isFlipY"></param>
		void SetIsFlipY(bool isFlipY) { this->isFlipY_ = isFlipY; }
		/// <summary>
		/// テクスチャ左上座標の設定。
		/// </summary>
		/// <param name="textureLeftTop"></param>
		void SetTextureLeftTop(const Vector2& textureLeftTop) { this->textureLeftTop_ = textureLeftTop; }
		/// <summary>
		/// テクスチャ切り出しサイズの設定。
		/// </summary>
		/// <param name="textureSize"></param>
		void SetTextureSize(const Vector2& textureSize) { this->textureSize_ = textureSize; }
		/// <summary>
		/// 親シーンの設定。
		/// </summary>
		/// <param name="parentScene"></param>
		void SetParentScene(BaseScene* parentScene);
		/// <summary>
		/// テクスチャサイズの自動調整の有効化・無効化。
		/// </summary>
		/// <param name="enable"></param>
		void SetAutoAdjustTextureSize(bool enable) { autoAdjustTextureSize_ = enable; }
		// ===========================================

	public://メンバ関数
		/// <summary>
		/// スプライトを初期化します。
		/// </summary>
		/// <param name="spriteCommon"></param>
		/// <param name="dxCommon"></param>
		/// <param name="textureFilePath"></param>
		void Initialize(SpriteCommon* spriteCommon, TKM::DirectXCommon* dxCommon, const std::string textureFilePath);
		/// <summary>
		/// スプライトを更新します。
		/// </summary>
		void Update();
		/// <summary>
		/// スプライトを描画します。
		/// </summary>
		void Draw();
		/// <summary>
		/// ImGuiデバッグ表示。
		/// </summary>
		void ImGuiDebug();

	private:
		SpriteCommon* spriteCommon_ = nullptr;
		TKM::DirectXCommon* dxCommon_;

		//バッファリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_ = nullptr;
		//バッファリソース内のデータを指すポインタ
		VertexData* vertexData_ = nullptr;
		uint32_t* indexData_ = nullptr;
		Material* materialData_ = nullptr;
		TransformationMatrix* transformationMatrixData_ = nullptr;
		//バッファリソースの使い道を補足するバッファビュー
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

		Vector2 position_ = { 100.0f,100.0f };

		Transform transformSprite_;
		Transform cameraTransform_;
		Transform transform_;

		//回転
		float rotation_ = 0.0f;

		//サイズ
		Vector2 size_ = { 360.0f,360.0f };

		//テクスチャ番号
		uint32_t textureIndex_ = 0;

		//アンカーポイント
		Vector2 anchorPoint_ = { 0.0f,0.0f };
		//左右フリップ
		bool isFlipX_ = false;
		//上下フリップ
		bool isFlipY_ = false;

		//テクスチャ左上座標
		Vector2 textureLeftTop_ = { 0.0f,0.0f };
		//テクスチャ切り出しサイズ
		Vector2 textureSize_ = { 64.0f,64.0f };

		/// <summary>
		/// テクスチャサイズを調整する。
		/// </summary>
		void AdjustTextureSize();

		//ファイルパスを保存するメンバー変数
		std::string textureFilePath_;

		BaseScene* parentScene_ = nullptr;

		inline static int activeCount_ = 0;

		bool autoAdjustTextureSize_ = true; // テクスチャサイズ自動調整フラグ
	};
} // namespace TKM