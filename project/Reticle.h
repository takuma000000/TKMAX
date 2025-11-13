#pragma once
#define NOMINMAX
#include <memory>
#include <functional>
#include <array>
#include <string>
#include "Object3d.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "engine/func/math/Vector3.h"
#include "MyMath.h"
#include "engine/io/Input.h"
#include <algorithm>
#include "WindowsAPI.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// =============================================================
// 3D Reticle クラス（パンツァードラグーン風・多層回転）
//  - Player（等）から「世界位置」と「ヨー角[rad]」をコールバックで受け取る
//  - +Z を前方とする前提で、前方距離を層ごとにズラして配置
//  - 各層は個別に回転速度/スケール/可視を持つ
//  - ImGuiでリアルタイム調整可（#define USE_IMGUI が必要）
// =============================================================
class Reticle {
public:
	Reticle() = default;
	~Reticle() = default;

	// ---------- API ----------
	// 初期化：モデル名は3層分を渡す。省略時は *_big/normal/small を使用
	void Initialize(Object3dCommon* common, DirectXCommon* dx,
		const char* modelBig = "reticle_big.obj",
		const char* modelNorm = "reticle_normal.obj",
		const char* modelSmall = "reticle_small.obj")
	{
		common_ = common; dx_ = dx;

		auto initLayer = [&](Layer& L, const char* model) {
			L.obj = std::make_unique<Object3d>();
			L.obj->Initialize(common_, dx_);
			L.obj->SetModel(model);
			L.obj->SetScale(L.scale);
			if (cam_) L.obj->SetCamera(cam_);
			};

		initLayer(layers_[0], modelBig);
		initLayer(layers_[1], modelNorm);
		initLayer(layers_[2], modelSmall);
	}

	// 毎フレ更新
	void Update(float dt) {
		if (!visible_ || !getPos_ || !getYaw_) return;

		// 自機（またはオーナー）の基準
		const Vector3 base = getPos_();
		const float ownerYaw = getYaw_();

		// +Z を前方としたヨー回転の前方
		const float yaw = ownerYaw + yawOffset_;
		const Vector3 fwd = { std::sinf(yaw), 0.0f, std::cosf(yaw) };

		// --- 1) カメラのRight/Upベクトル（正規化）を一度だけ求める ---
		Vector3 camRight = { 1,0,0 }, camUp = { 0,1,0 };
		if (cam_) {
			const auto& W = cam_->GetWorldMatrix();
			camRight = MyMath::Normalize({ W.m[0][0], W.m[0][1], W.m[0][2] });
			camUp = MyMath::Normalize({ W.m[1][0], W.m[1][1], W.m[1][2] });
		}

		// --- 2) 左スティック入力 → 累積オフセット更新 ---
		if (stickControl_) {
			auto* in = Input::GetInstance();

			float rx = static_cast<float>(in->GetLeftStickX());
			float ry = static_cast<float>(in->GetLeftStickY());

			// デッドゾーン
			const float dz = stickDeadZone_;
			if (std::fabs(rx) < dz) rx = 0; else rx = (rx > 0 ? rx - dz : rx + dz);
			if (std::fabs(ry) < dz) ry = 0; else ry = (ry > 0 ? ry - dz : ry + dz);

			float norm = 32767.0f - dz;
			if (norm < 1.0f) norm = 1.0f;
			rx /= norm;
			ry /= norm;

			// 累積（速度 = 倒し量 * 距離/秒）
			curX_ += rx * stickMovePerSec_ * dt;
			curY_ += ry * stickMovePerSec_ * dt;

			// --- ★画面外に出ないようにクランプ（画面全体を範囲に） ---
			const float w = static_cast<float>(WindowsAPI::kClientWidth);
			const float h = static_cast<float>(WindowsAPI::kClientHeight);
			const float halfW = w * 0.5f;
			const float halfH = h * 0.5f;
			curX_ = std::clamp(curX_, -halfW, halfW);
			curY_ = std::clamp(curY_, -halfH, halfH);
		}

		// --- 3) 各レイヤに反映 ---
		for (auto& L : layers_) {
			if (!L.obj) continue;

			// 前方＋右/上オフセット
			const float forward = (invertForward_ ? -L.forward : L.forward);
			Vector3 pos = base + fwd * forward;
			pos = pos + camRight * curX_ + camUp * curY_;  // ★ここを累積値で
			pos.y += up_;
			L.obj->SetTranslate(pos);

			// 向き：プレイヤーのヨーに合わせるか
			Vector3 rot = L.obj->GetRotate();
			rot.y = alignToOwnerYaw_ ? yaw : rot.y;
			L.obj->SetRotate(rot);

			// 自己回転（Z or Y）
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

			// スケール反映
			L.obj->SetScale(L.scale);

			L.obj->Update();
		}
	}

	// 描画
	void Draw(DirectXCommon* dx) {
		if (!visible_) return;
		for (auto& L : layers_) {
			if (L.obj && L.visible) L.obj->Draw(dx);
		}
	}

	// 参照元（Playerなど）から位置とヨー角をもらう
	void BindOwner(std::function<Vector3(void)> getWorldPos,
		std::function<float(void)>   getYawRad)
	{
		getPos_ = std::move(getWorldPos);
		getYaw_ = std::move(getYawRad);
	}

	// カメラを反映
	void SetCamera(Camera* cam) {
		cam_ = cam;
		for (auto& L : layers_) if (L.obj) L.obj->SetCamera(cam_);
	}

	// 前方距離セット（3層まとめて）
	void SetDepths(float big, float normal, float mini) {
		layers_[0].forward = big;
		layers_[1].forward = normal;
		layers_[2].forward = mini;
	}

	// 回転速度セット（rad/s、+で反時計回り）3層まとめて
	void SetSpin(float big, float normal, float mini) {
		layers_[0].spinSpeed = big;
		layers_[1].spinSpeed = normal;
		layers_[2].spinSpeed = mini;
	}

	// 全体の有効/無効
	void SetVisible(bool v) { visible_ = v; }

	// 中心（Normalレイヤー）のワールド座標を返す
	Vector3 GetCenterWorldPos() const {
		// 通常レイヤーがあればそれを使う
		if (layers_[1].obj) {
			return layers_[1].obj->GetTranslate();
		}
		// 念のためBigレイヤーでもフォールバック
		if (layers_[0].obj) {
			return layers_[0].obj->GetTranslate();
		}
		// それも無ければオーナー位置を返す
		if (getPos_) {
			return getPos_();
		}
		return Vector3{};
	}

#ifdef USE_IMGUI
	void ImGuiDebug() {
		if (ImGui::CollapsingHeader("Reticle 3D (Panzer style)")) {
			ImGui::Checkbox("Visible", &visible_);
			ImGui::Checkbox("Align To Owner Yaw", &alignToOwnerYaw_);
			ImGui::Checkbox("Invert Forward (+Z/-Z)", &invertForward_);
			ImGui::Checkbox("Self Spin Axis = Y (else Z)", &selfSpinAxisY_);
			ImGui::DragFloat("Up Offset", &up_, 0.01f, -20.0f, 20.0f);
			ImGui::DragFloat("Yaw Offset (rad)", &yawOffset_, 0.001f, -3.14159f, 3.14159f);

			if (ImGui::Button("Preset: Panzer-ish")) {
				// ユーザー指定の好み（例）：距離と回転を逆回転で段差
				layers_[0].forward = 50.0f;  layers_[0].spinSpeed = 1.6f;
				layers_[1].forward = 56.0f;  layers_[1].spinSpeed = -1.0f;
				layers_[2].forward = 59.5f;  layers_[2].spinSpeed = 2.2f;
				layers_[0].scale = { 1.10f,1.10f,1.10f };
				layers_[1].scale = { 1.00f,1.00f,1.00f };
				layers_[2].scale = { 0.90f,0.90f,0.90f };
			}
			ImGui::SameLine();
			if (ImGui::Button("Preset: Tight")) {
				layers_[0].forward = 35.0f; layers_[1].forward = 40.0f; layers_[2].forward = 44.0f;
				layers_[0].spinSpeed = 1.2f; layers_[1].spinSpeed = -0.8f; layers_[2].spinSpeed = 1.8f;
			}

			// 各レイヤ
			static const char* names[3] = { "Big", "Normal", "Small" };
			for (int i = 0; i < 3; ++i) {
				if (ImGui::TreeNode(names[i])) {
					ImGui::Checkbox("Visible", &layers_[i].visible);
					ImGui::DragFloat("Forward", &layers_[i].forward, 0.1f, -500.0f, 500.0f);
					ImGui::DragFloat3("Scale", &layers_[i].scale.x, 0.01f, 0.01f, 20.0f);
					ImGui::DragFloat("Spin Speed (rad/s)", &layers_[i].spinSpeed, 0.01f, -20.0f, 20.0f);
					ImGui::TreePop();
				}
			}
		}
	}
#endif

private:
	struct Layer {
		std::unique_ptr<Object3d> obj;

		// 配置
		float   forward = 0.0f;                 // 前方距離（+Z想定）
		Vector3 scale = Vector3(1.0f, 1.0f, 1.0f);

		// 回転
		float   spinSpeed = 0.0f;               // 自己回転速度（rad/s）
		float   selfAngle = 0.0f;               // 自己回転角（Z回転時に使用）

		// 表示
		bool    visible = true;
	};

	Object3dCommon* common_ = nullptr;
	DirectXCommon* dx_ = nullptr;
	Camera* cam_ = nullptr;

	// 呼び出し元からもらう情報
	std::function<Vector3(void)> getPos_;
	std::function<float(void)>   getYaw_;

	// 3層
	std::array<Layer, 3> layers_ = {
	/// 引数 : 第一引数=nullptr（後で初期化）, 第二引数=前方距離, 第三引数=スケール, 第四引数=回転速度(rad/s), 第五引数=初期角度, 第六引数=表示
	Layer{nullptr, 20.0f, {1.6f,1.6f,1.6f},  1.6f, 0.0f, true},
	Layer{nullptr, 26.0f, {1.5f,1.5f,1.5f}, -1.0f, 0.0f, true},
	Layer{nullptr, 29.5f, {1.7f,1.7f,1.7f},  2.2f, 0.0f, true}
	};

	// 全体パラメータ
	bool   visible_ = true;
	bool   alignToOwnerYaw_ = true;    // レティクルのY回転を自機ヨーに合わせる
	bool   invertForward_ = false;   // +Z/-Zの反転（座標系の食い違いに対応）
	bool   selfSpinAxisY_ = false;   // 自己回転軸：true=Y, false=Z
	float  up_ = 0.0f;    // 上下オフセット
	float  yawOffset_ = 0.0f;    // 自機ヨーに加算する微調整

	// --- Right Stick control ---
	bool  stickControl_ = true;     // 右スティックで動かす ON/OFF
	float stickDeadZone_ = 8000.0f; // デッドゾーン（XInputの生値）
	float stickSensitivity_ = 0.000040f; // 感度（正規化後に掛ける係数）
	float stickMaxOffset_ = 8.0f;   // オフセット最大距離（ワールド単位）

	// --- Right Stick accumulate mode ---
	float curX_ = 0.0f;             // 累積オフセット（右/左, ワールド距離）
	float curY_ = 0.0f;             // 累積オフセット（上/下, ワールド距離）
	float stickMovePerSec_ = 20.0f; // スティック全倒しで1秒間に動く距離（ワールド単位）
	float stickFriction_ = 0.0f;    // 0なら戻らない。>0で徐々に中央へ（/sec）

	bool  screenClamp_ = true;        // 画面クランプON/OFF
	float cursorXpx_ = -1.0f;       // 画面上のカーソルX(px) 初回に中央へ初期化
	float cursorYpx_ = -1.0f;       // 画面上のカーソルY(px)
	float movePxPerSec_ = 1000.0f;     // 全倒しでの移動速度（px/sec）
	float fovYRad_ = 60.0f * 3.14159265f / 180.0f; // 垂直FOV（rad）※ImGuiで調整可
};
