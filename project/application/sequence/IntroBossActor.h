#pragma once
#include <memory>
#include "BossEnemy.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "MyMath.h"

namespace TKM {

	//=============================================================
	// IntroBossActorクラス
	// 開幕演出専用のボスアクターを管理するクラス。
	// 本編用のBossEnemyとは別に、出現・待機・驚き・逃走などの
	// 演出用挙動をまとめて扱う。
	//=============================================================
	class IntroBossActor {
	public:
		//=============================================================
		// 初期化・リセット
		//=============================================================

		/// <summary>
		/// 演出用ボスアクターの初期化を行います。
		/// </summary>
		/// <param name="object3dCommon">3Dオブジェクト描画に必要な共通データ</param>
		/// <param name="dxCommon">DirectX共通管理</param>
		void Initialize(TKM::Object3dCommon* object3dCommon, DirectXCommon* dxCommon);

		/// <summary>
		/// ボス位置、各種フラグ、演出時間を初期状態に戻します。
		/// </summary>
		void Reset();

		//=============================================================
		// 出現前演出
		//=============================================================

		/// <summary>
		/// ボス出現前の予兆エフェクト状態を開始します。
		/// </summary>
		void BeginPreSpawn();

		/// <summary>
		/// ボス出現前の予兆エフェクトを更新します。
		/// </summary>
		/// <param name="dt">1フレームあたりの経過時間</param>
		void UpdatePreSpawn(float dt);

		/// <summary>
		/// ボス出現前の予兆エフェクトが終了したかを返します。
		/// </summary>
		/// <returns>終了していればtrue</returns>
		bool IsPreSpawnFinished() const;

		//=============================================================
		// ボス生成・出現演出
		//=============================================================

		/// <summary>
		/// 演出用のボス本体を生成します。
		/// </summary>
		/// <param name="camera">ボス描画に使用するカメラ</param>
		void Spawn(Camera* camera);

		/// <summary>
		/// ボスが奥から手前に現れる出現演出を開始します。
		/// </summary>
		void BeginAppear();

		/// <summary>
		/// ボスが奥から手前に現れる出現演出を更新します。
		/// </summary>
		/// <param name="dt">1フレームあたりの経過時間</param>
		/// <returns>出現演出が終了していればtrue</returns>
		bool UpdateAppear(float dt);

		//=============================================================
		// 待機演出
		//=============================================================

		/// <summary>
		/// 出現後、次の演出へ移る前の短い静止待機を開始します。
		/// </summary>
		void BeginPause();

		/// <summary>
		/// 出現後の短い静止待機を更新します。
		/// </summary>
		/// <param name="dt">1フレームあたりの経過時間</param>
		/// <returns>待機時間が終了していればtrue</returns>
		bool UpdatePause(float dt);

		//=============================================================
		// 注意ジャンプ演出
		//=============================================================

		/// <summary>
		/// プレイヤーへ注意を向けさせるためのジャンプ演出を開始します。
		/// </summary>
		void BeginNoticeHop();

		/// <summary>
		/// 注意喚起用のジャンプ演出を更新します。
		/// </summary>
		/// <param name="dt">1フレームあたりの経過時間</param>
		/// <returns>ジャンプ演出が終了していればtrue</returns>
		bool UpdateNoticeHop(float dt);

		//=============================================================
		// パニック演出
		//=============================================================

		/// <summary>
		/// ボスが焦って左右上下に揺れるパニック演出を開始します。
		/// </summary>
		void BeginPanic();

		/// <summary>
		/// ボスが焦って左右上下に揺れるパニック演出を更新します。
		/// </summary>
		/// <param name="dt">1フレームあたりの経過時間</param>
		/// <returns>パニック演出が終了していればtrue</returns>
		bool UpdatePanic(float dt);

		//=============================================================
		// 逃走演出
		//=============================================================

		/// <summary>
		/// ボスが奥方向へ逃げていく逃走演出を開始します。
		/// </summary>
		void BeginEscape();

		/// <summary>
		/// ボスが奥方向へ逃げていく逃走演出を更新します。
		/// </summary>
		/// <param name="dt">1フレームあたりの経過時間</param>
		/// <returns>逃走演出が終了していればtrue</returns>
		bool UpdateEscape(float dt);

		//=============================================================
		// 描画
		//=============================================================

		/// <summary>
		/// 演出用ボスアクターを描画します。
		/// </summary>
		/// <param name="dxCommon">DirectX共通管理</param>
		void Draw(DirectXCommon* dxCommon) const;

		//=============================================================
		// 状態取得
		//=============================================================

		/// <summary>
		/// 演出用ボスが生成済みかを返します。
		/// </summary>
		/// <returns>生成済みならtrue</returns>
		bool Exists() const { return boss_ != nullptr; }

		//=============================================================
		// Getter
		//=============================================================

		/// <summary>
		/// 現在のボス位置を取得します。
		/// </summary>
		const Vector3& GetPosition() const { return pos_; }

		/// <summary>
		/// 各演出の基準になるボス位置を取得します。
		/// </summary>
		const Vector3& GetBasePosition() const { return basePos_; }

		/// <summary>
		/// 出現演出開始時のZ座標を取得します。
		/// </summary>
		float GetAppearStartZ() const { return appearStartZ_; }

		/// <summary>
		/// 逃走演出終了時のZ座標を取得します。
		/// </summary>
		float GetEscapeEndZ() const { return escapeEndZ_; }

		/// <summary>
		/// 出現前演出の経過時間を取得します。
		/// </summary>
		float GetPreSpawnElapsed() const { return preSpawnElapsed_; }

		/// <summary>
		/// 出現前エフェクトの発生間隔を管理する蓄積時間を取得します。
		/// </summary>
		float GetPreSpawnEmitAccum() const { return preSpawnEmitAccum_; }

		/// <summary>
		/// 出現演出の進行率を取得します。
		/// </summary>
		/// <returns>0.0fから1.0fの進行率</returns>
		float GetAppearRatio() const;

		/// <summary>
		/// 待機演出の進行率を取得します。
		/// </summary>
		/// <returns>0.0fから1.0fの進行率</returns>
		float GetPauseRatio() const;

		/// <summary>
		/// 注意ジャンプ演出の進行率を取得します。
		/// </summary>
		/// <returns>0.0fから1.0fの進行率</returns>
		float GetNoticeHopRatio() const;

		/// <summary>
		/// パニック演出の進行率を取得します。
		/// </summary>
		/// <returns>0.0fから1.0fの進行率</returns>
		float GetPanicRatio() const;

		/// <summary>
		/// 逃走演出の進行率を取得します。
		/// </summary>
		/// <returns>0.0fから1.0fの進行率</returns>
		float GetEscapeRatio() const;

		/// <summary>
		/// 演出用ボス本体を取得します。
		/// </summary>
		BossEnemy* GetBoss() { return boss_.get(); }

		/// <summary>
		/// 演出用ボス本体を取得します。
		/// </summary>
		const BossEnemy* GetBoss() const { return boss_.get(); }

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// 各演出の基準になるボス位置を設定します。
		/// </summary>
		void SetBasePosition(const Vector3& pos) { basePos_ = pos; }

		/// <summary>
		/// 現在のボス位置を設定します。
		/// </summary>
		void SetPosition(const Vector3& pos) { pos_ = pos; }

		/// <summary>
		/// 注意マークを出したかどうかを設定します。
		/// </summary>
		void SetNoticeMarkEmitted(bool f) { noticeMarkEmitted_ = f; }

		/// <summary>
		/// 出現前エフェクト完了フラグを設定します。
		/// </summary>
		void SetSpawnFxFinished(bool f) { spawnFxFinished_ = f; }

		/// <summary>
		/// 逃走時のワープバーストを出したかどうかを設定します。
		/// </summary>
		void SetEscapeWarpBurstEmitted(bool f) { escapeWarpBurstEmitted_ = f; }

		/// <summary>
		/// 出現前エフェクトの発生間隔を管理する蓄積時間を設定します。
		/// </summary>
		void SetPreSpawnEmitAccum(float v) { preSpawnEmitAccum_ = v; }

		//=============================================================
		// 演出フラグ取得
		//=============================================================

		/// <summary>
		/// 出現前エフェクトが完了しているかを返します。
		/// </summary>
		bool ConsumeSpawnFxFinished() const { return spawnFxFinished_; }

		/// <summary>
		/// 出現前エフェクトが完了しているかを返します。
		/// </summary>
		bool IsSpawnFxFinished() const { return spawnFxFinished_; }

		/// <summary>
		/// 注意マークを既に出したかを返します。
		/// </summary>
		bool IsNoticeMarkEmitted() const { return noticeMarkEmitted_; }

		/// <summary>
		/// 逃走時のワープバーストを既に出したかを返します。
		/// </summary>
		bool IsEscapeWarpBurstEmitted() const { return escapeWarpBurstEmitted_; }

	private:
		//=============================================================
		// 外部参照
		//=============================================================

		std::unique_ptr<BossEnemy> boss_ = nullptr;      // 演出用に生成して保持するボス本体
		TKM::Object3dCommon* object3dCommon_ = nullptr; // 3Dオブジェクト生成・描画に使う共通データ
		DirectXCommon* dxCommon_ = nullptr;             // DirectXの描画・リソース管理に使う共通クラス

		//=============================================================
		// 共通演出状態
		//=============================================================

		float phaseElapsed_ = 0.0f; // 現在の演出フェーズに入ってからの経過時間

		Vector3 pos_{ 0.0f, 6.0f, 48.0f };     // 現在のボス位置
		Vector3 basePos_{ 0.0f, 6.0f, 48.0f }; // 出現・待機・逃走などの基準になるボス位置

		//=============================================================
		// 各演出の所要時間
		//=============================================================

		float appearSec_ = 1.90f;    // 奥から手前に出現するまでの時間
		float pauseSec_ = 0.28f;     // 出現後に一瞬止まる待機時間
		float panicSec_ = 1.20f;     // 焦り・動揺演出を行う時間
		float escapeSec_ = 2.10f;    // 奥方向へ逃走するまでの時間
		float noticeHopSec_ = 2.10f; // 注意喚起用ジャンプ演出の時間

		//=============================================================
		// 出現・逃走位置
		//=============================================================

		float appearStartZ_ = 120.0f; // 出現演出の開始Z座標
		float appearEndZ_ = 44.0f;    // 出現演出の終了Z座標
		float escapeEndZ_ = 120.0f;   // 逃走演出の終了Z座標

		//=============================================================
		// パニック・逃走・注意ジャンプ演出パラメータ
		//=============================================================

		float panicAmpX_ = 2.8f;      // パニック演出中の左右揺れ幅
		float panicAmpY_ = 0.55f;     // パニック演出中の上下揺れ幅
		float escapeAmpX_ = 7.5f;     // 逃走演出中の左右移動幅
		float escapeHopY_ = 2.0f;     // 逃走演出中の上下跳ね量
		float escapeSpeedZ_ = 28.0f;  // 逃走演出中に奥方向へ進む速度
		float noticeHopY_ = 2.6f;     // 注意ジャンプ演出で跳ね上がる高さ

		float escapeTargetX_ = 0.0f;         // 逃走中に向かう一時的なX目標位置
		float escapeTargetTimer_ = 0.0f;     // 次のX目標へ切り替えるまでの残り時間
		float escapeTargetInterval_ = 0.10f; // 逃走中のX目標を切り替える間隔

		//=============================================================
		// 出現中のふわつき演出パラメータ
		//=============================================================

		float appearFloatAmpX_ = 2.0f;  // 出現中のX方向ふわつき幅
		float appearFloatAmpY_ = 3.4f;  // 出現中のY方向ふわつき幅
		float appearFloatFreqX_ = 1.9f; // 出現中のX方向ふわつき速度
		float appearFloatFreqY_ = 2.1f; // 出現中のY方向ふわつき速度
		float appearTiltZ_ = 0.14f;     // 出現中にボスを傾けるZ回転量

		//=============================================================
		// 出現前エフェクト状態
		//=============================================================

		float preSpawnElapsed_ = 0.0f;   // 出現前エフェクトの経過時間
		float preSpawnEmitAccum_ = 0.0f; // エフェクトを一定間隔で出すための蓄積時間

		//=============================================================
		// 演出発生済みフラグ
		//=============================================================

		bool spawnFxFinished_ = false;       // 出現前エフェクトが完了しているか
		bool noticeMarkEmitted_ = false;     // 注意マーク演出を既に出したか
		bool escapeWarpBurstEmitted_ = false;// 逃走時のワープバーストを既に出したか
	};

}