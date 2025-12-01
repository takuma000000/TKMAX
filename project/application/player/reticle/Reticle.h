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
#include "engine/func/math/Vector3.h"
#include "MyMath.h"
#include "engine/io/Input.h"
#include "WindowsAPI.h"
#include "engine/effect/line/LineRenderer.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

//============================================================
// 3D Reticle（パンツァードラグーン風・四層モデル）
//============================================================
class Reticle {
public:
	Reticle() = default;
	~Reticle() = default;

	/// <summary>
	/// レティクルの一層分
	/// </summary>
	/// <param name="common"></param>
	/// <param name="dx"></param>
	/// <param name="modelBig"></param>
	/// <param name="modelMid"></param>
	/// <param name="modelSmall"></param>
	/// <param name="modelFar"></param>
	void Initialize(
		Object3dCommon* common,
		DirectXCommon* dx,
		const char* modelBig = "reticle_big.obj",
		const char* modelMid = "reticle_normal.obj",
		const char* modelSmall = "reticle_small.obj",
		const char* modelFar = "reticle_small.obj" // 4枚目は small 流用
	)
	{
		common_ = common;
		dx_ = dx;

		auto initLayer = [&](Layer& L, const char* model) {
			L.obj = std::make_unique<Object3d>();
			L.obj->Initialize(common_, dx_);
			L.obj->SetModel(model);
			L.obj->SetScale(L.scale);
			if (cam_) L.obj->SetCamera(cam_);
			};

		initLayer(layers_[0], modelBig);   // 手前（最大）
		initLayer(layers_[1], modelMid);   // 2番目
		initLayer(layers_[2], modelSmall); // 3番目
		initLayer(layers_[3], modelFar);   // 一番奥
	}
	/// <summary>
	/// 毎フレーム更新
	/// </summary>
	/// <param name="dt"></param>
	void Update(float dt) {
		if (!visible_ || !getPos_ || !getYaw_) return;

		// 初回のみ
		if (!baseInitialized_) {
			basePos_ = getPos_();
			baseInitialized_ = true;
		}

		//--------------------------------------------------
		// 1) カメラのRight/Up取得
		//--------------------------------------------------
		Vector3 camRight = { 1,0,0 };
		Vector3 camUp = { 0,1,0 };
		if (cam_) {
			const auto& W = cam_->GetWorldMatrix();
			camRight = MyMath::Normalize({ W.m[0][0], W.m[0][1], W.m[0][2] });
			camUp = MyMath::Normalize({ W.m[1][0], W.m[1][1], W.m[1][2] });
		}

		//--------------------------------------------------
		// 2) スティック入力 → curX_/curY_ は「累積」
		//--------------------------------------------------
		bool hasStickInput = false;

		if (stickControl_) {
			auto* in = Input::GetInstance();

			float rx = static_cast<float>(in->GetLeftStickX());
			float ry = static_cast<float>(in->GetLeftStickY());

			// デッドゾーン
			const float dz = stickDeadZone_;
			if (std::fabs(rx) < dz) rx = 0; else rx = (rx > 0 ? rx - dz : rx + dz);
			if (std::fabs(ry) < dz) ry = 0; else ry = (ry > 0 ? ry - dz : ry + dz);

			// 正規化
			float norm = 32767.0f - dz;
			if (norm < 1.0f) norm = 1.0f;
			rx /= norm;
			ry /= norm;

			// 2乗カーブでスムーズ化
			float lx = rx * std::fabs(rx);
			float ly = ry * std::fabs(ry);

			hasStickInput = (std::fabs(lx) > 0.00001f || std::fabs(ly) > 0.00001f);

			// 累積
			curX_ += lx * stickMovePerSec_ * dt;
			curY_ += ly * stickMovePerSec_ * dt;

			// 範囲制限（画面サイズ基準）
			const float w = static_cast<float>(WindowsAPI::kClientWidth);
			const float h = static_cast<float>(WindowsAPI::kClientHeight);
			const float halfW = w * 0.5f;
			const float halfH = h * 0.5f;

			curX_ = std::clamp(curX_, -halfW, halfW);
			curY_ = std::clamp(curY_, -halfH, halfH);
		}

		//--------------------------------------------------
		// 3) 離した瞬間に内部リセット（カクつき防止）
		//--------------------------------------------------
		{
			static bool prev = false;
			if (!hasStickInput && prev) {
				basePos_ = getPos_();
				curX_ = 0.0f;
				curY_ = 0.0f;
			}
			prev = hasStickInput;
		}

		//--------------------------------------------------
		// 4) プレイヤーの向き ＋ スティックを含めた「射線方向」
		//--------------------------------------------------
		const float ownerYaw = getYaw_();
		const float yaw = ownerYaw + yawOffset_;

		// プレイヤーの純粋な前方（+Z 前提）
		Vector3 fwd = { std::sinf(yaw), 0.0f, std::cosf(yaw) };

		// スティック入力を少しだけ方向に混ぜて「狙っている方向」にする
		Vector3 aimDir = fwd;
		aimDir += camRight * (curX_ * 0.03f);   // 横
		aimDir += camUp * (curY_ * 0.03f);      // 縦

		if (MyMath::Length(aimDir) < 0.001f) {
			aimDir = fwd;
		}
		aimDir = MyMath::Normalize(aimDir);

		// 起点は毎フレームのプレイヤー位置
		Vector3 origin = getPos_();

		// ここで「最後の狙い線」を記録しておく
		lastOrigin_ = origin;
		lastAimDir_ = aimDir;
		hasAim_ = true;

		// 一番奥の狙い点（ここまで線を伸ばす）
		Vector3 aimPoint = origin + aimDir * maxDist;

		// ─────────────────────────────
		// レティクル用のガイドラインをデバッグ描画に登録
		// ─────────────────────────────
		LineRenderer::GetInstance()->AddLine( // デバッグ用ガイドライン
			origin,
			aimPoint,
			LineRenderer::Color{ 0.0f, 1.0f, 0.0f, 1.0f }  // 緑色
		);

		//--------------------------------------------------
		// 5) 線分 origin→aimPoint を割合で割って、4枚並べる
		//    手前ほどプレイヤー寄り・奥ほど遠く＆小さく
		//--------------------------------------------------
		float t[4] = {
			0.22f, // 手前
			0.36f,
			0.50f,
			0.64f // 奥
		};

		for (int i = 0; i < 4; ++i) {
			auto& L = layers_[i];
			if (!L.obj) continue;

			float ti = t[i];

			// 線形補間 origin + (aimPoint - origin) * t
			Vector3 pos = {
				origin.x + (aimPoint.x - origin.x) * ti,
				origin.y + (aimPoint.y - origin.y) * ti,
				origin.z + (aimPoint.z - origin.z) * ti
			};

			pos.y += up_;   // 全体の上下オフセット（ImGuiで弄れるやつ）

			L.obj->SetTranslate(pos);

			// 真ん中あたりを「中心」とみなして Player が追尾する
			// （GetCenterWorldPos() は layer[1] を返しているので、
			//   2番目のレイヤーが「ロック中心」になるイメージ）

			// 向きはプレイヤーのヨーに合わせる
			Vector3 rot = L.obj->GetRotate();
			if (alignToOwnerYaw_) {
				rot.y = yaw;
			}
			L.obj->SetRotate(rot);

			// 自己回転
			L.selfAngle += L.spinSpeed * dt;
			if (selfSpinAxisY_) {
				Vector3 r = L.obj->GetRotate();
				r.y += L.spinSpeed * dt;
				L.obj->SetRotate(r);
			} else {
				Vector3 r = L.obj->GetRotate();
				r.z = L.selfAngle;
				L.obj->SetRotate(r);
			}

			// スケール
			L.obj->SetScale(L.scale);

			L.obj->Update();
		}
	}
	/// <summary>
	/// 描画
	/// </summary>
	/// <param name="dx"></param>
	void Draw(DirectXCommon* dx) {
		if (!visible_) return;
		for (auto& L : layers_) {
			if (L.obj && L.visible) L.obj->Draw(dx);
		}
	}

	/// <summary>
	/// 所有者情報のバインド
	/// </summary>
	/// <param name="getWorldPos"></param>
	/// <param name="getYawRad"></param>
	void BindOwner(
		std::function<Vector3(void)> getWorldPos,
		std::function<float(void)>   getYawRad)
	{
		getPos_ = std::move(getWorldPos);
		getYaw_ = std::move(getYawRad);
	}
	/// <summary>
	/// カメラ設定
	/// </summary>
	/// <param name="cam"></param>
	void SetCamera(Camera* cam) {
		cam_ = cam;
		for (auto& L : layers_) {
			if (L.obj) L.obj->SetCamera(cam_);
		}
	}
	/// <summary>
	/// 最後に更新された狙いの起点座標を取得
	/// </summary>
	/// <returns></returns>
	Vector3 GetCenterWorldPos() const {
		// 今回は 2番目レイヤー(= index 1)を「中心」と扱う
		if (layers_[1].obj) return layers_[1].obj->GetTranslate();
		if (layers_[0].obj) return layers_[0].obj->GetTranslate();
		if (getPos_) return getPos_();
		return {};
	}
	/// <summary>
	/// 最後に更新された狙い方向ベクトルを取得
	/// </summary>
	/// <returns></returns>
	Vector3 GetAimDirection() const {
		if (hasAim_) {
			return lastAimDir_;
		}
		// まだ一度もUpdateされてないなどの場合の保険
		return Vector3{ 0.0f, 0.0f, 1.0f };
	}

#ifdef USE_IMGUI
	void ImGuiDebug() {
		if (ImGui::CollapsingHeader("Reticle 3D")) {
			ImGui::Checkbox("Visible", &visible_);
			ImGui::Checkbox("Align To Owner Yaw", &alignToOwnerYaw_);
			ImGui::Checkbox("Self Spin Axis = Y", &selfSpinAxisY_);
			ImGui::DragFloat("Up Offset", &up_, 0.01f, -20.0f, 20.0f);
			ImGui::DragFloat("Yaw Offset", &yawOffset_, 0.001f, -3.14f, 3.14f);

			for (int i = 0; i < 4; ++i) {
				auto& L = layers_[i];
				char name[32];
				sprintf_s(name, "Layer %d", i);
				if (ImGui::TreeNode(name)) {

					// ▼ 位置表示（読み取り専用）
					if (L.obj) {
						Vector3 pos = L.obj->GetTranslate();
						ImGui::Text("Pos: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
					} else {
						ImGui::Text("Pos: (---, ---, ---)");
					}

					ImGui::Checkbox("Visible", &L.visible);
					ImGui::DragFloat3("Scale", &L.scale.x, 0.01f, 0.01f, 10.f);
					ImGui::DragFloat("SpinSpeed", &L.spinSpeed, 0.01f, -20.f, 20.f);
					ImGui::TreePop();
				}
			}
		}
	}
#endif

private:
	//--------------------------------------------------
	// 各レイヤ
	//--------------------------------------------------
	struct Layer { // 上から順に引数
		std::unique_ptr<Object3d> obj; // 3Dオブジェクト本体
		Vector3 scale = { 1,1,1 }; // スケール
		float   spinSpeed = 0.0f; // 自己回転速度（ラジアン/秒）
		float   selfAngle = 0.0f; // 自己回転角度（ラジアン）
		bool    visible = true; // 表示/非表示
	};

	//--------------------------------------------------
	// 内部データ
	//--------------------------------------------------
	Object3dCommon* common_ = nullptr;
	DirectXCommon* dx_ = nullptr;
	Camera* cam_ = nullptr;

	std::function<Vector3(void)> getPos_;
	std::function<float(void)>   getYaw_;

	// 手前→奥の順に4層
	std::array<Layer, 4> layers_ = { // 第一引数: Object3dポインタ 第二引数: 前後位置（ImGui用） 第三引数: スケール　第四引数: 自己回転速度　第五引数: 自己回転角度　第六引数: 表示/非表示
		Layer{ nullptr,{1.80f, 1.80f, 1.80f},  2.5f, 0.0f, true }, // 0: 一番手前
		Layer{ nullptr,{1.55f, 1.55f, 1.55f}, -2.3f, 0.0f, true }, // 1
		Layer{ nullptr,{1.48f, 1.48f, 1.48f},  2.5f, 0.0f, true }, // 2
		Layer{ nullptr,{1.20f, 1.20f, 1.20f}, -2.3f, 0.0f, true }  // 3: 奥
	};

	// 全体設定
	bool  visible_ = true;
	bool  alignToOwnerYaw_ = true;
	bool  selfSpinAxisY_ = false;
	float up_ = 0.0f;
	float yawOffset_ = 0.0f;

	// 累積オフセット
	float curX_ = 0.0f;
	float curY_ = 0.0f;
	float stickMovePerSec_ = 20.0f;
	float stickDeadZone_ = 8000.0f;

	// 基準座標
	Vector3 basePos_ = { 0,0,0 };
	bool    baseInitialized_ = false;

	// 右スティック制御
	bool stickControl_ = true;

	Vector3 lastOrigin_ = { 0.0f, 0.0f, 0.0f };
	Vector3 lastAimDir_ = { 0.0f, 0.0f, 1.0f };
	bool    hasAim_ = false;
	float maxDist = 150.0f; // ラインをどこまで伸ばすか
};