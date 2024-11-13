#pragma once
#include <map>
#include <string>
#include <memory>

class Model;
class ModelCommmon;

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
	ModelCommmon* modelCommon = nullptr;

public:
	//シングルトンインスタンスの取得
	static ModelManager* GetInstance();
	//終了
	void Finalize();

};

