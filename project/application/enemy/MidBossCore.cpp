#define NOMINMAX
#include "MidBossCore.h"
#include "ModelManager.h"
#include <algorithm>

void MidBossCore::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj"); // 核用の見た目

	if (camera_) {
		object_->SetCamera(camera_);
	}

	// スケール調整
	baseScale_ = { 0.8f, 0.8f, 0.8f };
	object_->SetScale(baseScale_);
	colliderScale_ = { 1.71f, 1.71f, 1.71f };
}

void MidBossCore::SetCamera(TKM::Camera* cam) {
	camera_ = cam;
	if (object_) {
		object_->SetCamera(cam);
	}
}

void MidBossCore::SetPosition(const Vector3& pos) {
	if (!object_) return;
	object_->SetTranslate(pos);
}

Vector3 MidBossCore::GetWorldPosition() const {
	if (!object_) return {};
	return object_->GetTranslate();
}

void MidBossCore::SetScale(const Vector3& s) {
	baseScale_ = s;
	if (object_) {
		object_->SetScale(s);
	}
}

void MidBossCore::Update(float dt) {
	if (!object_) return;

	// 死亡演出中
	if (isDying_) {
		deathTimer_ += fixedDt_;
		float t = std::min(deathTimer_ / deathDuration_, 1.0f);

		Vector3 pos_ = object_->GetTranslate();
		Vector3 rot_ = object_->GetRotate();
		Vector3 scale_ = baseScale_;

		// シンプルに上にふわっと上がって縮む感じ
		pos_ += deathVelocity_ * fixedDt_;
		rot_.y += deathRotateSpeed_.y * fixedDt_;

		float s = 1.0f - t;
		scale_ = { baseScale_.x * s, baseScale_.y * s, baseScale_.z * s };

		object_->SetTranslate(pos_);
		object_->SetRotate(rot_);
		object_->SetScale(scale_);

		deathAlpha_ = 1.0f - t;
		object_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });

		object_->Update();

		if (deathTimer_ >= deathDuration_) {
			// 消える瞬間にエフェクト
			TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
			Vector3 emitPos_ = GetWorldPosition();
			pm_->Emit("enemyDeath_core", emitPos_, 1);
			pm_->Emit("enemyDeath_smoke", emitPos_, 4);

			isDead_ = true;
		}
		return;
	}

	// 通常時（今は動かない核なのでロジックほぼ無し）
	object_->Update();

#ifdef USE_IMGUI
	// ---- 当たり判定の可視化（Enemy と同じ箱描画）----
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

	// ============================
	// 核チャージ演出（蘇生エネルギー）
	// データドリブン版（挙動そのまま）
	// ============================
	{
		TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
		Vector3 center_ = GetWorldPosition();

		struct EmitRule {
			const char* name_;   // パーティクル名
			int emitCount_;      // pm->Emit の第3引数
			int repeat_;         // 同フレームで何回 Emit するか
			int probability_;   // 1なら毎回、3なら1/3、5なら1/5…
		};

		static const EmitRule kChargeRules_[] = {
			// 外殻：拡大球リング（1/3）
			{ "core_charge_shell",  1, 1, 3 },

			// 中心に吸い込まれる粒子（毎フレーム2回）
			{ "core_charge_inward", 1, 2, 1 },

			// ぐるぐる回る細い帯（1/5）
			{ "core_charge_ribbon", 1, 1, 5 },

			// 放電フラッシュ（1/20）
			{ "core_charge_flash",  3, 1, 20 },
		};

		for (const auto& rule : kChargeRules_) {
			if (rule.probability_ <= 1 || (std::rand() % rule.probability_) == 0) {
				for (int i = 0; i < rule.repeat_; ++i) {
					pm_->Emit(rule.name_, center_, rule.emitCount_);
				}
			}
		}
	}
}

void MidBossCore::Draw(TKM::DirectXCommon* dxCommon) {
	if (!object_) return;
	object_->Draw(dxCommon);
}

void MidBossCore::ImGuiDebug() {
#ifdef USE_IMGUI
	if (!object_) return;

	ImGui::Begin("蘇生コア");

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

void MidBossCore::OnHitWithDamage(int damage) {
	if (isDead_ || isDying_) return;
	hp_ -= damage;
	if (hp_ <= 0) {
		hp_ = 0;
		StartDeathReaction({ 0.0f, 0.0f, 1.0f });
	}
}

void MidBossCore::StartDeathReaction(const Vector3& hitDir) {
	if (isDying_) return;

	isDying_ = true;
	deathTimer_ = 0.0f;
	deathAlpha_ = 1.0f;

	Vector3 dir_ = hitDir;
	if (MyMath::Length(dir_) < 0.001f) {
		dir_ = { 0.0f, 0.0f, 1.0f };
	}
	dir_ = MyMath::Normalize(dir_);

	deathDuration_ = 0.8f;
	deathVelocity_ = dir_ * 2.5f + Vector3{ 0.0f, 1.2f, 0.0f };
	deathRotateSpeed_ = { 0.0f, 2.0f, 0.0f };
}

void MidBossCore::SyncTransform() {
	if (!object_) return;
	object_->Update();  // 行列と定数バッファだけ更新
}