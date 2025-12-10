#pragma once
#include "MyMath.h"
#include <string>
#include <vector>
#include "DirectXCommon.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

class Object3dCommon;
class Model;
class Camera;
class BaseScene;

//座標変換情報
struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

//頂点データ
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

//マテリアルデータ
struct MaterialData {
	std::string textureFilePath;
	//テクスチャ番号
	uint32_t textureIndex = 0;
};

//モデルデータ
struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};

//マテリアルデータ
struct Material {
	Vector4	color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
	float shininess;//明るさ
};

//座標変換行列データ
struct TransformationMatrix {
	Matrix4x4 wvp;
	Matrix4x4 World;
	Matrix4x4 WorldInverseTranspose;
};

//ライト構造体
struct DirectionalLightEX {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

//PointLight構造体
struct PointLightEX {
	Vector4 color;
	Vector3 position;
	float intensity;
	float radius;
	float decay;
	float padding[2];
};

//SpotLight構造体
struct SpotLightEX {
	Vector4 color;
	Vector3 position;
	float intensity;
	Vector3 direction;
	float distance;
	float decay;
	float cosAngle;
	float cosFalloffStart;
	float padding[2];
};

//カメラ構造体
struct CameraForGPU {
	Vector3 worldPosition;//カメラの位置
	float padding;//16byte境界に合わせるためのパディング
};

// 環境マップ構造体
struct EnvironmentEX {
	bool useEnvironment = false; // 環境マップを使用するかどうか
	Vector3 padding;
};

//=============================================================
// Object3dクラス
// 3Dオブジェクトの描画・変換・ライト設定を行うクラス。
//=============================================================
class Object3d {

public://メンバ関数

	Object3d(); // コンストラクタ
	~Object3d(); // デストラクタ

	/// <summary>
	/// 3Dオブジェクトを初期化します。
	/// </summary>
	/// <param name="object3dCommon"></param>
	/// <param name="dxCommon"></param>
	void Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon);
	/// <summary>
	/// 3Dオブジェクトを終了します。
	/// </summary>
	void Update();
	/// <summary>
	/// 3Dオブジェクトを描画します。
	/// </summary>
	/// <param name="dxCommon"></param>
	void Draw(DirectXCommon* dxCommon);

	// Getter===================================
	/// <summary>スケール、回転、平行移動の取得。</summary>
	const Vector3& GetScale() const { return transform.scale; }
	/// <summary>回転の取得。</summary>
	const Vector3& GetRotate() const { return transform.rotate; }
	/// <summary>平行移動の取得。</summary>
	const Vector3& GetTranslate() const { return transform.translate; }
	/// <summary>アクティブオブジェクト数の取得。</summary>
	static int GetActiveCount() { return activeCount_; }
	/// <summary>モデルの取得。</summary>
	Vector4 GetColor() const { return materialData ? materialData->color : Vector4{ 1,1,1,1 }; }
	// =========================================
	// Setter===================================
	/// <summary>
	/// スケール、回転、平行移動の設定。
	/// </summary>
	/// <param name="scale"></param>
	void SetScale(const Vector3& scale) { this->transform.scale = scale; }
	/// <summary>
	/// 回転の設定。
	/// </summary>
	/// <param name="rotate"></param>
	void SetRotate(const Vector3& rotate) { this->transform.rotate = rotate; }
	/// <summary>
	/// 平行移動の設定。
	/// </summary>
	/// <param name="translate"></param>
	void SetTranslate(const Vector3& translate) { this->transform.translate = translate; }
	/// <summary>
	/// モデルの設定。
	/// </summary>
	/// <param name="model"></param>
	void SetModel(Model* model) { this->model_ = model; }
	/// <summary>
	/// カメラの設定。
	/// </summary>
	/// <param name="camera"></param>
	void SetCamera(Camera* camera) { this->camera = camera; }
	/// <summary>
	/// 親シーンの設定。
	/// </summary>
	/// <param name="parentScene"></param>
	void SetParentScene(BaseScene* parentScene);
	/// <summary>
	/// 環境マップの設定。
	/// </summary>
	/// <param name="filename"></param>
	void SetEnvironment(const std::string& filename);
	/// <summary>
	/// 色の設定。
	/// </summary>
	/// <param name="color"></param>
	void SetColor(const Vector4& color) { if (materialData) { materialData->color = color; } }
	/// <summary>
	/// デバッグ用ImGui表示。
	/// </summary>
	/// <param name="filePath"></param>
	void SetModel(const std::string& filePath);
	// =========================================
private:
	Object3dCommon* object3dCommon = nullptr;
	DirectXCommon* dxCommon_;
	Model* model_ = nullptr;
	Camera* camera = nullptr;

	//Objファイルのデータ
	ModelData modelData;

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

	//頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	//頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	//頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	//マテリアル用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	//マテリアルにデータを書き込む
	Material* materialData = nullptr;

	//WVP用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
	//データを書き込む
	TransformationMatrix* wvpData = nullptr;

	//Light用のマテリアルリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceLight;
	//データを書き込む
	DirectionalLightEX* directionalLightData = nullptr;

	//PointLight用のマテリアルリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource;
	//データを書き込む
	PointLightEX* pointLightData = nullptr;

	//SpotLight用のマテリアルリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource;
	//データを書き込む
	SpotLightEX* spotLightData = nullptr;

	//カメラ用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource;
	//データを書き込む
	CameraForGPU* cameraData = nullptr;

	// 映り込み
	Microsoft::WRL::ComPtr<ID3D12Resource> environment;
	// 環境マップデータ
	EnvironmentEX* environmentData = nullptr;
	// 環境マップ...GPUハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE environmentSrvHandleGPU_;


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
	/// <summary>
	/// WVPリソースを作成します。
	/// </summary>
	/// <param name="dxCommon"></param>
	void WVPResource(DirectXCommon* dxCommon);
	/// <summary>
	/// カメラリソースを作成します。
	/// </summary>
	/// <param name="dxCommon"></param>
	void CameraResource(DirectXCommon* dxCommon);
	/// <summary>
	/// Lightリソースを作成します。
	/// </summary>
	/// <param name="dxCommon"></param>
	void Light(DirectXCommon* dxCommon);
	/// <summary>
	/// PointLightリソースを作成します。
	/// </summary>
	/// <param name="dxCommon"></param>
	void PointLight(DirectXCommon* dxCommon);
	/// <summary>
	/// SpotLightリソースを作成します。
	/// </summary>
	/// <param name="dxCommon"></param>
	void SpotLight(DirectXCommon* dxCommon);
	/// <summary>
	/// 環境マップリソースを作成します。
	/// </summary>
	/// <param name="dxCommon"></param>
	void Environment(DirectXCommon* dxCommon);

	Transform transform;
	Transform cameraTransform;

	//SRV切り替え
	bool useMonsterBall = true;

	BaseScene* parentScene_ = nullptr;

	inline static int activeCount_ = 0; // 静的メンバ変数
};

