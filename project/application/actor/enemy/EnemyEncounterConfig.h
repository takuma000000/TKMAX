#pragma once
#include <array>
#include <string>
#include <vector>
#include "MyMath.h"
#include "Enemy.h"

//=============================================================
// EnemyEncounterConfigクラス
// 敵の遭遇パターンや行動パターンなどの設定を管理するクラス。
//=============================================================
class EnemyEncounterConfig {
public:
	// Enemy Params ==========================================
	struct SmallEnemyPhase {
		int defeatTarget_ = 0; // このフェーズの目標撃破数
	};
	// =======================================================

	/// <summary>
	/// 設定ファイルを読み込みます。
	/// 拡張子に応じて CSV / JSON の読込関数へ振り分けます。
	/// </summary>
	/// <param name="path">読み込む設定ファイルのパス</param>
	/// <returns>読み込みに成功した場合 true、それ以外は false</returns>
	bool Load(const char* path);
	/// <summary>
	/// CSV形式の設定ファイルを読み込みます。
	/// </summary>
	/// <param name="path">読み込む CSV ファイルのパス</param>
	/// <returns>読み込みに成功した場合 true、それ以外は false</returns>
	bool LoadCsv(const char* path);
	/// <summary>
	/// JSON形式の設定ファイルを読み込みます。
	/// </summary>
	/// <param name="path">読み込む JSON ファイルのパス</param>
	/// <returns>読み込みに成功した場合 true、それ以外は false</returns>
	bool LoadJson(const char* path);

	// Getter==========================================
	/// <summary>
	/// SmallEnemyPhase の設定を取得します。
	/// </summary>
	/// <returns>SmallEnemyPhase の設定</returns>
	const SmallEnemyPhase& GetSmallEnemyPhase() const { return smallEnemyPhase_; }
	// ================================================

private:
	/// <summary>
	/// 文字列を等価比較します。
	/// </summary>
	/// <param name="a">比較対象となる文字列</param>
	/// <param name="b">比較対象となる C 文字列</param>
	/// <returns>等しい場合 true、それ以外は false</returns>
	static bool StrEq(const std::string& a, const char* b);
	/// <summary>
	/// 文字列を float 値に変換します。
	/// </summary>
	/// <param name="s">変換元となる文字列</param>
	/// <returns>変換後の float 値</returns>
	static float ToF(const std::string& s);
	/// <summary>
	/// 文字列を int 値に変換します。
	/// </summary>
	/// <param name="s">変換元となる文字列</param>
	/// <returns>変換後の int 値</returns>
	static int   ToI(const std::string& s);
	/// <summary>
	/// ファイルパスの拡張子が一致するかを判定します。
	/// </summary>
	/// <param name="path">ファイルパス</param>
	/// <param name="ext">比較する拡張子（例: ".csv"）</param>
	/// <returns>一致したら true</returns>
	static bool HasExtension(const std::string& path, const char* ext);

	//===================================================
	// 文字列を EnemyBehavior 列挙型に変換します。
	//===================================================
	SmallEnemyPhase smallEnemyPhase_;

public:
	// Enemy Params ==========================================
	struct MainEnemyParams {
		std::string model_;                                    // 敵本体のモデル名
		int hp_ = 1;                                           // 敵のHP
		Vector3 scale_ = { 1.0f, 1.0f, 1.0f };                 // 敵本体の表示スケール

		EnemyType type_ = EnemyType::MainSquad;                // 敵の種類

		bool useTentacle_ = false;                             // 触手を使用するか
		std::string tentacleModel_;                            // 触手モデル名
		Vector3 tentacleLocalPosition_ = { 0.0f, 0.0f, 0.0f }; // 触手ローカル位置
		Vector3 tentacleLocalRotation_ = { 0.0f, 0.0f, 0.0f }; // 触手ローカル回転
		Vector3 tentacleLocalScale_ = { 1.0f, 1.0f, 1.0f };    // 触手ローカルスケール

		float formationMoveSpeed_ = 0.08f;                     // 円形隊列への追従速度
	};
	// =======================================================

	/// <summary>
	/// 雑魚敵フェーズ本隊の敵パラメータを取得します。
	/// </summary>
	/// <returns>雑魚敵フェーズ本隊の敵パラメータ</returns>
	const MainEnemyParams& GetMainEnemyParams() const { return mainEnemyParams_; }
private:
	// ===================================================
	// 雑魚敵のパラメータ
	// ===================================================
	MainEnemyParams mainEnemyParams_{};
};