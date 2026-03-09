#pragma once
#include "DirectXCommon.h"
#include "MyMath.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "BaseScene.h"
#include "Transform.h"

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

		// Transformクラスのエイリアス
		using Transform = TKM::Transform;

		//頂点データ
		struct VertexData {
			Vector4 position;
			Vector2 texcoord;
			Vector3 normal;
		};

		//マテリアルデータ
		struct Material {
			Vector4 color;       // 通常色
			Vector4 glowColor;   // 発光色

			Vector4 glowParam;   // x=intensity, y=width, z=threshold, w=softness

			int32_t glowEnabled = 0;
			float padding[3] = {};

			Matrix4x4 uvTransform;
		};

		//座標変換行列データ.
		struct TransformationMatrix {
			Matrix4x4 wvp;
			Matrix4x4 World;
		};

		/// <summary>
		/// スプライトを初期化します。
		/// </summary>
		/// <param name="spriteCommon">スプライト共通管理クラス</param>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="textureFilePath">テクスチャファイルのパス</param>
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
		// Setter=====================================
		/// <summary>
		/// 位置の設定。
		/// </summary>
		/// <param name="position"></param>
		void SetPosition(const Vector2& position);
		/// <summary>
		/// 変換情報の設定。
		/// </summary>
		/// <param name="transform"></param>
		void SetTransform(const Transform& transform);
		/// <summary>
		/// 回転角の設定。
		/// </summary>
		/// <param name="rotation"></param>
		void SetRotation(float rotation);
		/// <summary>
		/// 色の設定。
		/// </summary>
		/// <param name="color"></param>
		void SetColor(const Vector4& color);
		/// <summary>
		/// サイズの設定。
		/// </summary>
		/// <param name="size"></param>
		void SetSize(const Vector2& size);
		/// <summary>
		/// アンカーポイントの設定。
		/// </summary>
		/// <param name="anchorPoint"></param>
		void SetAnchorPoint(const Vector2& anchorPoint);
		/// <summary>
		/// 左右フリップの設定。
		/// </summary>
		/// <param name="isFlipX"></param>
		void SetIsFlipX(bool isFlipX);
		/// <summary>
		/// 上下フリップの設定。
		/// </summary>
		/// <param name="isFlipY"></param>
		void SetIsFlipY(bool isFlipY);
		/// <summary>
		/// テクスチャ左上座標の設定。
		/// </summary>
		/// <param name="textureLeftTop"></param>
		void SetTextureLeftTop(const Vector2& textureLeftTop);
		/// <summary>
		/// テクスチャ切り出しサイズの設定。
		/// </summary>
		/// <param name="textureSize"></param>
		void SetTextureSize(const Vector2& textureSize);
		/// <summary>
		/// 親シーンの設定。
		/// </summary>
		/// <param name="parentScene"></param>
		void SetParentScene(BaseScene* parentScene);
		/// <summary>
		/// テクスチャサイズの自動調整の有効化・無効化。
		/// </summary>
		/// <param name="enable"></param>
		void SetAutoAdjustTextureSize(bool enable);
		/// <summary>
		/// 発光の有効 / 無効を設定。
		/// </summary>
		void SetGlowEnabled(bool enable);
		/// <summary>
		/// 発光色を設定。
		/// </summary>
		void SetGlowColor(const Vector4& color);
		/// <summary>
		/// 発光の強さを設定。
		/// </summary>
		void SetGlowIntensity(float intensity);
		/// <summary>
		/// 発光の広がりを設定。
		/// </summary>
		void SetGlowWidth(float width);
		/// <summary>
		/// 輪郭判定のしきい値を設定。
		/// </summary>
		void SetGlowThreshold(float threshold);
		/// <summary>
		/// 輪郭のにじみ具合を設定。
		/// </summary>
		void SetGlowSoftness(float softness);
		/// <summary>
		/// 発光パラメータをまとめて設定。
		/// </summary>
		void SetGlowParams(bool enable, const Vector4& color, float intensity, float width, float threshold = 0.05f, float softness = 2.0f);
		// ===========================================

	private:
		//======================================================================
		// 外部参照
		//======================================================================
		SpriteCommon* spriteCommon_ = nullptr;
		TKM::DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;
		//======================================================================
		// GPUリソース（バッファ）
		//======================================================================
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
		//======================================================================
		// Transform / 表示パラメータ
		//======================================================================
		Vector2 position_ = { 100.0f,100.0f };
		// スプライトの変換情報（スケール、回転、平行移動）をまとめた構造体。これを元にワールド行列を計算する。
		Transform transformSprite_;
		Transform cameraTransform_;
		Transform transform_;
		//回転
		float rotation_ = 0.0f;
		//サイズ
		Vector2 size_ = { 360.0f,360.0f };
		//======================================================================
		// テクスチャ
		//======================================================================
		//テクスチャ番号
		uint32_t textureIndex_ = 0;
		//ファイルパスを保存するメンバー変数
		std::string textureFilePath_;
		//テクスチャ左上座標
		Vector2 textureLeftTop_ = { 0.0f,0.0f };
		//テクスチャ切り出しサイズ
		Vector2 textureSize_ = { 64.0f,64.0f };
		bool autoAdjustTextureSize_ = true; // テクスチャサイズ自動調整フラグ
		//======================================================================
		// UV / アンカー / フリップ
		//======================================================================
		//アンカーポイント
		Vector2 anchorPoint_ = { 0.0f,0.0f };
		//左右フリップ
		bool isFlipX_ = false;
		//上下フリップ
		bool isFlipY_ = false;
		//======================================================================
		// 内部状態
		//======================================================================
		inline static int activeCount_ = 0; // アクティブスプライト数
		//======================================================================
		// 内部処理
		//======================================================================
		/// <summary>
		/// テクスチャサイズを調整する。
		/// </summary>
		void AdjustTextureSize();
	};
} // namespace TKM