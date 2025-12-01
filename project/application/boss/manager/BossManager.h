#pragma once
#include <memory>
#include <vector>
#include "DirectXCommon.h"
#include "Camera.h"
#include "engine/func/math/Vector3.h"
#include "Object3dCommon.h"
#include "engine/audio/AudioManager.h"
#include "application/boss/BossEnemy.h"
#include "application/boss/BossBullet.h"
#include "application/player/Player.h"
#include "BaseScene.h"

//=============================================================
// BossManagerクラス
// ボス本体＋ボス弾の管理を行うクラス。
//=============================================================
class BossManager {
public:
	BossManager() = default;
	~BossManager() = default;

	/// <summary>参照を渡して初期化。</summary>
	void Initialize(DirectXCommon* dxCommon, Camera* camera, BaseScene* parent, Player* player);

	/// <summary>ボス戦を開始（ボスを生成）。</summary>
	void StartBattle();

	/// <summary>更新。</summary>
	void Update(float dt);

	/// <summary>描画。</summary>
	void Draw(DirectXCommon* dxCommon);

	/// <summary>ボス弾生成（GameScene から呼ばれる入り口）。</summary>
	void SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame);

	/// <summary>ボス戦中か？</summary>
	bool IsBattleActive() const;

	/// <summary>ボスが生きているか？（ロックオン用）</summary>
	bool IsBossAlive() const;

	/// <summary>ボスが倒されているか？</summary>
	bool IsBossDead() const;

	/// <summary>ボス本体への生ポインタ（ImGui やロックオン用）。</summary>
	BossEnemy* GetBoss() const { return boss_.get(); }

	/// <summary>クリア演出開始時にボス関連を全部消す。</summary>
	void OnClearSequenceStart();

private:
	DirectXCommon* dxCommon_ = nullptr;
	Camera* camera_ = nullptr;
	BaseScene* parentScene_ = nullptr;
	Player* player_ = nullptr;

	bool bossBattle_ = false;                     // ボス戦フラグ
	bool bossP2BgmPlayed_ = false;                // P2BGMを1回だけ再生したか

	std::unique_ptr<BossEnemy> boss_;             // ボス本体
	std::vector<std::unique_ptr<BossBullet>> bossBullets_; // ボス弾リスト
};
