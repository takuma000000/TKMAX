#include "ClearSequenceController.h"

namespace TKM {
	void ClearSequenceController::Initialize(Player* player, BossManager* bossManager, GameFlowController* flow, DirectXCommon* dxCommon, Skybox* skybox, FireworkController* fireworkController) {
		player_ = player;
		bossManager_ = bossManager;
		flow_ = flow;
		dxCommon_ = dxCommon;
		skybox_ = skybox;
		fireworkController_ = fireworkController;

		active_ = false;
		phase_ = Phase::None;
		timer_ = 0.0f;
		irisClosing_ = false;
	}

	void ClearSequenceController::Start() {
		active_ = true;
		phase_ = Phase::CamZoom;
		timer_ = 0.0f;

		//  外部Iris描画は一旦OFF
		if (flow_) {
			flow_->SetExternalIrisDraw(false);
		}

		// --- ボス、ボス弾、レティクルを消し、プレイヤー操作をロック ---
		if (bossManager_) {
			bossManager_->OnClearSequenceStart();
		}
		if (player_) {
			player_->SetControlEnabled(false);
			player_->SetReticleVisible(false);
		}

		if (dxCommon_) {
			dxCommon_->SetVignettingEffect(nullptr);
		}

		// --- カメラ初期位置・目標位置の設定 ---
		auto* cam = TKM::CameraManager::GetInstance()->GetMainCamera();
		if (cam) {
			camStartPos_ = cam->GetTranslate(); // カメラの開始位置を保存
		}

		if (cam && player_) {
			Vector3 camPos = cam->GetTranslate(); // カメラの現在位置
			Vector3 playerPos = player_->GetPosition(); // プレイヤーの現在位置

			float targetZ = MyMath::Lerp(camPos.z, playerPos.z - 15.0f, 1.0f); // プレイヤーの少し手前にズーム
			camTargetPos_ = { camPos.x, camPos.y + 2.0f, targetZ }; // 目標位置はカメラのXを維持し、Yを少し上げ、Zをプレイヤーの手前に設定
			// プレイヤーの開始位置も保存
			playerStartPos_ = player_->GetPosition();
		}
		// --- アイリスクローズの初期化 ---
		irisClosing_ = false;
	}

	bool ClearSequenceController::Update(float dt) {
		if (!active_) {
			return false;
		}

		timer_ += dt;

		// skyboxはずっと回し続ける
		if (skybox_) {
			skybox_->UpdateRotation();
		}

		bool finished = false;

		switch (phase_) {
		case Phase::CamZoom:
			UpdateCamZoom(dt, finished);
			break;
		case Phase::PlayerFly:
			UpdatePlayerFly(dt, finished);
			break;
		case Phase::IrisClose:
			UpdateIrisClose(dt, finished);
			break;
		default:
			break;
		}

		if (finished) {
			active_ = false;
			phase_ = Phase::None;
		}

		return finished;
	}

	void ClearSequenceController::SetPlayerSpeed(float v) {
		playerSpeed_ = v; // プレイヤーの飛行速度を設定
	}

	void ClearSequenceController::SetPlayerFlyDistance(float v) {
		playerFlyDistance_ = v; // プレイヤーの飛行距離を設定
	}

	void ClearSequenceController::UpdateCamZoom(float dt, bool& finished) {
		(void)dt; // 未使用
		(void)finished; // 未使用（このフェーズは時間経過で終了するため、Update内で完了判定は行わない）

		auto* cam = TKM::CameraManager::GetInstance()->GetMainCamera(); // Main固定
		if (!cam) { return; } // カメラがない場合は何もしない

		float t = std::clamp(timer_ / 1.0f, 0.0f, 1.0f); // 1秒かけてカメラを開始位置から目標位置へ線形補間
		Vector3 camPos = MyMath::Vector3Lerp(camStartPos_, camTargetPos_, t); // 開始位置と目標位置の間を補間してカメラ位置を計算
		cam->SetTranslate(camPos); // カメラの平行移動を更新
		cam->Update(); // カメラの行列を更新して反映させる

		if (t >= 1.0f) { // 補間が完了したら次のフェーズへ
			phase_ = Phase::PlayerFly; // 次のフェーズへ
			timer_ = 0.0f; // タイマーをリセットして次のフェーズの時間計測を開始

			if (fireworkController_) { // 花火コントローラーがある場合はリセットして開始
				fireworkController_->Reset(); // 花火生成のリセット
			}
		}
	}

	void ClearSequenceController::UpdatePlayerFly(float dt, bool& finished) {
		(void)finished; // 未使用（このフェーズはプレイヤーの位置と時間経過で終了するため、Update内で完了判定は行わない）

		auto* cam = TKM::CameraManager::GetInstance()->GetMainCamera(); // Main固定
		if (!cam || !player_) { return; } // カメラまたはプレイヤーがない場合は何もしない

		cam->SetTranslate(camTargetPos_); // カメラ位置を目標位置に固定（プレイヤーの移動に合わせてカメラも移動させるため）
		cam->Update(); // カメラの行列を更新して反映させる
		// プレイヤーを前方（Z方向）に移動させる
		Vector3 pos = player_->GetPosition();
		pos.z += playerSpeed_ * dt; // プレイヤーのZ位置を速度に基づいて更新
		player_->SetPosition(pos); // プレイヤーの位置を更新
		player_->UpdateVisualOnly(dt); // 移動に合わせて見た目も更新（当たり判定はなし）

		if (fireworkController_) { // 花火コントローラーがある場合は更新
			fireworkController_->Update(dt, cam); // 花火の更新（カメラを渡して視点に合わせる）
		}

		if (timer_ >= playerFlyMinTime_ && 
			pos.z > playerStartPos_.z + playerFlyDistance_) { // プレイヤーが一定距離を飛んだかつ最短時間が経過したら次のフェーズへ

			phase_ = Phase::IrisClose; // 次のフェーズへ
			timer_ = 0.0f; // タイマーをリセットして次のフェーズの時間計測を開始

			// Irisは flow_->Draw() 側で描かれるので、外部描画ON
			if (flow_) {
				flow_->SetExternalIrisDraw(true); // アイリス描画を外部制御に切り替える（これ以降、アイリスは flow_->Draw() で描画されるようになる）
			}

			irisClosing_ = true; // アイリスクローズ開始フラグを立てる
			irisCloseTween_.Reset( // アイリスクローズのTweenを初期化して開始
				0.0f,
				flow_ ? flow_->GetIrisMaxScale() : 0.0f,
				kIrisDurationSec_,
				Ease::Type::InBack
			);
		}
	}

	void ClearSequenceController::UpdateIrisClose(float dt, bool& finished) {
		if (!irisClosing_) { // アイリスクローズが開始されていない場合は何もしない
			return; // ただし、念のためアイリスクローズの更新は行う（外部制御されている可能性があるため）
		}
		// アイリスクローズの更新
		UpdateIrisScale(flow_ ? flow_->GetIrisSprite() : nullptr, irisCloseTween_, dt);
		// アイリスクローズが完了したら遷移完了
		if (irisCloseTween_.Finished()) {
			// ここで externalIrisDraw をOFFにしない（遷移まで保持）
			finished = true;
		}
	}
}