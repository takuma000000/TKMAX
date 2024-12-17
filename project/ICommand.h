#pragma once
#include <Object3d.h>

class ICommand
{
public:
	virtual ~ICommand() = default;
	virtual void Exec(Object3d& object) = 0;
};

