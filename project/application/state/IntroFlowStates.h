#pragma once
#include "IState.h"

namespace TKM {

	//=============================================================
	// IntroIrisOpenStateクラス
	// アイリスが開く演出の状態
	//=============================================================
	class IntroIrisOpenState : public IState {
	public:
		/// <summary>
		/// 状態に入るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		void Enter(IStateContext& ctx) override;
		/// <summary>
		/// 状態の更新処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		/// <param name="dt">デルタタイム</param>
		void Update(IStateContext& ctx, float dt) override;
	};

	//=============================================================
	// IntroIrisOpenBurstStateクラス
	// アイリスが開ききる瞬間のバーストエフェクトの状態
	//=============================================================
	class IntroCameraIntroState : public IState {
	public:
		/// <summary>
		/// 状態に入るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		void Enter(IStateContext& ctx) override;
		/// <summary>
		/// 状態の更新処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		/// <param name="dt">デルタタイム</param>
		void Update(IStateContext& ctx, float dt) override;
	};

	//=============================================================
	// IntroBossPreSpawnStateクラス
	// ボスがスポーンする前の待機状態
	//=============================================================
	class IntroBossPreSpawnState : public IState {
	public:
		/// <summary>
		/// 状態に入るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		void Enter(IStateContext& ctx) override;
		/// <summary>
		/// 状態の更新処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		/// <param name="dt">デルタタイム</param>
		void Update(IStateContext& ctx, float dt) override;
	};

	//=============================================================
	// IntroBossSpawnStateクラス
	// ボスがスポーンする状態
	//=============================================================
	class IntroBossAppearState : public IState {
	public:
		/// <summary>
		/// 状態に入るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		void Enter(IStateContext& ctx) override;
		/// <summary>
		/// 状態の更新処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		/// <param name="dt">デルタタイム</param>
		void Update(IStateContext& ctx, float dt) override;
	};

	//=============================================================
	// IntroBossPauseStateクラス
	// ボスがスポーンしてしばらくの間、プレイヤーを見つめる状態
	//=============================================================
	class IntroBossPauseState : public IState {
	public:
		/// <summary>
		/// 状態に入るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		void Enter(IStateContext& ctx) override;
		/// <summary>
		/// 状態の更新処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		/// <param name="dt">デルタタイム</param>
		void Update(IStateContext& ctx, float dt) override;
	};

	//=============================================================
	// IntroBossNoticeHopStateクラス
	// ボスがプレイヤーに気づいて、軽くホップする状態
	//=============================================================
	class IntroBossNoticeHopState : public IState {
	public:
		/// <summary>
		/// 状態に入るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		void Enter(IStateContext& ctx) override;
		/// <summary>
		/// 状態の更新処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		/// <param name="dt">デルタタイム</param>
		void Update(IStateContext& ctx, float dt) override;
	};

	//=============================================================
	// IntroBossPanicStateクラス
	// ボスがパニックになって暴れる状態
	//=============================================================
	class IntroBossPanicState : public IState {
	public:
		/// <summary>
		/// 状態に入るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		void Enter(IStateContext& ctx) override;
		/// <summary>
		/// 状態の更新処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		/// <param name="dt">デルタタイム</param>
		void Update(IStateContext& ctx, float dt) override;
	};

	//=============================================================
	// IntroBossEscapeStateクラス
	// ボスが逃げる状態
	//=============================================================
	class IntroBossEscapeState : public IState {
	public:
		/// <summary>
		/// 状態に入るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		void Enter(IStateContext& ctx) override;
		/// <summary>
		/// 状態の更新処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		/// <param name="dt">デルタタイム</param>
		void Update(IStateContext& ctx, float dt) override;
	};

	//=============================================================
	// IntroCameraBlendToBossStateクラス
	// カメラがボスにブレンドする状態
	//=============================================================
	class IntroShowStartState : public IState {
	public:
		/// <summary>
		/// 状態に入るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		void Enter(IStateContext& ctx) override;
		/// <summary>
		/// 状態の更新処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		/// <param name="dt">デルタタイム</param>
		void Update(IStateContext& ctx, float dt) override;
	};

	//=============================================================
	// IntroCameraBlendBackStateクラス
	// カメラが元の位置にブレンドバックする状態
	//=============================================================
	class IntroDoneState : public IState {
	public:
		/// <summary>
		/// 状態に入るときの処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		void Enter(IStateContext& ctx) override;
		/// <summary>
		/// 状態の更新処理を行います。
		/// </summary>
		/// <param name="ctx">状態遷移のコンテキスト</param>
		/// <param name="dt">デルタタイム</param>
		void Update(IStateContext& ctx, float dt) override;
	};

}