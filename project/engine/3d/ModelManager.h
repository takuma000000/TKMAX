#pragma once
#include <map>
#include <string>
#include <memory>

class Model;
class ModelCommon;
class DirectXCommon;

//=============================================================
// ModelManagerクラス
// モデルの生成・管理を行うシングルトンクラス。
//=============================================================
class ModelManager
{
private:
	static ModelManager* instance;

	////シングルトン-----------------------------------------------

	//コンストラクタ、デストラクタの隠蔽
	ModelManager() = default;
	//コピーコンストラクタの封印
	ModelManager(ModelManager&) = delete;
	//コピー代入演算子の封印
	ModelManager& operator=(ModelManager&) = delete;

	////---------------------------------------------------------

	//モデルデータコンテナ
	std::map<std::string, std::unique_ptr<Model>> models;

	//モデル共通部
	ModelCommon* modelCommon = nullptr;
	DirectXCommon* dxCommon_ = nullptr;

public:
	//シングルトンインスタンスの取得
	/// <summary>シングルトンインスタンスを取得します。</summary>
	static ModelManager* GetInstance();
	//終了
	/// <summary>モデルマネージャを終了します。</summary>
	void Finalize();
	//初期化
	/// <summary>モデルマネージャを初期化します。</summary>
	/// <param name="dxCommon">DirectX共通。</param>
	void Initialize(DirectXCommon* dxCommon);
	//モデルのファイルに読み込み
	/// <summary>モデルをファイルから読み込みます。</summary>
	/// <param name="filePath">モデルファイルのパス。</param>
	/// <param name="dxCommon">DirectX共通。</param>
	void LoadModel(const std::string& filePath, DirectXCommon* dxCommon);
	//モデルの検索
	/// <summary>モデルを検索します。</summary>
	/// <param name="filePath">モデルファイルのパス。</param>
	Model* FindModel(const std::string& filePath);

};

