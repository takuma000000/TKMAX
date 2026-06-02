#include "PostEffectController.h"
#include "MyMath.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {
	void PostEffectController::Initialize(DirectXCommon* dxCommon, Player* player, BossManager* bossManager) {
		// DirectX共通情報を保持する
		dxCommon_ = dxCommon;

		// プレイヤー参照を保持する
		player_ = player;

		//=========================================================
		// RadialBlurEffect 初期化
		//=========================================================

		// 放射ブラーエフェクトを生成する
		radialBlur_ = std::make_unique<TKM::RadialBlurEffect>();

		// DirectX共通情報を渡して初期化する
		radialBlur_->Initialize(dxCommon_);

		// DirectXCommon側へ放射ブラーを登録する
		dxCommon_->SetRadialBlurEffect(radialBlur_.get());

		// プレイヤー側からも放射ブラーを発火できるように渡す
		if (player) {
			player->SetRadialBlurEffect(radialBlur_.get());
		}

		//=========================================================
		// VignettingEffect 初期化
		//=========================================================

		// ビネットエフェクトを生成する
		vignetting_ = std::make_unique<TKM::VignettingEffect>();

		// DirectX共通情報を渡して初期化する
		vignetting_->Initialize(dxCommon_);

		//=========================================================
		// FogEffect 初期化
		//=========================================================

		// フォグエフェクトを生成する
		fog_ = std::make_unique<TKM::FogEffect>();

		// DirectX共通情報を渡して初期化する
		fog_->Initialize(dxCommon_);

		// 初期状態ではフォグを無効化する
		fog_->SetActive(false);

		// DirectXCommon側へフォグを登録する
		dxCommon_->SetFogEffect(fog_.get());

		//=========================================================
		// AuraEffect 初期化
		//=========================================================

		// オーラエフェクトを生成する
		aura_ = std::make_unique<TKM::AuraEffect>();

		// DirectX共通情報を渡して初期化する
		aura_->Initialize(dxCommon_);

		// DirectXCommon側へオーラを登録する
		dxCommon_->SetAuraEffect(aura_.get());

		//=========================================================
		// WaterRippleEffect 初期化
		//=========================================================

		// 水面波紋エフェクトを生成する
		waterRipple_ = std::make_unique<TKM::WaterRippleEffect>();

		// DirectX共通情報を渡して初期化する
		waterRipple_->Initialize(dxCommon_);

		// DirectXCommon側へ水面波紋を登録する
		dxCommon_->SetWaterRippleEffect(waterRipple_.get());

		// BossManager側からも水面波紋を発火できるように渡す
		if (bossManager) {
			bossManager->SetWaterRippleEffect(waterRipple_.get());
		}

		//=========================================================
		// FogVolume3D 初期化
		//=========================================================

		// 空間霧ボリュームを生成する
		fogVolume3D_ = std::make_unique<TKM::FogVolume3D>();

		// DirectX共通情報を渡して初期化する
		fogVolume3D_->Initialize(dxCommon_);

		{
			// 現在の設定をコピーで受け取る
			auto d = fogVolume3D_->GetDesc();

			// 空間霧の中心位置を設定する
			d.centerWS_ = { 0.0f, 6.0f, 20.0f };

			// 空間霧の半サイズを設定する
			d.halfSizeWS_ = { 520.0f, 160.0f, 520.0f };

			// 空間霧のスライス数を設定する
			d.sliceCount_ = 40;

			// 空間霧の密度を設定する
			d.density_ = 0.19f;

			// 変更した設定をまとめて反映する
			fogVolume3D_->SetDesc(d);
		}

		//=========================================================
		// SmokeVolume3D 初期化
		//=========================================================

		// 空間スモークボリュームを生成する
		smokeVolume3D_ = std::make_unique<TKM::SmokeVolume3D>();

		// DirectX共通情報を渡して初期化する
		smokeVolume3D_->Initialize(dxCommon_);

		//=========================================================
		// MotionBlurEffect 初期化
		//=========================================================

		// モーションブラーエフェクトを生成する
		motionBlur_ = std::make_unique<TKM::MotionBlurEffect>();

		// DirectX共通情報を渡して初期化する
		motionBlur_->Initialize(dxCommon_);

		// DirectXCommon側へモーションブラーを登録する
		dxCommon_->SetMotionBlurEffect(motionBlur_.get());
	}

	void PostEffectController::Finalize() {
		// DirectXCommonが存在する場合だけ登録解除する
		if (dxCommon_) {
			// 放射ブラー参照を解除する
			dxCommon_->SetRadialBlurEffect(nullptr);

			// ビネット参照を解除する
			dxCommon_->SetVignettingEffect(nullptr);

			// フォグ参照を解除する
			dxCommon_->SetFogEffect(nullptr);

			// オーラ参照を解除する
			dxCommon_->SetAuraEffect(nullptr);

			// 水面波紋参照を解除する
			dxCommon_->SetWaterRippleEffect(nullptr);

			// モーションブラー参照を解除する
			dxCommon_->SetMotionBlurEffect(nullptr);
		}
	}

	void PostEffectController::Update(float dt, BossManager* bossManager) {
		//=========================================================
		// 画面エフェクト更新
		//=========================================================

		// 放射ブラーを更新する
		radialBlur_->Update(dt);

		// プレイヤーHPが少ないかどうかを判定する
		bool lowHp = (player_ && player_->GetHP() <= 2);

		// 低HP状態をビネットへ渡す
		vignetting_->SetLowHP(lowHp);

		// ビネットを更新する
		vignetting_->Update(dt);

		// 水面波紋を更新する
		waterRipple_->Update(dt);

		// 空間スモークを更新する
		smokeVolume3D_->Update(dt);

		// モーションブラーの目標強度を決める
		float targetStrength = 0.0f;
		// プレイヤーが回避行動を取っている場合は強めのモーションブラーをかける
		if (player_ && player_->IsDodging()) {
			targetStrength = 0.85f;
		}

		// 現在値取得
		float currentStrength = motionBlur_->GetStrength();
		// なめらか補間
		constexpr float kBlurEaseSpeed = 10.0f;
		// 現在の強度から目標の強度へ、なめらかに補間していく
		currentStrength +=
			(targetStrength - currentStrength) *
			kBlurEaseSpeed *
			dt;

		// ほぼ0なら切る
		if (currentStrength < 0.01f) {
			currentStrength = 0.0f;
			motionBlur_->SetActive(false);
		} else {
			motionBlur_->SetActive(true);
		}
		// 補間後の強度をモーションブラーへ渡す
		motionBlur_->SetStrength(currentStrength);
		// モーションブラーを更新する
		motionBlur_->Update(dt);
	}

	void PostEffectController::OnCameraUpdated(TKM::Camera* activeCamera) {
		// フォグまたはカメラが無ければ更新しない
		if (!fog_ || !activeCamera) {
			return;
		}

		// カメラのワールド行列を取得する
		const Matrix4x4& camW = activeCamera->GetWorldMatrix();

		// ワールド行列からカメラ位置を取り出す
		Vector3 camPos{
			camW.m[3][0],
			camW.m[3][1],
			camW.m[3][2]
		};

		// フォグ側へカメラ位置を渡す
		fog_->SetWorldPos(camPos);
	}

	void PostEffectController::SetRadialBlurManual(bool enable, float strength) {
		// 放射ブラーが存在する場合だけ手動制御を設定する
		if (radialBlur_) {
			radialBlur_->SetManualBlur(enable, strength);
		}
	}

	void PostEffectController::StartMotionBlurBurst(float strength, float duration) {
		if (motionBlur_) {
			motionBlur_->StartBurst(strength, duration);
		}
	}

	void PostEffectController::StopMotionBlur() {
		if (motionBlur_) {
			motionBlur_->Stop();
		}
	}

	void PostEffectController::ClearRadialBlurManual() {
		// 放射ブラーが存在する場合だけ手動制御を解除する
		if (radialBlur_) {
			radialBlur_->ClearManualBlur();
		}
	}

	bool PostEffectController::IsPlayerDodging() const {
		// プレイヤーが存在しない場合は回避行動を取っていないとみなす
		if (!player_) {
			return false;
		}

		return player_->IsDodging(); // プレイヤーの回避行動状態を返す
	}

	void PostEffectController::DrawVolumes(TKM::Camera* activeCamera) {
		// カメラが無ければボリューム描画できない
		if (!activeCamera) {
			return;
		}

		//=========================================================
		// カメラ基準ベクトル取得
		//=========================================================

		// カメラのワールド行列を取得する
		const Matrix4x4& camW = activeCamera->GetWorldMatrix();

		// カメラ右方向を取り出す
		Vector3 right{ camW.m[0][0], camW.m[0][1], camW.m[0][2] };

		// カメラ上方向を取り出す
		Vector3 up{ camW.m[1][0], camW.m[1][1], camW.m[1][2] };

		// カメラ前方向を取り出す
		Vector3 fwd{ camW.m[2][0], camW.m[2][1], camW.m[2][2] };

		// カメラ位置を取り出す
		Vector3 camPos{
			camW.m[3][0],
			camW.m[3][1],
			camW.m[3][2]
		};

		// カメラのビュー射影行列を取得する
		Matrix4x4 vp = activeCamera->GetViewProjectionMatrix();

		// 空間スモークを描画する
		smokeVolume3D_->Draw(vp, right, up, fwd);
	}

	void PostEffectController::ImGuiDebug() {
#ifdef USE_IMGUI
		// ポストエフェクト用ImGuiウィンドウを開く
		if (ImGui::Begin("ポストエフェクト")) {
			// 放射ブラーのImGuiデバッグ表示
			smokeVolume3D_->ImGuiDebug();
			// モーションブラーのImGuiデバッグ表示
			motionBlur_->ImGuiDebug();
		}

		// ImGuiウィンドウを閉じる
		ImGui::End();
#endif
	}
}