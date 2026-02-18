#include "ModelManager.h"
#include "ModelCommon.h"
#include "Model.h"
#include "DirectXCommon.h"

namespace TKM {
	ModelManager* ModelManager::GetInstance() {
		static ModelManager instance;
		return &instance;
	}

	void ModelManager::Finalize() {}

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
		if (models_.contains(filePath)) {
			//読み込み済みなら早期return
			return;
		}
		//モデルの生成とファイル読み込み、初期化
		std::unique_ptr<TKM::Model> model = std::make_unique<TKM::Model>();
		model->Initialize(modelCommon_, dxCommon_, "resources/obj", filePath);
		//モデルをmapコンテナに格納する
		models_.insert(std::make_pair(filePath, std::move(model)));
	}

	TKM::Model* ModelManager::FindModel(const std::string& filePath) {
		//読み込み済みモデルを検索
		if (models_.contains(filePath)) {
			//読み込みモデルを戻り値としてreturn
			return models_.at(filePath).get();
		}
		//ファイル名一致なし
		return nullptr;
	}
}