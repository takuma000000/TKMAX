#pragma once
#include <string>

//=============================================================
// PlayerShotConfigクラス
// プレイヤーのRB/LB弾の設定値を管理するクラス。
//=============================================================
class PlayerShotConfig {
public:
	struct RBConfig {
		float bulletSpeed_ = 10.0f;       // RB弾の速度
		int ammoMax_ = 500;               // RB弾の最大弾数
		float refillWaitSec_ = 1.0f;      // RB弾が回復開始するまでの待機時間
		float refillSec_ = 2.0f;          // RB弾が満タンになるまでの時間
		float shotCooldownSec_ = 0.08f;   // RB弾の発射間隔
	};

	struct LBConfig {
		int ammoMax_ = 6;                 // LB弾の最大弾数
		float refillWaitSec_ = 2.0f;      // LB弾が満タン回復するまでの待機時間
		float arcHeight_ = 18.0f;         // LB弾の山なり高さ
		float arcDuration_ = 0.55f;       // LB弾の飛翔時間
		float forwardOffsetZ_ = 28.0f;    // ターゲットが無い時の前方終点距離
	};

	/// <summary>
	/// 設定ファイルを読み込みます。
	/// 拡張子に応じて JSON 読込関数へ振り分けます。
	/// </summary>
	/// <param name="path">読み込む設定ファイルパス</param>
	/// <returns>読み込みに成功したら true</returns>
	bool Load(const char* path);
	/// <summary>
	/// JSON形式の設定ファイルを読み込みます。
	/// </summary>
	/// <param name="path">読み込むJSONファイルパス</param>
	/// <returns>読み込みに成功したら true</returns>
	bool LoadJson(const char* path);

	// Getter========================================
	/// <summary>
	/// RB/LB弾の設定を取得します。
	/// </summary>
	/// <returns>RB/LB弾の設定</returns>
	const RBConfig& GetRB() const { return rb_; }
	/// <summary>
	/// LB弾の設定を取得します。
	/// </summary>
	/// <returns>LB弾の設定</returns>
	const LBConfig& GetLB() const { return lb_; }
	// ==============================================

private:
	/// <summary>
	/// ファイルパスの拡張子が一致するかを判定します。
	/// </summary>
	static bool HasExtension(const std::string& path, const char* ext);

	// ========================================
	// 設定値
	// ========================================
	RBConfig rb_; // RB弾の設定
	LBConfig lb_; // LB弾の設定
};