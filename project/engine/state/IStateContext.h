#pragma once

namespace TKM {
	//======================================
	// IStateContextクラス
	// 状態遷移のコンテキストを表すインターフェース
	//======================================
	class IStateContext {
	public:
		/// <summary>
		/// IStateContextの仮想デストラクタ
		/// </summary>
		virtual ~IStateContext() = default; // 仮想デストラクタ
	};
}