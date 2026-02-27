#include "StateMachine.h"
#include <cassert>

namespace TKM {
	void StateMachine::Initialize(IStateContext* owner) {
		owner_ = owner; // オーナーをセット
		assert(owner_ && "StateMachine::Initialize owner is null"); // オーナーのnullチェック
	}

	void StateMachine::Update(float dt) {
		if (!owner_) { return; } // オーナーがいない場合は更新できないため、早期リターン
		if (!state_) { return; } // 状態がない場合は更新できないため、早期リターン
		state_->Update(*owner_, dt); // 現在の状態のUpdateを呼び出す
	}

	void StateMachine::Change(std::unique_ptr<IState> next) {
		if (!owner_) { return; } // オーナーがいない場合は状態遷移できないため、早期リターン

		if (state_) { // 現在の状態がある場合はExitを呼び出す
			state_->Exit(*owner_); // 現在の状態から出るときの処理
		}

		state_ = std::move(next); // 状態を遷移（所有権を移動）

		if (state_) { // 次の状態がある場合はEnterを呼び出す
			state_->Enter(*owner_); // 次の状態に入るときの処理
		}
	}
}