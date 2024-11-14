#pragma once
#include <map>
#include <string>
#include <memory>

class Model;
class ModelCommon;
class DirectXCommon;

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
	static ModelManager* GetInstance();
	//終了
	void Finalize();
	//初期化
	void Initialize(DirectXCommon* dxCommon);
	//モデルのファイルに読み込み
	void LoadModel(const std::string& filePath, DirectXCommon* dxCommon);
	//モデルの検索
	Model* FindModel(const std::string& filePath);

};

