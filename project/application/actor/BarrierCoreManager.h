#pragma once

#include <memory>
#include <vector>
#include <functional>
#include "BarrierCore.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "BaseScene.h"
#include "MyMath.h"

class Player;
class Reticle;

//=============================================================
// BarrierCoreManagerクラス
// バリアコア（複数）を生成・更新・管理するクラス。
//=============================================================
class BarrierCoreManager {
public:
	BarrierCoreManager() = default;
	~BarrierCoreManager() = default;

	/// <summary>
	/// バリアコア管理クラスを初期化します。
	/// </summary>
	/// <param name="common">Object3d 共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	/// <param name="camera">描画に使用するカメラ</param>
	/// <param name="parent">親シーン</param>
	/// <param name="player">プレイヤー参照</param>
	void Initialize(
		TKM::Object3dCommon* common,
		TKM::DirectXCommon* dxCommon,
		TKM::Camera* camera,
		TKM::BaseScene* parent,
		Player* player
	);
	/// <summary>
	/// バリアコア全体の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void Update(float dt);
	/// <summary>
	/// バリアコア全体を描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dxCommon);

	/// <summary>
	/// すべてのコアを削除します。
	/// </summary>
	void Clear();
	/// <summary>
	/// 指定位置を中心としてコアを生成します。
	/// </summary>
	/// <param name="center">生成中心位置（ワールド座標）</param>
	void Spawn(const Vector3& center);

	/// <summary>
	/// すべてのコアが破壊されたかを判定します。
	/// </summary>
	/// <returns>全破壊済みなら true、それ以外は false</returns>
	bool IsAllDestroyed() const;

	// Getter==================================
	/// <summary>
	/// 生存しているコア数を取得します。
	/// </summary>
	/// <returns>生存コア数</returns>
	int GetAliveCount() const;
	/// <summary>
	/// 破壊されていないコアのリストを取得します。
	/// </summary>
	/// <returns>生存コアポインタ配列</returns>
	std::vector<BarrierCore*> GetAliveCores() const;
	// ========================================
	// Setter==================================
	/// <summary>
	/// 使用するカメラを設定します。
	/// </summary>
	/// <param name="camera">描画に使用するカメラ</param>
	void SetCamera(TKM::Camera* camera);
	/// <summary>
	/// 親シーンを設定します。
	/// </summary>
	/// <param name="parent">親シーン</param>
	void SetParentScene(TKM::BaseScene* parent);
	/// <summary>
	/// プレイヤー参照を設定します。
	/// </summary>
	/// <param name="player">プレイヤーオブジェクト</param>
	void SetPlayer(Player* player);
	// ========================================

private:
	//======================================================================
	// 内部メソッド
	//======================================================================
	/// <summary>
	/// 単体のコアを生成します。
	/// </summary>
	/// <param name="pos">生成位置（ワールド座標）</param>
	void SpawnOne_(const Vector3& pos);
	/// <summary>
	/// プレイヤーのターゲットをコアに同期します。
	/// </summary>
	void SyncPlayerTarget_();
	/// <summary>
	/// 最初に見つかった生存コアを取得します。
	/// </summary>
	/// <returns>生存コア（なければ nullptr）</returns>
	BarrierCore* FindFirstAliveCore_() const;

	//======================================================================
	// 参照ポインタ / 共通オブジェクト
	//======================================================================
	TKM::Object3dCommon* common_ = nullptr;
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::Camera* camera_ = nullptr;
	TKM::BaseScene* parent_ = nullptr;
	Player* player_ = nullptr;
	//======================================================================
	// コア管理
	//======================================================================
	std::vector<std::unique_ptr<BarrierCore>> cores_;
	//======================================================================
	// コア基本設定
	//======================================================================
	static constexpr int kCoreCount_ = 5; // コアの生成数
	const Vector3 kCoreScale_ = { 1.8f, 1.8f, 1.8f }; // 描画スケール
	const Vector3 kCoreColliderScale_ = { 3.1f, 3.1f, 3.1f }; // 当たり判定スケール
	const int kCoreHP_ = 3; // コアの耐久値
	//======================================================================
	// 配置パラメータ（円形配置）
	//======================================================================
	static constexpr float kBarrierOuterRadius_ = 18.0f; // バリアのおおよその半径
	static constexpr float kCoreOuterMargin_ = 6.0f;     // バリア外周からのオフセット
	static constexpr float kCoreRingStartAngleDeg_ = -90.0f; // 配置開始角度（上から）
	static constexpr float kCoreZOffset_ = -13.0f;       // Z方向のオフセット
};