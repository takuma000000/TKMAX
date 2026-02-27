#pragma once

namespace TKM {
	//======================================
	// IStateContextクラス
	// 状態遷移のコンテキストを表すインターフェース
	//======================================
	class IStateContext {
	public:
		virtual ~IStateContext() = default; // 仮想デストラクタ
	};
}