#include "Model.h"
#include "ModelCommon.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include "TextureManager.h"

namespace TKM {
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
				materialData.textureFilePath_ = directoryPath + "/" + textureFilename;
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
		// マテリアルマップ
		MaterialMap mtlMap;
		std::string currentMtl = "";
		int32_t currentSubmesh = -1;

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
				// サブメッシュがまだ作られてないなら作る
				if (modelData.submeshes_.empty()) {
					SubMeshData sm{};
					sm.startVertex_ = (uint32_t)modelData.vertices_.size();
					sm.vertexCount_ = 0;
					modelData.submeshes_.push_back(sm);
					currentSubmesh = 0;
				}

				VertexData triangle[3]{};

				for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
					std::string vertexDefinition;
					s >> vertexDefinition;
					std::istringstream v(vertexDefinition);

					uint32_t elementIndices[3]{};
					for (int32_t element = 0; element < 3; ++element) {
						std::string index;
						std::getline(v, index, '/');
						elementIndices[element] = std::stoi(index);
					}

					Vector4 position = positions[elementIndices[0] - 1];
					Vector2 texcoord = texcoords[elementIndices[1] - 1];
					Vector3 normal = normals[elementIndices[2] - 1];

					VertexData vertex = { position, texcoord, normal };
					triangle[faceVertex] = vertex;
				}

				// 面の裏表を反転（必要な順だけ push）
				modelData.vertices_.push_back(triangle[2]);
				modelData.vertices_.push_back(triangle[1]);
				modelData.vertices_.push_back(triangle[0]);

				modelData.submeshes_[currentSubmesh].vertexCount_ += 3;
			} else if (identifier == "mtllib") { // マテリアルファイル
				// マテリアルファイル名を取得して読み込む
				std::string materialFilename;
				s >> materialFilename;
				mtlMap = LoadMaterialTemplateFileMulti(directoryPath, materialFilename);
			} else if (identifier == "usemtl") {
				std::string mtlName;
				s >> mtlName;
				currentMtl = mtlName;

				// サブメッシュをまだ作ってないなら作る
				// まず同名があるか探す（なければ新規）
				currentSubmesh = -1;
				for (int i = 0; i < (int)modelData.submeshes_.size(); ++i) {
					if (modelData.submeshes_[i].material_.textureFilePath_ == mtlMap[currentMtl].textureFilePath_) {
						currentSubmesh = i;
						break;
					}
				}
				if (currentSubmesh == -1) {
					SubMeshData sm{};
					sm.startVertex_ = (uint32_t)modelData.vertices_.size();
					sm.vertexCount_ = 0;
					if (mtlMap.contains(currentMtl)) {
						sm.material_ = mtlMap[currentMtl];
					}
					modelData.submeshes_.push_back(sm);
					currentSubmesh = (int)modelData.submeshes_.size() - 1;
				}
			}
		}

		return std::move(modelData);
	}

	Model::MaterialMap Model::LoadMaterialTemplateFileMulti(const std::string& directoryPath, const std::string& filename) {
		MaterialMap out;
		std::ifstream file(directoryPath + "/" + filename);
		assert(file.is_open());

		std::string line;
		std::string currentMtl;

		while (std::getline(file, line)) {
			std::istringstream s(line);
			std::string id;
			s >> id;

			if (id == "newmtl") {
				s >> currentMtl;
				out[currentMtl] = MaterialData{};
			} else if (id == "map_Kd") {
				std::string tex;
				s >> tex;
				if (!currentMtl.empty()) {
					out[currentMtl].textureFilePath_ = directoryPath + "/" + tex;
				}
			}
		}
		return out;
	}

	void Model::VertexResource(DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;

		vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData_.vertices_.size()); // 頂点リソースの作成
		vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress(); // 頂点バッファビューの作成
		vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices_.size()); // 頂点バッファのサイズ
		vertexBufferView_.StrideInBytes = sizeof(VertexData); // 頂点バッファの頂点1つ分のサイズ

		vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_)); // 頂点リソースにデータを書き込む
		std::memcpy(vertexData_, modelData_.vertices_.data(), sizeof(VertexData) * modelData_.vertices_.size()); // 頂点データのコピー
	}

	void Model::MaterialResource(DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;

		// マテリアル用のリソースを作る
		materialResource_ = dxCommon->CreateBufferResource(sizeof(Material));
		materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
		// マテリアルデータの設定
		materialData_->color_ = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		materialData_->enableLighting_ = true;
		materialData_->uvTransform_ = MyMath::MakeIdentity4x4();
		materialData_->shininess_ = 48.3f;
	}

	void Model::Initialize(ModelCommon* modelCommon, DirectXCommon* dxCommon, const std::string& directorypath, const std::string& filename) {
		modelCommon_ = modelCommon;
		dxCommon_ = dxCommon;

		modelData_ = std::move(LoadObjFile(directorypath, filename));

		VertexResource(dxCommon_);
		MaterialResource(dxCommon_);

		// マルチ対応：submeshes_ があるなら全部ロード
		if (!modelData_.submeshes_.empty()) {
			// 互換用：先頭を material_ にも入れておく
			modelData_.material_ = modelData_.submeshes_.front().material_;

			for (auto& sm : modelData_.submeshes_) {
				if (!sm.material_.textureFilePath_.empty()) {
					TextureManager::GetInstance()->LoadTexture(sm.material_.textureFilePath_);
					sm.material_.textureIndex_ =
						TextureManager::GetInstance()->GetTextureIndexByFilePath(sm.material_.textureFilePath_);
				}
			}
		} else {
			// 従来
			TextureManager::GetInstance()->LoadTexture(modelData_.material_.textureFilePath_);
			modelData_.material_.textureIndex_ =
				TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData_.material_.textureFilePath_);
		}
	}

	void Model::Draw() {
		if (!dxCommon_) return;

		dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);

		// ★マルチマテリアル
		if (modelData_.submeshes_.size() > 1) {
			for (auto& sm : modelData_.submeshes_) {
				// テクスチャ（RootTable #2）をここで差し替える
				if (!sm.material_.textureFilePath_.empty()) {
					dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(
						2,
						TextureManager::GetInstance()->GetSrvHandleGPU(sm.material_.textureFilePath_)
					);
				}

				dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
				dxCommon_->GetCommandList()->DrawInstanced(sm.vertexCount_, 1, sm.startVertex_, 0);
			}
			return;
		}

		// 単一
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
		dxCommon_->GetCommandList()->DrawInstanced(UINT(modelData_.vertices_.size()), 1, 0, 0);
	}
}