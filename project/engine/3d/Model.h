#pragma once
#include "DirectXCommon.h"
#include "MyMath.h"
#include <string>
#include <vector>
#include "ModelTypes.h"
#include <wrl.h>
#include <unordered_map>

namespace TKM {
	class ModelCommon;
}

namespace TKM {

	//=============================================================
	// Modelクラス
	// 3Dモデルの読み込みと描画を管理するクラス
	//=============================================================
	class Model {
	private:
		//=============================================================
		// 共通参照
		//=============================================================

		ModelCommon* modelCommon_ = nullptr; // モデル共通管理
		DirectXCommon* dxCommon_ = nullptr;  // DirectX共通管理

		//=============================================================
		// モデルデータ
		//=============================================================

		ModelData modelData_; // 読み込み済みモデルデータ

		using MaterialMap = std::unordered_map<std::string, MaterialData>; // マテリアル名とデータの対応表

		//=============================================================
		// リソース
		//=============================================================

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_; // 頂点リソース
		VertexData* vertexData_ = nullptr;                      // 頂点データ書き込み先
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};           // 頂点バッファビュー

		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_; // マテリアルリソース
		Material* materialData_ = nullptr;                        // マテリアルデータ書き込み先

		//=============================================================
		// Transform
		//=============================================================

		Transform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }; // モデルTransform

		//=============================================================
		// 読み込み
		//=============================================================

		/// <summary>
		/// マテリアルテンプレートファイルを読み込みます。
		/// </summary>
		static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

		/// <summary>
		/// Objファイルを読み込みます。
		/// </summary>
		static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

		/// <summary>
		/// マルチマテリアル用テンプレートファイルを読み込みます。
		/// </summary>
		static MaterialMap LoadMaterialTemplateFileMulti(const std::string& directoryPath, const std::string& filename);

		//=============================================================
		// リソース生成
		//=============================================================

		/// <summary>
		/// 頂点リソースを生成します。
		/// </summary>
		void VertexResource(DirectXCommon* dxCommon);

		/// <summary>
		/// マテリアルリソースを生成します。
		/// </summary>
		void MaterialResource(DirectXCommon* dxCommon);

	public:
		//=============================================================
		// 生成・禁止事項
		//=============================================================

		Model() = default;
		~Model() = default;

		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		//=============================================================
		// ムーブ
		//=============================================================

		Model(Model&& other) noexcept
			: modelCommon_(other.modelCommon_),
			dxCommon_(other.dxCommon_),
			modelData_(std::move(other.modelData_)),
			vertexResource_(std::move(other.vertexResource_)),
			materialResource_(std::move(other.materialResource_)) {
			other.modelCommon_ = nullptr;
			other.dxCommon_ = nullptr;
		}

		Model& operator=(Model&& other) noexcept {
			if (this != &other) {
				modelCommon_ = other.modelCommon_;
				dxCommon_ = other.dxCommon_;
				modelData_ = std::move(other.modelData_);
				vertexResource_ = std::move(other.vertexResource_);
				materialResource_ = std::move(other.materialResource_);
				other.modelCommon_ = nullptr;
				other.dxCommon_ = nullptr;
			}
			return *this;
		}

		//=============================================================
		// 初期化・描画
		//=============================================================

		/// <summary>
		/// モデルを初期化します。
		/// </summary>
		void Initialize(ModelCommon* modelCommon, DirectXCommon* dxCommon, const std::string& directorypath, const std::string& filename);

		/// <summary>
		/// モデルを描画します。
		/// </summary>
		void Draw();

		/// <summary>
		/// マテリアル上書きなしで描画します。
		/// </summary>
		void DrawWithoutMaterialOverride();

		//=============================================================
		// 状態取得
		//=============================================================

		/// <summary>
		/// マルチマテリアルかを返します。
		/// </summary>
		bool IsMultiMaterial() const { return modelData_.submeshes_.size() > 1; }

		//=============================================================
		// Getter
		//=============================================================

		/// <summary>
		/// テクスチャパスを取得します。
		/// </summary>
		std::string GetTexturePath() const { return modelData_.material_.textureFilePath_; }

		/// <summary>
		/// スケールを取得します。
		/// </summary>
		const Vector3& GetScale() const { return transform_.scale_; }

		/// <summary>
		/// 回転を取得します。
		/// </summary>
		const Vector3& GetRotate() const { return transform_.rotate_; }

		/// <summary>
		/// 平行移動を取得します。
		/// </summary>
		const Vector3& GetTranslate() const { return transform_.translate_; }

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// スケールを設定します。
		/// </summary>
		void SetScale(const Vector3& scale) { this->transform_.scale_ = scale; }

		/// <summary>
		/// 回転を設定します。
		/// </summary>
		void SetRotate(const Vector3& rotate) { this->transform_.rotate_ = rotate; }

		/// <summary>
		/// 平行移動を設定します。
		/// </summary>
		void SetTranslate(const Vector3& translate) { this->transform_.translate_ = translate; }
	};
}