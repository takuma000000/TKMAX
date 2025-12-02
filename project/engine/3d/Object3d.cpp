#include "Object3d.h"
#include "Object3dCommon.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include "TextureManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "engine/3d/camera/Camera.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include <numbers>
#include "BaseScene.h"

Object3d::Object3d(){
	++activeCount_; // 静的メンバ変数のインクリメント
}

Object3d::~Object3d(){
	--activeCount_; // 静的メンバ変数のデクリメント
}

void Object3d::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon){
	//引数で受け取ってメンバ変数に記録する
	this->object3dCommon = object3dCommon;
	dxCommon_ = dxCommon;

	transform.scale = { 1.0f, 1.0f, 1.0f }; //スケール0.1倍
	transform.rotate = { 0.0f, 0.0f, 0.0f };

	//モデル読み込み
	modelData = LoadObjFile("resources", "plane.obj"); //.objファイル読み込み

	VertexResource(dxCommon_); //頂点リソース作成
	MaterialResource(dxCommon_); //マテリアルリソース作成
	WVPResource(dxCommon_); //WVPリソース作成
	Light(dxCommon_); //ライトリソース作成
	CameraResource(dxCommon_); //カメラリソース作成
	PointLight(dxCommon_); //Pointライトリソース作成
	SpotLight(dxCommon_); //Spotライトリソース作成
	Environment(dxCommon_); //環境マップリソース作成

	//.objの参照しているテクスチャファイル読み込み
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	//読み込んだテクスチャの番号を取得
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
	//マテリアルデータにテクスチャ番号をセット
	this->camera = object3dCommon->GetDefaultCamera();
	// 環境マップ設定
	environmentSrvHandleGPU_.ptr = 0;

	// Object3d::Initialize()
	environmentSrvHandleGPU_ = TextureManager::GetInstance()->GetSrvHandleGPU("./resources/rostock_laage_airport_4k.dds");
	if (environmentData) { // 環境マップデータが存在する場合
		environmentData->useEnvironment = false;
	}
}

void Object3d::Update(){
	// TransformからWorldMatrixを作る
	Matrix4x4 worldMatrix = MyMath::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	// ワールドビュー射影行列を計算
	Matrix4x4 worldViewProjectionMatrix;

	if (camera) { // カメラが設定されている場合
		const Matrix4x4& viewProjectionMatrix = camera->GetViewProjectionMatrix(); // カメラのビュー射影行列を取得
		worldViewProjectionMatrix = MyMath::Multiply(worldMatrix, viewProjectionMatrix); // ワールド行列とビュー射影行列を掛け合わせる
	} else { // カメラが設定されていない場合
		worldViewProjectionMatrix = worldMatrix; // ワールド行列のみを使用
	}

	wvpData->wvp = worldViewProjectionMatrix; // ワールドビュー射影行列をセット
	wvpData->World = worldMatrix; // ワールド行列をセット
	wvpData->WorldInverseTranspose = MyMath::Inverse4x4(worldMatrix); // ワールド行列の逆行列をセット

#ifdef USE_IMGUI

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

#endif // USE_IMGUI
}


void Object3d::Draw(DirectXCommon* dxCommon){
	if (parentScene_) { // 親シーンが設定されている場合
		parentScene_->AddDrawCallCount(); // 描画コール数をカウント
	}

	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);// VBVを設定
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress()); // マテリアルをセット
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress()); // WVPをセット

	// Object3d のテクスチャを適用
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData.material.textureFilePath));

	// ライト関連の定数バッファをセット
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, materialResourceLight->GetGPUVirtualAddress());
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraResource->GetGPUVirtualAddress());
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, pointLightResource->GetGPUVirtualAddress());
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, spotLightResource->GetGPUVirtualAddress());

	if (environmentSrvHandleGPU_.ptr != 0) { // 環境マップが設定されている場合
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(7, environmentSrvHandleGPU_); // 環境マップをセット
	}

	// 環境マップ用定数バッファをセット
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(8, environment->GetGPUVirtualAddress());

	// ここで model_ のテクスチャを適用する
	if (model_) {
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(model_->GetTexturePath()));
		model_->Draw();
	}
}

void Object3d::SetModel(const std::string& filePath){
	//モデルを検索してセットする
	model_ = ModelManager::GetInstance()->FindModel(filePath);
}

void Object3d::SetParentScene(BaseScene* parentScene){
	parentScene_ = parentScene; // 親シーンを設定
}

void Object3d::SetEnvironment(const std::string& filename) {
	environmentSrvHandleGPU_ = TextureManager::GetInstance()->GetSrvHandleGPU(filename); // 環境マップのGPUハンドルを取得
	if (environmentData) { // 環境マップデータが存在する場合
		environmentData->useEnvironment = true; // 環境マップを使用するように設定
	}
}

MaterialData Object3d::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename){
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
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

ModelData Object3d::LoadObjFile(const std::string& directoryPath, const std::string& filename){
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
				modelData.vertices.push_back(vertex);
				triangle[faceVertex] = { position,texcoord,normal };

			}
			//頂点を逆順で登録することで、周り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		} else if (identifier == "mtllib") { //マテリアルライブラリ
			//materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			//基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	return modelData;
}

void Object3d::VertexResource(DirectXCommon* dxCommon){
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

void Object3d::MaterialResource(DirectXCommon* dxCommon){
	dxCommon_ = dxCommon;

	//materialResourceを作る
	materialResource = dxCommon->CreateBufferResource(sizeof(Material));
	//materialResourceにデータを書き込むためのアドレスを取得してmaterialDataに割り当てる
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//今回は白を書き込んでみる
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = false;
	materialData->uvTransform = MyMath::MakeIdentity4x4();
	materialData->shininess = 48.3f;//明るさ
}

void Object3d::WVPResource(DirectXCommon* dxCommon){
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

void Object3d::CameraResource(DirectXCommon* dxCommon){
	dxCommon_ = dxCommon;

	cameraResource = dxCommon_->CreateBufferResource(sizeof(CameraForGPU)); // カメラ用のリソースを作る
	cameraResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraData)); // 書き込むためのアドレスを取得
	// カメラ位置を設定
	cameraData->worldPosition = { 0.0f, 5.0f, -10.0f }; // 必要に応じて変更
}

void Object3d::Light(DirectXCommon* dxCommon){
	dxCommon_ = dxCommon;

	//並行光源リソースを作る
	materialResourceLight = dxCommon_->CreateBufferResource(sizeof(DirectionalLightEX));
	//書き込むためのアドレスを取得
	materialResourceLight->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//デフォルト値を書き込んでおく
	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f }; // 白色光
	directionalLightData->direction = { 1.0f, 0.0f, 0.0f }; // 斜め上から光を当てる
	directionalLightData->intensity = 1.0f;//光の強さ
}

void Object3d::PointLight(DirectXCommon* dxCommon){
	dxCommon_ = dxCommon;

	//並行光源リソースを作る
	pointLightResource = dxCommon_->CreateBufferResource(sizeof(PointLightEX));
	//書き込むためのアドレスを取得
	pointLightResource->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));
	//デフォルト値を書き込んでおく
	pointLightData->color = { 1.0f,1.0f,1.0f,1.0f }; // 白色光
	pointLightData->position = { 0.0f,2.0f,0.0f }; // 斜め上から光を当てる
	pointLightData->intensity = 0.0f;//光の強さ
	pointLightData->radius = 10.0f; // 光の届く距離
	pointLightData->decay = 1.0f; // 減衰率
}

void Object3d::SpotLight(DirectXCommon* dxCommon){
	dxCommon_ = dxCommon;

	//並行光源リソースを作る
	spotLightResource = dxCommon_->CreateBufferResource(sizeof(SpotLightEX));
	//書き込むためのアドレスを取得
	spotLightResource->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData));
	//デフォルト値を書き込んでおく
	spotLightData->color = { 1.0f,1.0f,1.0f,1.0f }; // 白色光
	spotLightData->position = { 0.0f,50.0f,0.0f }; // 上から光を当てる
	spotLightData->intensity = 4.0f;//光の強さ
	spotLightData->direction = MyMath::Normalize({ 0.0f, -1.0f, 0.0f }); // 下方向
	spotLightData->distance = 80.0f; // 光の届く距離
	spotLightData->decay = 2.0f; // 減衰率
	spotLightData->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f); // ライトの角度
	spotLightData->cosFalloffStart = std::cos(std::numbers::pi_v<float> / 3.0f); // ライトの減衰開始角度
}

void Object3d::Environment(DirectXCommon* dxCommon){
	dxCommon_ = dxCommon;
	//環境マップのリソースを作る
	environment = dxCommon_->CreateBufferResource(sizeof(EnvironmentEX));
	//書き込むためのアドレスを取得
	environment->Map(0, nullptr, reinterpret_cast<void**>(&environmentData));
	//デフォルト値を書き込んでおく
	environmentData->useEnvironment = false;
}
