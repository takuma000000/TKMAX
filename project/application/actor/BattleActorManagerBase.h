#pragma once
#include "DirectXCommon.h"
#include "CameraManager.h"
#include "BaseScene.h"

class Player;

//=============================================================
// BattleActorManagerBase
// EnemyManager / BossManager の共通部分をまとめる基底クラス。
//=============================================================
class BattleActorManagerBase {
public:
	BattleActorManagerBase() = default;
	virtual ~BattleActorManagerBase() = default;

	/// <summary>
	/// EnemyManager / BossManager の両方で必要な共通の初期化処理を行います。
	/// </summary>
	/// <param name="dx">DirectXCommon へのポインタ</param>
	/// <param name="camera">Camera へのポインタ</param>
	/// <param name="parent">BaseScene へのポインタ</param>
	/// <param name="player">Player へのポインタ</param>
	void InitializeCommon(
		TKM::DirectXCommon* dx,
		TKM::Camera* camera,
		TKM::BaseScene* parent,
		Player* player
	);
	/// <summary>
	/// EnemyManager / BossManager の両方で必要な共通の終了処理を行います。
	/// </summary>
	/// <param name="dx">DirectXCommon へのポインタ</param>
	virtual void Update(float dt) = 0;
	/// <summary>
	/// EnemyManager / BossManager の両方で必要な共通の描画処理を行います。
	/// </summary>
	/// <param name="dx">DirectXCommon へのポインタ</param>
	virtual void Draw(TKM::DirectXCommon* dx) = 0;

	// Setter========================================
	/// <summary>
	/// カメラを設定します。これを呼ぶと、派生クラスの OnCameraChanged() が呼ばれます。
	/// </summary>
	/// <param name="camera">Camera へのポインタ</param>
	virtual void SetCamera(TKM::Camera* camera);
	// ==============================================
protected:
	/// <summary>
	/// カメラが変更されたときに呼ばれる純粋仮想関数。派生クラスはこれを実装して、カメラ変更時の必要な処理を行います。
	/// </summary>
	virtual void OnCameraChanged() = 0;
	//======================================
	// 共通で参照を保持するためのメンバ変数
	//======================================
	TKM::DirectXCommon* dx_ = nullptr;
	TKM::Camera* camera_ = nullptr;
	TKM::BaseScene* parent_ = nullptr;
	Player* player_ = nullptr;
};