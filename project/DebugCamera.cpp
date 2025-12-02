#include "DebugCamera.h"
#include "Input.h"
#include "WindowsAPI.h"
#include <cmath>
#include <algorithm>

void DebugCamera::Initialize(const Vector3& pos, const Vector3& target) {
	pos_ = pos;

	// pos → target の向きから初期 yaw/pitch を求める
	Vector3 toTarget = target - pos;
	float lenXZ = std::sqrtf(toTarget.x * toTarget.x + toTarget.z * toTarget.z);

	if (lenXZ > 0.0001f) { // XZ平面への射影ベクトルの長さが十分に大きいなら
		yaw_ = std::atan2f(toTarget.x, toTarget.z);
		pitch_ = std::atan2f(toTarget.y, lenXZ);
	}

	SetTranslate(pos_); // カメラ位置セット
	SetRotate({ pitch_, yaw_, 0.0f }); // カメラ回転セット
	Camera::Update();

	// マウス初期位置
	HWND hwnd = GetActiveWindow();
	GetCursorPos(&prevMouse_); // スクリーン座標系
	ScreenToClient(hwnd, &prevMouse_); // クライアント座標系に変換
}

void DebugCamera::Update() {
	Input* input = Input::GetInstance();

	// ===================== マウス移動量 =====================
	HWND hwnd = GetActiveWindow();
	POINT cur{};
	GetCursorPos(&cur);          // スクリーン座標系
	ScreenToClient(hwnd, &cur);  // クライアント座標系に変換

	if (firstMouse_) {
		prevMouse_ = cur;
		firstMouse_ = false;
	}

	float dx = float(cur.x - prevMouse_.x);
	float dy = float(cur.y - prevMouse_.y);
	prevMouse_ = cur;

	// ===================== 向きベクトル計算 =====================
	Vector3 forward;
	forward.x = std::sinf(yaw_) * std::cosf(pitch_);
	forward.y = std::sinf(pitch_);
	forward.z = std::cosf(yaw_) * std::cosf(pitch_);

	Vector3 right = { forward.z, 0.0f, -forward.x };
	Vector3 up = { 0.0f, 1.0f, 0.0f };

	// ===================== マウスホイールで前後ズーム =====================
	{
		int wheel = input->GetWheel();  // -1, 0, 1 の想定
		if (wheel != 0) {
			const float wheelZoomSpeed = 0.3f;
			pos_ = pos_ + forward * (wheel * wheelZoomSpeed);

			// ここで一度使ったのでリセット（1フレーム分だけ有効）
			input->SetWheel(0);
		}
	}

	// ===================== 右ドラッグで回転 =====================
	if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
		const float mouseRotateSpeed = 0.003f;
		yaw_ += dx * mouseRotateSpeed;
		pitch_ += dy * mouseRotateSpeed;

		const float limit = 1.55f;
		pitch_ = std::clamp(pitch_, -limit, limit);
	}

	// ===================== 左ドラッグで座標移動（パン） =====================
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
		const float panSpeed = 0.02f;
		pos_ = pos_
			+ right * (-dx * panSpeed)
			+ up * (dy * panSpeed);
	}

	// ===================== カメラ更新 =====================
	SetTranslate(pos_);
	SetRotate({ pitch_, yaw_, 0.0f });
	Camera::Update();
}