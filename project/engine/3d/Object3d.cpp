#include "Object3d.h"
#include "Object3dCommon.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include "TextureManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "Camera.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

#include <numbers>
#include "BaseScene.h"

using TKM::TextureManager;
using TKM::ModelManager;

namespace TKM {
	Object3d::Object3d() {
		++activeCount_; // 静的メンバ変数のインクリメント
	}

	Object3d::~Object3d() {
		--activeCount_; // 静的メンバ変数のデクリメント
	}

	void Object3d::Initialize(TKM::Object3dCommon* object3dCommon, TKM::DirectXCommon* dxCommon) {
		//引数で受け取ってメンバ変数に記録する
		this->object3dCommon_ = object3dCommon;
		dxCommon_ = dxCommon;

		transform_.scale_ = { 1.0f, 1.0f, 1.0f }; //スケール0.1倍
		transform_.rotate_ = { 0.0f, 0.0f, 0.0f };

		//モデル読み込み
		modelData_ = LoadObjFile("resources/obj", "plane.obj"); //.objファイル読み込み

		VertexResource(dxCommon_); //頂点リソース作成
		MaterialResource(dxCommon_); //マテリアルリソース作成
		WVPResource(dxCommon_); //WVPリソース作成
		Light(dxCommon_); //ライトリソース作成
		CameraResource(dxCommon_); //カメラリソース作成
		PointLight(dxCommon_); //Pointライトリソース作成
		SpotLight(dxCommon_); //Spotライトリソース作成
		Environment(dxCommon_); //環境マップリソース作成

		//.objの参照しているテクスチャファイル読み込み
		TextureManager::GetInstance()->LoadTexture(modelData_.material_.textureFilePath_);
		//読み込んだテクスチャの番号を取得
		modelData_.material_.textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData_.material_.textureFilePath_);
		//マテリアルデータにテクスチャ番号をセット
		this->camera_ = object3dCommon->GetDefaultCamera();
		// 環境マップ設定
		environmentSrvHandleGPU_.ptr = 0;

		// Object3d::Initialize()
		environmentSrvHandleGPU_ = TextureManager::GetInstance()->GetSrvHandleGPU("./resources/texture/rostock_laage_airport_4k.dds");
		if (environmentData_) { // 環境マップデータが存在する場合
			environmentData_->useEnvironment_ = false;
		}
	}

	void Object3d::Update() {
		// ローカル行列
		Matrix4x4 localMatrix = MyMath::MakeAffineMatrix(transform_.scale_, transform_.rotate_, transform_.translate_);

		// 親が居るなら「ローカル * 親ワールド」でワールド化
		Matrix4x4 worldMatrix = localMatrix;
		if (parent_) {
			worldMatrix = MyMath::Multiply(localMatrix, parent_->GetWorldMatrix());
		}
		worldMatrix_ = worldMatrix;

		// ワールドビュー射影行列を計算
		Matrix4x4 worldViewProjectionMatrix;

		if (camera_) {
			const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
			worldViewProjectionMatrix = MyMath::Multiply(worldMatrix, viewProjectionMatrix);
		} else {
			worldViewProjectionMatrix = worldMatrix;
		}

		wvpData_->wvp_ = worldViewProjectionMatrix;
		wvpData_->World_ = worldMatrix;
		wvpData_->WorldInverseTranspose_ = MyMath::Inverse4x4(worldMatrix);
	}

	void Object3d::Draw(TKM::DirectXCommon* dxCommon) {

		dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);// VBVを設定
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress()); // マテリアルをセット
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress()); // WVPをセット

		// Object3d のテクスチャを適用
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData_.material_.textureFilePath_));

		// ライト関連の定数バッファをセット
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, materialResourceLight_->GetGPUVirtualAddress());
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraResource_->GetGPUVirtualAddress());
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, pointLightResource_->GetGPUVirtualAddress());
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, spotLightResource_->GetGPUVirtualAddress());

		if (environmentSrvHandleGPU_.ptr != 0) { // 環境マップが設定されている場合
			dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(7, environmentSrvHandleGPU_); // 環境マップをセット
		}

		// 環境マップ用定数バッファをセット
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(8, environment_->GetGPUVirtualAddress());

		// ここで model_ のテクスチャを適用する
		if (model_) {
			if (!model_->IsMultiMaterial()) {
				dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(
					2,
					TextureManager::GetInstance()->GetSrvHandleGPU(model_->GetTexturePath())
				);
			}
			model_->Draw();
		}
	}

	void Object3d::SetModel(const std::string& filePath) {
		//モデルを検索してセットする
		model_ = ModelManager::GetInstance()->FindModel(filePath);
	}

	void Object3d::SetParentScene(TKM::BaseScene* parentScene) {
		parentScene_ = parentScene; // 親シーンを設定
	}

	void Object3d::SetEnvironment(const std::string& filename) {
		environmentSrvHandleGPU_ = TextureManager::GetInstance()->GetSrvHandleGPU(filename); // 環境マップのGPUハンドルを取得
		if (environmentData_) { // 環境マップデータが存在する場合
			environmentData_->useEnvironment_ = true; // 環境マップを使用するように設定
		}
	}

	MaterialData Object3d::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
		//中で必要となる変数の宣言
		MaterialData materialData;//構築するMaterialData
		std::string line;//ファイルから読んだ1行を格納するもの
		std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
		assert(file.is_open());//とりあえず開けなかったら止める
		while (std::getline(file, line)) { //1行読み込む
			std::string identifier;
			std::istringstream s(line);
			s >> identifier;

			//identifierに応じた処理
			if (identifier == "map_Kd") { //拡散反射マップ
				std::string textureFilename;
				s >> textureFilename;
				//連結してファイルパスにする
				materialData.textureFilePath_ = directoryPath + "/" + textureFilename;
			}
		}
		return materialData;
	}

	ModelData Object3d::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
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
			if (identifier == "v") { //頂点の位置
				Vector4 position;
				s >> position.x >> position.y >> position.z;
				position.w = 1.0f;
				//position.x *= -1;
				positions.push_back(position);
			} else if (identifier == "vt") { //頂点のテクスチャ座標
				Vector2 texcoord;
				s >> texcoord.x >> texcoord.y;
				texcoord.y = 1.0f - texcoord.y;
				texcoords.push_back(texcoord);
			} else if (identifier == "vn") { //頂点の法線
				Vector3 normal;
				s >> normal.x >> normal.y >> normal.z;
				normal.x *= -1;
				normals.push_back(normal);
			} else if (identifier == "f") { //面
				VertexData triangle[3];
				//面は三角形限定。その他は未対応
				for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) { //3頂点分ループ
					std::string vertexDefinition;
					s >> vertexDefinition;
					//頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分離してIndexを取得する
					std::istringstream v(vertexDefinition);
					uint32_t elementIndices[3];
					for (int32_t element = 0; element < 3; ++element) { //位置、UV、法線の3要素分ループ
						std::string index;
						std::getline(v, index, '/');//区切りでインデックスを読んでいく
						elementIndices[element] = std::stoi(index);
					}
					//要素へのIndexから、実際の要素の値を取得して、頂点を構築する
					Vector4 position = positions[elementIndices[0] - 1];
					Vector2 texcoord = texcoords[elementIndices[1] - 1];
					Vector3 normal = normals[elementIndices[2] - 1];
					VertexData vertex = { position,texcoord,normal };
					modelData.vertices_.push_back(vertex);
					triangle[faceVertex] = { position,texcoord,normal };

				}
				//頂点を逆順で登録することで、周り順を逆にする
				modelData.vertices_.push_back(triangle[2]);
				modelData.vertices_.push_back(triangle[1]);
				modelData.vertices_.push_back(triangle[0]);
			} else if (identifier == "mtllib") { //マテリアルライブラリ
				//materialTemplateLibraryファイルの名前を取得する
				std::string materialFilename;
				s >> materialFilename;
				//基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
				modelData.material_ = LoadMaterialTemplateFile(directoryPath, materialFilename);
			}
		}
		return modelData;
	}

	void Object3d::VertexResource(TKM::DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;

		//VertexResourceを作る
		vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData_.vertices_.size());
		//VertexBufferViewを作成する( 値を設定するだけ )
		vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
		vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices_.size());
		vertexBufferView_.StrideInBytes = sizeof(VertexData);
		//VertexResourceにデータを書き込むためのアドレスを取得してvertexDataに割り当てる
		vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
		std::memcpy(vertexData_, modelData_.vertices_.data(), sizeof(VertexData) * modelData_.vertices_.size());
	}

	void Object3d::MaterialResource(TKM::DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;

		//materialResourceを作る
		materialResource_ = dxCommon->CreateBufferResource(sizeof(Material));
		//materialResourceにデータを書き込むためのアドレスを取得してmaterialDataに割り当てる
		materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
		//今回は白を書き込んでみる
		materialData_->color_ = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		materialData_->enableLighting_ = false;
		materialData_->uvTransform_ = MyMath::MakeIdentity4x4();
		materialData_->shininess_ = 48.3f;//明るさ
	}

	void Object3d::WVPResource(TKM::DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;

		//座標変換行列リソースを作る
		wvpResource_ = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
		//書き込むためのアドレスを取得
		wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
		//単位行列を書き込んでおく
		wvpData_->wvp_ = MyMath::MakeIdentity4x4();
		wvpData_->World_ = MyMath::MakeIdentity4x4();
		wvpData_->WorldInverseTranspose_ = MyMath::MakeIdentity4x4();
	}

	void Object3d::CameraResource(TKM::DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;

		cameraResource_ = dxCommon_->CreateBufferResource(sizeof(CameraForGPU)); // カメラ用のリソースを作る
		cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_)); // 書き込むためのアドレスを取得
		// カメラ位置を設定
		cameraData_->worldPosition_ = { 0.0f, 5.0f, -10.0f }; // 必要に応じて変更
	}

	void Object3d::Light(TKM::DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;

		//並行光源リソースを作る
		materialResourceLight_ = dxCommon_->CreateBufferResource(sizeof(DirectionalLightEX));
		//書き込むためのアドレスを取得
		materialResourceLight_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));
		//デフォルト値を書き込んでおく
		directionalLightData_->color_ = { 1.0f,1.0f,1.0f,1.0f }; // 白色光
		directionalLightData_->direction_ = { 1.0f, 0.0f, 0.0f }; // 斜め上から光を当てる
		directionalLightData_->intensity_ = 1.0f;//光の強さ
	}

	void Object3d::PointLight(TKM::DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;

		//並行光源リソースを作る
		pointLightResource_ = dxCommon_->CreateBufferResource(sizeof(PointLightEX));
		//書き込むためのアドレスを取得
		pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));
		//デフォルト値を書き込んでおく
		pointLightData_->color_ = { 1.0f,1.0f,1.0f,1.0f }; // 白色光
		pointLightData_->position_ = { 0.0f,2.0f,0.0f }; // 斜め上から光を当てる
		pointLightData_->intensity_ = 0.0f;//光の強さ
		pointLightData_->radius_ = 10.0f; // 光の届く距離
		pointLightData_->decay_ = 1.0f; // 減衰率
	}

	void Object3d::SpotLight(TKM::DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;

		//並行光源リソースを作る
		spotLightResource_ = dxCommon_->CreateBufferResource(sizeof(SpotLightEX));
		//書き込むためのアドレスを取得
		spotLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData_));
		//デフォルト値を書き込んでおく
		spotLightData_->color_ = { 1.0f,1.0f,1.0f,1.0f }; // 白色光
		spotLightData_->position_ = { 0.0f,50.0f,0.0f }; // 上から光を当てる
		spotLightData_->intensity_ = 4.0f;//光の強さ
		spotLightData_->direction_ = MyMath::Normalize({ 0.0f, -1.0f, 0.0f }); // 下方向
		spotLightData_->distance_ = 80.0f; // 光の届く距離
		spotLightData_->decay_ = 2.0f; // 減衰率
		spotLightData_->cosAngle_ = std::cos(std::numbers::pi_v<float> / 3.0f); // ライトの角度
		spotLightData_->cosFalloffStart_ = std::cos(std::numbers::pi_v<float> / 3.0f); // ライトの減衰開始角度
	}

	void Object3d::Environment(TKM::DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;
		//環境マップのリソースを作る
		environment_ = dxCommon_->CreateBufferResource(sizeof(EnvironmentEX));
		//書き込むためのアドレスを取得
		environment_->Map(0, nullptr, reinterpret_cast<void**>(&environmentData_));
		//デフォルト値を書き込んでおく
		environmentData_->useEnvironment_ = false;
	}
}