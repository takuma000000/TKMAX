#pragma once
#include "IState.h"

// 前方宣言
class BossController;

//=====================================================
// BossEnterState
//=====================================================
class BossEnterState : public TKM::IState {
public:
	/// <summary>
	/// 状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// 状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

//=====================================================
// BossOrbitState
//=====================================================
class BossOrbitState : public TKM::IState {
public:
	/// <summary>
	/// 状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// 状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

//=====================================================
// BossRecoverState
//（Recover中に「ミサイルバースト実行」「スラッシュ溜め→発射」もここへ移植）
//=====================================================
class BossRecoverState : public TKM::IState {
public:
	/// <summary>
	/// 状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// 状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

//=====================================================
// BossJudgementWindupState
// 深海の審判：中央奥ちょい上へ移動して溜める
//=====================================================
class BossJudgementWindupState : public TKM::IState {
public:
	/// <summary>
	/// 状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// 状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};