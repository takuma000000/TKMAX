#include "Model.h"
#include "ModelCommon.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include "TextureManager.h"

MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	// マテリアルデータ
	MaterialData materialData;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	// ファイルを1行ずつ読み込む
	std::string line;
	while (std::getline(file, line)) { // 1行読み込み
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd") { // ディフューズマップ
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	// モデルデータ
	ModelData modelData;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;

	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	std::string line; // 1行読み込み
	while (std::getline(file, line)) { // 1行読み込み
		std::istringstream s(line);
		std::string identifier;
		s >> identifier;

		if (identifier == "v") { // 頂点座標
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			positions.push_back(position);
		} else if (identifier == "vt") { // テクスチャ座標
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		} else if (identifier == "vn") { // 法線ベクトル
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1;
			normals.push_back(normal);
		} else if (identifier == "f") { // 面
			VertexData triangle[3];
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) { // 三角形の3頂点
				// 頂点定義をスラッシュで分割して格納
				std::string vertexDefinition;
				s >> vertexDefinition;
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];

				for (int32_t element = 0; element < 3; ++element) { // 頂点の要素（頂点座標、テクスチャ座標、法線ベクトル）
					std::string index;
					std::getline(v, index, '/');
					elementIndices[element] = std::stoi(index);
				}

				Vector4 position = positions[elementIndices[0] - 1]; // OBJファイルのインデックスは1始まりなので-1する
				Vector2 texcoord = texcoords[elementIndices[1] - 1]; // OBJファイルのインデックスは1始まりなので-1する
				Vector3 normal = normals[elementIndices[2] - 1]; // OBJファイルのインデックスは1始まりなので-1する

				VertexData vertex = { position, texcoord, normal }; // 頂点データの作成
				modelData.vertices.push_back(vertex); // 頂点データの追加
				triangle[faceVertex] = vertex; // 三角形の頂点データを保存
			}

			// 面の裏表を反転させる
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		} else if (identifier == "mtllib") { // マテリアルファイル
			// マテリアルファイル名を取得して読み込む
			std::string materialFilename;
			s >> materialFilename;
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	return std::move(modelData);
}

void Model::VertexResource(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size()); // 頂点リソースの作成
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress(); // 頂点バッファビューの作成
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // 頂点バッファのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData); // 頂点バッファの頂点1つ分のサイズ

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData)); // 頂点リソースにデータを書き込む
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size()); // 頂点データのコピー
}

void Model::MaterialResource(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	// マテリアル用のリソースを作る
	materialResource = dxCommon->CreateBufferResource(sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// マテリアルデータの設定
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = true;
	materialData->uvTransform = MyMath::MakeIdentity4x4();
	materialData->shininess = 48.3f;
}

void Model::Initialize(ModelCommon* modelCommon, DirectXCommon* dxCommon, const std::string& directorypath, const std::string& filename) {
	modelCommon_ = modelCommon;
	dxCommon_ = dxCommon;

	// `std::move` を適用して不要なコピーを削減
	modelData = std::move(LoadObjFile(directorypath, filename));

	VertexResource(dxCommon_); // 頂点リソースの作成
	MaterialResource(dxCommon_); // マテリアルリソースの作成

	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath); // テクスチャの読み込み
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath); // テクスチャ番号の取得
}

void Model::Draw() {
	if (!dxCommon_) return; // DirectXCommonが設定されていない場合は描画しない
	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView); // 頂点バッファの設定
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress()); // マテリアルデータの設定
	dxCommon_->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0); // 描画
}
