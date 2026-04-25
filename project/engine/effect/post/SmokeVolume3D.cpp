#include "SmokeVolume3D.h"
#include "DirectXCommon.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	void SmokeVolume3D::Initialize(DirectXCommon* dx) {
		// DirectXCommonを保持する
		dxCommon_ = dx;

		// 煙アニメーション用の時間を初期化する
		time_ = 0.0f;

		// 初期状態では有効にする
		active_ = true;
	}

	void SmokeVolume3D::Update(float dt) {
		// シェーダー側へ渡す時間を進める
		time_ += dt;

		// 前方向スクロールが有効な場合、煙の中心位置をZ方向へ流す
		if (desc_.scrollForward_) {
			desc_.centerWS_.z -= desc_.scrollSpeed_ * dt;

			// 前方限界を超えたら、奥側の位置へ戻してループさせる
			if (desc_.centerWS_.z < desc_.frontLimitZ_) {
				desc_.centerWS_.z = desc_.resetZ_;
			}
		}

		//======================================
		// クリア演出：中央から消す
		//======================================
		if (clearFromCenterActive_) {
			// 指定時間に対する消滅進行率を進める
			clearFromCenterT_ += dt / clearFromCenterDuration_;

			// 進行率を0.0f～1.0fに収める
			if (clearFromCenterT_ > 1.0f) {
				clearFromCenterT_ = 1.0f;
			}
			if (clearFromCenterT_ < 0.0f) {
				clearFromCenterT_ = 0.0f;
			}

			float t = clearFromCenterT_;

			// OutQuadで、終盤ほどゆるやかに消える進行率へ変換する
			float e = 1.0f - (1.0f - t) * (1.0f - t);

			// 煙の高さを中央から縮める
			desc_.halfSizeWS_.y = MyMath::Lerp(clearStartHalfSizeWS_.y, 0.0f, e);

			// 煙の密度を下げて薄くする
			desc_.density_ = MyMath::Lerp(clearStartDensity_, 0.0f, e);

			// 最大アルファも下げて、見た目上も消えていくようにする
			desc_.alphaMax_ = MyMath::Lerp(clearStartAlphaMax_, 0.0f, e);

			// 完全に消えたら演出を終了し、描画も無効にする
			if (t >= 1.0f) {
				clearFromCenterActive_ = false;
				active_ = false;
			}
		}
	}

	void SmokeVolume3D::StartClearFromCenter(float duration) {
		// 中央から消える演出を開始する
		clearFromCenterActive_ = true;

		// 消滅進行率を最初からに戻す
		clearFromCenterT_ = 0.0f;

		// 0秒以下が渡された場合でも割り算が壊れないようにする
		if (duration <= 0.0f) {
			clearFromCenterDuration_ = 0.0001f;
		} else {
			clearFromCenterDuration_ = duration;
		}

		// 消え始める前の状態を保存して、そこから補間する
		clearStartHalfSizeWS_ = desc_.halfSizeWS_;
		clearStartDensity_ = desc_.density_;
		clearStartAlphaMax_ = desc_.alphaMax_;
	}

	void SmokeVolume3D::Draw(const Matrix4x4& viewProj, const Vector3& camRightWS, const Vector3& camUpWS, const Vector3& camFwdWS) {
		// 無効状態なら描画しない
		if (!active_) {
			return;
		}

		// DirectXCommonが未設定なら描画できない
		if (!dxCommon_) {
			return;
		}

		// シェーダー側で使うワールド位置を中心座標に合わせる
		desc_.worldPos_ = desc_.centerWS_;

		// DirectXCommon側の煙ボリューム描画処理へ必要なパラメータを渡す
		dxCommon_->DrawSmokeVolume(
			viewProj,
			desc_.centerWS_,
			desc_.halfSizeWS_,
			camRightWS,
			camUpWS,
			camFwdWS,
			desc_.sliceCount_,
			time_,
			desc_.color_,
			desc_.density_,
			desc_.baseScale_,
			desc_.detailScale_,
			desc_.detailStrength_,
			desc_.threshold_,
			desc_.softness_,
			desc_.flowSpeed_,
			desc_.riseSpeed_,
			desc_.alphaMax_,
			desc_.worldScale_,
			desc_.worldPos_
		);
	}

#ifdef USE_IMGUI
	void SmokeVolume3D::ImGuiDebug() {
		if (ImGui::Begin("煙ボリューム 3D")) {
			// 煙ボリュームの有効・無効を切り替える
			ImGui::Checkbox("有効", &active_);

			// 煙の中心位置と範囲サイズを調整する
			ImGui::DragFloat3("中心座標 (WS)", &desc_.centerWS_.x, 1.0f);
			ImGui::DragFloat3("半径サイズ (WS)", &desc_.halfSizeWS_.x, 1.0f, 1.0f, 5000.0f);

			// 煙の基本色と濃さを調整する
			ImGui::ColorEdit3("色", &desc_.color_.x);
			ImGui::DragFloat("密度", &desc_.density_, 0.001f, 0.0f, 2.0f);

			// ボリュームを何枚のスライスで描画するか調整する
			ImGui::DragInt("スライス数", (int*)&desc_.sliceCount_, 1.0f, 1, 256);

			ImGui::Separator();

			// ノイズの大きな流れと細かいディテールを調整する
			ImGui::DragFloat("ベーススケール", &desc_.baseScale_, 0.001f, 0.001f, 3.0f);
			ImGui::DragFloat("ディテールスケール", &desc_.detailScale_, 0.001f, 0.001f, 10.0f);
			ImGui::DragFloat("ディテール強度", &desc_.detailStrength_, 0.01f, 0.0f, 1.0f);

			// 煙として見える部分のしきい値と境界のぼかしを調整する
			ImGui::DragFloat("しきい値", &desc_.threshold_, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("ぼかし", &desc_.softness_, 0.005f, 0.0f, 1.0f);

			ImGui::Separator();

			// 煙の流れる方向感と上昇感を調整する
			ImGui::DragFloat("流れ速度（カメラ方向）", &desc_.flowSpeed_, 0.01f, 0.0f, 10.0f);
			ImGui::DragFloat("上昇速度", &desc_.riseSpeed_, 0.01f, 0.0f, 10.0f);

			// 煙の最大不透明度を調整する
			ImGui::DragFloat("最大アルファ", &desc_.alphaMax_, 0.01f, 0.0f, 1.0f);
		}

		ImGui::End();
	}
#endif

}