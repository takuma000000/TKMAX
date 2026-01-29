#pragma once
#include "ParticleManager.h"

namespace TKM {
	class ParticleGroupsCatalog {
	public:

		/// <summary>
		/// ゲームシーン用のパーティクルグループを登録します。
		/// </summary>
		/// <param name="pm">登録先となるパーティクルマネージャ</param>
		static void RegisterGameScene(ParticleManager* pm);

	};
}