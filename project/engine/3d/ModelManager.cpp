#include "ModelManager.h"
#include "ModelCommon.h"
#include "Model.h"
#include "DirectXCommon.h"

namespace TKM {
	ModelManager* ModelManager::instance = nullptr; //シングルトンインスタンスの初期化

	ModelManager* ModelManager::GetInstance() {
		if (instance == nullptr) { //インスタンスがなければ生成
			instance = new ModelManager;
		}
		return instance;
	}

	void ModelManager::Finalize() {
		delete instance;
		instance = nullptr;
	}

	void ModelManager::Initialize(TKM::DirectXCommon* dxCommon) {
		//ポインタ...ModelCommon
		std::unique_ptr<TKM::ModelCommon> modelCommon = nullptr;
		//Object3d共通部の初期化
		modelCommon = std::make_unique<TKM::ModelCommon>();
		modelCommon->Initialize(dxCommon);
	}

	void ModelManager::LoadModel(const std::string& filePath, TKM::DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;
		//読み込み済みモデルを検索
		if (models.contains(filePath)) {
			//読み込み済みなら早期return
			return;
		}
		//モデルの生成とファイル読み込み、初期化
		std::unique_ptr<TKM::Model> model = std::make_unique<TKM::Model>();
		model->Initialize(modelCommon, dxCommon_, "resources", filePath);
		//モデルをmapコンテナに格納する
		models.insert(std::make_pair(filePath, std::move(model)));
	}

	TKM::Model* ModelManager::FindModel(const std::string& filePath) {
		//読み込み済みモデルを検索
		if (models.contains(filePath)) {
			//読み込みモデルを戻り値としてreturn
			return models.at(filePath).get();
		}
		//ファイル名一致なし
		return nullptr;
	}
}