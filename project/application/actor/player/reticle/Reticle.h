#pragma once
#define NOMINMAX
#include <memory>
#include <functional>
#include <array>
#include <string>
#include <algorithm>
#include <cmath>

#include "Object3d.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "Vector3.h"
#include "MyMath.h"
#include "Input.h"
#include "WindowsAPI.h"
#include "LineRenderer.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

//=============================================================
// Reticleクラス
// プレイヤーの狙いを示すレティクルを管理するクラス。
//=============================================================
class Reticle {
public:
	Reticle() = default;
	~Reticle() = default;

	/// <summary>
	/// レティクルの一層分を初期化します。
	/// </summary>
	/// <param name="common">Object3d の共通管理クラス</param>
	/// <param name="dx">DirectX 共通管理クラス</param>
	/// <param name="modelBig">近距離用のレティクルモデル</param>
	/// <param name="modelMid">中距離用のレティクルモデル</param>
	/// <param name="modelSmall">遠距離用のレティクルモデル</param>
	/// <param name="modelFar">最遠距離用のレティクルモデル（small の流用）</param>
	void Initialize(
		TKM::Object3dCommon* common,
		TKM::DirectXCommon* dx,
		const char* modelBig = "reticle_big.obj",
		const char* modelMid = "reticle_normal.obj",
		const char* modelSmall = "reticle_small.obj",
		const char* modelFar = "reticle_small.obj" // 4枚目は small 流用
	);
	/// <summary>
	/// 毎フレームの更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void Update(float dt);
	/// <summary>
	/// レティクルを描画します。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dx);
	/// <summary>
	/// ImGuiデバッグ表示
	/// </summary>
	void ImGuiDebug();

	/// <summary>
	/// 所有者（追従対象）の情報をバインドします。
	/// </summary>
	/// <param name="getWorldPos">所有者のワールド位置を取得する関数</param>
	/// <param name="getYawRad">所有者のヨー角（ラジアン）を取得する関数</param>
	void BindOwner(
		std::function<Vector3(void)> getWorldPos,
		std::function<float(void)>   getYawRad
	);

	// Getter========================================
	/// <summary>
	/// 最後に更新された狙い方向ベクトルを取得
	/// </summary>
	/// <returns></returns>
	Vector3 GetAimDirection() const;
	/// <summary>
	/// 最後に更新された狙いの起点座標を取得（キャッシュ版）
	/// </summary>
	/// <returns></returns>
	Vector3 GetCenterWorldPos() const;
	// ==============================================
	// Setter========================================
	/// <summary>
	/// 使用するカメラを設定します。
	/// レティクルの全レイヤーに同じカメラを適用します。
	/// </summary>
	/// <param name="cam">描画に使用するカメラ</param>
	void SetCamera(TKM::Camera* cam);
	/// <summary>
	/// レティクル入力を有効/無効にします。
	/// false のときも見た目更新やカメラ反映は行います。
	/// </summary>
	void SetInputEnabled(bool enabled) { inputEnabled_ = enabled; }	
	/// <summary>
	/// レティクルの移動可能範囲を設定します。
	/// </summary>
	void SetMoveRange(const Vector3& minPos, const Vector3& maxPos) {
		moveMin_ = minPos;
		moveMax_ = maxPos;
	}
	// ==============================================
private:
	//--------------------------------------------------
	// 各レイヤ
	//--------------------------------------------------
	struct Layer { // 上から順に引数
		std::unique_ptr<TKM::Object3d> obj; // 3Dオブジェクト本体
		Vector3 scale_ = { 1,1,1 };     // スケール
		float   spinSpeed_ = 0.0f;      // 自己回転速度（ラジアン/秒）
		float   selfAngle_ = 0.0f;      // 自己回転角度（ラジアン）
		bool    visible_ = true;      // 表示/非表示
	};
	//--------------------------------------------------
	// 内部データ（共通）
	//--------------------------------------------------
	TKM::Object3dCommon* common_ = nullptr; // 3Dオブジェクト共通管理クラス
	TKM::DirectXCommon* dx_ = nullptr; // DirectX共通管理クラス
	TKM::Camera* cam_ = nullptr; // 描画に使用するカメラ
	std::function<Vector3(void)> getPos_; // 所有者のワールド位置を取得する関数
	std::function<float(void)>   getYaw_; // 所有者のヨー角（ラジアン）を取得する関数
	//--------------------------------------------------
	// レイヤ構成（手前 → 奥）
	//--------------------------------------------------
	// 手前→奥の順に4層
	std::array<Layer, 4> layers_ = { // 第一引数: Object3dポインタ 第二引数: 前後位置（ImGui用） 第三引数: スケール　第四引数: 自己回転速度　第五引数: 自己回転角度　第六引数: 表示/非表示
		Layer{ nullptr,{1.80f, 1.80f, 1.80f},  2.5f, 0.0f, true }, // 0: 一番手前
		Layer{ nullptr,{1.55f, 1.55f, 1.55f}, -2.3f, 0.0f, true }, // 1
		Layer{ nullptr,{1.48f, 1.48f, 1.48f},  2.5f, 0.0f, true }, // 2
		Layer{ nullptr,{1.20f, 1.20f, 1.20f}, -2.3f, 0.0f, true }  // 3: 奥
	};
	// 全体設定
	bool  visible_ = true; // レティクル全体の表示/非表示
	bool  alignToOwnerYaw_ = true; // 所有者のヨー角に合わせるかどうか
	bool  selfSpinAxisY_ = false; // 自己回転の軸をYにするか（falseならZ軸回転）
	float up_ = 0.0f; // 全体の上方向オフセット
	float yawOffset_ = 0.0f; // 全体のヨー角オフセット（ラジアン）
	//--------------------------------------------------
	// スティック入力によるオフセット
	//--------------------------------------------------
	// 累積オフセット
	float curX_ = 0.0f; // スティック入力をオフセットに変換するための係数
	float curY_ = 0.0f; // スティック入力をオフセットに変換するための係数
	float stickMovePerSec_ = 50.0f;  // スティックで動かす速度
	float stickDeadZone_ = 8000.0f; // スティックのデッドゾーン（この値以下の入力は無視）
	bool stickControl_ = true; // 右スティックでレティクルを動かすかどうか
	bool inputEnabled_ = true; // 入力だけを受け付けるか
	Vector3 moveMin_ = { -100.0f, -20.0f, 0.0f }; // レティクル移動範囲の最小
	Vector3 moveMax_ = { 100.0f,  20.0f, 0.0f }; // レティクル移動範囲の最大
	//--------------------------------------------------
	// エイム / レイ情報
	//--------------------------------------------------
	Vector3 lastOrigin_ = { 0.0f, 0.0f, 0.0f }; // 最後に更新された狙いの起点座標（ワールド）
	Vector3 lastAimDir_ = { 0.0f, 0.0f, 1.0f }; // 最後に更新された狙い方向ベクトル（ワールド・正規化済み）
	bool    hasAim_ = false; // エイム情報が有効かどうか
	float   maxDist = 150.0f; // ラインをどこまで伸ばすか
	//--------------------------------------------------
	// レティクル中心座標
	//--------------------------------------------------
	// レティクルの中心ワールド座標
	Vector3 center_ = { 0,0,0 }; // キャッシュしておく（計算コストが高いので）
	bool    centerInitialized_ = false; // center_ が有効かどうか
};