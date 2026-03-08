#pragma once
#include <memory>
#include "IState.h"

namespace TKM {
	//======================================
	// StateMachineクラス
	// 状態遷移を管理するクラス
	//======================================
	class StateMachine {
	public:
		/// <summary>
		/// 状態遷移のコンテキストをセットして初期化します。
		/// </summary>
		/// <param name="owner">状態遷移のコンテキスト</param>
		void Initialize(IStateContext* owner);
		/// <summary>
		/// 状態遷移のコンテキストをリセットして終了処理します。
		/// </summary>
		/// <param name="owner">状態遷移のコンテキスト</param>
		void Update(float dt);

		/// <summary>
		/// 状態を遷移させます。現在の状態からExit()を呼び出し、次の状態にEnter()を呼び出します。
		/// </summary>
		/// <param name="next">次の状態</param>
		void Change(std::unique_ptr<IState> next);

		// Getter==================================
		/// <summary>
		/// 現在の状態を取得します。
		/// </summary>
		/// <returns>現在の状態</returns>
		IState* GetState() const { return state_.get(); }
		// ========================================

	private:
		//===========================
		// 内部データ
		//===========================
		IStateContext* owner_ = nullptr; // 状態遷移のコンテキスト
		//===========================
		// 現在の状態
		//===========================
		std::unique_ptr<IState> state_; // 現在の状態
	};
}