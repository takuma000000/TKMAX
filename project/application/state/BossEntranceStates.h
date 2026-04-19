#pragma once
#include "IState.h"

//=============================================================
// BossEntranceWaitStateクラス
// ボスの登場演出の待機状態
//=============================================================
class BossEntranceWaitState : public TKM::IState {
public:
	/// <summary>
	/// ボスの登場演出の待機状態に入る
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// ボスの登場演出の待機状態を更新する
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

//=============================================================
// BossEntranceSkyFadeInStateクラス
// ボスの登場演出の空のフェードイン状態
//=============================================================
class BossEntranceSkyFadeInState : public TKM::IState {
public:
	/// <summary>
	/// ボスの登場演出の空のフェードイン状態に入る
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// ボスの登場演出の空のフェードイン状態を更新する
	/// </summary>
	///　<param name="ctx">状態コンテキスト</param>
	///　<param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

//=============================================================
// BossEntranceGatherStateクラス
// ボスの登場演出の集まる状態
//=============================================================
class BossEntranceGatherState : public TKM::IState {
public:
	/// <summary>
	/// ボスの登場演出の集まる状態に入る
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// ボスの登場演出の集まる状態を更新する
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

//=============================================================
// BossEntranceCoverStateクラス
// ボスの登場演出の覆い状態
//=============================================================
class BossEntranceCoverState : public TKM::IState {
public:
	/// <summary>
	/// ボスの登場演出の覆い状態に入る
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// ボスの登場演出の覆い状態を更新する
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

//=============================================================
// BossEntranceBurstStateクラス
// ボスの登場演出の爆発状態
//=============================================================
class BossEntranceBurstState : public TKM::IState {
public:
	/// <summary>
	/// ボスの登場演出の爆発状態に入る
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// ボスの登場演出の爆発状態を更新する
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

//=============================================================
// BossEntrancePushStateクラス
// ボスの登場演出の押し出す状態
//=============================================================
class BossEntrancePushState : public TKM::IState {
public:
	/// <summary>
	/// ボスの登場演出の押し出す状態に入る
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// ボスの登場演出の押し出す状態を更新する
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};

//=============================================================
// BossEntranceDoneStateクラス
// ボスの登場演出の完了状態
//=============================================================
class BossEntranceDoneState : public TKM::IState {
public:
	/// <summary>
	/// ボスの登場演出の完了状態に入る
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	void Enter(TKM::IStateContext& ctx) override;
	/// <summary>
	/// ボスの登場演出の完了状態を更新する
	/// </summary>
	/// <param name="ctx">状態コンテキスト</param>
	/// <param name="dt">デルタタイム</param>
	void Update(TKM::IStateContext& ctx, float dt) override;
};