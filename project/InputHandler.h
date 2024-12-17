#pragma once
#include <MoveLeftCommand.h>
#include <MoveRightCommand.h>
#include <Input.h>

class InputHandler {
public:
	void AssignMoveCommands() {
		moveLeftCommand = std::make_unique<MoveLeftCommand>();
		moveRightCommand = std::make_unique<MoveRightCommand>();
	}

	ICommand* HandleInput(Input* input) {
		if (input->PushKey(DIK_A)) return moveLeftCommand.get();
		if (input->PushKey(DIK_D)) return moveRightCommand.get();
		return nullptr;
	}

private:
	std::unique_ptr<ICommand> moveLeftCommand;
	std::unique_ptr<ICommand> moveRightCommand;
};
