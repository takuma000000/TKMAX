#pragma once
#include "ParticleManager.h"

namespace TKM {

	//=============================================
	// ParticleGroupsCatalogクラス
	// ゲームシーンで使用するパーティクルグループの登録を行うクラス。
	//=============================================
	class ParticleGroupsCatalog {
	public:

		/// <summary>
		/// ゲームシーン用のパーティクルグループを登録します。
		/// </summary>
		/// <param name="pm">登録先となるパーティクルマネージャ</param>
		static void RegisterScene(ParticleManager* pm);

	};
}