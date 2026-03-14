#pragma once
#include <string>
#include "MyMath.h"

//=============================================================
// BossConfig
// ボス関連の外部設定をまとめて保持する構造体群
//=============================================================

//-------------------------------------------------------------
// BossEnemy 基本設定
//-------------------------------------------------------------
struct BossEnemyConfig {
	// モデルファイル名やHP、スケールなどの基本設定
	std::string model_ = "jerryfish_boss.obj"; // ボスのモデルファイル名
	std::string tentacleModel_ = "tentacle_boss.obj"; // ボスの触手のモデルファイル名

	int hp_ = 1000; // ボスのHP

	Vector3 scale_ = { 5.0f, 5.0f, 5.0f }; // ボスのスケール
	Vector3 colliderScale_ = { 12.180f, 26.970f, 11.560f }; // ボスのコライダースケール（AABB）

	float normalScale_ = 5.0f; // ボスの通常時のスケール（倍率）
	float lockBlinkSpeed_ = 0.2f; // ロックオン時の点滅速度
	float lockBlinkAmount_ = 0.2f; // ロックオン時の点滅の明るさの増加量
};

//-------------------------------------------------------------
// 撃破時の波紋設定
//-------------------------------------------------------------
struct BossKillRippleConfig {
	float duration_ = 0.35f; // 波紋の持続時間
	float radiusMax_ = 1.45f; // 波紋の最大半径
	float amplitude_ = 0.10f; // 波紋の振幅（高さ）
	float frequency_ = 85.0f; // 波紋の周波数（波の密度）
	float width_ = 10.0f; // 波紋の幅（この値が大きいほど波紋が広がる）
};

//-------------------------------------------------------------
// BossManager 側の戦闘設定
//-------------------------------------------------------------
struct BossBattleConfig {
	Vector3 spawnPos_ = { 0.0f, 0.0f, 200.0f }; // ボスの出現位置（ワールド座標）
	Vector3 arenaMin_ = { -18.0f, 3.0f, 35.0f }; // ボスの行動範囲の最小座標（ワールド座標）
	Vector3 arenaMax_ = { 18.0f, 12.0f, 70.0f }; // ボスの行動範囲の最大座標（ワールド座標）

	BossKillRippleConfig killRipple_{}; // 撃破時の波紋設定

	float killSlowScale_ = 0.00001f; // 撃破時のスローモーションの時間経過速度（この値が小さいほど長くスローモーションになる）
	float killSlowDuration_ = 1.7f; // 撃破時のスローモーションの持続時間（秒）
};

//-------------------------------------------------------------
// BossController の Orbit 設定
//-------------------------------------------------------------
struct BossOrbitConfig {
	float z_ = 55.0f; // プレイヤーを中心とした軌道のZ座標
	float y_ = 8.0f; // プレイヤーを中心とした軌道のY座標
	float radiusX_ = 10.0f; // プレイヤーを中心とした軌道のX方向の半径
	float radiusY_ = 2.5f; // プレイヤーを中心とした軌道のY方向の半径
	float angularSpeed_ = 1.3f; // プレイヤーを中心とした軌道の角速度（ラジアン/秒）
	float playerInfluence_ = 0.35f; // プレイヤーの位置が軌道に与える影響の強さ（0なら完全な円運動、1ならプレイヤーに完全に追従）
	float follow_ = 0.16f; // プレイヤーの位置を軌道が追従する強さ（0なら追従しない、1なら完全に追従）
	float duration_ = 3.2f; // この軌道パターンを維持する時間（秒、0以下なら無限に維持）
};

//-------------------------------------------------------------
// BossController の Recover 設定
//-------------------------------------------------------------
struct BossRecoverConfig {
	float follow_ = 0.18f; // プレイヤーの位置を回復行動が追従する強さ（0なら追従しない、1なら完全に追従）
	float duration_ = 1.0f; // 回復行動の持続時間（秒）
};

//-------------------------------------------------------------
// BossController の Rage 設定
//-------------------------------------------------------------
struct BossRageConfig {
	float gainPerHp_ = 0.08f; // HPが1減るごとに増加するゲージ量
	float decayDelay_ = 1.25f; // ダメージを受けてからゲージの減少が始まるまでの時間（秒）
	float decayPerSec_ = 0.25f; // ゲージの減少速度（秒あたりのゲージ量）
	float onThreshold_ = 1.0f; // ゲージがこの値以上になると怒り状態になる
	float offThreshold_ = 0.20f; // ゲージがこの値以下になると怒り状態が解除される
	float maxGauge_ = 1.5f; // ゲージの最大値（この値を超えないようにゲージは増加する）
};

//-------------------------------------------------------------
// BossController の Missile 設定
//-------------------------------------------------------------
struct BossMissileConfig {
	float muzzleYOffset_ = 1.0f; // ミサイルの発射位置のオフセット（ボスの中心から見たY方向の高さ）
	float chargeTime_ = 1.0f; // 発射前のチャージ時間（秒）
	int burstCount_ = 3; // 1回の攻撃で発射するミサイルの数
	float burstInterval_ = 0.5f; // 連続してミサイルを発射する場合の、ミサイル同士の発射間隔（秒）
	float speed_ = 70.0f; // ミサイルの速度
	float curveHeight_ = 2.5f; // ミサイルの軌道の曲がり具合（この値が大きいほど、ミサイルは放物線を描いて落ちる）
	int damage_ = 1; // ミサイルのダメージ
	int lifeFrame_ = 180; // ミサイルの寿命（フレーム、これを超えるとミサイルは消える）
};

//-------------------------------------------------------------
// BossController の Slash 設定
//-------------------------------------------------------------
struct BossSlashConfig {
	float cooldown_ = 2.0f; // 攻撃のクールダウン時間（秒）
	float selectRate_ = 0.45f; // この攻撃が選択される確率（0〜1の範囲で、他の攻撃と合計して1になるように調整する）
	float chargeTime_ = 0.55f; // 攻撃前のチャージ時間（秒）
	float speed_ = 95.0f; // 攻撃の速度
	int damage_ = 2; // 攻撃のダメージ
	int lifeFrame_ = 90; // 攻撃の寿命（フレーム、これを超えると攻撃は消える）
};

//-------------------------------------------------------------
// BossController の Laser 設定
//-------------------------------------------------------------
struct BossLaserConfig {
	float cooldown_ = 0.0f; // 攻撃のクールダウン時間（秒）
	float windup_ = 1.0f; // 攻撃の予備動作時間（秒、レーザーが発射される前の溜め時間）
	float fire_ = 1.2f; // 攻撃の発射時間（秒）
	float recover_ = 0.8f; // 攻撃の回復時間（秒、レーザーが消えた後の隙の時間）
	float radius_ = 1.6f; // レーザーの半径（この値が大きいほど、レーザーは太くなる）
	float muzzleYOffset_ = 1.5f; // レーザーの発射位置のオフセット（ボスの中心から見たY方向の高さ）
	float trackStrength_ = 0.18f; // レーザーの追尾の強さ（0なら全く追尾しない、1なら完全にプレイヤーを追尾する）
};

//-------------------------------------------------------------
// BossController の登場設定
//-------------------------------------------------------------
struct BossEnterConfig {
	float approachSpeedZ_ = 18.0f; // プレイヤーに近づくときの前進速度（Z方向）
	float approachSpeedX_ = 10.0f; // プレイヤーに近づくときの横移動速度（X方向、プレイヤーの位置に合わせて移動する際の速度）
	float approachSpeedY_ = 10.0f; // プレイヤーに近づくときの高さ移動速度（Y方向、プレイヤーの位置に合わせて移動する際の速度）
	float completeEpsilonZ_ = 0.05f; // 登場完了とみなすZ座標の誤差許容値（この値以内にプレイヤーに近づいたら登場完了とする）
};

//-------------------------------------------------------------
// BossController 全体設定
//-------------------------------------------------------------
struct BossControllerConfig {
	float predictLeadTime_ = 0.35f; // プレイヤーの位置予測のリードタイム（秒、攻撃がプレイヤーに当たるまでの時間を予測して、その分先の位置を狙うための時間）

	BossOrbitConfig orbit_{}; // Orbit 行動の設定
	BossRecoverConfig recover_{}; // Recover 行動の設定
	BossRageConfig rage_{}; // Rage ゲージの設定
	BossMissileConfig missile_{}; // Missile 攻撃の設定
	BossSlashConfig slash_{}; // Slash 攻撃の設定
	BossLaserConfig laser_{}; // Laser 攻撃の設定
	BossEnterConfig enter_{}; // 登場の設定
};

//-------------------------------------------------------------
// ボス設定全体
//-------------------------------------------------------------
struct BossConfig {
	BossEnemyConfig bossEnemy_{}; // ボスの基本設定
	BossBattleConfig bossBattle_{}; // ボス戦闘全体の設定
	BossControllerConfig bossController_{}; // ボスの行動パターン全体の設定
};