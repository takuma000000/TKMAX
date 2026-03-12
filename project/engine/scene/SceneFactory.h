#pragma once
#include "AbstractSceneFactory.h"
#include "Framework.h"
#include <memory>

//=============================================================
// SceneFactoryクラス
// シーン生成を行うクラス。
//=============================================================
namespace TKM {
	class SceneFactory : public TKM::AbstractSceneFactory {
	public:
		/// <summary>
		/// <span class="code-inline">SceneFactory</span>のコンストラクタ
		/// </summary>
		/// <param name="dxCommon"></param>
		/// <param name="srvManager"></param>
		SceneFactory(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager)
			: dxCommon_(dxCommon), srvManager_(srvManager) {
		}

		/// <summary>
		///	</span class="code-inline">CreateScene</span>シーンの生成
		/// </summary>
		/// <param name="sceneName"></param>
		/// <returns></returns>
		std::unique_ptr<TKM::BaseScene> CreateScene(const std::string& sceneName) override;

	private:
		TKM::DirectXCommon* dxCommon_ = nullptr;
		TKM::SrvManager* srvManager_ = nullptr;
	};
}