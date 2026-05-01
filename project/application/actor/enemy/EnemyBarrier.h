#pragma once
#include <memory>
#include <vector>
#include "Object3d.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "MyMath.h"
#include "BarrierCommon.h"
#include "BarrierConfig.h"

class Player;

//=============================================================
// EnemyBarrierクラス
// 雑魚敵フェーズ本隊を包むバリアの状態・描画・Player同期を管理する。
//=============================================================
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
	/// <param name="dt">経過時間（秒）</param>
	void Update(float dt);

	/// <summary>
	/// バリアを描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dxCommon);

	/// <summary>
	/// JSONから読み込んだバリア本体設定を適用します。
	/// </summary>
	/// <param name="config">バリア本体・破壊演出・シェーダー設定</param>
	void ApplyConfig(const BarrierConfig::Barrier& config);

	// Setter============================================
	/// <summary>
	/// 使用するカメラを設定します。
	/// </summary>
	/// <param name="camera">描画に使用するカメラ</param>
	void SetCamera(TKM::Camera* camera);

	/// <summary>
	/// Player への参照を設定します。
	/// バリア情報の同期や当たり判定連携に使用します。
	/// </summary>
	/// <param name="player">Player への参照</param>
	void SetPlayer(Player* player);

	/// <summary>
	/// バリアの表示状態を設定します。
	/// </summary>
	/// <param name="visible">true で表示、false で非表示</param>
	void SetVisible(bool visible) { visible_ = visible; }

	/// <summary>
	/// バリアの有効状態を設定します。
	/// 無効にすると描画も判定も行われなくなります。
	/// </summary>
	/// <param name="active">true で有効、false で無効</param>
	void SetActive(bool active);

	/// <summary>
	/// バリアの中心位置を設定します。
	/// 通常は EnemyManager 側で基準位置から更新されます。
	/// </summary>
	/// <param name="center">中心位置</param>
	void SetCenter(const Vector3& center);

	/// <summary>
	/// バリアの半径を設定します。
	/// </summary>
	/// <param name="radius">半径</param>
	void SetRadius(float radius);

	/// <summary>
	/// バリアの色を設定します。
	/// </summary>
	/// <param name="color">バリア色</param>
	void SetColor(const Vector4& color);

	/// <summary>
	/// バリアの形状スケールを設定します。
	/// 球体の見た目を変形させるために使用します。
	/// </summary>
	/// <param name="shapeScale">形状スケール</param>
	void SetShapeScale(const Vector3& shapeScale);
	// ==================================================

	// Getter============================================
	/// <summary>
	/// バリアの半径を取得します。
	/// </summary>
	/// <returns>バリア半径</returns>
	float GetRadius() const { return radius_; }

	/// <summary>
	/// バリアの中心位置を取得します。
	/// </summary>
	/// <returns>中心位置</returns>
	const Vector3& GetCenter() const { return center_; }

	/// <summary>
	/// バリアの AABB サイズを取得します。
	/// </summary>
	/// <returns>AABB サイズ</returns>
	Vector3 GetAABBSize() const;

	/// <summary>
	/// バリアの楕円体半径を取得します。
	/// </summary>
	/// <returns>楕円体半径</returns>
	Vector3 GetEllipsoidRadius() const;

	/// <summary>
	/// Player への参照を取得します。
	/// </summary>
	/// <returns>Player への参照</returns>
	Player* GetPlayer() const { return player_; }

	/// <summary>
	/// バリアの色を取得します。
	/// </summary>
	/// <returns>バリア色</returns>
	const Vector4& GetColor() const { return color_; }

	/// <summary>
	/// バリアの形状スケールを取得します。
	/// </summary>
	/// <returns>形状スケール</returns>
	const Vector3& GetShapeScale() const { return shapeScale_; }

	/// <summary>
	/// シェーダ用フレネル強度を取得します。
	/// </summary>
	/// <returns>フレネル強度</returns>
	float GetShaderFresnelPower() const { return shaderFresnelPower_; }

	/// <summary>
	/// シェーダ用ベース強度を取得します。
	/// </summary>
	/// <returns>ベース強度</returns>
	float GetShaderBaseStrength() const { return shaderBaseStrength_; }

	/// <summary>
	/// シェーダ用リム強度を取得します。
	/// </summary>
	/// <returns>リム強度</returns>
	float GetShaderRimStrength() const { return shaderRimStrength_; }

	/// <summary>
	/// シェーダ用ベースアルファを取得します。
	/// </summary>
	/// <returns>ベースアルファ</returns>
	float GetShaderAlphaBase() const { return shaderAlphaBase_; }

	/// <summary>
	/// シェーダ用リムアルファを取得します。
	/// </summary>
	/// <returns>リムアルファ</returns>
	float GetShaderAlphaRim() const { return shaderAlphaRim_; }

	/// <summary>
	/// シェーダ用色味を取得します。
	/// </summary>
	/// <returns>色味</returns>
	const Vector3& GetShaderTint() const { return shaderTint_; }
	// ==================================================

	/// <summary>
	/// バリアが有効かどうかを取得します。
	/// </summary>
	/// <returns>true なら有効、false なら無効</returns>
	bool IsActive() const { return active_; }

	/// <summary>
	/// バリアが表示されているかどうかを取得します。
	/// </summary>
	/// <returns>true なら表示、false なら非表示</returns>
	bool IsVisible() const { return visible_; }

	/// <summary>
	/// バリアの状態を Player に同期させます。
	/// </summary>
	void SyncToPlayer();

	/// <summary>
	/// バリアが攻撃にヒットしたときの処理を行います。
	/// </summary>
	/// <param name="pos">ヒット位置</param>
	void OnHit(const Vector3& pos);

	/// <summary>
	/// バリアの破壊を開始します。
	/// </summary>
	void StartBreak();

	/// <summary>
	/// バリアが破壊中かどうかを取得します。
	/// </summary>
	/// <returns>true なら破壊中、false なら通常状態</returns>
	bool IsBreaking() const { return isBreaking_; }

private:
	/// <summary>
	/// バリアの見た目を更新します。
	/// </summary>
	void UpdateVisual_();

	//======================================================================
	// 描画オブジェクト / 外部参照
	//======================================================================
	std::unique_ptr<TKM::Object3d> object_ = nullptr; // バリア描画用オブジェクト

	TKM::DirectXCommon* dxCommon_ = nullptr; // DirectX 共通管理クラス
	TKM::Camera* camera_ = nullptr;          // 描画に使用するカメラ
	Player* player_ = nullptr;               // 同期対象の Player
	TKM::BarrierCommon* barrierCommon_ = nullptr; // バリア共通描画情報

	//======================================================================
	// バリア基本状態
	//======================================================================
	bool active_ = false;  // バリアが有効かどうか
	bool visible_ = false; // バリアが表示されているかどうか

	Vector3 center_ = { 0.0f, 0.0f, 0.0f };    // バリア中心位置
	float radius_ = 18.0f;                     // バリア半径
	Vector3 shapeScale_ = { 1.0f, 1.0f, 1.0f }; // バリア形状スケール
	Vector4 color_ = { 0.0f, 0.0f, 0.0f, 1.0f }; // バリア色

	//======================================================================
	// ヒット演出
	//======================================================================
	float hitFlashTimer_ = 0.0f;              // ヒットフラッシュの残り時間
	Vector3 hitFlashPos_ = { 0.0f, 0.0f, 0.0f }; // ヒットフラッシュ位置

	//======================================================================
	// シェーダパラメータ
	//======================================================================
	float shaderFresnelPower_ = 2.2f;     // フレネル強度
	float shaderBaseStrength_ = 0.02f;    // ベース発光強度
	float shaderRimStrength_ = 1.0f;      // リム発光強度
	float shaderAlphaBase_ = 0.08f;       // ベースアルファ
	float shaderAlphaRim_ = 0.35f;        // リムアルファ
	Vector3 shaderTint_ = { 1.0f, 0.72f, 0.95f }; // シェーダ色味

	float shaderHexScale_ = 8.0f;         // 六角形模様のスケール
	float shaderHexLineWidth_ = 0.030f;   // 六角形ライン幅
	float shaderHexGlowStrength_ = 2.4f;  // 六角形発光強度
	float shaderHexAlpha_ = 0.85f;        // 六角形アルファ

	float shaderBreakProgress_ = 0.0f;    // 破壊進行度
	float shaderBreakEdgeWidth_ = 0.08f;  // 破壊境界幅
	float shaderBreakGlowStrength_ = 2.8f; // 破壊時発光強度
	float shaderBreakNoiseScale_ = 14.0f; // 破壊ノイズスケール
	Vector3 shaderBreakOrigin_ = { 0.0f, 0.0f, 0.0f }; // 破壊起点位置

	float collisionScaleZ_ = 1.0f; // Z方向の当たり判定補正

	//======================================================================
	// GPUリソース
	//======================================================================
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_; // マテリアル用GPUリソース
	Vector4* materialData_ = nullptr;                         // マテリアルデータ参照先

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_; // WVP行列用GPUリソース
	Matrix4x4* wvpData_ = nullptr;                      // WVP行列データ参照先

	/// <summary>
	/// バリアシェーダへ渡すパラメータ構造体
	/// </summary>
	struct BarrierShaderParam {
		float fresnelPower;      // フレネル強度
		float baseStrength;      // ベース発光強度
		float rimStrength;       // リム発光強度
		float alphaBase;         // ベースアルファ

		float alphaRim;          // リムアルファ
		float hexScale;          // 六角形模様スケール
		float hexLineWidth;      // 六角形ライン幅
		float hexGlowStrength;   // 六角形発光強度

		float hexAlpha;          // 六角形アルファ
		float breakProgress;     // 破壊進行度
		float breakEdgeWidth;    // 破壊境界幅
		float breakGlowStrength; // 破壊発光強度

		Vector3 tint;            // 色味
		float breakNoiseScale;   // 破壊ノイズスケール

		Vector3 breakOrigin;     // 破壊起点位置
		float padding1;          // パディング

		float hitFlashTime;      // ヒットフラッシュ時間
		Vector3 hitFlashPos;     // ヒットフラッシュ位置
	};

	Microsoft::WRL::ComPtr<ID3D12Resource> barrierShaderParamResource_;// シェーダパラメータ用GPUリソース
	BarrierShaderParam* barrierShaderParamData_ = nullptr;             // シェーダパラメータ参照先

	//======================================================================
	// 破壊状態
	//======================================================================
	bool isBreaking_ = false;      // バリアが破壊中かどうか
	float breakTimer_ = 0.0f;      // 破壊演出タイマー
	float breakDuration_ = 2.0f;   // 破壊演出時間
};