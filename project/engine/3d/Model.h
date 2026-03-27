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

//=============================================================
// Modelクラス
// 3Dモデルのデータ読み込みと描画を行うクラス。
//=============================================================
namespace TKM {
	class Model {
	private:
		ModelCommon* modelCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;

		// Objファイルのデータ
		ModelData modelData_;

		/// <summary>
		/// マテリアルテンプレートファイルを読み込みます。
		/// </summary>
		/// <param name="directoryPath"></param>
		/// <param name="filename"></param>
		/// <returns></returns>
		static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
		/// <summary>
		/// Objファイルを読み込みます。
		/// </summary>
		/// <param name="directoryPath"></param>
		/// <param name="filename"></param>
		/// <returns></returns>
		static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

		// 頂点リソースを作る
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
		// 頂点リソースにデータを書き込む
		VertexData* vertexData_ = nullptr;
		// 頂点バッファビューを作成する
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

		// マテリアル用のリソースを作る
		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
		// マテリアルにデータを書き込む
		Material* materialData_ = nullptr;

		using MaterialMap = std::unordered_map<std::string, MaterialData>; // マテリアル名とマテリアルデータのマップ

		/// <summary>
		/// マテリアルテンプレートファイルを読み込みます（マルチマテリアル対応版）。
		/// </summary>
		/// <param name="directoryPath"></param>
		/// <param name="filename"></param>
		/// <returns></returns>
		static MaterialMap LoadMaterialTemplateFileMulti(const std::string& directoryPath, const std::string& filename);
		/// <summary>
		/// 頂点リソースを作成します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void VertexResource(DirectXCommon* dxCommon);
		/// <summary>
		/// マテリアルリソースを作成します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void MaterialResource(DirectXCommon* dxCommon);

		Transform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	public://メンバ関数

		// デフォルトコンストラクタ
		Model() = default;
		// デストラクタ
		~Model() = default;

		// コピーコンストラクタとコピー代入演算子を禁止
		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		// ムーブコンストラクタ
		Model(Model&& other) noexcept
			: modelCommon_(other.modelCommon_),
			dxCommon_(other.dxCommon_),
			modelData_(std::move(other.modelData_)),
			vertexResource_(std::move(other.vertexResource_)),
			materialResource_(std::move(other.materialResource_)) {
			other.modelCommon_ = nullptr;
			other.dxCommon_ = nullptr;
		}
		// ムーブ代入演算子
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

		/// <summary>
		/// モデルを初期化します。
		/// </summary>
		/// <param name="modelCommon"></param>
		/// <param name="dxCommon"></param>
		/// <param name="directorypath"></param>
		/// <param name="filename"></param>
		void Initialize(ModelCommon* modelCommon, DirectXCommon* dxCommon, const std::string& directorypath, const std::string& filename);
		/// <summary>
		/// モデルを描画します。
		/// </summary>
		void Draw();
		/// <summary>
		/// マテリアルをオーバーライドせずにモデルを描画します。
		/// </summary>
		void DrawWithoutMaterialOverride();

		/// <summary>
		/// マルチマテリアルかどうかを返します。
		/// </summary>
		/// <returns></returns>
		bool IsMultiMaterial() const { return modelData_.submeshes_.size() > 1; }

		// Getter===================================
		/// <summary>
		/// テクスチャパスの取得。
		/// </summary>
		/// <returns></returns>
		std::string GetTexturePath() const { return modelData_.material_.textureFilePath_; }
		/// <summary>
		/// スケールの取得。
		/// </summary>
		/// <returns></returns>
		const Vector3& GetScale() const { return transform_.scale_; }
		/// <summary>
		/// 回転の取得。
		/// </summary>
		/// <returns></returns>
		const Vector3& GetRotate() const { return transform_.rotate_; }
		/// <summary>
		/// 平行移動の取得。
		/// </summary>
		/// <returns></returns>
		const Vector3& GetTranslate() const { return transform_.translate_; }
		// =========================================
		// Setter===================================
		/// <summary>
		/// スケールの設定。
		/// </summary>
		/// <param name="scale"></param>
		void SetScale(const Vector3& scale) { this->transform_.scale_ = scale; }
		/// <summary>
		/// 回転の設定。
		/// </summary>
		/// <param name="rotate"></param>
		void SetRotate(const Vector3& rotate) { this->transform_.rotate_ = rotate; }
		/// <summary>
		/// 平行移動の設定。
		/// </summary>
		/// <param name="translate"></param>
		void SetTranslate(const Vector3& translate) { this->transform_.translate_ = translate; }
		// ========================================
	};
}