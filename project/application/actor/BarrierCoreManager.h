#pragma once

#include <memory>
#include <vector>
#include <functional>
#include "MidBossCore.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "BaseScene.h"
#include "MyMath.h"

class Player;
class Reticle;

//=========================================================
// BarrierCoreManagerクラス
// バリアコアを管理するクラス
//=========================================================
class BarrierCoreManager {
public:
	BarrierCoreManager() = default;
	~BarrierCoreManager() = default;

	void Initialize(
		TKM::Object3dCommon* common,
		TKM::DirectXCommon* dxCommon,
		TKM::Camera* camera,
		TKM::BaseScene* parent,
		Player* player
	);

	void Update(float dt);
	void Draw(TKM::DirectXCommon* dxCommon);

	void Clear();
	void Spawn(const Vector3& center);

	bool IsAllDestroyed() const;
	int GetAliveCount() const;
	std::vector<MidBossCore*> GetAliveCores() const;

	void SetCamera(TKM::Camera* camera);
	void SetParentScene(TKM::BaseScene* parent);
	void SetPlayer(Player* player);

private:
	void SpawnOne_(const Vector3& pos);
	void SyncPlayerTarget_();
	MidBossCore* FindFirstAliveCore_() const;

	TKM::Object3dCommon* common_ = nullptr;
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::Camera* camera_ = nullptr;
	TKM::BaseScene* parent_ = nullptr;
	Player* player_ = nullptr;

	std::vector<std::unique_ptr<MidBossCore>> cores_;

	static constexpr int kCoreCount_ = 5;
	const Vector3 kCoreScale_ = { 1.8f, 1.8f, 1.8f };
	const Vector3 kCoreColliderScale_ = { 3.1f, 3.1f, 3.1f };
	const int kCoreHP_ = 3;

	static constexpr float kBarrierOuterRadius_ = 18.0f; // バリアのおおよその半径
	static constexpr float kCoreOuterMargin_ = 6.0f; // バリア外周からさらに外へ出す距離
	static constexpr float kCoreRingStartAngleDeg_ = -90.0f; // 上から配置開始
	static constexpr float kCoreZOffset_ = -13.0f;       // 全体を少し手前へ出す量
};