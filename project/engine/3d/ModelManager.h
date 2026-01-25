#pragma once
#include <map>
#include <string>
#include <memory>

namespace TKM {
	class Model;
	class ModelCommon;
	class DirectXCommon;
}

//=============================================================
// ModelManagerクラス
// モデルの生成・管理を行うシングルトンクラス。
//=============================================================
namespace TKM {
	class ModelManager {
	private:
		///シングルトン-----------------------------------------------
		//コンストラクタ、デストラクタの隠蔽
		ModelManager() = default;
		//コピーコンストラクタの封印
		ModelManager(ModelManager&) = delete;
		//コピー代入演算子の封印
		ModelManager& operator=(ModelManager&) = delete;
		///---------------------------------------------------------

		//モデルデータコンテナ
		std::map<std::string, std::unique_ptr<TKM::Model>> models_;
		//モデル共通部
		TKM::ModelCommon* modelCommon_ = nullptr;
		TKM::DirectXCommon* dxCommon_ = nullptr;

	public:
		/// <summary>
		/// モデルマネージャを終了処理します。
		/// </summary>
		void Finalize();
		/// <summary>
		/// モデルマネージャを初期化します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void Initialize(TKM::DirectXCommon* dxCommon);
		/// <summary>
		/// モデルを読み込みます。
		/// </summary>
		/// <param name="filePath"></param>
		/// <param name="dxCommon"></param>
		void LoadModel(const std::string& filePath, TKM::DirectXCommon* dxCommon);
		/// <summary>
		/// モデルを検索します。
		/// </summary>
		/// <param name="filePath"></param>
		/// <returns></returns>
		TKM::Model* FindModel(const std::string& filePath);

		// Getter===================================
		/// <summary>
		/// モデルマネージャのインスタンスを取得します。
		/// </summary>
		/// <returns></returns>
		static ModelManager* GetInstance();
		// =========================================
	};
}