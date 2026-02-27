#pragma once
#include <memory>
#include "IState.h"

class TitleScene;

//=====================================================
// TitleFlowIntroIrisOpenStateクラス
// タイトルシーンの「入場（アイリスオープン）」状態を表すクラス。
//=====================================================
class TitleFlowIntroIrisOpenState : public TKM::IState {
public:
	/// <summary>
	/// 状態に入るときの処理を行います。
	/// </summary>
	/// <param name="ctx">状態遷移のコンテキスト</param>
	/// 今回はタイトルシーンの状態なので、TitleScene* を IStateContext として受け取ることを想定しています。
	void Update(TKM::IStateContext& ctx, float dt) override;
};

//=====================================================
// TitleFlowIdleStateクラス
// タイトルシーンの「待機」状態を表すクラス。
//=====================================================
class TitleFlowIdleState : public TKM::IState {
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
// TitleFlowVanishingStateクラス
// タイトルシーンの「消滅（UI非表示のまま）」状態を表すクラス。
//=====================================================
class TitleFlowVanishingState : public TKM::IState {
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
// TitleFlowRippleStateクラス
// タイトルシーンの「波紋エフェクト発生」状態を表すクラス。
//=====================================================
class TitleFlowRippleState : public TKM::IState {
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
// TitleFlowIrisCloseStateクラス
// タイトルシーンの「退場（アイリスクローズ）」状態を表すクラス。
//=====================================================
class TitleFlowIrisCloseState : public TKM::IState {
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