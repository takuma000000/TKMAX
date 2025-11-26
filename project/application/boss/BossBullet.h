#pragma once
#include <memory>
#include "Object3d.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "engine/func/math/Vector3.h"

//=============================================================
// BossBulletクラス
// ボスの弾を管理するクラス。
//=============================================================
class BossBullet {
public:

	/// <summary>ボス弾を初期化します。</summary>
	/// <param name="common">Object3d共通。</param>
	/// <param name="dx">DirectX共通。</param>
	/// <param name="cam">カメラ。</param>
	/// <param name="pos">初期位置。</param>
	/// <param name="dir">進行方向（正規化推奨）。</param>
	/// <param name="speed">速度。</param>
	/// <param name="damage">与ダメージ。</param>
	/// <param name="lifeFrame">寿命フレーム。</param>
	void Initialize(Object3dCommon* common, DirectXCommon* dx, Camera* cam,
		const Vector3& pos, const Vector3& dir,
		float speed, int damage, int lifeFrame) {
		obj_ = std::make_unique<Object3d>();
		obj_->Initialize(common, dx);
		obj_->SetModel("sphere.obj");              // モデル指定
		obj_->SetScale({ kDefaultScale, kDefaultScale, kDefaultScale }); // スケール
		obj_->SetTranslate(pos);
		if (cam) obj_->SetCamera(cam);

		dir_ = dir;
		speed_ = speed;
		damage_ = damage;
		life_ = lifeFrame;
	}

	/// <summary>弾を更新します（移動・寿命判定）。</summary>
	void Update() {
		if (dead_) return;
		Vector3 p = obj_->GetTranslate();
		p.x += dir_.x * speed_;
		p.y += dir_.y * speed_;
		p.z += dir_.z * speed_;
		obj_->SetTranslate(p);
		obj_->Update();
		if (--life_ <= 0) dead_ = true;
	}

	/// <summary>弾を描画します。</summary>
	/// <param name="dx">DirectX共通。</param>
	void Draw(DirectXCommon* dx) {
		if (!dead_) obj_->Draw(dx);
	}

	/// <summary>弾が消滅済みかを返します。</summary>
	/// <returns>消滅なら true。</returns>
	bool IsDead() const { return dead_; }

	/// <summary>この弾のダメージ量を返します。</summary>
	int  Damage()  const { return damage_; }

	/// <summary>現在位置を返します。</summary>
	const Vector3& GetPos() const { return obj_->GetTranslate(); }

	/// <summary>当たり半径を返します。</summary>
	float Radius() const { return kDefaultScale; } // 簡易当たり半径

private:
	std::unique_ptr<Object3d> obj_;
	Vector3 dir_{ 0,0,-1 };
	float   speed_ = 0.8f;
	int     damage_ = 1;
	int     life_ = 180;
	bool    dead_ = false;

	// マジックナンバー解消用定数
	static constexpr float kDefaultScale = 0.6f;  // 見た目の大きさ
};