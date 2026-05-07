#include "IntroBossActor.h"
#include <algorithm>
#include <cstdlib>

namespace TKM {

	void IntroBossActor::Initialize(TKM::Object3dCommon* object3dCommon, DirectXCommon* dxCommon) {
		// Object3d共通情報を保持する
		object3dCommon_ = object3dCommon;

		// DirectX共通情報を保持する
		dxCommon_ = dxCommon;

		// 内部状態を初期化する
		Reset();
	}

	void IntroBossActor::Reset() {
		// 生成済みボスを破棄する
		boss_.reset();

		// 現在フェーズの経過時間を初期化する
		phaseElapsed_ = 0.0f;

		// ボスの初期位置を設定する
		pos_ = { 0.0f, 6.0f, appearStartZ_ };

		// 各フェーズで基準にする位置を保存する
		basePos_ = pos_;

		// 逃走時に向かうX座標を初期化する
		escapeTargetX_ = 0.0f;

		// 逃走時の目標変更タイマーを初期化する
		escapeTargetTimer_ = 0.0f;

		// 出現前フェーズの経過時間を初期化する
		preSpawnElapsed_ = 0.0f;

		// 出現前エフェクト発生用の蓄積時間を初期化する
		preSpawnEmitAccum_ = 0.0f;

		// 出現エフェクト完了フラグを初期化する
		spawnFxFinished_ = false;

		// 注意マーク発生済みフラグを初期化する
		noticeMarkEmitted_ = false;

		// 逃走ワープバースト発生済みフラグを初期化する
		escapeWarpBurstEmitted_ = false;
	}

	void IntroBossActor::BeginPreSpawn() {
		// 出現前フェーズの経過時間をリセットする
		preSpawnElapsed_ = 0.0f;

		// 出現前エフェクト発生用の蓄積時間をリセットする
		preSpawnEmitAccum_ = 0.0f;

		// 出現エフェクト完了フラグをリセットする
		spawnFxFinished_ = false;

		// 逃走ワープバースト発生済みフラグをリセットする
		escapeWarpBurstEmitted_ = false;

		// ボスの内部位置を出現開始位置へ戻す
		pos_ = { 0.0f, 6.0f, appearStartZ_ };

		// 基準位置も出現開始位置へ戻す
		basePos_ = pos_;
	}

	void IntroBossActor::UpdatePreSpawn(float dt) {
		// 出現前フェーズの経過時間を進める
		preSpawnElapsed_ += dt;

		// 出現前エフェクト発生用の蓄積時間を進める
		preSpawnEmitAccum_ += dt;
	}

	bool IntroBossActor::IsPreSpawnFinished() const {
		// 出現前フェーズが一定時間を超えたら終了扱いにする
		return preSpawnElapsed_ >= 1.8f;
	}

	void IntroBossActor::Spawn(Camera* camera) {
		// すでに生成済み、または必要な情報が無ければ生成しない
		if (boss_ || !object3dCommon_ || !dxCommon_ || !camera) { return; }

		// ボス本体を生成する
		boss_ = std::make_unique<BossEnemy>();

		// ボス本体を初期化する
		boss_->Initialize(object3dCommon_, dxCommon_);

		// ボスにカメラを設定する
		boss_->SetCamera(camera);

		// ボスを出現開始位置へ配置する
		boss_->SetPosition({ 0.0f, 6.0f, appearStartZ_ });

		// ボスの向きをこちら向きに設定する
		boss_->SetRotate({ 0.0f, 3.14159265f, 0.0f });

		// 演出中にロック状態として扱う
		boss_->SetLocked(true);

		// 初期Transformを同期する
		boss_->SyncTransform();

		// 内部位置も出現開始位置へ合わせる
		pos_ = { 0.0f, 6.0f, appearStartZ_ };

		// 基準位置も合わせる
		basePos_ = pos_;

		// フェーズ経過時間をリセットする
		phaseElapsed_ = 0.0f;
	}

	void IntroBossActor::BeginAppear() {
		// 出現フェーズの経過時間をリセットする
		phaseElapsed_ = 0.0f;
	}

	bool IntroBossActor::UpdateAppear(float dt) {
		// ボス未生成なら更新しない
		if (!boss_) { return false; }

		// 出現フェーズの経過時間を進める
		phaseElapsed_ += dt;

		// 出現フェーズの進行率を0.0～1.0に収める
		float t = std::clamp(phaseElapsed_ / appearSec_, 0.0f, 1.0f);

		// 移動用の進行率を作る
		float moveT = t;

		// SmoothStepで動きをなめらかにする
		moveT = moveT * moveT * (3.0f - 2.0f * moveT);

		// Z方向を出現開始位置から終了位置へ補間する
		float z = MyMath::Lerp(appearStartZ_, appearEndZ_, moveT);

		// 横方向のふらつきを作る
		float floatX = std::sinf(phaseElapsed_ * appearFloatFreqX_) * appearFloatAmpX_;

		// 上下方向のメイン揺れを作る
		float floatYMain = std::sinf(phaseElapsed_ * appearFloatFreqY_) * appearFloatAmpY_;

		// 上下方向の細かいサブ揺れを作る
		float floatYSub =
			std::sinf(phaseElapsed_ * (appearFloatFreqY_ * 2.15f) + 0.8f) *
			(appearFloatAmpY_ * 0.38f);

		// 上下揺れを合成する
		float floatY = floatYMain + floatYSub;

		// 出現完了に近づくほど揺れを抑える
		float damp = MyMath::Lerp(1.0f, 0.45f, moveT);

		// Z位置を反映する
		pos_.z = z;

		// 横揺れを減衰込みで反映する
		pos_.x = floatX * damp;

		// 全体の漂いを追加する
		float bodyDrift = std::sinf(phaseElapsed_ * 0.95f + 1.2f) * 0.9f;

		// Y位置へ漂いと上下揺れを反映する
		pos_.y = 6.0f + bodyDrift + floatY * damp;

		// Z軸回転の傾き揺れを作る
		float rotZ =
			std::sinf(phaseElapsed_ * 2.2f) * appearTiltZ_ * damp +
			std::sinf(phaseElapsed_ * 4.6f + 0.5f) * (appearTiltZ_ * 0.35f) * damp;

		// ボス位置を反映する
		boss_->SetPosition(pos_);

		// ボス回転を反映する
		boss_->SetRotate({ 0.0f, 3.14159265f, rotZ });

		// 出現中はパニック表情を無効にする
		boss_->SetIntroPanic(false, 0.0f);

		// ボス本体を更新する
		boss_->Update(dt);

		// 出現フェーズが完了したら次フェーズへ進める状態にする
		if (t >= 1.0f) {
			phaseElapsed_ = 0.0f;
			basePos_ = pos_;
			return true;
		}

		// まだフェーズ継続
		return false;
	}

	void IntroBossActor::BeginPause() {
		// 停止フェーズの経過時間をリセットする
		phaseElapsed_ = 0.0f;
	}

	bool IntroBossActor::UpdatePause(float dt) {
		// ボス未生成なら更新しない
		if (!boss_) { return false; }

		// 停止フェーズの経過時間を進める
		phaseElapsed_ += dt;

		// 停止フェーズの進行率を0.0～1.0に収める
		float t = std::clamp(phaseElapsed_ / pauseSec_, 0.0f, 1.0f);

		// 基準位置へ戻す
		pos_ = basePos_;

		// 基準位置をボスに反映する
		boss_->SetPosition(pos_);

		// 傾き無しの正面向きに戻す
		boss_->SetRotate({ 0.0f, 3.14159265f, 0.0f });

		// 待機中の小さい上下揺れを作る
		float idleY = std::sinf(phaseElapsed_ * 5.0f) * 0.10f;

		// 待機中の上下揺れを反映する
		boss_->SetPosition({ pos_.x, pos_.y + idleY, pos_.z });

		// 停止中はパニック表情を無効にする
		boss_->SetIntroPanic(false, 0.0f);

		// ボス本体を更新する
		boss_->Update(dt);

		// 停止フェーズが終わったら次フェーズへ進める状態にする
		if (t >= 1.0f) {
			phaseElapsed_ = 0.0f;
			basePos_ = { pos_.x, pos_.y + idleY, pos_.z };
			return true;
		}

		// まだフェーズ継続
		return false;
	}

	void IntroBossActor::BeginNoticeHop() {
		// 驚きジャンプフェーズの経過時間をリセットする
		phaseElapsed_ = 0.0f;

		// 注意マーク発生済みフラグをリセットする
		noticeMarkEmitted_ = false;
	}

	bool IntroBossActor::UpdateNoticeHop(float dt) {
		// ボス未生成なら更新しない
		if (!boss_) { return false; }

		// 驚きジャンプフェーズの経過時間を進める
		phaseElapsed_ += dt;

		// 驚きジャンプフェーズの進行率を0.0～1.0に収める
		float t = std::clamp(phaseElapsed_ / noticeHopSec_, 0.0f, 1.0f);

		// ジャンプ用の進行率を初期化する
		float hopT = 0.0f;

		// 少し間を置いてからジャンプを始める
		if (t >= 0.20f) {
			hopT = (t - 0.20f) / 0.80f;
			if (hopT > 1.0f) { hopT = 1.0f; }
		}

		// サイン波で上がって下がるジャンプ形状を作る
		float hop = std::sinf(hopT * 3.14159265f);

		// ジャンプのY移動量を作る
		float hopY = hop * noticeHopY_;

		// 驚き時の横揺れを作る
		float surpriseX = std::sinf(t * 3.14159265f) * 0.35f;

		// 基準位置から驚き位置を作る
		Vector3 pos = basePos_;

		// 横揺れを反映する
		pos.x += surpriseX;

		// ジャンプ量を反映する
		pos.y += hopY;

		// 驚き時のZ軸傾きを作る
		float rotZ = std::sinf(t * 3.14159265f) * 0.12f;

		// 内部位置を更新する
		pos_ = pos;

		// ボス位置を反映する
		boss_->SetPosition(pos);

		// ボス回転を反映する
		boss_->SetRotate({ 0.0f, 3.14159265f, rotZ });

		// 最初の間はまだパニック表情にしない
		if (t < 0.20f) {
			boss_->SetIntroPanic(false, 0.0f);
		} else {
			// ジャンプ開始後はパニック表情にする
			boss_->SetIntroPanic(true, 0.85f);
		}

		// ボス本体を更新する
		boss_->Update(dt);

		// 驚きジャンプフェーズが完了したら次フェーズへ進める状態にする
		if (t >= 1.0f) {
			boss_->SetIntroPanic(true, 1.0f);
			phaseElapsed_ = 0.0f;
			basePos_ = pos_;
			noticeMarkEmitted_ = false;
			return true;
		}

		// まだフェーズ継続
		return false;
	}

	void IntroBossActor::BeginPanic() {
		// パニックフェーズの経過時間をリセットする
		phaseElapsed_ = 0.0f;
	}

	bool IntroBossActor::UpdatePanic(float dt) {
		// ボス未生成なら更新しない
		if (!boss_) { return false; }

		// パニックフェーズの経過時間を進める
		phaseElapsed_ += dt;

		// パニックフェーズの進行率を0.0～1.0に収める
		float t = std::clamp(phaseElapsed_ / panicSec_, 0.0f, 1.0f);

		// 時間が進むほど大きくなる横揺れを作る
		float shakeX = std::sinf(phaseElapsed_ * 12.0f) * panicAmpX_ * (0.30f + t * 0.70f);

		// 上方向に跳ねるような揺れを作る
		float shakeY = std::fabs(std::sinf(phaseElapsed_ * 15.0f)) * panicAmpY_;

		// 慌てているようなZ軸回転を作る
		float wobbleRotZ = std::sinf(phaseElapsed_ * 13.0f) * 0.14f;

		// 基準位置から横揺れを反映する
		pos_.x = basePos_.x + shakeX;

		// 基準位置から縦揺れを反映する
		pos_.y = basePos_.y + shakeY;

		// Z位置は基準位置のままにする
		pos_.z = basePos_.z;

		// ボス位置を反映する
		boss_->SetPosition(pos_);

		// ボス回転を反映する
		boss_->SetRotate({ 0.0f, 3.14159265f, wobbleRotZ });

		// パニック表情の強さを時間に応じて上げる
		boss_->SetIntroPanic(true, 0.55f + t * 0.45f);

		// ボス本体を更新する
		boss_->Update(dt);

		// パニックフェーズが完了したら逃走フェーズへ進める状態にする
		if (t >= 1.0f) {
			phaseElapsed_ = 0.0f;
			escapeTargetX_ = pos_.x;
			escapeTargetTimer_ = 0.0f;
			return true;
		}

		// まだフェーズ継続
		return false;
	}

	void IntroBossActor::BeginEscape() {
		// 逃走フェーズの経過時間をリセットする
		phaseElapsed_ = 0.0f;

		// 逃走中の左右目標変更タイマーをリセットする
		escapeTargetTimer_ = 0.0f;
	}

	bool IntroBossActor::UpdateEscape(float dt) {
		// ボス未生成なら更新しない
		if (!boss_) { return false; }

		// 逃走フェーズの経過時間を進める
		phaseElapsed_ += dt;

		// 左右目標変更タイマーを進める
		escapeTargetTimer_ += dt;

		// 逃走フェーズの進行率を0.0～1.0に収める
		float t = std::clamp(phaseElapsed_ / escapeSec_, 0.0f, 1.0f);

		// 一定間隔で左右の逃走目標Xをランダムに変える
		if (escapeTargetTimer_ >= escapeTargetInterval_) {
			escapeTargetTimer_ = 0.0f;

			// 左右どちらへ動くかをランダムに決める
			float sign = (std::rand() % 2 == 0) ? -1.0f : 1.0f;

			// 後半ほど揺れ幅を大きくする
			float ampGrow = MyMath::Lerp(0.65f, 1.25f, t);

			// ランダムな移動幅を作る
			float mag = 2.0f + (static_cast<float>(std::rand()) / RAND_MAX) * escapeAmpX_ * ampGrow;

			// 次に向かうX座標を設定する
			escapeTargetX_ = sign * mag;
		}

		// X方向を目標へなめらかに近づけるための追従率
		float follow = 16.0f * dt;

		// X方向はランダム目標へ補間する
		pos_.x = MyMath::Lerp(pos_.x, escapeTargetX_, follow);

		// Z方向は奥へ逃げるように進める
		pos_.z += escapeSpeedZ_ * dt * (1.0f + t * 0.30f);

		// Y方向は慌てて跳ねるように上下させる
		pos_.y = 6.0f + std::fabs(std::sinf(phaseElapsed_ * 14.0f)) * escapeHopY_;

		// 左右へふらつく傾きを作る
		float leanZ = std::sinf(phaseElapsed_ * 16.0f) * 0.22f;

		// ボス位置を反映する
		boss_->SetPosition(pos_);

		// ボス回転を反映する
		boss_->SetRotate({ 0.0f, 3.14159265f + std::sinf(phaseElapsed_ * 8.0f) * 0.10f, leanZ });

		// 逃走中は最大パニック表情にする
		boss_->SetIntroPanic(true, 1.0f);

		// ボス本体を更新する
		boss_->Update(dt);

		// 十分奥へ逃げた、または時間切れなら逃走完了
		if (pos_.z >= escapeEndZ_ || t >= 1.0f) {
			boss_->SetIntroPanic(false, 0.0f);
			return true;
		}

		// まだフェーズ継続
		return false;
	}

	void IntroBossActor::Draw(DirectXCommon* dxCommon) const {
		// ボスが存在する場合だけ描画する
		if (boss_) {
			boss_->Draw(dxCommon);
		}
	}

	float IntroBossActor::GetAppearRatio() const {
		// 出現フェーズの進行率を返す
		return std::clamp(phaseElapsed_ / appearSec_, 0.0f, 1.0f);
	}

	float IntroBossActor::GetPauseRatio() const {
		// 停止フェーズの進行率を返す
		return std::clamp(phaseElapsed_ / pauseSec_, 0.0f, 1.0f);
	}

	float IntroBossActor::GetNoticeHopRatio() const {
		// 驚きジャンプフェーズの進行率を返す
		return std::clamp(phaseElapsed_ / noticeHopSec_, 0.0f, 1.0f);
	}

	float IntroBossActor::GetPanicRatio() const {
		// パニックフェーズの進行率を返す
		return std::clamp(phaseElapsed_ / panicSec_, 0.0f, 1.0f);
	}

	float IntroBossActor::GetEscapeRatio() const {
		// 逃走フェーズの進行率を返す
		return std::clamp(phaseElapsed_ / escapeSec_, 0.0f, 1.0f);
	}

}