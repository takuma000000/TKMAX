#include "Reticle.h"

void Reticle::Initialize(
	TKM::Object3dCommon* common,
	TKM::DirectXCommon* dx,
	const char* modelBig,
	const char* modelMid,
	const char* modelSmall,
	const char* modelFar // 4枚目は small 流用
) {
	common_ = common;
	dx_ = dx;

	auto initLayer = [&](Layer& L, const char* model) { // レイヤー初期化の共通処理
		L.obj = std::make_unique<TKM::Object3d>();
		L.obj->Initialize(common_, dx_);
		L.obj->SetModel(model);
		L.obj->SetScale(L.scale_);
		if (cam_) L.obj->SetCamera(cam_);
		};
	// レイヤーごとにモデルをセットして初期化
	initLayer(layers_[0], modelBig);   // 手前（最大）
	initLayer(layers_[1], modelMid);   // 2番目
	initLayer(layers_[2], modelSmall); // 3番目
	initLayer(layers_[3], modelFar);   // 一番奥
}

void Reticle::Update(float dt) {
	if (!visible_ || !getPos_ || !getYaw_) return; // 位置とヨー角が取れないなら更新しない
	// オーナー（プレイヤー）の位置
	Vector3 ownerPos = getPos_();

	//--------------------------------------------------
	// 0) 初回だけ「プレイヤーの少し前」に中心を作る
	//--------------------------------------------------
	if (!centerInitialized_) { // 最初はプレイヤーの前にレティクルを置いておく
		float yaw = getYaw_() + yawOffset_; // プレイヤーのヨーにオフセットを加えた方向を向く
		Vector3 fwd = { std::sinf(yaw), 0.0f, std::cosf(yaw) }; // 前方向ベクトル
		center_ = ownerPos + fwd * 40.0f; // プレイヤーの前方40の位置にレティクルの中心を置く（初期位置）
		centerInitialized_ = true; // 以降はスティックで動かすので初期化はここだけでいい
	}
	//--------------------------------------------------
	// 1) カメラの Right / Up を取る
	//--------------------------------------------------
	Vector3 camRight = { 1,0,0 }; // カメラの右方向（初期値はワールドの右方向）
	Vector3 camUp = { 0,1,0 }; // カメラの上方向（初期値はワールドの上方向）
	if (cam_) {
		const auto& W = cam_->GetWorldMatrix(); // ワールド行列の向きから Right と Up を抜き取る
		camRight = MyMath::Normalize({ W.m[0][0], W.m[0][1], W.m[0][2] }); // カメラの右方向
		camUp = MyMath::Normalize({ W.m[1][0], W.m[1][1], W.m[1][2] }); // カメラの上方向
	}
	//--------------------------------------------------
	// 2) 左スティックで center_ を直接動かす
	//--------------------------------------------------
	if (stickControl_) {
		auto* in = TKM::Input::GetInstance();
		// スティックの値を -32768 〜 32767 の範囲で取得
		float rx = static_cast<float>(in->GetLeftStickX()); // 左スティックのX軸
		float ry = static_cast<float>(in->GetLeftStickY()); // 左スティックのY軸（通常は前が -32768、後ろが 32767）

		const float dz = stickDeadZone_; // 0〜32767 想定のデッドゾーン
		// デッドゾーン内はゼロ、外はデッドゾーン分を引いて正規化（-1〜1の範囲に）
		if (std::fabs(rx) < dz) rx = 0; else rx = (rx > 0 ? rx - dz : rx + dz);
		if (std::fabs(ry) < dz) ry = 0; else ry = (ry > 0 ? ry - dz : ry + dz);

		float norm = 32767.0f - dz; // デッドゾーンを引いた後の最大値で割って正規化
		if (norm < 1.0f) norm = 1.0f; // 念のためゼロ割り防止
		rx /= norm; // ry は通常、前が -32768、後ろが 32767 なので符号を反転しておく（前が正になるように）
		ry /= norm; // ry は通常、前が -32768、後ろが 32767 なので符号を反転しておく（前が正になるように）

		// 2乗カーブでスティック端だけ強く
		float lx = rx * std::fabs(rx); // スティックのX軸（左右）を -1〜1 の範囲で取得（デッドゾーン処理済み）
		float ly = ry * std::fabs(ry); // スティックのY軸（前後）を -1〜1 の範囲で取得（デッドゾーン処理済み）

		// 入力があるときだけ動かす（離したら center_ はその場で完全停止）
		if (std::fabs(lx) > 0.00001f || std::fabs(ly) > 0.00001f) {
			const float moveSpeed = stickMovePerSec_; // 既存の速度パラメータを流用
			center_ += camRight * (lx * moveSpeed * dt) // カメラの向きに応じて左右と上下に動かす
				+ camUp * (ly * moveSpeed * dt); // カメラの向きに応じて左右と上下に動かす
		}
	}
	//--------------------------------------------------
	// 3) プレイヤー → レティクルへの方向ベクトル
	//--------------------------------------------------
	Vector3 origin = ownerPos; // レティクルの「狙う中心」の位置
	Vector3 dir = center_ - origin; // プレイヤーからレティクルへの方向ベクトル
	if (MyMath::Length(dir) < 0.001f) { // ほぼ同じ位置ならゼロベクトルになってしまうので、適当な前方向ベクトルを向いておく
		// ほぼ同じ位置なら「前方向き」にしておく
		float yaw = getYaw_() + yawOffset_;
		dir = { std::sinf(yaw), 0.0f, std::cosf(yaw) }; // プレイヤーのヨーにオフセットを加えた方向を向く
	}
	dir = MyMath::Normalize(dir); // 正規化して向きベクトルにする

	lastOrigin_ = origin; // キャッシュしておく
	lastAimDir_ = dir; // キャッシュしておく
	hasAim_ = true; // これ以降は GetAimDirection() で有効な値が返るようになる

	float dist = maxDist; // プレイヤーからレティクルまでの距離（狙う距離）。必要に応じて調整してOK
	Vector3 aimPoint = origin + dir * dist; // プレイヤーからレティクルまでの狙う先端の位置

	// ガイドライン（プレイヤー→先端）
	TKM::LineRenderer::GetInstance()->AddLine(
		origin, aimPoint,
		TKM::LineRenderer::Color{ 0.0f, 1.0f, 0.0f, 1.0f } // 緑色
	);
	//--------------------------------------------------
	// 4) 4層レティクルの配置
	//--------------------------------------------------
	// center_ を「2層目の位置」として、その前後に並べるイメージ
	float nearOffset = 8.0f; // 1層目と3層目の前後位置のオフセット（中心からの距離）
	float farOffset = 20.0f; // 4層目の前後位置のオフセット（中心からの距離）。必要に応じて調整してOK

	for (int i = 0; i < 4; ++i) { // 4層ループ
		auto& L = layers_[i]; // レイヤー参照
		if (!L.obj) continue; // オブジェクトがないレイヤーはスキップ
		if (!L.visible_) continue; // 非表示のレイヤーはスキップ

		Vector3 pos; // レイヤーの位置
		switch (i) {
		case 0: // 手前
			pos = center_ - dir * nearOffset; // 1層目は中心からプレイヤー側に少しオフセット
			break;
		case 1: // 中央（ロック中心）
			pos = center_; // 2層目は center_ そのまま
			break;
		case 2: // 少し奥
			pos = center_ + dir * nearOffset; // 3層目は中心から奥側に少しオフセット
			break;
		case 3: // いちばん奥
			pos = center_ + dir * farOffset; // 4層目は中心からさらに奥側にオフセット
			break;
		}

		pos.y += up_; // 全体を上下にずらす量

		L.obj->SetTranslate(pos); // 位置をセット

		// 向きはプレイヤーのヨーに合わせる
		Vector3 rot = L.obj->GetRotate();
		if (alignToOwnerYaw_) { // オーナーのヨーに合わせるオプションがあるときはヨーだけ合わせる
			float yaw = getYaw_() + yawOffset_; // プレイヤーのヨーにオフセットを加えた角度を向く
			rot.y = yaw; // ヨーをセット
		}
		L.obj->SetRotate(rot); // 向きをセット

		// 自己回転
		L.selfAngle_ += L.spinSpeed_ * dt; // 自己回転角度を更新
		if (selfSpinAxisY_) { // 自己回転の軸がYのときは、ヨーに加算して回す
			Vector3 r = L.obj->GetRotate(); // 現在の回転を取得
			r.y += L.spinSpeed_ * dt; // ヨーに自己回転分を加算
			L.obj->SetRotate(r); // 回転をセット
		} else { // 自己回転の軸がY以外のときは、ロール（Z軸回転）にして回す
			Vector3 r = L.obj->GetRotate(); // 現在の回転を取得
			r.z = L.selfAngle_; // ロールに自己回転角度をセット
			L.obj->SetRotate(r); // 回転をセット
		}
		
		L.obj->SetScale(L.scale_); // スケールをセット（毎フレーム同じ値をセットしてもOK）
		L.obj->Update(); // 更新してワールド行列を計算
	}
}

void Reticle::Draw(TKM::DirectXCommon* dx) {
	if (!visible_) return;
	for (auto& L : layers_) { // 4層ループ
		if (L.obj && L.visible_) L.obj->Draw(dx); // オブジェクトがあって表示のときだけ描画
	}
}

void Reticle::BindOwner(
	std::function<Vector3(void)> getWorldPos,
	std::function<float(void)>   getYawRad
) {
	getPos_ = std::move(getWorldPos); // 所有者のワールド位置を取得する関数をセット
	getYaw_ = std::move(getYawRad); // 所有者のヨー角（ラジアン）を取得する関数をセット
}

Vector3 Reticle::GetAimDirection() const {
	if (hasAim_) {
		return lastAimDir_; // 最後に更新された狙い方向ベクトルを返す
	}
	// まだ一度もUpdateされてないなどの場合の保険
	return Vector3{ 0.0f, 0.0f, 1.0f };
}

Vector3 Reticle::GetCenterWorldPos() const {
	return center_; // 最後に更新された狙いの起点座標を返す（キャッシュ版）
}

void Reticle::SetCamera(TKM::Camera* cam) {
	cam_ = cam; // カメラをセット
	for (auto& L : layers_) {
		if (L.obj) L.obj->SetCamera(cam_); // レイヤーのオブジェクトがあればカメラをセット
	}
}

#ifdef USE_IMGUI
/// <summary>
/// ImGuiデバッグ表示
/// </summary>
void Reticle::ImGuiDebug() {
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

				ImGui::Checkbox("Visible", &L.visible_);
				ImGui::DragFloat3("Scale", &L.scale_.x, 0.01f, 0.01f, 10.f);
				ImGui::DragFloat("SpinSpeed", &L.spinSpeed_, 0.01f, -20.f, 20.f);
				ImGui::TreePop();
			}
		}
	}
}
#endif