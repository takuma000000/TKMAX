#pragma once
class Framework
{
public:
	// 仮想関数（派生クラスで必ず実装する必要がある）
	virtual void Initialize();   // 初期化
	virtual void Finalize();     // 終了
	virtual void Update();       // 毎フレーム更新

	// 純粋仮想関数（必ずオーバーライドしないとエラーになる）
	virtual void Draw() = 0;     // 描画

	// ゲーム終了のチェック
	virtual bool IsEndRequest() { return endRequest_; }

	virtual ~Framework() = default;

	//実行
	void Run();

protected:
	bool endRequest_ = false;    // 終了フラグ
};

