#pragma once
#include "ICommand.h"

class MoveRightCommand : public ICommand {
public:
	void Exec(Object3d& object) override {
		Vector3 pos = object.GetTranslate();
		pos.x += 0.5f;
		object.SetTranslate(pos);
	}
};
