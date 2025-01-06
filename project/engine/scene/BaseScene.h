#pragma once


class SceneManager;

class BaseScene
{
public:
	virtual ~BaseScene() = default;

	// 純粋仮想関数として宣言
	virtual void Initialize() = 0;
	virtual void Finalize() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;

	virtual void SetSceneManager(SceneManager* sceneManager) {
		sceneManager_ = sceneManager;
	}

protected:
	// シーンマネージャへのポインタ
	SceneManager* sceneManager_ = nullptr;
};

