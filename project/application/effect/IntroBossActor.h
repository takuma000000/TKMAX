#pragma once
#include <memory>
#include "BossEnemy.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "MyMath.h"

namespace TKM {

	class IntroBossActor {
	public:
		/// <summary>
		/// 演出用ボスアクターを初期化します。
		/// </summary>
		/// <param name="object3dCommon">3D描画共通データ</param>
		/// <param name="dxCommon">DirectX共通管理</param>
		void Initialize(TKM::Object3dCommon* object3dCommon, DirectXCommon* dxCommon);

		/// <summary>
		/// 演出状態を初期状態に戻します。
		/// </summary>
		void Reset();

		/// <summary>
		/// 出現前エフェクト開始状態に入ります。
		/// </summary>
		void BeginPreSpawn();

		/// <summary>
		/// 出現前エフェクトを更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		void UpdatePreSpawn(float dt);

		/// <summary>
		/// 出現前エフェクトが終了したかを返します。
		/// </summary>
		bool IsPreSpawnFinished() const;

		/// <summary>
		/// ボス本体を生成します。
		/// </summary>
		/// <param name="camera">描画や初期化に使うカメラ</param>
		void Spawn(Camera* camera);

		/// <summary>
		/// 出現演出を開始します。
		/// </summary>
		void BeginAppear();

		/// <summary>
		/// 出現演出を更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		/// <returns>演出が終了したらtrue</returns>
		bool UpdateAppear(float dt);

		/// <summary>
		/// 静止待機演出を開始します。
		/// </summary>
		void BeginPause();

		/// <summary>
		/// 静止待機演出を更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		/// <returns>演出が終了したらtrue</returns>
		bool UpdatePause(float dt);

		/// <summary>
		/// 注意喚起用のジャンプ演出を開始します。
		/// </summary>
		void BeginNoticeHop();

		/// <summary>
		/// 注意喚起用のジャンプ演出を更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		/// <returns>演出が終了したらtrue</returns>
		bool UpdateNoticeHop(float dt);

		/// <summary>
		/// 焦り・動揺演出を開始します。
		/// </summary>
		void BeginPanic();

		/// <summary>
		/// 焦り・動揺演出を更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		/// <returns>演出が終了したらtrue</returns>
		bool UpdatePanic(float dt);

		/// <summary>
		/// 退避・逃走演出を開始します。
		/// </summary>
		void BeginEscape();

		/// <summary>
		/// 退避・逃走演出を更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		/// <returns>演出が終了したらtrue</returns>
		bool UpdateEscape(float dt);

		/// <summary>
		/// ボス演出アクターを描画します。
		/// </summary>
		/// <param name="dxCommon">DirectX共通管理</param>
		void Draw(DirectXCommon* dxCommon) const;

		/// <summary>
		/// ボスが存在しているかを返します。
		/// </summary>
		bool Exists() const { return boss_ != nullptr; }

		// Getter=====================================
		/// <summary>
		/// 現在位置を取得します。
		/// </summary>
		const Vector3& GetPosition() const { return pos_; }

		/// <summary>
		/// 基準位置を取得します。
		/// </summary>
		const Vector3& GetBasePosition() const { return basePos_; }

		/// <summary>
		/// 出現開始Z座標を取得します。
		/// </summary>
		float GetAppearStartZ() const { return appearStartZ_; }

		/// <summary>
		/// 退避終了Z座標を取得します。
		/// </summary>
		float GetEscapeEndZ() const { return escapeEndZ_; }

		/// <summary>
		/// 出現前演出の経過時間を取得します。
		/// </summary>
		float GetPreSpawnElapsed() const { return preSpawnElapsed_; }

		/// <summary>
		/// 出現前演出の発生間隔管理用蓄積値を取得します。
		/// </summary>
		float GetPreSpawnEmitAccum() const { return preSpawnEmitAccum_; }

		/// <summary>
		/// 出現演出の進行率を取得します。
		/// </summary>
		float GetAppearRatio() const;

		/// <summary>
		/// 待機演出の進行率を取得します。
		/// </summary>
		float GetPauseRatio() const;

		/// <summary>
		/// 注意ジャンプ演出の進行率を取得します。
		/// </summary>
		float GetNoticeHopRatio() const;

		/// <summary>
		/// パニック演出の進行率を取得します。
		/// </summary>
		float GetPanicRatio() const;

		/// <summary>
		/// 逃走演出の進行率を取得します。
		/// </summary>
		float GetEscapeRatio() const;

		/// <summary>
		/// ボス本体を取得します。
		/// </summary>
		BossEnemy* GetBoss() { return boss_.get(); }

		/// <summary>
		/// ボス本体を取得します。
		/// </summary>
		const BossEnemy* GetBoss() const { return boss_.get(); }
		// ===========================================
		// Setter=====================================
		/// <summary>
		/// 基準位置を設定します。
		/// </summary>
		void SetBasePosition(const Vector3& pos) { basePos_ = pos; }

		/// <summary>
		/// 現在位置を設定します。
		/// </summary>
		void SetPosition(const Vector3& pos) { pos_ = pos; }

		/// <summary>
		/// 注意マークを出したかどうかを設定します。
		/// </summary>
		void SetNoticeMarkEmitted(bool f) { noticeMarkEmitted_ = f; }

		/// <summary>
		/// 出現エフェクト完了フラグを設定します。
		/// </summary>
		void SetSpawnFxFinished(bool f) { spawnFxFinished_ = f; }

		/// <summary>
		/// 逃走時ワープバーストを出したかどうかを設定します。
		/// </summary>
		void SetEscapeWarpBurstEmitted(bool f) { escapeWarpBurstEmitted_ = f; }

		/// <summary>
		/// 出現前演出の発生間隔管理用蓄積値を設定します。
		/// </summary>
		void SetPreSpawnEmitAccum(float v) { preSpawnEmitAccum_ = v; }
		// ===========================================
		/// <summary>
		/// 出現エフェクト完了フラグを返します。
		/// </summary>
		bool ConsumeSpawnFxFinished() const { return spawnFxFinished_; }

		/// <summary>
		/// 出現エフェクト完了フラグを返します。
		/// </summary>
		bool IsSpawnFxFinished() const { return spawnFxFinished_; }

		/// <summary>
		/// 注意マークを既に出したかを返します。
		/// </summary>
		bool IsNoticeMarkEmitted() const { return noticeMarkEmitted_; }

		/// <summary>
		/// 逃走時ワープバーストを既に出したかを返します。
		/// </summary>
		bool IsEscapeWarpBurstEmitted() const { return escapeWarpBurstEmitted_; }

	private:
		std::unique_ptr<BossEnemy> boss_ = nullptr;                   // 演出用に生成・保持するボス本体
		TKM::Object3dCommon* object3dCommon_ = nullptr;              // 3Dオブジェクト描画共通データ
		DirectXCommon* dxCommon_ = nullptr;                          // DirectX共通管理

		float phaseElapsed_ = 0.0f;                                  // 現在の演出フェーズでの経過時間

		Vector3 pos_{ 0.0f, 6.0f, 48.0f };                           // 現在のボス位置
		Vector3 basePos_{ 0.0f, 6.0f, 48.0f };                       // 各演出の基準になるボス位置

		float appearSec_ = 1.90f;                                    // 出現演出にかける秒数
		float pauseSec_ = 0.28f;                                     // 出現後の静止待機秒数
		float panicSec_ = 1.20f;                                     // パニック演出にかける秒数
		float escapeSec_ = 2.10f;                                    // 逃走演出にかける秒数
		float noticeHopSec_ = 2.10f;                                 // 注意ジャンプ演出にかける秒数

		float appearStartZ_ = 120.0f;                                // 出現開始時のZ座標
		float appearEndZ_ = 44.0f;                                   // 出現終了時のZ座標
		float escapeEndZ_ = 120.0f;                                  // 逃走終了時のZ座標

		float panicAmpX_ = 2.8f;                                     // パニック演出のX揺れ幅
		float panicAmpY_ = 0.55f;                                    // パニック演出のY揺れ幅
		float escapeAmpX_ = 7.5f;                                    // 逃走演出中の左右移動幅
		float escapeHopY_ = 2.0f;                                    // 逃走演出中の上下跳ね量
		float escapeSpeedZ_ = 28.0f;                                 // 逃走演出中の前後移動速度
		float noticeHopY_ = 2.6f;                                    // 注意ジャンプ演出の跳ね上がり量

		float escapeTargetX_ = 0.0f;                                 // 逃走中に目指す一時的なX目標位置
		float escapeTargetTimer_ = 0.0f;                             // 次のX目標を切り替えるまでのタイマー
		float escapeTargetInterval_ = 0.10f;                         // X目標を切り替える間隔

		float appearFloatAmpX_ = 2.0f;                               // 出現演出中のX方向ふわつき幅
		float appearFloatAmpY_ = 3.4f;                               // 出現演出中のY方向ふわつき幅
		float appearFloatFreqX_ = 1.9f;                              // 出現演出中のX方向ふわつき周波数
		float appearFloatFreqY_ = 2.1f;                              // 出現演出中のY方向ふわつき周波数
		float appearTiltZ_ = 0.14f;                                  // 出現演出中のZ回転傾き量

		float preSpawnElapsed_ = 0.0f;                               // 出現前エフェクトの経過時間
		float preSpawnEmitAccum_ = 0.0f;                             // 出現前エフェクトの発生間隔制御用蓄積値
		bool  spawnFxFinished_ = false;                              // 出現前エフェクトが完了したか
		bool  noticeMarkEmitted_ = false;                            // 注意マーク演出を既に出したか
		bool  escapeWarpBurstEmitted_ = false;                       // 逃走ワープバーストを既に出したか
	};

}