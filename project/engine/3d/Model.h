#pragma once
#include "DirectXCommon.h"
#include "MyMath.h"
#include <string>
#include <vector>
#include "Object3d.h"
#include <wrl.h>

class ModelCommon;

class Model {
private:
	ModelCommon* modelCommon_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;

	// Objファイルのデータ
	ModelData modelData;

	// .mtlファイル読み込み
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	// .objファイル読み込み
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

	// 頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	// マテリアル用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	// マテリアルにデータを書き込む
	Material* materialData = nullptr;

	// VertexResource関数
	void VertexResource(DirectXCommon* dxCommon);
	// MaterialResource関数
	void MaterialResource(DirectXCommon* dxCommon);

	// Transform情報
	Transform transform;

public:
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
		modelData(std::move(other.modelData)),
		vertexResource(std::move(other.vertexResource)),
		materialResource(std::move(other.materialResource)) {
		other.modelCommon_ = nullptr;
		other.dxCommon_ = nullptr;
	}

	// ムーブ代入演算子
	Model& operator=(Model&& other) noexcept {
		if (this != &other) {
			modelCommon_ = other.modelCommon_;
			dxCommon_ = other.dxCommon_;
			modelData = std::move(other.modelData);
			vertexResource = std::move(other.vertexResource);
			materialResource = std::move(other.materialResource);
			other.modelCommon_ = nullptr;
			other.dxCommon_ = nullptr;
		}
		return *this;
	}

	// モデルの初期化
	void Initialize(ModelCommon* modelCommon, DirectXCommon* dxCommon, const std::string& directorypath, const std::string& filename);
	// モデルを描画
	void Draw();

	// getter
	const Vector3& GetScale() const { return transform.scale; }
	const Vector3& GetRotate() const { return transform.rotate; }
	const Vector3& GetTranslate() const { return transform.translate; }

	// setter
	void SetScale(const Vector3& scale) { this->transform.scale = scale; }
	void SetRotate(const Vector3& rotate) { this->transform.rotate = rotate; }
	void SetTranslate(const Vector3& translate) { this->transform.translate = translate; }
};
