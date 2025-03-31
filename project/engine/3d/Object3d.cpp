#include "Object3d.h"
#include "Object3dCommon.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include "TextureManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "Camera.h"

#include "imgui/imgui.h"
#include <numbers>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void Object3d::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon)
{
	//引数で受け取ってメンバ変数に記録する
	this->object3dCommon = object3dCommon;
	dxCommon_ = dxCommon;

	transform.scale = { 1.0f, 1.0f, 1.0f };
	transform.rotate = { 0.0f, 1.6f, 0.0f };

	//モデル読み込み
	modelData = LoadModelFile("resources", "plane.gltf");

	VertexResource(dxCommon_);
	MaterialResource(dxCommon_);
	WVPResource(dxCommon_);
	Light(dxCommon_);
	CameraResource(dxCommon_);
	PointLight(dxCommon_);
	SpotLight(dxCommon_);

	//.objの参照しているテクスチャファイル読み込み
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	//読み込んだテクスチャの番号を取得
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);

	//materialData->enableLighting = false;

	this->camera = object3dCommon->GetDefaultCamera();

}

void Object3d::Update()
{
	// TransformからWorldMatrixを作る
	Matrix4x4 worldMatrix = MyMath::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 worldViewProjectionMatrix;

	if (camera) {
		const Matrix4x4& viewProjectionMatrix = camera->GetViewProjectionMatrix();
		worldViewProjectionMatrix = MyMath::Multiply(worldMatrix, viewProjectionMatrix);
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}

	wvpData->wvp = worldViewProjectionMatrix;
	wvpData->World = worldMatrix;
	wvpData->WorldInverseTranspose = MyMath::Inverse4x4(worldMatrix);

	// ---- ImGui のライト設定 ----
	ImGui::Begin("Light Settings");

	// Directional Light
	if (ImGui::ColorEdit3("Directional Light Color", &directionalLightData->color.x)) {
		// 変更があったら適用
	}

	if (ImGui::DragFloat3("Directional Light Direction", &directionalLightData->direction.x, 0.01f, -1.0f, 1.0f)) {
		// ライトの方向を更新
	}

	if (ImGui::DragFloat("Directional Light Intensity", &directionalLightData->intensity, 0.01f, 0.0f, 10.0f)) {
		// 強度を更新
	}

	ImGui::Separator(); // UIを区切る

	// Point Light
	if (ImGui::ColorEdit3("Point Light Color", &pointLightData->color.x)) {
		// 変更があったら適用
	}

	if (ImGui::DragFloat3("Point Light Position", &pointLightData->position.x, 0.1f, -50.0f, 50.0f)) {
		// 位置を更新
	}

	if (ImGui::DragFloat("Point Light Intensity", &pointLightData->intensity, 0.01f, 0.0f, 10.0f)) {
		// 強度を更新
	}

	if (ImGui::DragFloat("Point Light Radius", &pointLightData->radius, 0.01f, 0.0f, 100.0f)) {
		// 半径を更新
	}

	if (ImGui::DragFloat("Point Light Decay", &pointLightData->decay, 0.01f, 0.0f, 10.0f)) {
		// 減衰率を更新
	}

	//Spot Light
	if (ImGui::ColorEdit3("Spot Light Color", &spotLightData->color.x)) {
		// 変更があったら適用
	}
	if (ImGui::DragFloat3("Spot Light Position", &spotLightData->position.x, 0.1f, -50.0f, 50.0f)) {
		// 位置を更新
	}
	if (ImGui::DragFloat("Spot Light Intensity", &spotLightData->intensity, 0.01f, 0.0f, 10.0f)) {
		// 強度を更新
	}
	if (ImGui::DragFloat3("Spot Light Direction", &spotLightData->direction.x, 0.01f, -1.0f, 1.0f)) {
		// ライトの方向を更新
	}
	if (ImGui::DragFloat("Spot Light Distance", &spotLightData->distance, 0.01f, 0.0f, 100.0f)) {
		// 距離を更新
	}
	if (ImGui::DragFloat("Spot Light Decay", &spotLightData->decay, 0.01f, 0.0f, 10.0f)) {
		// 減衰率を更新
	}
	if (ImGui::DragFloat("Spot Light CosAngle", &spotLightData->cosAngle, 0.01f, 0.0f, 1.0f)) {
		// 角度を更新
	}
	if (ImGui::DragFloat("Spot Light CosFalloff", &spotLightData->cosFalloffStart, 0.01f, 0.0f, 1.0f)) {
		// 角度を更新
	}


	ImGui::End();
}


void Object3d::Draw(DirectXCommon* dxCommon)
{
	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);// VBVを設定

	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

	// Object3d のテクスチャを適用
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData.material.textureFilePath));

	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, materialResourceLight->GetGPUVirtualAddress());
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraResource->GetGPUVirtualAddress());
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, pointLightResource->GetGPUVirtualAddress());
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, spotLightResource->GetGPUVirtualAddress());


	// ここで model_ のテクスチャを適用する
	if (model_) {
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(model_->GetTexturePath()));
		model_->Draw();
	}

	//dxCommon_->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
}


void Object3d::SetModel(const std::string& filePath)
{
	//モデルを検索してセットする
	model_ = ModelManager::GetInstance()->FindModel(filePath);
}

MaterialData Object3d::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
{
	//中で必要となる変数の宣言
	MaterialData materialData;//構築するMaterialData
	std::string line;//ファイルから読んだ1行を格納するもの
	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
	assert(file.is_open());//とりあえず開けなかったら止める
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		//identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			//連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

ModelData Object3d::LoadModelFile(const std::string& directoryPath, const std::string& filename)
{
	//必要となる変数の宣言
	ModelData modelData;//構築するモデルデータ
	Assimp::Importer importer;

	std::string filePath = directoryPath + "/" + filename;

	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);

	assert(scene->HasMeshes()); // メッシュがないのは対応しない

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals()); // 法線がないMeshは今回は非対応
		assert(mesh->HasTextureCoords(0)); // TexcoordがないMesh
		// ここからMeshの中身(Face)の解析を行っていく

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
		{
			aiFace& face = mesh->mFaces[faceIndex];

			assert(face.mNumIndices == 3); // 三角形のみサポート
			// ここからFaceの中身(Vertex)の解析を行っていく

			for (uint32_t element = 0; element < face.mNumIndices; ++element)
			{
				uint32_t vertexIndex = face.mIndices[element];
				aiVector3D& position = mesh->mVertices[vertexIndex];
				aiVector3D& normal = mesh->mNormals[vertexIndex];
				aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
				VertexData vertex;
				vertex.position = { position.x,position.y,position.z,1.0f };
				vertex.normal = { normal.x,normal.y,normal.z };
				vertex.texcoord = { texcoord.x,texcoord.y };
				// 
				vertex.position.x *= -1.0f;
				vertex.normal.x *= -1.0f;
				modelData.vertices.push_back(vertex);
			}

		}
	}

	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
	{
		aiMaterial* material = scene->mMaterials[materialIndex];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0)
		{
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			modelData.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
		}
	}

	return modelData;
}

void Object3d::VertexResource(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	//VertexResourceを作る
	vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	//VertexBufferViewを作成する( 値を設定するだけ )
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);
	//VertexResourceにデータを書き込むためのアドレスを取得してvertexDataに割り当てる
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void Object3d::MaterialResource(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	//materialResourceを作る
	materialResource = dxCommon->CreateBufferResource(sizeof(Material));
	//materialResourceにデータを書き込むためのアドレスを取得してmaterialDataに割り当てる
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//materialDataに初期値を書き込む
	//今回は白を書き込んでみる
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = true;
	materialData->uvTransform = MyMath::MakeIdentity4x4();
	materialData->shininess = 48.3f;//明るさ
}

void Object3d::WVPResource(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	//座標変換行列リソースを作る
	wvpResource = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
	//書き込むためのアドレスを取得
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	//単位行列を書き込んでおく
	wvpData->wvp = MyMath::MakeIdentity4x4();
	wvpData->World = MyMath::MakeIdentity4x4();
	wvpData->WorldInverseTranspose = MyMath::MakeIdentity4x4();
}

void Object3d::CameraResource(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	cameraResource = dxCommon_->CreateBufferResource(sizeof(CameraForGPU));
	cameraResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraData));
	// カメラ位置を設定
	cameraData->worldPosition = { 0.0f, 5.0f, -10.0f }; // 必要に応じて変更
}

void Object3d::Light(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	//並行光源リソースを作る
	materialResourceLight = dxCommon_->CreateBufferResource(sizeof(DirectionalLightEX));
	//書き込むためのアドレスを取得
	materialResourceLight->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//デフォルト値を書き込んでおく
	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 1.0f, 0.0f, 0.0f }; // 斜め上から光を当てる
	directionalLightData->intensity = 0.0f;//光の強さ
}

void Object3d::PointLight(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	//並行光源リソースを作る
	pointLightResource = dxCommon_->CreateBufferResource(sizeof(PointLightEX));
	//書き込むためのアドレスを取得
	pointLightResource->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));
	//デフォルト値を書き込んでおく
	pointLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	pointLightData->position = { 0.0f,2.0f,0.0f };
	pointLightData->intensity = 0.0f;//光の強さ
	pointLightData->radius = 10.0f;
	pointLightData->decay = 1.0f;
}

void Object3d::SpotLight(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	//並行光源リソースを作る
	spotLightResource = dxCommon_->CreateBufferResource(sizeof(SpotLightEX));
	//書き込むためのアドレスを取得
	spotLightResource->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData));
	//デフォルト値を書き込んでおく
	spotLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	spotLightData->position = { 2.0f,1.25f,0.0f };
	spotLightData->intensity = 4.0f;//光の強さ
	spotLightData->direction = MyMath::Normalize({ -1.0f, -1.0f, 0.0f });
	spotLightData->distance = 7.0f;
	spotLightData->decay = 2.0f;
	spotLightData->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
	spotLightData->cosFalloffStart = std::cos(std::numbers::pi_v<float> / 3.0f);
}
