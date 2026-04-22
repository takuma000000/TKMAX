#pragma once
namespace TKM {
	class DirectXCommon;
}

//=============================================================
// BaseEffectクラス
// ポストエフェクトの基底クラス
//=============================================================
namespace TKM {
	class BaseEffect {
	public:
		//=============================================================
		// 生成・破棄
		//=============================================================

		virtual ~BaseEffect() = default;

		//=============================================================
		// 初期化・更新・描画
		//=============================================================

		/// <summary>
		/// ポストエフェクトを初期化します。
		/// </summary>
		/// <param name="dx">DirectX共通管理</param>
		virtual void Initialize(TKM::DirectXCommon* dx) {
			dxCommon_ = dx;
		}

		/// <summary>
		/// ポストエフェクトを更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		virtual void Update(float dt) = 0;

		/// <summary>
		/// ポストエフェクトを描画します。
		/// </summary>
		virtual void Draw() = 0;

	protected:
		//=============================================================
		// 共通参照
		//=============================================================

		TKM::DirectXCommon* dxCommon_ = nullptr; // DirectX共通管理
	};
}