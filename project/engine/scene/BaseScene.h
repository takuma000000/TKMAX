#pragma once

class SceneManager;

//=============================================================
// BaseSceneクラス
// シーンの基本機能を定義する抽象基底クラス。
//=============================================================
class BaseScene{
public:
	virtual ~BaseScene() = default;

	// 純粋仮想関数として宣言
	/// <summary>シーンを初期化します。</summary>
	virtual void Initialize() = 0;
	/// <summary>シーンを終了します。</summary>
	virtual void Finalize() = 0;
	/// <summary>シーンを更新します。</summary>
	virtual void Update() = 0;
	/// <summary>シーンを描画します。</summary>
	virtual void Draw() = 0;

	/// <summary>
	/// </summary>シーンマネージャを設定します。</summary>
	/// </summary>
	/// <param name="sceneManager"></param>
	virtual void SetSceneManager(SceneManager* sceneManager) {
		sceneManager_ = sceneManager;
	}

	/// <summary>DrawCall数を加算します。</summary>
	void AddDrawCallCount() { drawCallCount_++; }

protected:
	// シーンマネージャへのポインタ
	SceneManager* sceneManager_ = nullptr;

protected:
	float fps_ = 0.0f;          // フレームレート
	float timeCount_ = 0.0f;    // 経過時間
	int frameCount_ = 0;        // フレーム数
	float frameTimeMs_ = 0.0f;  // フレームタイム(ms)

	int drawCallCount_ = 0;  // DrawCall数カウント用

	/// <summary>
	/// </summary>パフォーマンス情報を更新します。</summary>
	/// </summary>
	void UpdatePerformanceInfo();// TKMAXパフォーマンス可視化
	/// <summary>
	/// </summary>DrawCall数をリセットします。</summary>
	/// </summary>
	void ResetDrawCallCount(); // カウントリセット
};