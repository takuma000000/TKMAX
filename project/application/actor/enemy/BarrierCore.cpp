#define NOMINMAX
#include "BarrierCore.h"
#include "ModelManager.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

//=============================================================
// 初期化
//=============================================================
void BarrierCore::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	//=========================================================
	// Object3d生成・初期化
	//=========================================================
	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon);

	// カメラが設定済みなら反映
	if (camera_) {
		object_->SetCamera(camera_);
	}

	// 設定適用
	if (config_) {
		ApplyConfig(*config_);
	}
}

//=============================================================
// カメラ設定
//=============================================================
void BarrierCore::SetCamera(TKM::Camera* cam) {
	camera_ = cam;
	if (object_) {
		object_->SetCamera(cam);
	}
}

//=============================================================
// 親シーン設定
//=============================================================
void BarrierCore::SetParentScene(TKM::BaseScene* scene) {
	parent_ = scene; // Object3d の親シーンも保持
}

//=============================================================
// 当たり判定サイズ設定
//=============================================================
void BarrierCore::SetColliderScale(const Vector3& s) {
	colliderScale_ = s; // 当たり判定サイズ変更
}

//=============================================================
// レティクル設定
//=============================================================
void BarrierCore::SetReticle(Reticle* r) {
	reticle_ = r; // 当たり判定可視化用に保持
}

//=============================================================
// プレイヤー位置取得関数設定
//=============================================================
void BarrierCore::SetPlayer(std::function<Vector3()> getter) {
	playerGetter_ = std::move(getter);
}

//=============================================================
// HP設定
//=============================================================
void BarrierCore::SetHP(int hp) {
	hp_ = hp;
	maxHP_ = hp;
}

//=============================================================
// 位置設定
//=============================================================
void BarrierCore::SetPosition(const Vector3& pos) {
	if (!object_) {
		return;
	}
	object_->SetTranslate(pos);
}

//=============================================================
// ワールド位置取得
//=============================================================
Vector3 BarrierCore::GetWorldPosition() const {
	if (!object_) {
		return {};
	}
	return object_->GetTranslate();
}

//=============================================================
// スケール設定
//=============================================================
void BarrierCore::SetScale(const Vector3& s) {
	baseScale_ = s;
	if (object_) {
		object_->SetScale(s);
	}
}

//=============================================================
// 更新
//=============================================================
void BarrierCore::Update(float dt) {
	if (!object_) {
		return;
	}

	time_ += dt;

	//=========================================================
	// 死亡演出中
	//=========================================================
	if (isDying_) {
		deathTimer_ += fixedDt_;

		// 0.0 ～ 1.0 の進行率
		float t = std::min(deathTimer_ / deathDuration_, 1.0f);

		Vector3 pos_ = object_->GetTranslate();
		Vector3 rot_ = object_->GetRotate();
		Vector3 scale_ = baseScale_;

		//=====================================================
		// 演出内容
		// 上にふわっと上がりつつ縮小し、回転しながら消えていく
		//=====================================================
		pos_ += deathVelocity_ * fixedDt_;
		rot_.y += deathRotateSpeed_.y * fixedDt_;

		float s = 1.0f - t;
		scale_ = {
			baseScale_.x * s,
			baseScale_.y * s,
			baseScale_.z * s
		};

		//=====================================================
		// オブジェクトへ反映
		//=====================================================
		object_->SetTranslate(pos_);
		object_->SetRotate(rot_);
		object_->SetScale(scale_);

		// 徐々に透明化
		deathAlpha_ = 1.0f - t;
		object_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });

		object_->Update();

		//=====================================================
		// 演出完了
		//=====================================================
		if (deathTimer_ >= deathDuration_) {
			TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
			Vector3 emitPos_ = GetWorldPosition();

			pm_->Emit("enemyDeath_core", emitPos_, 1);
			pm_->Emit("enemyDeath_smoke", emitPos_, 4);

			// 完全に消す
			isDead_ = true;
		}
		return;
	}

	//=========================================================
	// 常時Y軸回転
	//=========================================================
	{
		Vector3 rot_ = object_->GetRotate();
		const float kRotateSpeedY_ = -2.0f; // ラジアン/秒

		rot_.y += kRotateSpeedY_ * dt;
		object_->SetRotate(rot_);
	}

	//=========================================================
	// 脈動（ドクンっ）
	// 一瞬で膨らみ、すぐ戻って、少し止まる
	//=========================================================
	{
		const float kBeatCycle_ = 0.85f;     // 1拍の周期
		const float kBeatAmplitude_ = 0.22f; // 膨らみ量
		const float kAttackTime_ = 0.06f;    // 一気に膨らむ時間
		const float kReleaseTime_ = 0.08f;   // 戻る時間

		float phase = std::fmod(time_, kBeatCycle_);
		float pulseAdd = 0.0f;

		if (phase < kAttackTime_) {
			// 一瞬で膨らむ
			float t = phase / kAttackTime_;
			pulseAdd = kBeatAmplitude_ * t;
		} else if (phase < (kAttackTime_ + kReleaseTime_)) {
			// すぐ戻る
			float t = (phase - kAttackTime_) / kReleaseTime_;
			float inv = 1.0f - t;
			pulseAdd = kBeatAmplitude_ * (inv * inv);
		}

		float pulse = 1.0f + pulseAdd;

		Vector3 scale_;
		scale_.x = baseScale_.x * pulse;
		scale_.y = baseScale_.y * pulse;
		scale_.z = baseScale_.z * pulse;

		object_->SetScale(scale_);
	}

	object_->Update();

#ifdef USE_IMGUI
	//=========================================================
	// 当たり判定可視化
	// Enemy と同じ箱描画方式
	//=========================================================
	{
		Vector3 center_ = GetWorldPosition();
		Vector3 size_ = colliderScale_;

		auto* lr = TKM::LineRenderer::GetInstance();

		TKM::LineRenderer::Color normal_{ 0.0f, 1.0f, 0.0f, 1.0f };
		TKM::LineRenderer::Color hit_{ 1.0f, 0.0f, 0.0f, 1.0f };

		if (reticle_) {
			const Vector3 rayOrigin_ = playerGetter_ ? playerGetter_() : reticle_->GetCenterWorldPos();
			const Vector3 rayDir_ = reticle_->GetAimDirection();
			lr->AddAABBWithRayHighlight(center_, size_, rayOrigin_, rayDir_, normal_, hit_);
		} else {
			lr->AddAABB(center_, size_, normal_);
		}
	}
#endif

	//=========================================================
	// 核チャージ演出（蘇生エネルギー）
	// データドリブン版
	//=========================================================
	{
		TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
		Vector3 center_ = GetWorldPosition();

		struct EmitRule {
			const char* name_;   // パーティクル名
			int emitCount_;      // Emit の第3引数
			int repeat_;         // 同フレームで何回 Emit するか
			int probability_;    // 1なら毎回、3なら1/3、5なら1/5
		};

		static const EmitRule kChargeRules_[] = {
			// 外殻：拡大球リング
			{ "core_charge_shell",  1, 1, 3 },

			// 中心へ吸い込まれる粒子
			{ "core_charge_inward", 1, 2, 1 },

			// ぐるぐる回る細帯
			{ "core_charge_ribbon", 1, 1, 5 },

			// 放電フラッシュ
			{ "core_charge_flash",  3, 1, 20 },
		};

		// ルールに従ってパーティクル放出
		for (const auto& rule : kChargeRules_) {
			if (rule.probability_ <= 1 || (std::rand() % rule.probability_) == 0) {
				for (int i = 0; i < rule.repeat_; ++i) {
					pm_->Emit(rule.name_, center_, rule.emitCount_);
				}
			}
		}
	}
}

//=============================================================
// 描画
//=============================================================
void BarrierCore::Draw(TKM::DirectXCommon* dxCommon) {
	if (!object_) {
		return;
	}
	object_->Draw(dxCommon);
}

//=============================================================
// ImGuiデバッグ表示
//=============================================================
void BarrierCore::ImGuiDebug() {
#ifdef USE_IMGUI
	if (!object_) {
		return;
	}

	ImGui::Begin("バリアコア");

	Vector3 pos_ = object_->GetTranslate();
	Vector3 scale_ = baseScale_;
	Vector3 col_ = colliderScale_;

	if (ImGui::DragFloat3("位置", &pos_.x, 0.01f)) {
		object_->SetTranslate(pos_);
	}
	if (ImGui::DragFloat3("拡縮", &scale_.x, 0.01f)) {
		SetScale(scale_);
	}
	if (ImGui::DragFloat3("当たり判定サイズ", &col_.x, 0.01f, 0.01f, 50.0f)) {
		SetColliderScale(col_);
	}

	ImGui::Text("HP: %d / %d", hp_, maxHP_);
	ImGui::Text("状態: %s", isDead_ ? "死" : (isDying_ ? "死亡演出中" : "生"));

	ImGui::End();
#endif
}

//=============================================================
// 被弾ダメージ処理
//=============================================================
void BarrierCore::OnHitWithDamage(int damage) {
	if (isDead_ || isDying_) {
		return;
	}

	hp_ -= damage;

	// HPが尽きたら死亡リアクション開始
	if (hp_ <= 0) {
		hp_ = 0;
		StartDeathReaction({ 0.0f, 0.0f, 1.0f });
	}
}

//=============================================================
// 死亡リアクション開始
//=============================================================
void BarrierCore::StartDeathReaction(const Vector3& hitDir) {
	if (isDying_) {
		return;
	}

	isDying_ = true;
	deathTimer_ = 0.0f;
	deathAlpha_ = 1.0f;

	Vector3 dir_ = hitDir;

	// ゼロベクトル対策
	if (MyMath::Length(dir_) < 0.001f) {
		dir_ = { 0.0f, 0.0f, 1.0f };
	}

	dir_ = MyMath::Normalize(dir_);

	//=========================================================
	// 死亡演出パラメータ設定
	// JSONで読み込んだ死亡演出設定を使う。
	// 被弾方向への押し出し速度と、上方向への浮き上がり速度を合成する。
	//=========================================================
	if (config_) {
		deathDuration_ = config_->death_.duration_;
		deathVelocity_ = dir_ * config_->death_.hitDirSpeed_ + config_->death_.upVelocity_;
		deathRotateSpeed_ = config_->death_.rotateSpeed_;
	} else {
		// JSON設定がまだ適用されていない場合の保険。
		// 既存の挙動と同じ値を入れておく。
		deathDuration_ = 0.8f;
		deathVelocity_ = dir_ * 2.5f + Vector3{ 0.0f, 1.2f, 0.0f };
		deathRotateSpeed_ = { 0.0f, 2.0f, 0.0f };
	}
}

//=============================================================
// Transform同期
//=============================================================
void BarrierCore::SyncTransform() {
	if (!object_) {
		return;
	}
	object_->Update(); // 行列と定数バッファのみ更新
}

//=============================================================
// 設定適用
//=============================================================
void BarrierCore::ApplyConfig(const BarrierConfig::Core& config) {

	//=========================================================
	// 設定参照保持
	// 死亡リアクション開始時にも同じ設定を参照するため、
	// 渡された設定のアドレスを保持しておく。
	//=========================================================
	config_ = &config;

	//=========================================================
	// Object3d未生成対策
	// Initialize前に呼ばれた場合は、参照だけ保持して処理を抜ける。
	// Initialize内で object_ が生成された後、再度 ApplyConfig が呼ばれる。
	//=========================================================
	if (!object_) {
		return;
	}

	//=========================================================
	// モデル設定
	// JSONの model で指定されたモデルを使用する。
	//=========================================================
	object_->SetModel(config.model_);

	//=========================================================
	// 表示スケール設定
	// JSONの scale を基準スケールとして保存し、Object3dにも反映する。
	// 死亡時の縮小演出もこの baseScale_ を基準に行う。
	//=========================================================
	baseScale_ = config.scale_;
	object_->SetScale(baseScale_);

	//=========================================================
	// 当たり判定サイズ設定
	// JSONの colliderScale を当たり判定用サイズとして使う。
	//=========================================================
	colliderScale_ = config.colliderScale_;

	//=========================================================
	// HP設定
	// 現在HPと最大HPをJSONの hp で揃える。
	//=========================================================
	hp_ = config.hp_;
	maxHP_ = config.hp_;

	//=========================================================
	// 死亡演出初期値
	// 実際の被弾方向は StartDeathReaction で決まるため、
	// ここでは時間・上昇速度・回転速度の初期値だけ反映する。
	//=========================================================
	deathDuration_ = config.death_.duration_;
	deathVelocity_ = config.death_.upVelocity_;
	deathRotateSpeed_ = config.death_.rotateSpeed_;
}