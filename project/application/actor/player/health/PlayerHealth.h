#pragma once

//=============================================================
// PlayerHealthクラス
// HP、無敵状態、被弾フラッシュ、プレイヤーのHP管理全般を担当するクラス。
//=============================================================
class PlayerHealth {
public:

	/// <summary>
	/// HP管理を初期化します。
	/// </summary>
	/// <param name="maxHp">最大HP</param>
	void Initialize(int maxHp);
	/// <summary>
	/// HP関連の時間処理を更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void Update(float dt);

	/// <summary>
	/// ダメージを受けます。
	/// </summary>
	/// <param name="value">受けるダメージ量</param>
	/// <returns>ダメージが通った場合 true、それ以外は false</returns>
	bool Damage(int value);
	/// <summary>
	/// 攻撃ID付きでダメージを受けます。
	/// 同じ攻撃IDによる短時間の連続ヒットを防ぎます。
	/// </summary>
	/// <param name="damage">受けるダメージ量</param>
	/// <param name="attackId">攻撃を識別するID</param>
	/// <returns>ダメージが通った場合 true、それ以外は false</returns>
	bool TryDamageFromAttack(int damage, int attackId);
	/// <summary>
	/// HPを最大値に戻します。
	/// </summary>
	void Reset();

	// Setter=========================================
	/// <summary>
	/// HPを設定します。
	/// </summary>
	/// <param name="hp">設定するHP</param>
	void SetHP(int hp);
	// ===============================================
	// Getter=========================================
	/// <summary>
	/// 現在HPを取得します。
	/// </summary>
	/// <returns></returns>
	int GetHP() const { return hp_; }
	/// <summary>
	/// 最大HPを取得します。
	/// </summary>
	/// <returns></returns>
	int GetMaxHP() const { return maxHp_; }
	/// <summary>
	/// HP割合を取得します。
	/// </summary>
	/// <returns></returns>
	float GetHPRate() const;
	/// <summary>
	/// 直前に当たった攻撃IDを取得します。
	/// </summary>
	/// <returns></returns>
	int GetLastHitAttackId() const { return lastHitAttackId_; }
	// ===============================================

	/// <summary>
	/// HPが0かどうか。
	/// </summary>
	/// <returns></returns>
	bool IsDead() const { return hp_ <= 0; }
	/// <summary>
	/// 無敵中かどうか。
	/// </summary>
	/// <returns></returns>
	bool IsInvincible() const { return isInvincible_; }
	/// <summary>
	/// 無敵点滅で本体を表示するかどうか。
	/// </summary>
	/// <returns></returns>
	bool IsVisible() const { return invincibleVisible_; }
	/// <summary>
	/// 被弾フラッシュ中かどうか。
	/// </summary>
	/// <returns></returns>
	bool IsHitFlashActive() const { return hitFlashTimer_ > 0.0f; }

private:

	//=============================================================
	// HP
	//=============================================================
	int maxHp_ = 5; // 最大HP
	int hp_ = 5;    // 現在HP

	//=============================================================
	// 被弾フラッシュ
	//=============================================================
	float hitFlashTimer_ = 0.0f; // 被弾フラッシュ用タイマー

	//=============================================================
	// 無敵 & 点滅
	//=============================================================
	bool isInvincible_ = false;     // 無敵中か
	float invincibleT_ = 0.0f;      // 無敵経過秒
	float blinkT_ = 0.0f;           // 点滅用タイマー
	bool invincibleVisible_ = true; // 点滅表示フラグ

	static constexpr float kInvincibleSec_ = 2.0f;  // 無敵時間
	static constexpr float kBlinkInterval_ = 0.08f; // 点滅間隔（秒）

	//=============================================================
	// 被弾管理（同一攻撃IDの連続ヒット防止）
	//=============================================================
	int lastHitAttackId_ = -1;       // 最後に当たった攻撃ID
	float sameAttackLockT_ = 0.0f;   // 同一攻撃IDロック残り時間（秒）

	static constexpr float kSameAttackLockSec_ = 0.20f; // 同一攻撃IDロック時間
	static constexpr float kHitFlashSec_ = 0.15f;       // 被弾フラッシュ時間
};