#pragma once
#include "IState.h"

//=============================================================
// ClearComedyWaitAfterClearState
// クリア後の間を置く状態
//=============================================================
class ClearComedyWaitAfterClearState : public TKM::IState {
public:
	/// <summary>
	/// クリア後の間を置く状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// クリア後の間を置く状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

// ============================================================
// ClearComedySpawnState
// コメディ出現状態
// ============================================================
class ClearComedySpawnState : public TKM::IState {
public:
	/// <summary>
	/// コメディ出現状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// コメディ出現状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

// ============================================================
// ClearComedySlowNoticeState
// コメディのスローモーションでの気づき状態
// ============================================================
class ClearComedySlowNoticeState : public TKM::IState {
public:
	/// <summary>
	/// コメディのスローモーションでの気づき状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// コメディのスローモーションでの気づき状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

// ============================================================
// ClearComedyRunAwayState
// コメディの逃げる状態
// ============================================================
class ClearComedyRunAwayState : public TKM::IState {
public:
	/// <summary>
	/// コメディの逃げる状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// コメディの逃げる状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

// ============================================================
// ClearComedyFallDownState
// コメディの転ぶ状態
// ============================================================
class ClearComedyFallDownState : public TKM::IState {
public:
	/// <summary>
	/// コメディの転ぶ状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// コメディの転ぶ状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

// ============================================================
// ClearComedyStandUpState
// コメディの立ち上がる状態
// ============================================================
class ClearComedyStandUpState : public TKM::IState {
public:
	/// <summary>
	/// コメディの立ち上がる状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// コメディの立ち上がる状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

// ============================================================
// ClearComedyRecoverRunState
// コメディの走る状態に回復する状態
// ============================================================
class ClearComedyRecoverRunState : public TKM::IState {
public:
	/// <summary>
	/// コメディの走る状態に回復する状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// コメディの走る状態に回復する状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

// ============================================================
// ClearComedyDoneState
// コメディの完了状態
// ============================================================
class ClearComedyDoneState : public TKM::IState {
public:
	/// <summary>
	/// コメディの完了状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// コメディの完了状態の更新処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};