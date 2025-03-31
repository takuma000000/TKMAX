#include "Model.h"
#include "ModelCommon.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include "TextureManager.h"
#include "Object3d.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	std::string line;
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;

	//Assimp::Importer importer;
	//std::string filePath = directoryPath + "/" + filename;
	//const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	//assert(scene->HasMeshes());

	//for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
	//	aiMesh* mesh = scene->mMeshes[meshIndex];
	//	assert(mesh->HasNormals());//法線ベクトルがあるか
	//	assert(mesh->HasTextureCoords(0));//テクスチャ座標があるか
	//	for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
	//		aiFace face = mesh->mFaces[faceIndex];//面情報
	//		assert(face.mNumIndices == 3);//三角形ポリゴンのみ対応
	//		for (uint32_t element = 0; element < face.mNumIndices; ++element) {
	//			uint32_t vertexIndex = face.mIndices[element];
	//			aiVector3D& position = mesh->mVertices[vertexIndex];
	//			aiVector3D& normal = mesh->mNormals[vertexIndex];
	//			aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
	//			VertexData vertex;
	//			vertex.position = { position.x, position.y, position.z, 1.0f };
	//			vertex.normal = { normal.x, normal.y, normal.z };
	//			vertex.texcoord = { texcoord.x, texcoord.y };
	//			
	//			vertex.position.x *= -1.0f;
	//			vertex.normal.x *= -1.0f;
	//			modelData.vertices.push_back(vertex);
	//		}
	//		
	//	}
	//}

	//for (uint32_t maaterialIndex = 0; maaterialIndex < scene->mNumMaterials; ++maaterialIndex) {
	//	aiMaterial* material = scene->mMaterials[maaterialIndex];
	//	if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
	//		aiString textureFilePath;
	//		material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
	//		modelData.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
	//	}
	//}

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream s(line);
		std::string identifier;
		s >> identifier;

		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			positions.push_back(position);
		} else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		} else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1;
			normals.push_back(normal);
		} else if (identifier == "f") {
			VertexData triangle[3];
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];

				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');
					elementIndices[element] = std::stoi(index);
				}

				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];

				VertexData vertex = { position, texcoord, normal };
				modelData.vertices.push_back(vertex);
				triangle[faceVertex] = vertex;
			}

			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		} else if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	return std::move(modelData);
}

void Model::VertexResource(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void Model::MaterialResource(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	materialResource = dxCommon->CreateBufferResource(sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

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

	VertexResource(dxCommon_);
	MaterialResource(dxCommon_);

	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
}

void Model::Draw() {
	if (!dxCommon_) return; // `dxCommon_` が `nullptr` なら描画しない

	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

	dxCommon_->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
}
