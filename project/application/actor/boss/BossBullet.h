#pragma once
#include <memory>
#include <string>
#include "Object3d.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "MyMath.h"
#include "ParticleManager.h"
#include "LineRenderer.h"
#include "TrailRibbonRenderer.h"
#include <vector>
#include "AABB.h"

//=============================================================
// BossBulletクラス
// ボスの弾を管理するクラス。
//=============================================================
class BossBullet {
public:
	// 弾のデフォルトスケール
	enum class FxType {
		MissileEvil, // ミサイル（bossEvil_*）
		SlashWave,   // 斬撃（bossSlash_*）
	};

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
	);
	/// <summary>
	/// 弾を更新します。
	/// </summary>
	void Update();
	/// <summary>
	/// 弾オブジェクトを描画します。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dx);
	/// <summary>
	/// 弾の軌跡エフェクトを描画します。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void DrawTrail(TKM::DirectXCommon* dx);

	/// <summary>
	/// ターゲットへの曲線移動を有効にします。
	/// </summary>
	/// <param name="start">開始位置（ワールド座標）</param>
	/// <param name="end">終了位置（ターゲット位置、ワールド座標）</param>
	/// <param name="curveHeight">曲線の高さ（Y方向オフセット量）</param>
	/// <param name="speedPerFrame">1フレームあたりの移動速度</param>
	void EnableCurveToTarget(const Vector3& start, const Vector3& end, float curveHeight, float speedPerFrame);
	/// <summary>
	/// 斬撃エフェクトの当たり判定を行います。
	/// </summary>
	/// <param name="targetCenter"></param>
	/// <param name="targetSize"></param>
	/// <returns></returns>
	bool HitTestSlashX(const Vector3& targetCenter, const Vector3& targetSize) const;
	/// <summary>
	/// この弾が死亡しているかを返します。
	/// </summary>
	/// <returns></returns>
	bool IsDead() const { return dead_; }
	/// <summary>
	/// 弾を強制的に死亡状態にします。
	/// </summary>
	void Kill();
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
	/// <summary>
	/// ターゲットへの曲線移動を、制御点オフセット指定で有効にします。
	/// </summary>
	/// <param name="start">開始位置（ワールド座標）</param>
	/// <param name="target">終了位置（ターゲット位置、ワールド座標）</param>
	/// <param name="controlOffset">制御点のオフセット（開始位置からの相対座標）</param>
	/// <param name="speed">1フレームあたりの移動速度</param>
	void EnableCurveToTargetWithControlOffset(const Vector3& start, const Vector3& target, const Vector3& controlOffset, float speed);

	// Getter===================================
	/// <summary>
	/// 弾の位置を返します。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetPos() const { return obj_->GetTranslate(); }
	/// <summary>
	/// 弾のエフェクトタイプを返します。
	/// </summary>
	/// <returns></returns>
	FxType GetFxType() const { return fxType_; }
	/// <summary>
	/// 斬撃の攻撃IDを返します。
	/// </summary>
	/// <returns></returns>
	int GetAttackId() const { return attackId_; }
	// =========================================
	// Setter===================================
	/// <summary>
	/// 使用するカメラを設定します。
	/// </summary>
	/// <param name="cam">描画に使用するカメラ</param>
	void SetCamera(TKM::Camera* cam);
	/// <summary>
	/// 弾道カーブのヨー回転量を設定します。
	/// </summary>
	/// <param name="yawRadPerFrame">1フレームあたりのヨー回転量（ラジアン）</param>
	void SetCurveYaw(float yawRadPerFrame);
	/// <summary>
	/// モデルを設定します。
	/// </summary>
	/// <param name="model">使用するモデルファイル名</param>
	void SetModel(const std::string& model);
	/// <summary>
	/// スケールを設定します。
	/// </summary>
	/// <param name="s">設定するスケール値</param>
	void SetScale(const Vector3& s);
	/// <summary>
	/// エフェクトタイプを設定します。
	/// </summary>
	/// <param name="t"></param>
	void SetFxType(FxType t);
	/// <summary>
	/// 斬撃の攻撃IDを設定します。
	/// </summary>
	/// <param name="id">設定する攻撃ID</param>
	void SetAttackId(int id);
	// =========================================
private:
	//======================================================================
	// 参照
	//======================================================================
	TKM::Camera* cam_ = nullptr; // 描画に使用するカメラ（nullptr 可）
	//======================================================================
	// 本体データ
	//======================================================================
	std::unique_ptr<TKM::Object3d> obj_; // モデル本体
	//======================================================================
	// 移動・状態
	//======================================================================
	Vector3 dir_{ 0,0,-1 };   // 移動方向
	float   speed_ = 0.8f;    // 移動速度
	int     damage_;      // 与えるダメージ
	int     life_ = 180;      // 寿命フレーム
	bool    dead_ = false;          // 完全削除フラグ（Managerが消す用）
	bool    bodyHidden_ = false;    // 本体非表示フラグ
	bool    isTrailFading_ = false; // トレイル後処理中フラグ
	//======================================================================
	// 定数（マジックナンバー解消）
	//======================================================================
	static constexpr float kDefaultScale_ = 0.6f;  // 見た目の大きさ
	//======================================================================
	// 曲線移動用
	//======================================================================
	bool   useCurve_ = false; // 曲線移動有効フラグ
	float  curveYawRad_ = 0.0f; // 毎フレームのY回転量（ラジアン）
	int    curveTotalFrames_ = 0; // 曲線到達までの総フレーム数
	int    curveFrame_ = 0; // 現在の曲線フレーム数
	int    postCurveLifeFrames_ = 45; // 曲線到達後の生存フレーム数
	Vector3 curveStart_{ 0.0f,0.0f,0.0f }; // 曲線開始位置
	Vector3 curveEnd_{ 0.0f,0.0f,0.0f }; // 曲線終了位置
	Vector3 curveMid_{ 0.0f,0.0f,0.0f }; // 曲線中間位置
	Vector3 curveCtrl_{ 0.0f,0.0f,0.0f }; // 曲線制御点位置
	Vector3 velocity_{ 0.0f, 0.0f, 0.0f }; // 曲線移動時の速度ベクトル
	int fxFrame_ = 0; // 通常弾エフェクト用フレームカウンタ
	FxType fxType_ = FxType::MissileEvil; // エフェクトタイプ
	int ageFrame_ = 0; // 経過フレーム数
	static constexpr int kSlashHitActiveFrames_ = 18; // 斬撃の判定が生きるフレーム
	int attackId_ = 0; // 斬撃の攻撃ID（連続ヒット防止用）
	Vector3 curveControlOffset_{ 0.0f, 0.0f, 0.0f }; // 曲線制御点のオフセット（開始位置からの相対座標）
	//======================================================================
	// 軌跡エフェクト用
	//======================================================================
	std::vector<Vector3> trailPts_{}; // 軌跡ポイントのワールド座標リスト
	Vector3 prevPos_{ 0.0f, 0.0f, 0.0f }; // 前フレームの位置（軌跡生成用）
	float trailDistAcc_ = 0.0f; // 軌跡生成の距離蓄積値
	// 軌跡エフェクトのパラメータ
	static constexpr float kTrailStep_ = 0.55f; // 軌跡ポイント生成の距離ステップ
	static constexpr size_t kTrailHardCap_ = 18; // 軌跡ポイントの最大数（古いポイントから削除していく）
	// 軌跡の見た目パラメータ
	float trailHeadWidth_ = 2.55f; // 軌跡の頭の幅
	float trailTailWidth_ = 1.35f; // 軌跡の尻尾の幅
	float trailIntensity_ = 100.0f; // 軌跡の明るさ
	Vector3 trailColor_ = { 1.0f, 0.0f, 1.0f }; // 軌跡の色（RGB、0〜1の範囲）
	float trailUvTiling_ = 1.15f; // 軌跡のUVタイルリング（テクスチャの繰り返し回数）
	float trailUvScroll_ = 2.2f; // 軌跡のUVスクロール速度
	/// <summary>
	/// 軌跡エフェクト用のポイントを更新します。弾の移動距離が一定以上になるたびにポイントを追加し、古いポイントは削除していきます。
	/// </summary>
	/// <param name="p">弾の現在位置</param>
	void UpdateTrail(const Vector3& p);
};