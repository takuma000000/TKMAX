#define NOMINMAX
#include "PlayerDodge.h"
#include "AudioManager.h"
#include <algorithm>
#include <cmath>

void PlayerDodge::Initialize(
	TKM::Object3dCommon* common,
	TKM::DirectXCommon* dxCommon,
	TKM::Camera* camera
) {
	// 外部参照を保持する
	common_ = common;
	dxCommon_ = dxCommon;
	camera_ = camera;

	//=========================================================
	// 回避残像生成
	//=========================================================

	// 残像用の配列を初期化する
	for (auto& ghost : ghosts_) {

		// 残像本体生成
		ghost.body_ = std::make_unique<TKM::Object3d>();
		ghost.body_->Initialize(common_, dxCommon_);
		ghost.body_->SetModel("turtle.obj");
		ghost.body_->SetUseObjectColor(true); // モデルの色を無視してObjectの色を使うようにする

		// 残像ヒレ生成
		ghost.flipper_ = std::make_unique<TKM::Object3d>();
		ghost.flipper_->Initialize(common_, dxCommon_);
		ghost.flipper_->SetModel("turtle_flipper.obj");
		ghost.flipper_->SetParent(ghost.body_.get());
		ghost.flipper_->SetUseObjectColor(true); // モデルの色を無視してObjectの色を使うようにする

		// カメラ設定
		if (camera_) {
			ghost.body_->SetCamera(camera_); // カメラ参照があれば渡す
			ghost.flipper_->SetCamera(camera_); // カメラ参照があれば渡す
		}
	}
}

void PlayerDodge::Update(
	float dt,
	TKM::Object3d* ownerObject,
	bool controlEnabled,
	const Vector3& moveMin,
	const Vector3& moveMax,
	float bankAngle
) {
	// 本体が無ければ更新しない
	if (!ownerObject) {
		return;
	}
	// 回避入力と回避移動を更新する
	if (controlEnabled) {
		UpdateDodge_(dt, ownerObject, moveMin, moveMax, bankAngle);
	}
	// 回避残像を更新する
	UpdateGhost_(dt, ownerObject);
}

void PlayerDodge::UpdateDodge_(
	float dt,
	TKM::Object3d* ownerObject,
	const Vector3& moveMin,
	const Vector3& moveMax,
	float bankAngle
) {
	auto* input = TKM::Input::GetInstance();

	//=========================================================
	// 回避クールタイム更新
	//=========================================================
	// 回避クールタイムタイマーが0より大きければ経過時間を減算する
	if (dodgeCooldownTimer_ > 0.0f) {
		dodgeCooldownTimer_ -= dt;
		// クールタイムタイマーが0未満にならないようにする
		if (dodgeCooldownTimer_ < 0.0f) {
			dodgeCooldownTimer_ = 0.0f;
		}
	}

	// 回避入力があって、回避クールタイムが0で、回避中でなければ回避を開始する
	if (!isDodging_ &&
		dodgeCooldownTimer_ <= 0.0f &&
		(input->PushButton(XINPUT_GAMEPAD_X) || input->TriggerKey(DIK_J))) {
		// 回避を開始する
		StartDodge_(ownerObject);
	}
	// 回避中でなければ回避移動を更新しない
	if (!isDodging_) {
		return;
	}

	dodgeTimer_ += dt; // 回避開始からの経過時間を更新する

	// 回避移動と回避回転の進行度を計算する
	float uMove = dodgeTimer_ / std::max(0.001f, kDodgeDuration_);
	if (uMove > 1.0f) {
		uMove = 1.0f;
	}
	// 回避移動と回避回転の進行度をイージングする
	float uSpin = dodgeTimer_ / std::max(0.001f, kDodgeSpinDuration_);
	if (uSpin > 1.0f) {
		uSpin = 1.0f;
	}
	// イージング関数は0.0f～1.0fの範囲で滑らかに変化する関数を使用する
	float eMove = 0.5f - 0.5f * std::cos(MyMath::GetPI() * uMove); // 0.0f～1.0fの範囲でイージングされた回避移動の進行度
	float eSpin = 0.5f - 0.5f * std::cos(MyMath::GetPI() * uSpin); // 0.0f～1.0fの範囲でイージングされた回避回転の進行度

	//=========================================================
	// 位置更新
	//=========================================================

	Vector3 pos = dodgeStartPos_ + dodgeDirection_ * (kDodgeDistance_ * eMove); // 回避開始位置から回避方向に回避距離をイージングして加算した位置
	// 移動可能範囲内にクランプする
	pos.x = std::clamp(pos.x, moveMin.x, moveMax.x);
	pos.y = std::clamp(pos.y, moveMin.y, moveMax.y);
	pos.z = 0.0f;

	ownerObject->SetTranslate(pos);

	//=========================================================
	// 回転更新
	//=========================================================

	float spin = (MyMath::GetPI() * 2.0f) * kDodgeSpinTurns_ * eSpin; // 回避回転の進行度に応じた回転角（ラジアン）。回避回転の最大角は360度×kDodgeSpinTurns_。

	Vector3 rot = ownerObject->GetRotate(); // 現在の回転を取得する
	// 回避回転の進行度に応じて、回避開始時の回転から回避回転を加算した回転を計算する
	rot.x = dodgeBaseRot_.x + dodgeSpinPitchSign_ * spin * dodgeSpinWPitch_;
	rot.z = bankAngle + dodgeSpinRollSign_ * spin * dodgeSpinWRoll_;

	ownerObject->SetRotate(rot); // 計算した回転を本体に設定する

	//=========================================================
	// 回避終了
	//=========================================================

	// 回避移動と回避回転の両方が終了していたら回避状態を終了する
	if (uMove >= 1.0f && uSpin >= 1.0f) {
		isDodging_ = false; // 回避状態を終了する
		// 回避クールタイム開始
		dodgeCooldownTimer_ = kDodgeCooldown_;

		// 回避終了後の回転を設定する。回避開始時の回転にバンク角を加算した回転にする。
		Vector3 r = ownerObject->GetRotate();
		r.x = dodgeBaseRot_.x;
		r.z = bankAngle;
		ownerObject->SetRotate(r); // 回避終了後の回転を設定する
	}
}

void PlayerDodge::StartDodge_(TKM::Object3d* ownerObject) {
	// 本体が無ければ開始しない
	if (!ownerObject) {
		return;
	}

	// すでに回避中なら開始しない
	if (isDodging_) {
		return;
	}

	auto* input = TKM::Input::GetInstance();

	float moveX = 0.0f;
	float moveY = 0.0f;

	// 左スティック入力を取得する
	float stickX = static_cast<float>(input->GetLeftStickX());
	float stickY = static_cast<float>(input->GetLeftStickY());

	constexpr float kStickDeadZone = 6000.0f;
	constexpr float kStickNormalize = 32767.0f;

	if (std::fabs(stickX) < kStickDeadZone) {
		stickX = 0.0f;
	}

	if (std::fabs(stickY) < kStickDeadZone) {
		stickY = 0.0f;
	}

	moveX = stickX / kStickNormalize;
	moveY = stickY / kStickNormalize;

	// キーボード入力を取得する
	float keyX = 0.0f;
	float keyY = 0.0f;

	if (input->PushKey(DIK_A) || input->PushKey(DIK_LEFT)) { keyX -= 1.0f; }
	if (input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT)) { keyX += 1.0f; }
	if (input->PushKey(DIK_W) || input->PushKey(DIK_UP)) { keyY += 1.0f; }
	if (input->PushKey(DIK_S) || input->PushKey(DIK_DOWN)) { keyY -= 1.0f; }

	// キーボード入力があればそちらを優先する
	if (std::fabs(keyX) > 0.0001f || std::fabs(keyY) > 0.0001f) {
		float keyLen = std::sqrt(keyX * keyX + keyY * keyY);

		if (keyLen > 0.0001f) {
			moveX = keyX / keyLen;
			moveY = keyY / keyLen;
		}
	}

	// カメラ基準の方向を作る
	Vector3 camRight = { 1.0f, 0.0f, 0.0f };
	Vector3 camUp = { 0.0f, 1.0f, 0.0f };

	if (camera_) {
		const auto& world = camera_->GetWorldMatrix();
		camRight = MyMath::Normalize({ world.m[0][0], world.m[0][1], world.m[0][2] });
		camUp = MyMath::Normalize({ world.m[1][0], world.m[1][1], world.m[1][2] });
	}

	Vector3 dir = camRight * moveX + camUp * moveY;
	dir.z = 0.0f;

	// 入力方向が無ければ回避しない
	if (MyMath::Length(dir) < 0.001f) {
		return;
	}

	// 回避状態を開始する
	isDodging_ = true;
	dodgeTimer_ = 0.0f;
	dodgeStartPos_ = ownerObject->GetTranslate();
	dodgeDirection_ = MyMath::Normalize(dir);
	dodgeBaseRot_ = ownerObject->GetRotate();

	{
		// 回避方向の水平・垂直を判定して、回避回転の軸と符号を決める
		const float ax = std::fabs(dodgeDirection_.x);
		const float ay = std::fabs(dodgeDirection_.y);
		// 水平方向の入力が垂直方向の入力より大きければ水平回避、そうでなければ垂直回避と判定する
		const bool horizontal = (ax >= ay);

		// 水平回避ならロール回転、垂直回避ならピッチ回転を行うようにする。回避方向の符号に応じて回転の符号も決める。
		if (horizontal) {
			dodgeSpinWRoll_ = 1.0f; // 水平回避はロール回転
			dodgeSpinWPitch_ = 0.0f; // 水平回避はピッチ回転なし

			// 水平方向の入力が正なら右回避でロールを反時計回り（左から見て右肩が下がる）にする。負なら左回避でロールを時計回り（左から見て右肩が上がる）にする。
			dodgeSpinRollSign_ = (dodgeDirection_.x >= 0.0f) ? -1.0f : +1.0f;
			dodgeSpinPitchSign_ = +1.0f;
		} else { // 垂直回避
			dodgeSpinWRoll_ = 0.0f; // 垂直回避はロール回転なし
			dodgeSpinWPitch_ = 1.0f; // 垂直回避はピッチ回転

			// 垂直方向の入力が正なら上回避でピッチを下向き（左から見て頭が下がる）にする。負なら下回避でピッチを上向き（左から見て頭が上がる）にする。
			dodgeSpinPitchSign_ = (dodgeDirection_.y >= 0.0f) ? +1.0f : -1.0f;
			dodgeSpinRollSign_ = +1.0f;
		}
	}

	// 回避SEを鳴らす
	TKM::AudioManager::GetInstance()->PlaySound("avoid", 0.1f);
}

void PlayerDodge::UpdateGhost_(
	float dt,
	TKM::Object3d* ownerObject
) {
	//=========================================================
	// 回避中なら一定間隔で残像生成
	//=========================================================

	if (isDodging_) {

		ghostSpawnTimer_ += dt;

		// 一定間隔ごとに残像追加
		if (ghostSpawnTimer_ >= kGhostInterval_) {

			ghostSpawnTimer_ = 0.0f;

			AddGhost_(ownerObject);
		}
	} else {

		// 回避していない時はタイマー初期化
		ghostSpawnTimer_ = 0.0f;
	}

	//=========================================================
	// 全残像更新
	//=========================================================

	for (auto& ghost : ghosts_) {

		// 未使用ならスキップ
		if (!ghost.active_) {
			continue;
		}

		// 経過時間加算
		ghost.age_ += dt;

		//=====================================================
		// 寿命終了
		//=====================================================

		// 寿命終了していたら非アクティブにしてスキップ
		if (ghost.age_ >= ghost.life_) {
			ghost.active_ = false;
			continue;
		}

		//=====================================================
		// フェード更新
		//=====================================================

		float rate = ghost.age_ / ghost.life_; // 経過時間の割合（0.0～1.0）
		float alpha = 0.35f * (1.0f - rate);   // 残像の透明度（最初は0.35、最後は0.0に向かって減る）
		// 残像の色（暗いグレーで透明度のみ変化）
		Vector4 shadowColor = {
			0.04f,
			0.04f,
			0.04f,
			alpha
		};

		//=====================================================
		// 本体更新
		//=====================================================

		// 位置・回転・拡縮反映
		ghost.body_->SetTranslate(ghost.pos_);
		ghost.body_->SetRotate(ghost.rot_);
		ghost.body_->SetScale(ghost.scale_);
		ghost.body_->SetColor(shadowColor);

		//=====================================================
		// ヒレ更新
		//=====================================================

		// 位置・回転・拡縮反映
		ghost.flipper_->SetColor(shadowColor);

		//=====================================================
		// 行列更新
		//=====================================================

		ghost.body_->Update();
		ghost.flipper_->Update();
	}
}

void PlayerDodge::AddGhost_(TKM::Object3d* ownerObject) {

	// 現在書き込み位置取得
	auto& ghost = ghosts_[ghostWriteIndex_];

	//=========================================================
	// プレイヤー状態コピー
	//=========================================================

	// 位置・回転・拡縮をプレイヤーと同じにする
	ghost.pos_ = ownerObject->GetTranslate();
	ghost.rot_ = ownerObject->GetRotate();
	ghost.scale_ = ownerObject->GetScale();

	//=========================================================
	// 状態初期化
	//=========================================================

	// 経過時間初期化
	ghost.age_ = 0.0f;
	ghost.life_ = kGhostLife_;
	ghost.active_ = true;

	//=========================================================
	// 初期見た目設定
	//=========================================================

	// 位置・回転・拡縮反映
	ghost.body_->SetTranslate(ghost.pos_);
	ghost.body_->SetRotate(ghost.rot_);
	ghost.body_->SetScale(ghost.scale_);

	// 色は暗いグレーで半透明
	ghost.body_->SetColor({
		0.04f,
		0.04f,
		0.04f,
		0.35f
		});
	// ヒレも同じ色
	ghost.flipper_->SetColor({
		0.04f,
		0.04f,
		0.04f,
		0.35f
		});

	//=========================================================
	// 行列更新
	//=========================================================

	ghost.body_->Update();
	ghost.flipper_->Update();

	//=========================================================
	// 次回書き込み位置更新
	//=========================================================

	ghostWriteIndex_++;

	// 最大数超過時は先頭へ戻る
	if (ghostWriteIndex_ >= kGhostMax_) {
		ghostWriteIndex_ = 0; // 次回は先頭から上書きする
	}
}

void PlayerDodge::Draw(TKM::DirectXCommon* dxCommon) {

	//=========================================================
	// 全残像描画
	//=========================================================

	// 全残像ループ
	for (auto& ghost : ghosts_) {
		// 未使用なら描画しない
		if (!ghost.active_) {
			continue;
		}
		// 本体描画
		ghost.body_->Draw(dxCommon);
		// ヒレ描画
		ghost.flipper_->Draw(dxCommon);
	}
}

void PlayerDodge::SetCamera(TKM::Camera* camera) {

	// カメラ参照保持
	camera_ = camera;

	//=========================================================
	// 全残像へカメラ設定
	//=========================================================

	// 全残像ループ
	for (auto& ghost : ghosts_) {
		// 本体へ設定
		if (ghost.body_) {
			ghost.body_->SetCamera(camera_); // カメラ参照があれば渡す
		}
		// ヒレへ設定
		if (ghost.flipper_) {
			ghost.flipper_->SetCamera(camera_); // カメラ参照があれば渡す
		}
	}
}