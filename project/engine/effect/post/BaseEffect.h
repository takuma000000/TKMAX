#pragma once
namespace TKM{
	class DirectXCommon;
}

//=============================================================
// BaseEffectクラス
// ポストエフェクトの基底クラス。
//=============================================================
namespace TKM {
	class BaseEffect {
	public:
		virtual ~BaseEffect() = default;

		/// <summary>
		/// ポストエフェクトの初期化
		/// </summary>
		/// <param name="dx"></param>
		virtual void Initialize(TKM::DirectXCommon* dx) {
			dxCommon_ = dx;
		}
		/// <summary>
		/// ポストエフェクトの更新
		/// </summary>
		/// <param name="dt"></param>
		virtual void Update(float dt) = 0;
		/// <summary>
		/// ポストエフェクトの描画
		/// </summary>
		virtual void Draw() = 0;
	protected:
		TKM::DirectXCommon* dxCommon_ = nullptr;
	};
}