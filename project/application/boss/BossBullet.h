#pragma once
#include <memory>
#include "Object3d.h"
#include "DirectXCommon.h"
#include "camera/Camera.h"
#include "MyMath.h"
#include "ParticleManager.h"

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

		Vector3 newPos_{};
		bool reachedCurveEnd_ = false;

		if (useCurve_) {
			// t: 0→1
			++curveFrame_;
			float t_ = (curveTotalFrames_ > 0) ? (float)curveFrame_ / (float)curveTotalFrames_ : 1.0f;
			if (t_ > 1.0f) t_ = 1.0f;

			// 2次ベジェ
			const float u_ = 1.0f - t_;
			newPos_.x = (u_ * u_) * curveStart_.x + 2.0f * u_ * t_ * curveCtrl_.x + (t_ * t_) * curveEnd_.x;
			newPos_.y = (u_ * u_) * curveStart_.y + 2.0f * u_ * t_ * curveCtrl_.y + (t_ * t_) * curveEnd_.y;
			newPos_.z = (u_ * u_) * curveStart_.z + 2.0f * u_ * t_ * curveCtrl_.z + (t_ * t_) * curveEnd_.z;

			if (t_ >= 1.0f) {
				reachedCurveEnd_ = true;
			}
		} else {
			// 直進
			Vector3 p_ = obj_->GetTranslate();
			newPos_.x = p_.x + dir_.x * speed_;
			newPos_.y = p_.y + dir_.y * speed_;
			newPos_.z = p_.z + dir_.z * speed_;
		}

		// まず位置更新（どのモードでもここを通る）
		obj_->SetTranslate(newPos_);
		obj_->Update();

		// ==========================
		// Particle FX（ここに置けば発射時から必ず出る）
		// ==========================
		{
			auto* pm_ = TKM::ParticleManager::GetInstance();
			if (pm_) {
				Vector3 fxPos_ = newPos_;

				pm_->Emit("bossEvil_core", fxPos_, 1);

				if ((fxFrame_ % 2) == 0) {
					pm_->Emit("bossEvil_smoke", fxPos_, 2);
				}
				if ((fxFrame_ % 4) == 0) {
					pm_->Emit("bossEvil_spark", fxPos_, 2);
				}
				if ((fxFrame_ % 6) == 0) {
					pm_->Emit("bossEvil_ring", fxPos_, 1);
				}
				pm_->Emit("bossEvil_trail", fxPos_, 2);
			}
			++fxFrame_;
		}

		// 曲線→直進へ切り替え
		if (useCurve_ && reachedCurveEnd_) {
			// 終点の接線方向（end - ctrl）
			Vector3 dirT_{};
			dirT_.x = curveEnd_.x - curveCtrl_.x;
			dirT_.y = curveEnd_.y - curveCtrl_.y;
			dirT_.z = curveEnd_.z - curveCtrl_.z;

			const float len_ = std::sqrt(dirT_.x * dirT_.x + dirT_.y * dirT_.y + dirT_.z * dirT_.z);
			if (len_ > 0.0001f) {
				dirT_.x /= len_;
				dirT_.y /= len_;
				dirT_.z /= len_;
			} else {
				dirT_ = { 0.0f, 0.0f, 1.0f };
			}

			dir_ = dirT_;
			useCurve_ = false;

			// 通り過ぎ寿命
			life_ = 45;
			return;
		}

		// 寿命（直進中のみ減らす）
		if (!useCurve_) {
			if (--life_ <= 0) {
				dead_ = true;
			}
		}
	}
	/// <summary>
	/// 弾オブジェクトを描画します。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dx) {
		if (!dead_) obj_->Draw(dx);
	}

	/// <summary>
	/// ターゲットへの曲線移動を有効にします。
	/// </summary>
	/// <param name="start">開始位置（ワールド座標）</param>
	/// <param name="end">終了位置（ターゲット位置、ワールド座標）</param>
	/// <param name="curveHeight">曲線の高さ（Y方向オフセット量）</param>
	/// <param name="speedPerFrame">1フレームあたりの移動速度</param>
	void EnableCurveToTarget(const Vector3& start, const Vector3& end, float curveHeight, float speedPerFrame) {
		useCurve_ = true;
		curveStart_ = start;
		curveEnd_ = end;

		// 距離
		const Vector3 d_{ end.x - start.x, end.y - start.y, end.z - start.z };
		const float dist_ = std::sqrt(d_.x * d_.x + d_.y * d_.y + d_.z * d_.z);

		speed_ = speedPerFrame;
		const float sp_ = (speedPerFrame > 0.0001f) ? speedPerFrame : 0.0001f;
		curveTotalFrames_ = (int)std::ceil(dist_ / sp_);
		if (curveTotalFrames_ < 1) curveTotalFrames_ = 1;
		curveFrame_ = 0;

		// 中点
		curveMid_ = { (start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f, (start.z + end.z) * 0.5f };

		// 進行方向（XZ）
		Vector3 dirXZ_{ d_.x, 0.0f, d_.z };
		const float lenXZ_ = std::sqrt(dirXZ_.x * dirXZ_.x + dirXZ_.z * dirXZ_.z);
		if (lenXZ_ > 0.0001f) {
			dirXZ_.x /= lenXZ_;
			dirXZ_.z /= lenXZ_;
		} else {
			dirXZ_ = { 0.0f, 0.0f, 1.0f };
		}

		// 左右方向（up × dir）
		Vector3 perp_{ -dirXZ_.z, 0.0f, dirXZ_.x };

		// curveYawRad_ を「曲がり量」に変換（総回転量っぽく使う）
		const float totalYaw_ = curveYawRad_ * (float)curveTotalFrames_;

		// 0〜1に丸めた強さ（±は左右）
		float s_ = totalYaw_ / (3.14159265f * 0.5f); // 90度で1
		if (s_ > 1.0f) s_ = 1.0f;
		if (s_ < -1.0f) s_ = -1.0f;

		// 横ズレ量：距離に比例（見た目で分かるように）
		const float sideOffset_ = dist_ * 0.35f * s_; // ここ好みで 0.2〜0.6

		// 制御点：中点 + 横ズレ + 高さ
		curveCtrl_ = curveMid_;
		curveCtrl_.x += perp_.x * sideOffset_;
		curveCtrl_.z += perp_.z * sideOffset_;
		curveCtrl_.y += curveHeight;

		// 寿命調整
		if (life_ < curveTotalFrames_) {
			life_ = curveTotalFrames_; // 最低でも到達までは生かす
		}
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
	/// <summary>
	/// 弾道カーブのヨー回転量を設定します。
	/// </summary>
	/// <param name="yawRadPerFrame">1フレームあたりのヨー回転量（ラジアン）</param>
	void SetCurveYaw(float yawRadPerFrame) { curveYawRad_ = yawRadPerFrame; }
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
	// ======================================================================
	// 曲線移動用
	// ======================================================================
	float curveYawRad_ = 0.0f; // 毎フレームのY回転量（ラジアン）
	bool useCurve_ = false; // 曲線移動有効フラグ
	Vector3 curveStart_{ 0.0f,0.0f,0.0f }; // 曲線開始位置
	Vector3 curveEnd_{ 0.0f,0.0f,0.0f }; // 曲線終了位置
	Vector3 curveMid_{ 0.0f,0.0f,0.0f }; // 曲線中間位置
	Vector3 curveCtrl_{ 0.0f,0.0f,0.0f }; // 曲線制御点位置
	int curveTotalFrames_ = 0; // 曲線到達までの総フレーム数
	int curveFrame_ = 0; // 現在の曲線フレーム数
	Vector3 velocity_{ 0.0f, 0.0f, 0.0f }; // 曲線移動時の速度ベクトル
	int postCurveLifeFrames_ = 45; // 曲線到達後の生存フレーム数
	int fxFrame_ = 0; // 通常弾エフェクト用フレームカウンタ
};