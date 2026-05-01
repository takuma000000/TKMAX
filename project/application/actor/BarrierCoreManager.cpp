#include "BarrierCoreManager.h"
#include "Player.h"
#include "reticle/Reticle.h"
#include <algorithm>

void BarrierCoreManager::Initialize(
	TKM::Object3dCommon* common,
	TKM::DirectXCommon* dxCommon,
	TKM::Camera* camera,
	TKM::BaseScene* parent,
	Player* player
) {
	// Object3d共通情報を保持する
	common_ = common;

	// DirectX共通情報を保持する
	dxCommon_ = dxCommon;

	// カメラ参照を保持する
	camera_ = camera;

	// 親シーン参照を保持する
	parent_ = parent;

	// プレイヤー参照を保持する
	player_ = player;
}

void BarrierCoreManager::Update(float dt) {
	// すべてのバリアコアを順番に更新する
	for (auto it = cores_.begin(); it != cores_.end();) {
		// nullptrになっているコアは削除する
		if (!(*it)) {
			it = cores_.erase(it);
			continue;
		}

		// コア本体を更新する
		(*it)->Update(dt);

		// 死亡済みになったコアはプレイヤーへ通知してから削除する
		if ((*it)->IsDead()) {
			if (player_) {
				player_->OnBarrierCoreDestroyed((*it).get());
			}
			it = cores_.erase(it);
		} else {
			// 生きているコアは次へ進める
			++it;
		}
	}

	// 生存中コアのうち、プレイヤーが狙う対象を同期する
	SyncPlayerTarget_();
}

void BarrierCoreManager::Draw(TKM::DirectXCommon* dxCommon) {
	// 管理中のコアをすべて描画する
	for (auto& core : cores_) {
		// nullptrは描画しない
		if (!core) {
			continue;
		}

		// コアを描画する
		core->Draw(dxCommon);
	}
}

void BarrierCoreManager::Clear() {
	// 管理中のコアをすべて破棄する
	cores_.clear();

	// プレイヤー側のターゲットコア参照も外す
	if (player_) {
		player_->SetBarrierCore(nullptr);
	}
}

void BarrierCoreManager::Spawn(const Vector3& center) {
	// 既存のコアを一度消してから再生成する
	Clear();

	//=========================================================
	// コア数チェック
	// JSON側の count が0以下の場合は生成しない。
	//=========================================================
	if (config_.count_ <= 0) {
		return;
	}

	//=========================================================
	// 配置半径計算
	// バリア外周半径 + 外側余白で、コアをバリアの外側に配置する。
	//=========================================================
	const float radius_ =
		config_.placement_.barrierOuterRadius_ +
		config_.placement_.outerMargin_;

	//=========================================================
	// 角度幅計算
	// 360度をコア数で割って、円周上に等間隔で配置する。
	//=========================================================
	const float stepDeg_ = 360.0f / static_cast<float>(config_.count_);

	//=========================================================
	// コア生成
	// JSONの count 分だけ円形に並べて生成する。
	//=========================================================
	for (int i = 0; i < config_.count_; ++i) {
		// 開始角度 + 等間隔角度で、現在のコアの角度を決める
		const float angleDeg_ =
			config_.placement_.startAngleDeg_ +
			stepDeg_ * static_cast<float>(i);

		// std::sin / std::cos 用に度数法からラジアンへ変換する
		const float angleRad_ = angleDeg_ * 3.1415926535f / 180.0f;

		// 中心位置を基準に配置位置を作る
		Vector3 pos_ = center;

		// X方向へ円周配置する
		pos_.x += std::cos(angleRad_) * radius_;

		// Y方向へ円周配置する
		pos_.y += std::sin(angleRad_) * radius_;

		// Z方向はJSONの固定オフセット分だけずらす
		pos_.z += config_.placement_.zOffset_;

		// 計算した位置にコアを1つ生成する
		SpawnOne_(pos_);
	}

	// 生成後、プレイヤー側のターゲットコアを同期する
	SyncPlayerTarget_();
}

bool BarrierCoreManager::IsAllDestroyed() const {
	// 1つでも生きているコアがあれば全破壊ではない
	for (const auto& core : cores_) {
		// nullptrは無視する
		if (!core) {
			continue;
		}

		// 死亡中でも死亡済みでもないコアがあればfalse
		if (!core->IsDead() && !core->IsDying()) {
			return false;
		}
	}

	// 生存中のコアが無ければ全破壊扱い
	return true;
}

int BarrierCoreManager::GetAliveCount() const {
	// 生存中コア数
	int count_ = 0;

	// 管理中のコアを数える
	for (const auto& core : cores_) {
		// nullptrは無視する
		if (!core) {
			continue;
		}

		// 死亡中でも死亡済みでもないものを生存扱いにする
		if (!core->IsDead() && !core->IsDying()) {
			++count_;
		}
	}

	// 生存数を返す
	return count_;
}

std::vector<BarrierCore*> BarrierCoreManager::GetAliveCores() const {
	// 生存中コアのポインタ一覧を返すための配列
	std::vector<BarrierCore*> result;

	// 最大数分を先に確保して再確保を減らす
	result.reserve(cores_.size());

	// 管理中のコアを順番に確認する
	for (const auto& core : cores_) {
		// nullptrは無視する
		if (!core) {
			continue;
		}

		// 死亡済み・死亡演出中のコアは対象外にする
		if (core->IsDead() || core->IsDying()) {
			continue;
		}

		// 生存中コアとして結果に追加する
		result.push_back(core.get());
	}

	// 生存中コア一覧を返す
	return result;
}

void BarrierCoreManager::SetCamera(TKM::Camera* camera) {
	// 新しいカメラ参照を保持する
	camera_ = camera;

	// 既に生成済みのコアにもカメラを反映する
	for (auto& core : cores_) {
		// nullptrは無視する
		if (!core) {
			continue;
		}

		// コアへカメラを設定する
		core->SetCamera(camera_);
	}
}

void BarrierCoreManager::SetParentScene(TKM::BaseScene* parent) {
	// 新しい親シーン参照を保持する
	parent_ = parent;

	// 既に生成済みのコアにも親シーンを反映する
	for (auto& core : cores_) {
		// nullptrは無視する
		if (!core) {
			continue;
		}

		// コアへ親シーンを設定する
		core->SetParentScene(parent_);
	}
}

void BarrierCoreManager::SetPlayer(Player* player) {
	// 新しいプレイヤー参照を保持する
	player_ = player;

	// 既に生成済みのコアにもプレイヤー関連情報を反映する
	for (auto& core : cores_) {
		// nullptrは無視する
		if (!core) {
			continue;
		}

		// プレイヤーのレティクルをコアへ渡す
		core->SetReticle(player_ ? player_->GetReticle() : nullptr);

		// コアがプレイヤー位置を取得できるようにコールバックを設定する
		core->SetPlayer([this]() {
			return player_ ? player_->GetPosition() : Vector3{ 0.0f, 0.0f, 0.0f };
			});
	}

	// プレイヤー側のターゲットコアを同期する
	SyncPlayerTarget_();
}

//=============================================================
// 設定適用
//=============================================================
void BarrierCoreManager::SetConfig(const BarrierConfig::Core& config) {
	// JSONから読み込んだコア設定を保持する
	config_ = config;

	// 既に生成済みのコアがある場合は、そのコアにも即反映する
	for (auto& core : cores_) {
		// nullptrは無視する
		if (!core) {
			continue;
		}

		// 既存コアへJSON設定を反映する
		core->ApplyConfig(config_);
	}
}

void BarrierCoreManager::SpawnOne_(const Vector3& pos) {
	// 初期化に必要な共通情報が無ければ生成しない
	if (!common_ || !dxCommon_) {
		return;
	}

	// バリアコアを生成する
	auto core_ = std::make_unique<BarrierCore>();

	// コアを初期化する
	core_->Initialize(common_, dxCommon_);

	//=========================================================
	// JSON設定適用
	// モデル、HP、表示スケール、当たり判定、死亡演出設定をまとめて反映する。
	// ここで固定値を直接入れないことで、コア設定をJSON側で調整できるようにする。
	//=========================================================
	core_->ApplyConfig(config_);

	// カメラを設定する
	core_->SetCamera(camera_);

	// 親シーンを設定する
	core_->SetParentScene(parent_);

	// 配置位置を設定する
	core_->SetPosition(pos);

	// プレイヤーのレティクルを設定する
	core_->SetReticle(player_ ? player_->GetReticle() : nullptr);

	// プレイヤー位置取得コールバックを設定する
	core_->SetPlayer([this]() {
		return player_ ? player_->GetPosition() : Vector3{ 0.0f, 0.0f, 0.0f };
		});

	// 生成直後のTransformを同期する
	core_->SyncTransform();

	// 管理リストへ追加する
	cores_.push_back(std::move(core_));
}

BarrierCore* BarrierCoreManager::FindFirstAliveCore_() const {
	// 管理中のコアから最初に見つかった生存中コアを返す
	for (const auto& core : cores_) {
		// nullptrは無視する
		if (!core) {
			continue;
		}

		// 死亡しておらず、死亡演出中でもないコアを返す
		if (!core->IsDead() && !core->IsDying()) {
			return core.get();
		}
	}

	// 生存中コアが無ければnullptrを返す
	return nullptr;
}

void BarrierCoreManager::SyncPlayerTarget_() {
	// プレイヤーがいなければ同期しない
	if (!player_) {
		return;
	}

	// プレイヤー側へ現在狙うべき生存中コアを渡す
	player_->SetBarrierCore(FindFirstAliveCore_());
}