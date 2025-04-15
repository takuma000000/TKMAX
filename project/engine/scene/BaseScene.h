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

	void UpdatePerformanceInfo();

protected:
	// シーンマネージャへのポインタ
	SceneManager* sceneManager_ = nullptr;

protected:
	float fps_ = 0.0f;          // フレームレート
	float timeCount_ = 0.0f;    // 経過時間
	int frameCount_ = 0;        // フレーム数
	float frameTimeMs_ = 0.0f;  // フレームタイム(ms)

};

