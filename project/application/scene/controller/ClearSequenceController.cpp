#include "ClearSequenceController.h"
#include "IntroStartBanner.h"

namespace TKM {
	void ClearSequenceController::Initialize(Player* player, BossManager* bossManager, GameFlowController* flow, DirectXCommon* dxCommon, Skybox* skybox, FireworkController* fireworkController, SmokeVolume3D* smokeVolume) {
		// プレイヤー参照を保持する
		player_ = player;

		// ボスマネージャー参照を保持する
		bossManager_ = bossManager;

		// ゲームフロー制御参照を保持する
		flow_ = flow;

		// DirectX共通情報を保持する
		dxCommon_ = dxCommon;

		// スカイボックス参照を保持する
		skybox_ = skybox;

		// 花火制御参照を保持する
		fireworkController_ = fireworkController;

		// クリア用スモークボリューム参照を保持する
		smokeVolume_ = smokeVolume;

		// シーケンスを未実行状態にする
		active_ = false;

		// フェーズを無しにする
		phase_ = Phase::None;

		// フェーズ用タイマーを初期化する
		timer_ = 0.0f;

		// アイリスクローズ状態を初期化する
		irisClosing_ = false;
	}

	void ClearSequenceController::Start() {
		// クリアシーケンスを有効化する
		active_ = true;

		// 最初のフェーズをカメラズームにする
		phase_ = Phase::CamZoom;

		// フェーズ用タイマーを初期化する
		timer_ = 0.0f;

		// 外部Iris描画は一旦OFFにする
		if (flow_) {
			flow_->SetExternalIrisDraw(false);
		}

		// ボスやボス弾など、クリア演出に不要な要素を止める
		if (bossManager_) {
			bossManager_->OnClearSequenceStart();
		}

		// プレイヤー操作とレティクルを止める
		if (player_) {
			player_->SetControlEnabled(false);
			player_->SetReticleVisible(false);
		}

		// 低HPビネットなどを消すため、ビネットエフェクトを解除する
		if (dxCommon_) {
			dxCommon_->SetVignettingEffect(nullptr);
		}

		// メインカメラを取得する
		auto* cam = TKM::CameraManager::GetInstance()->GetMainCamera();

		// 現在のカメラ位置を開始位置として保存する
		if (cam) {
			camStartPos_ = cam->GetTranslate();
		}

		// カメラとプレイヤーがある場合、ズーム先のカメラ目標位置を作る
		if (cam && player_) {
			// 現在のカメラ位置を取得する
			Vector3 camPos = cam->GetTranslate();

			// 現在のプレイヤー位置を取得する
			Vector3 playerPos = player_->GetPosition();

			// プレイヤーの少し手前を目標Zにする
			float targetZ = MyMath::Lerp(camPos.z, playerPos.z - 15.0f, 1.0f);

			// Xは維持し、Yを少し上げ、Zをプレイヤー手前にする
			camTargetPos_ = { camPos.x, camPos.y + 2.0f, targetZ };

			// プレイヤーの飛行開始位置を保存する
			playerStartPos_ = player_->GetPosition();
		}

		// アイリスクローズはまだ開始していない状態にする
		irisClosing_ = false;
	}

	bool ClearSequenceController::Update(float dt) {
		// シーケンスが動いていなければ終了していない扱いで返す
		if (!active_) {
			return false;
		}

		// フェーズ用タイマーを進める
		timer_ += dt;

		// クリア演出中もスカイボックスは回転させ続ける
		if (skybox_) {
			skybox_->UpdateRotation();
		}

		// このフレームでシーケンスが完了したかどうか
		bool finished = false;

		// 現在フェーズに応じた更新を行う
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

		// 完了したら内部状態を停止状態へ戻す
		if (finished) {
			active_ = false;
			phase_ = Phase::None;
		}

		// 呼び出し元へ完了状態を返す
		return finished;
	}

	void ClearSequenceController::SetPlayerSpeed(float v) {
		// プレイヤーの飛行速度を設定する
		playerSpeed_ = v;
	}

	void ClearSequenceController::SetPlayerFlyDistance(float v) {
		// プレイヤーが飛ぶ距離を設定する
		playerFlyDistance_ = v;
	}

	void ClearSequenceController::UpdateCamZoom(float dt, bool& finished) {
		// このフェーズではdtを直接使わない
		(void)dt;

		// このフェーズ内ではfinishedを直接変更しない
		(void)finished;

		// メインカメラを取得する
		auto* cam = TKM::CameraManager::GetInstance()->GetMainCamera();

		// カメラが無ければ更新しない
		if (!cam) { return; }

		// 1秒かけてズームするための進行率を作る
		float t = std::clamp(timer_ / 1.0f, 0.0f, 1.0f);

		// 開始位置から目標位置へ補間する
		Vector3 camPos = MyMath::Vector3Lerp(camStartPos_, camTargetPos_, t);

		// カメラ位置を反映する
		cam->SetTranslate(camPos);

		// カメラ行列を更新する
		cam->Update();

		// カメラズームが完了したらプレイヤー飛行フェーズへ進む
		if (t >= 1.0f) {
			// 次のフェーズへ切り替える
			phase_ = Phase::PlayerFly;

			// 次フェーズ用にタイマーをリセットする
			timer_ = 0.0f;

			// クリア用スモーク演出を中心から開始する
			if (smokeVolume_) {
				smokeVolume_->StartClearFromCenter(2.3f);
			}

			// 花火コントローラーをリセットして、飛行フェーズ中に発生できる状態にする
			if (fireworkController_) {
				fireworkController_->Reset();
			}
		}
	}

	void ClearSequenceController::UpdatePlayerFly(float dt, bool& finished) {
		// このフェーズ内ではfinishedを直接変更しない
		(void)finished;

		// メインカメラを取得する
		auto* cam = TKM::CameraManager::GetInstance()->GetMainCamera();

		// カメラまたはプレイヤーが無ければ更新しない
		if (!cam || !player_) { return; }

		// カメラ位置を目標位置に固定する
		cam->SetTranslate(camTargetPos_);

		// カメラ行列を更新する
		cam->Update();

		// プレイヤーの現在位置を取得する
		Vector3 pos = player_->GetPosition();

		// プレイヤーをZ方向へ前進させる
		pos.z += playerSpeed_ * dt;

		// プレイヤー位置を反映する
		player_->SetPosition(pos);

		// クリア演出用に見た目だけ更新する
		player_->UpdateVisualOnly(dt);

		// カメラ基準で花火を更新する
		if (fireworkController_) {
			fireworkController_->Update(dt, cam);
		}

		// 最短時間を超え、指定距離まで飛んだらアイリスクローズへ進む
		if (timer_ >= playerFlyMinTime_ &&
			pos.z > playerStartPos_.z + playerFlyDistance_) {

			// 次のフェーズへ切り替える
			phase_ = Phase::IrisClose;

			// 次フェーズ用にタイマーをリセットする
			timer_ = 0.0f;

			// Irisはflow側で描画するため、外部描画をONにする
			if (flow_) {
				flow_->SetExternalIrisDraw(true);
			}

			// アイリスクローズ開始フラグを立てる
			irisClosing_ = true;

			// アイリスクローズ用Tweenを開始する
			irisCloseTween_.Reset(
				0.0f,
				flow_ ? flow_->GetIrisMaxScale() : 0.0f,
				kIrisDurationSec_,
				Ease::Type::InBack
			);
		}
	}

	void ClearSequenceController::UpdateIrisClose(float dt, bool& finished) {
		// アイリスクローズが始まっていなければ更新しない
		if (!irisClosing_) {
			return;
		}

		// IrisスプライトのサイズをTweenに合わせて更新する
		UpdateIrisScale(flow_ ? flow_->GetIrisSprite() : nullptr, irisCloseTween_, dt);

		// アイリスクローズが終わったらシーケンス完了にする
		if (irisCloseTween_.Finished()) {
			// externalIrisDraw は遷移まで保持するためここではOFFにしない
			finished = true;
		}
	}
}