#pragma once
#include <memory>
#include <vector>
#include "Object3d.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "MyMath.h"

class Player;

// =============================================================
// EnemyBarrierクラス
// Wave1本隊を包むバリアの状態・描画器・Player同期を管理する。
// =============================================================
class EnemyBarrier {
public:
	EnemyBarrier() = default;
	~EnemyBarrier() = default;

	/// <summary>
	/// バリアを初期化します。
	/// </summary>
	/// <param name="common">Object3d の共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// バリアを更新します。
	/// </summary>
	void Update();
	/// <summary>
	/// バリアを描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dxCommon);

	// Setter============================================
	/// <summary>
	/// カメラの設定。
	/// </summary>
	/// <param name="camera"></param>
	void SetCamera(TKM::Camera* camera);
	/// <summary>
	/// Playerの設定。バリアはPlayerの位置に追従するため、Playerへの参照を保持します。
	/// </summary>
	/// <param name="player"></param>
	void SetPlayer(Player* player);
	/// <summary>
	/// バリアの表示設定。表示状態は内部で管理され、描画処理で反映されます。
	/// </summary>
	/// <param name="visible"></param>
	void SetVisible(bool visible) { visible_ = visible; }
	/// <summary>
	/// バリアの有効設定。無効にすると描画も判定も行われなくなります。
	/// </summary>
	/// <param name="active"></param>
	void SetActive(bool active);
	/// <summary>
	/// バリアの中心位置の設定。通常はPlayerの位置に追従させるため、Playerの座標を渡すことが想定されます。
	/// </summary>
	/// <param name="center"></param>
	void SetCenter(const Vector3& center);
	/// <summary>
	/// バリアの半径の設定。バリアは球体として扱われ、半径を設定することで大きさを調整できます。
	/// </summary>
	/// <param name="radius"></param>
	void SetRadius(float radius);
	// ==================================================
	// Getter============================================
	/// <summary>
	/// バリアの中心位置の取得。通常はPlayerの位置に追従させるため、Playerの座標を返すことが想定されます。
	/// </summary>
	/// <returns></returns>
	float GetRadius() const { return radius_; }
	/// <summary>
	/// バリアの半径の取得。バリアは球体として扱われ、半径を取得することで大きさを知ることができます。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetCenter() const { return center_; }
	/// <summary>
	/// バリアのAABBサイズの取得。バリアは球体として扱われるため、AABBサイズは半径の2倍になります。
	/// </summary>
	/// <returns></returns>
	Vector3 GetAABBSize() const { return { radius_ * 2.0f, radius_ * 2.0f, radius_ * 2.0f }; }
	/// <summary>
	/// バリアの色の取得。描画処理で使用される色を取得します。
	/// </summary>
	/// <returns></returns>
	Player* GetPlayer() const { return player_; }
	// ==================================================

	/// <summary>
	/// バリアが有効かどうかを取得します。有効な場合、描画と判定が行われます。無効な場合、描画も判定も行われません。
	/// </summary>
	/// <returns></returns>
	bool IsActive() const { return active_; }
	/// <summary>
	/// バリアが表示されているかどうかを取得します。表示されている場合、描画処理でバリアが描かれます。非表示の場合、描画処理でバリアは描かれませんが、判定は行われる状態になります。
	/// </summary>
	/// <returns></returns>
	bool IsVisible() const { return visible_; }

	/// <summary>
	/// バリアの状態をPlayerの位置に同期させます。通常はPlayerの座標をバリアの中心位置に設定することで、バリアがPlayerを包むようにします。
	/// </summary>
	void SyncToPlayer();

private:
	/// <summary>
	/// バリアの見た目を更新します。通常はバリアの中心位置や半径、色などを反映させるために、Object3d のワールド行列やマテリアルパラメータを更新する処理が含まれます。
	/// </summary>
	void UpdateVisual_();

	std::unique_ptr<TKM::Object3d> object_ = nullptr;

	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::Camera* camera_ = nullptr;
	Player* player_ = nullptr;

	bool active_ = false;
	bool visible_ = false;

	Vector3 center_ = { 0.0f, 0.0f, 0.0f };
	float radius_ = 18.0f;

	Vector4 color_ = { 0.2f, 0.8f, 1.0f, 0.35f };

	float time_ = 0.0f;

	float dt_ = 0.016f; // 仮のフレーム時間（秒）。実際のゲームループでは、前フレームからの経過時間を計算して使用することが想定されます。
};