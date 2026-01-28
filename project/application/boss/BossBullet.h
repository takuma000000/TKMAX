#pragma once
#include <memory>
#include "Object3d.h"
#include "DirectXCommon.h"
#include "camera/Camera.h"
#include "Vector3.h"

//=============================================================
// BossBulletクラス
// ボスの弾を管理するクラス。
//=============================================================
class BossBullet {
public:

	/// <summary>
/// 弾オブジェクトを初期化します。
/// </summary>
/// <param name="common">Object3d の共通管理クラス</param>
/// <param name="dx">DirectX 共通管理クラス</param>
/// <param name="cam">描画および判定に使用するカメラ（nullptr 可）</param>
/// <param name="pos">弾の初期位置（ワールド座標）</param>
/// <param name="dir">弾の進行方向（正規化ベクトル）</param>
/// <param name="speed">弾の移動速度</param>
/// <param name="damage">ヒット時に与えるダメージ量</param>
/// <param name="lifeFrame">弾が消滅するまでの生存フレーム数</param>
	void Initialize(
		TKM::Object3dCommon* common,
		TKM::DirectXCommon* dx,
		TKM::Camera* cam,
		const Vector3& pos,
		const Vector3& dir,
		float speed,
		int damage,
		int lifeFrame
	) {
		obj_ = std::make_unique<TKM::Object3d>();
		obj_->Initialize(common, dx);
		obj_->SetModel("sphere.obj"); // モデル指定
		obj_->SetScale({ kDefaultScale_, kDefaultScale_, kDefaultScale_ }); // スケール
		obj_->SetTranslate(pos);
		if (cam) obj_->SetCamera(cam);

		dir_ = dir;
		speed_ = speed;
		damage_ = damage;
		life_ = lifeFrame;
	}
	/// <summary>
	/// 弾を更新します。
	/// </summary>
	void Update() {
		if (dead_) return;
		Vector3 p_ = obj_->GetTranslate();
		p_.x += dir_.x * speed_;
		p_.y += dir_.y * speed_;
		p_.z += dir_.z * speed_;
		obj_->SetTranslate(p_);
		obj_->Update();
		if (--life_ <= 0) dead_ = true;
	}
	/// <summary>
	/// 弾オブジェクトを描画します。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dx) {
		if (!dead_) obj_->Draw(dx);
	}

	/// <summary>
	/// この弾が死亡しているかを返します。
	/// </summary>
	/// <returns></returns>
	bool IsDead() const { return dead_; }
	/// <summary>
	/// ダメージ値を返します。
	/// </summary>
	/// <returns></returns>
	int  Damage()  const { return damage_; }
	/// <summary>
	/// 簡易当たり判定半径を返します。
	/// </summary>
	/// <returns></returns>
	float Radius() const { return kDefaultScale_; } // 簡易当たり半径

	// Getter===================================
	/// <summary>
	/// 弾の位置を返します。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetPos() const { return obj_->GetTranslate(); }
	// =========================================
	// Setter===================================
	/// <summary>
	/// 使用するカメラを設定します。
	/// </summary>
	/// <param name="cam">描画に使用するカメラ</param>
	void SetCamera(TKM::Camera* cam) {
		if (obj_) { obj_->SetCamera(cam); }
	}
	// =========================================
private:
	//======================================================================
	// 参照
	//======================================================================
	TKM::Camera* cam_ = nullptr;
	//======================================================================
	// 本体データ
	//======================================================================
	std::unique_ptr<TKM::Object3d> obj_; // モデル本体
	//======================================================================
	// 移動・状態
	//======================================================================
	Vector3 dir_{ 0,0,-1 };   // 移動方向
	float   speed_ = 0.8f;    // 移動速度
	int     damage_ = 1;      // 与えるダメージ
	int     life_ = 180;      // 寿命フレーム
	bool    dead_ = false;    // 死亡フラグ（消去判定に使用）
	//======================================================================
	// 定数（マジックナンバー解消）
	//======================================================================
	static constexpr float kDefaultScale_ = 0.6f;  // 見た目の大きさ
};