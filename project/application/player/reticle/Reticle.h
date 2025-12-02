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
#include "engine/3d/camera/Camera.h"
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

		// オーナー（プレイヤー）の位置
		Vector3 ownerPos = getPos_();

		//--------------------------------------------------
		// 0) 初回だけ「プレイヤーの少し前」に中心を作る
		//--------------------------------------------------
		if (!centerInitialized_) {
			float yaw = getYaw_() + yawOffset_;
			Vector3 fwd = { std::sinf(yaw), 0.0f, std::cosf(yaw) };
			center_ = ownerPos + fwd * 40.0f; // 好きな距離にしてOK
			centerInitialized_ = true;
		}
		//--------------------------------------------------
		// 1) カメラの Right / Up を取る
		//--------------------------------------------------
		Vector3 camRight = { 1,0,0 };
		Vector3 camUp = { 0,1,0 };
		if (cam_) {
			const auto& W = cam_->GetWorldMatrix();
			camRight = MyMath::Normalize({ W.m[0][0], W.m[0][1], W.m[0][2] });
			camUp = MyMath::Normalize({ W.m[1][0], W.m[1][1], W.m[1][2] });
		}
		//--------------------------------------------------
		// 2) 左スティックで center_ を直接動かす
		//--------------------------------------------------
		if (stickControl_) {
			auto* in = Input::GetInstance();

			float rx = static_cast<float>(in->GetLeftStickX());
			float ry = static_cast<float>(in->GetLeftStickY());

			const float dz = stickDeadZone_; // 0〜32767 想定のデッドゾーン
			if (std::fabs(rx) < dz) rx = 0; else rx = (rx > 0 ? rx - dz : rx + dz);
			if (std::fabs(ry) < dz) ry = 0; else ry = (ry > 0 ? ry - dz : ry + dz);

			float norm = 32767.0f - dz;
			if (norm < 1.0f) norm = 1.0f;
			rx /= norm;
			ry /= norm;

			// 2乗カーブでスティック端だけ強く
			float lx = rx * std::fabs(rx);
			float ly = ry * std::fabs(ry);

			// 入力があるときだけ動かす（離したら center_ はその場で完全停止）
			if (std::fabs(lx) > 0.00001f || std::fabs(ly) > 0.00001f) {
				const float moveSpeed = stickMovePerSec_; // 既存の速度パラメータを流用
				center_ += camRight * (lx * moveSpeed * dt)
					+ camUp * (ly * moveSpeed * dt);
			}
		}
		//--------------------------------------------------
		// 3) プレイヤー → レティクルへの方向ベクトル
		//--------------------------------------------------
		Vector3 origin = ownerPos;
		Vector3 dir = center_ - origin;
		if (MyMath::Length(dir) < 0.001f) {
			// ほぼ同じ位置なら「前方向き」にしておく
			float yaw = getYaw_() + yawOffset_;
			dir = { std::sinf(yaw), 0.0f, std::cosf(yaw) };
		}
		dir = MyMath::Normalize(dir);

		lastOrigin_ = origin;
		lastAimDir_ = dir;
		hasAim_ = true;

		float dist = maxDist;
		Vector3 aimPoint = origin + dir * dist;

		// ガイドライン（プレイヤー→先端）
		LineRenderer::GetInstance()->AddLine(
			origin, aimPoint,
			LineRenderer::Color{ 0.0f, 1.0f, 0.0f, 1.0f }
		);
		//--------------------------------------------------
		// 4) 4層レティクルの配置
		//--------------------------------------------------
		// center_ を「2層目の位置」として、その前後に並べるイメージ
		float nearOffset = 8.0f;
		float farOffset = 20.0f;

		for (int i = 0; i < 4; ++i) {
			auto& L = layers_[i];
			if (!L.obj) continue;
			if (!L.visible) continue;

			Vector3 pos;
			switch (i) {
			case 0: // 手前
				pos = center_ - dir * nearOffset;
				break;
			case 1: // 中央（ロック中心）
				pos = center_;
				break;
			case 2: // 少し奥
				pos = center_ + dir * nearOffset;
				break;
			case 3: // いちばん奥
				pos = center_ + dir * farOffset;
				break;
			}

			pos.y += up_; // 全体を上下にずらす量

			L.obj->SetTranslate(pos);

			// 向きはプレイヤーのヨーに合わせる
			Vector3 rot = L.obj->GetRotate();
			if (alignToOwnerYaw_) {
				float yaw = getYaw_() + yawOffset_;
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

	/// <summary>
	/// 最後に更新された狙いの起点座標を取得（キャッシュ版）
	/// </summary>
	/// <returns></returns>
	Vector3 GetCenterWorldPos() const {
		return center_;
	}

#ifdef USE_IMGUI
	void ImGuiDebug() {
		if (ImGui::CollapsingHeader("レティクル")) {
			ImGui::Checkbox("Visible", &visible_);
			ImGui::Checkbox("Align To Owner Yaw", &alignToOwnerYaw_);
			ImGui::Checkbox("Self Spin Axis = Y", &selfSpinAxisY_);
			ImGui::DragFloat("Up Offset", &up_, 0.01f, -20.0f, 20.0f);
			ImGui::DragFloat("Yaw Offset", &yawOffset_, 0.001f, -3.14f, 3.14f);

			// ─────────── 線で区切り（操作系パラメータ）───────────
			ImGui::Separator();
			ImGui::Text("操作パラメータ");
			ImGui::DragFloat(
				"感度",
				&stickMovePerSec_,
				10.0f,        // 1ステップの変化量
				20.0f,       // 最小
				500.0f       // 最大（必要ならもっと上げてもOK）
			);

			// ─────────── 線で区切り（各レイヤー設定）───────────
			ImGui::Separator();
			for (int i = 0; i < 4; ++i) {
				auto& L = layers_[i];
				char name[32];
				sprintf_s(name, "レイヤー %d", i);
				if (ImGui::TreeNode(name)) {

					// 位置表示（読み取り専用）
					if (L.obj) {
						Vector3 pos = L.obj->GetTranslate();
						ImGui::Text("位置: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
					} else {
						ImGui::Text("位置: (---, ---, ---)");
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
	float stickMovePerSec_ = 50.0f; // スティックで動かす速度
	float stickDeadZone_ = 8000.0f;

	// 右スティック制御
	bool stickControl_ = true;

	Vector3 lastOrigin_ = { 0.0f, 0.0f, 0.0f };
	Vector3 lastAimDir_ = { 0.0f, 0.0f, 1.0f };
	bool    hasAim_ = false;
	float maxDist = 150.0f; // ラインをどこまで伸ばすか

	// レティクルの中心ワールド座標
	Vector3 center_ = { 0,0,0 };
	bool centerInitialized_ = false;
};