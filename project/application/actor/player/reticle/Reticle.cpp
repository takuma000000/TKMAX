#include "Reticle.h"

void Reticle::Initialize(
	TKM::Object3dCommon* common,
	TKM::DirectXCommon* dx,
	const char* modelBig,
	const char* modelMid,
	const char* modelSmall,
	const char* modelFar // 4枚目は small 流用
) {
	// 共通描画情報への参照を保持する
	common_ = common;

	// DirectX共通への参照を保持する
	dx_ = dx;

	// 1レイヤー分の初期化をまとめて行う共通ラムダ
	auto initLayer = [&](Layer& L, const char* model) {

		// レイヤー用の3Dオブジェクトを生成する
		L.obj = std::make_unique<TKM::Object3d>();

		// 描画に必要な情報を渡して初期化する
		L.obj->Initialize(common_, dx_);

		// レイヤーごとのモデルを設定する
		L.obj->SetModel(model);

		// 初期スケールを反映する
		L.obj->SetScale(L.scale_);

		// カメラが設定済みならそのまま適用する
		if (cam_) L.obj->SetCamera(cam_);
		};

	// 手前の一番大きいレイヤーを初期化する
	initLayer(layers_[0], modelBig);

	// 2枚目レイヤーを初期化する
	initLayer(layers_[1], modelMid);

	// 3枚目レイヤーを初期化する
	initLayer(layers_[2], modelSmall);

	// 一番奥の4枚目レイヤーを初期化する
	initLayer(layers_[3], modelFar);
}

void Reticle::Update(float dt) {
	// レティクル非表示、または所有者の位置/向き取得関数が無いなら更新しない
	if (!visible_ || !getPos_ || !getYaw_) return;

	// 所有者（プレイヤー）の現在位置を取得する
	Vector3 ownerPos = getPos_();

	//--------------------------------------------------
	// 0) 初回だけ「プレイヤーの少し前」に中心を作る
	//--------------------------------------------------
	if (!centerInitialized_) {

		// プレイヤーのヨー角にオフセットを足した向きを使う
		float yaw = getYaw_() + yawOffset_;

		// そのヨー角から前方向ベクトルを作る
		Vector3 fwd = { std::sinf(yaw), 0.0f, std::cosf(yaw) };

		// プレイヤー前方40の位置を初期中心にする
		center_ = ownerPos + fwd * 40.0f;

		// 初期中心設定済みにする
		centerInitialized_ = true;
	}

	//--------------------------------------------------
	// 1) カメラの Right / Up を取る
	//--------------------------------------------------

	// カメラ右方向の初期値はワールド右
	Vector3 camRight = { 1,0,0 };

	// カメラ上方向の初期値はワールド上
	Vector3 camUp = { 0,1,0 };

	if (cam_) {
		// カメラのワールド行列を取得する
		const auto& W = cam_->GetWorldMatrix();

		// ワールド行列から右方向ベクトルを抜き出して正規化する
		camRight = MyMath::Normalize({ W.m[0][0], W.m[0][1], W.m[0][2] });

		// ワールド行列から上方向ベクトルを抜き出して正規化する
		camUp = MyMath::Normalize({ W.m[1][0], W.m[1][1], W.m[1][2] });
	}

	//--------------------------------------------------
	// 2) ゲームパッド左スティック + キーボードWASDで center_ を直接動かす
	//--------------------------------------------------
	if (stickControl_ && inputEnabled_) {
		auto* in = TKM::Input::GetInstance();

		//==============================
		// ゲームパッド入力
		//==============================

		// 左スティックX入力を取得する
		float padX = static_cast<float>(in->GetLeftStickX());

		// 左スティックY入力を取得する
		float padY = static_cast<float>(in->GetLeftStickY());

		// デッドゾーン値を取得する
		const float dz = stickDeadZone_;

		// X入力がデッドゾーン内なら0にする
		if (std::fabs(padX) < dz) { padX = 0.0f; } else { padX = (padX > 0.0f) ? (padX - dz) : (padX + dz); }

		// Y入力がデッドゾーン内なら0にする
		if (std::fabs(padY) < dz) { padY = 0.0f; } else { padY = (padY > 0.0f) ? (padY - dz) : (padY + dz); }

		// デッドゾーンを引いた後の最大値で正規化するための値
		float norm = 32767.0f - dz;

		// 0除算防止
		if (norm < 1.0f) { norm = 1.0f; }

		// X入力を -1.0 ～ 1.0 に正規化する
		padX /= norm;

		// Y入力を -1.0 ～ 1.0 に正規化する
		padY /= norm;

		// スティック入力に2乗カーブをかけて細かい操作をしやすくする
		float lx = padX * std::fabs(padX);
		float ly = padY * std::fabs(padY);

		//==============================
		// キーボード入力（WASD）
		//==============================

		// キーボードの左右入力用
		float keyX = 0.0f;

		// キーボードの上下入力用
		float keyY = 0.0f;

		// Aで左入力
		if (GetAsyncKeyState('A') & 0x8000) { keyX -= 1.0f; }

		// Dで右入力
		if (GetAsyncKeyState('D') & 0x8000) { keyX += 1.0f; }

		// Wで上入力
		if (GetAsyncKeyState('W') & 0x8000) { keyY += 1.0f; }

		// Sで下入力
		if (GetAsyncKeyState('S') & 0x8000) { keyY -= 1.0f; }

		// 斜め入力時に移動量が大きくなりすぎないよう正規化する
		if (keyX != 0.0f || keyY != 0.0f) {
			float len = std::sqrt(keyX * keyX + keyY * keyY);
			if (len > 0.0001f) {
				keyX /= len;
				keyY /= len;
			}
		}

		//==============================
		// パッドとキーボードを合成
		//==============================

		// 左右移動量を合成する
		float moveX = lx + keyX;

		// 上下移動量を合成する
		float moveY = ly + keyY;

		// 合成後の長さを求める
		float moveLen = std::sqrt(moveX * moveX + moveY * moveY);

		// 1.0を超える場合は正規化して最大速度を抑える
		if (moveLen > 1.0f) {
			moveX /= moveLen;
			moveY /= moveLen;
		}

		//==============================
		// 実際に移動
		//==============================
		if (std::fabs(moveX) > 0.00001f || std::fabs(moveY) > 0.00001f) {

			// 1秒あたりの移動速度
			const float moveSpeed = stickMovePerSec_;

			// カメラ基準の右方向・上方向へレティクル中心を動かす
			center_ += camRight * (moveX * moveSpeed * dt)
				+ camUp * (moveY * moveSpeed * dt);
		}

		// X方向の可動範囲に収める
		center_.x = std::clamp(center_.x, moveMin_.x, moveMax_.x);

		// Y方向の可動範囲に収める
		center_.y = std::clamp(center_.y, moveMin_.y, moveMax_.y);
	}

	//--------------------------------------------------
	// 3) プレイヤー → レティクルへの方向ベクトル
	//--------------------------------------------------

	// 狙いの起点はプレイヤー位置
	Vector3 origin = ownerPos;

	// プレイヤーからレティクル中心への方向ベクトルを求める
	Vector3 dir = center_ - origin;

	// ほぼ同位置で長さが無い場合は前方向を仮の向きとして使う
	if (MyMath::Length(dir) < 0.001f) {
		float yaw = getYaw_() + yawOffset_;
		dir = { std::sinf(yaw), 0.0f, std::cosf(yaw) };
	}

	// 正規化して純粋な向きベクトルにする
	dir = MyMath::Normalize(dir);

	// 起点をキャッシュする
	lastOrigin_ = origin;

	// 狙い方向をキャッシュする
	lastAimDir_ = dir;

	// 有効な狙い情報を持った状態にする
	hasAim_ = true;

	// 狙い線の長さとして使う距離
	float dist = maxDist;

	// プレイヤーから一定距離先の狙い先端位置を求める
	Vector3 aimPoint = origin + dir * dist;

	// プレイヤーから狙い先端までのガイドラインを描画する
	TKM::LineRenderer::GetInstance()->AddLine(
		origin, aimPoint,
		TKM::LineRenderer::Color{ 0.0f, 1.0f, 0.0f, 1.0f }
	);

	//--------------------------------------------------
	// 4) 4層レティクルの配置
	//--------------------------------------------------

	// 手前と奥の近距離オフセット
	float nearOffset = 8.0f;

	// 一番奥の遠距離オフセット
	float farOffset = 20.0f;

	for (int i = 0; i < 4; ++i) {
		// 現在レイヤーへの参照
		auto& L = layers_[i];

		// オブジェクト未生成なら処理しない
		if (!L.obj) continue;

		// 非表示レイヤーは処理しない
		if (!L.visible_) continue;

		// このレイヤーの配置位置
		Vector3 pos;

		switch (i) {
		case 0:
			// 1層目は中心より手前側に置く
			pos = center_ - dir * nearOffset;
			break;

		case 1:
			// 2層目は中心位置そのまま
			pos = center_;
			break;

		case 2:
			// 3層目は中心より少し奥に置く
			pos = center_ + dir * nearOffset;
			break;

		case 3:
			// 4層目はさらに奥に置く
			pos = center_ + dir * farOffset;
			break;
		}

		// 全体の上下オフセットを加える
		pos.y += up_;

		// 位置を反映する
		L.obj->SetTranslate(pos);

		// 向きを更新するため現在回転を取得する
		Vector3 rot = L.obj->GetRotate();

		// オーナーのヨーに合わせる設定ならY回転をそろえる
		if (alignToOwnerYaw_) {
			float yaw = getYaw_() + yawOffset_;
			rot.y = yaw;
		}

		// 回転を反映する
		L.obj->SetRotate(rot);

		// 自己回転角を進める
		L.selfAngle_ += L.spinSpeed_ * dt;

		if (selfSpinAxisY_) {
			// Y軸回転型ならヨーに加算して回す
			Vector3 r = L.obj->GetRotate();
			r.y += L.spinSpeed_ * dt;
			L.obj->SetRotate(r);
		} else {
			// それ以外はZ軸回転として回す
			Vector3 r = L.obj->GetRotate();
			r.z = L.selfAngle_;
			L.obj->SetRotate(r);
		}

		// レイヤー固有スケールを毎フレ反映する
		L.obj->SetScale(L.scale_);

		// ワールド行列などを更新する
		L.obj->Update();
	}
}

void Reticle::Draw(TKM::DirectXCommon* dx) {
	// レティクル全体が非表示なら描画しない
	if (!visible_) return;

	// 各レイヤーを順番に描画する
	for (auto& L : layers_) {
		if (L.obj && L.visible_) L.obj->Draw(dx);
	}
}

void Reticle::BindOwner(
	std::function<Vector3(void)> getWorldPos,
	std::function<float(void)>   getYawRad
) {
	// 所有者のワールド位置取得関数を保持する
	getPos_ = std::move(getWorldPos);

	// 所有者のヨー角取得関数を保持する
	getYaw_ = std::move(getYawRad);
}

Vector3 Reticle::GetAimDirection() const {
	// 有効な狙い方向があるならその値を返す
	if (hasAim_) {
		return lastAimDir_;
	}

	// まだ未更新なら正面方向を仮で返す
	return Vector3{ 0.0f, 0.0f, 1.0f };
}

Vector3 Reticle::GetCenterWorldPos() const {
	// 現在のレティクル中心座標を返す
	return center_;
}

void Reticle::SetCamera(TKM::Camera* cam) {
	// カメラ参照を保持する
	cam_ = cam;

	// すべてのレイヤーへ同じカメラを設定する
	for (auto& L : layers_) {
		if (L.obj) L.obj->SetCamera(cam_);
	}
}

#ifdef USE_IMGUI
/// <summary>
/// ImGuiデバッグ表示
/// </summary>
void Reticle::ImGuiDebug() {
	if (ImGui::CollapsingHeader("レティクル")) {
		// レティクル全体の表示/非表示
		ImGui::Checkbox("Visible", &visible_);

		// オーナーのヨーに合わせるかどうか
		ImGui::Checkbox("Align To Owner Yaw", &alignToOwnerYaw_);

		// 自己回転軸をY軸にするかどうか
		ImGui::Checkbox("Self Spin Axis = Y", &selfSpinAxisY_);

		// 全体の上下オフセット調整
		ImGui::DragFloat("Up Offset", &up_, 0.01f, -20.0f, 20.0f);

		// ヨーオフセット調整
		ImGui::DragFloat("Yaw Offset", &yawOffset_, 0.001f, -3.14f, 3.14f);

		// 操作系パラメータ区切り
		ImGui::Separator();
		ImGui::Text("操作パラメータ");

		// スティック/キー操作の感度調整
		ImGui::DragFloat(
			"感度",
			&stickMovePerSec_,
			10.0f,
			20.0f,
			500.0f
		);

		// 各レイヤー設定区切り
		ImGui::Separator();

		for (int i = 0; i < 4; ++i) {
			auto& L = layers_[i];
			char name[32];
			sprintf_s(name, "レイヤー %d", i);

			if (ImGui::TreeNode(name)) {
				// 現在位置を読み取り専用で表示
				if (L.obj) {
					Vector3 pos = L.obj->GetTranslate();
					ImGui::Text("位置: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
				} else {
					ImGui::Text("位置: (---, ---, ---)");
				}

				// レイヤー個別の表示/非表示
				ImGui::Checkbox("Visible", &L.visible_);

				// レイヤー個別スケール調整
				ImGui::DragFloat3("Scale", &L.scale_.x, 0.01f, 0.01f, 10.f);

				// レイヤー個別回転速度調整
				ImGui::DragFloat("SpinSpeed", &L.spinSpeed_, 0.01f, -20.f, 20.f);

				ImGui::TreePop();
			}
		}
	}
}
#endif