#include "Object3d.h"
#include "Object3dCommon.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include "TextureManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "Camera.h"

void Object3d::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon)
{
	//引数で受け取ってメンバ変数に記録する
	this->object3dCommon = object3dCommon;
	dxCommon_ = dxCommon;

	transform.scale = { 1.0f, 1.0f, 1.0f };

	//モデル読み込み
	modelData = LoadObjFile("resources", "plane.obj");

	VertexResource(dxCommon_);
	MaterialResource(dxCommon_);
	WVPResource(dxCommon_);
	Light(dxCommon_);
	CameraResource(dxCommon_);

	//.objの参照しているテクスチャファイル読み込み
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	//読み込んだテクスチャの番号を取得
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);

	materialData->enableLighting = true;

	this->camera = object3dCommon->GetDefaultCamera();

}

void Object3d::Update()
{
	//TransformからWorldMatrixを作る
	Matrix4x4 worldMatrix = MyMath::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	//worldViewProjectionMatrixを作る	[ worldMatrix * ( viewMatrix * projectionMatrix ) ]
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

	// ここで model_ のテクスチャを適用する
	if (model_) {
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(model_->GetTexturePath()));
		model_->Draw();
	}

	dxCommon_->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
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

ModelData Object3d::LoadObjFile(const std::string& directoryPath, const std::string& filename)
{
	//必要となる変数の宣言
	ModelData modelData;//構築するモデルデータ
	std::vector<Vector4> positions;//位置
	std::vector<Vector3> normals;//法線
	std::vector<Vector2> texcoords;//テクスチャ座標
	std::string line;//ファイルから読んだ一行を格納するもの
	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
	assert(file.is_open());//とりあえず開けなかったら止める
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;//先頭の識別子を読む

		//identifierに応じた処理
		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			//position.x *= -1;
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
			//面は三角形限定。その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				//頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分離してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');//区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}
				//要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				VertexData vertex = { position,texcoord,normal };
				modelData.vertices.push_back(vertex);
				triangle[faceVertex] = { position,texcoord,normal };

			}
			//頂点を逆順で登録することで、周り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		} else if (identifier == "mtllib") {
			//materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			//基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
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
	materialData->shininess = 10.0f;
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
	directionalLightData->direction = { 1.0f,0.0f,0.0f };
	directionalLightData->intensity = 1.0f;
}