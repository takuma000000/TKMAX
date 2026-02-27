#pragma once
#include "IStateContext.h"

namespace TKM {
	//======================================
	// IStateクラス
	// 状態を表すインターフェース
	//======================================
	class IState {
	public:
		virtual ~IState() = default; // 仮想デストラクタ

		/// <summary>
		/// 状態に入るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		virtual void Enter(IStateContext& ctx) {}
		/// <summary>
		/// 状態の更新処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		/// <param name="dt">デルタタイム</param>
		virtual void Update(IStateContext& ctx, float dt) = 0;
		/// <summary>
		/// 状態から出るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		virtual void Exit(IStateContext& ctx) {}
	};
}