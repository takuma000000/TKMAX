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
		void Initialize(TKM::Object3dCommon* object3dCommon, DirectXCommon* dxCommon);
		void Reset();

		void BeginPreSpawn();
		void UpdatePreSpawn(float dt);
		bool IsPreSpawnFinished() const;

		void Spawn(Camera* camera);

		void BeginAppear();
		bool UpdateAppear(float dt);

		void BeginPause();
		bool UpdatePause(float dt);

		void BeginNoticeHop();
		bool UpdateNoticeHop(float dt);

		void BeginPanic();
		bool UpdatePanic(float dt);

		void BeginEscape();
		bool UpdateEscape(float dt);

		void Draw(DirectXCommon* dxCommon) const;
		bool Exists() const { return boss_ != nullptr; }

		const Vector3& GetPosition() const { return pos_; }
		const Vector3& GetBasePosition() const { return basePos_; }

		void SetBasePosition(const Vector3& pos) { basePos_ = pos; }
		void SetPosition(const Vector3& pos) { pos_ = pos; }

		bool ConsumeSpawnFxFinished() const { return spawnFxFinished_; }
		bool IsSpawnFxFinished() const { return spawnFxFinished_; }
		void SetSpawnFxFinished(bool f) { spawnFxFinished_ = f; }

		bool IsNoticeMarkEmitted() const { return noticeMarkEmitted_; }
		void SetNoticeMarkEmitted(bool f) { noticeMarkEmitted_ = f; }

		bool IsEscapeWarpBurstEmitted() const { return escapeWarpBurstEmitted_; }
		void SetEscapeWarpBurstEmitted(bool f) { escapeWarpBurstEmitted_ = f; }

		float GetAppearStartZ() const { return appearStartZ_; }
		float GetEscapeEndZ() const { return escapeEndZ_; }
		float GetPreSpawnElapsed() const { return preSpawnElapsed_; }
		float GetPreSpawnEmitAccum() const { return preSpawnEmitAccum_; }
		void SetPreSpawnEmitAccum(float v) { preSpawnEmitAccum_ = v; }

		float GetAppearRatio() const;
		float GetPauseRatio() const;
		float GetNoticeHopRatio() const;
		float GetPanicRatio() const;
		float GetEscapeRatio() const;

		BossEnemy* GetBoss() { return boss_.get(); }
		const BossEnemy* GetBoss() const { return boss_.get(); }

	private:
		std::unique_ptr<BossEnemy> boss_ = nullptr;
		TKM::Object3dCommon* object3dCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;

		float phaseElapsed_ = 0.0f;

		Vector3 pos_{ 0.0f, 6.0f, 48.0f };
		Vector3 basePos_{ 0.0f, 6.0f, 48.0f };

		float appearSec_ = 1.90f;
		float pauseSec_ = 0.28f;
		float panicSec_ = 1.20f;
		float escapeSec_ = 2.10f;
		float noticeHopSec_ = 2.10f;

		float appearStartZ_ = 120.0f;
		float appearEndZ_ = 44.0f;
		float escapeEndZ_ = 120.0f;

		float panicAmpX_ = 2.8f;
		float panicAmpY_ = 0.55f;
		float escapeAmpX_ = 7.5f;
		float escapeHopY_ = 2.0f;
		float escapeSpeedZ_ = 28.0f;
		float noticeHopY_ = 2.6f;

		float escapeTargetX_ = 0.0f;
		float escapeTargetTimer_ = 0.0f;
		float escapeTargetInterval_ = 0.10f;

		float appearFloatAmpX_ = 2.0f;
		float appearFloatAmpY_ = 3.4f;
		float appearFloatFreqX_ = 1.9f;
		float appearFloatFreqY_ = 2.1f;
		float appearTiltZ_ = 0.14f;

		float preSpawnElapsed_ = 0.0f;
		float preSpawnEmitAccum_ = 0.0f;
		bool  spawnFxFinished_ = false;
		bool  noticeMarkEmitted_ = false;
		bool  escapeWarpBurstEmitted_ = false;
	};

}