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

		Vector3 pos = object_->GetTranslate();
		Vector3 rot = object_->GetRotate();
		Vector3 scale = baseScale_;

		// シンプルに上にふわっと上がって縮む感じ
		pos += deathVelocity_ * fixedDt_;
		rot.y += deathRotateSpeed_.y * fixedDt_;

		float s = 1.0f - t;
		scale = { baseScale_.x * s, baseScale_.y * s, baseScale_.z * s };

		object_->SetTranslate(pos);
		object_->SetRotate(rot);
		object_->SetScale(scale);

		deathAlpha_ = 1.0f - t;
		object_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });

		object_->Update();

		if (deathTimer_ >= deathDuration_) {
			// 消える瞬間にエフェクト
			ParticleManager* pm = ParticleManager::GetInstance();
			Vector3 emitPos = GetWorldPosition();
			pm->Emit("enemyDeath_core", emitPos, 1);
			pm->Emit("enemyDeath_smoke", emitPos, 4);

			isDead_ = true;
		}
		return;
	}

	// 通常時（今は動かない核なのでロジックほぼ無し）
	object_->Update();

	// ---- 当たり判定の可視化（Enemy と同じ箱描画）----
	{
		Vector3 center = GetWorldPosition();
		Vector3 size = colliderScale_;

		auto* lr = LineRenderer::GetInstance();

		LineRenderer::Color normal{ 0.0f, 1.0f, 0.0f, 1.0f };
		LineRenderer::Color hit{ 1.0f, 0.0f, 0.0f, 1.0f };

		if (reticle_) {
			const Vector3 rayOrigin = playerGetter_ ? playerGetter_() : reticle_->GetCenterWorldPos();
			const Vector3 rayDir = reticle_->GetAimDirection();
			lr->AddAABBWithRayHighlight(center, size, rayOrigin, rayDir, normal, hit);
		} else {
			lr->AddAABB(center, size, normal);
		}
	}

	// ============================
// 核チャージ演出（蘇生エネルギー）
// データドリブン版（挙動そのまま）
// ============================
	{
		ParticleManager* pm = ParticleManager::GetInstance();
		Vector3 center = GetWorldPosition();

		struct EmitRule {
			const char* name;   // パーティクル名
			int emitCount;      // pm->Emit の第3引数
			int repeat;         // 同フレームで何回 Emit するか
			int probability;   // 1なら毎回、3なら1/3、5なら1/5…
		};

		static const EmitRule kChargeRules[] = {
			// 外殻：拡大球リング（1/3）
			{ "core_charge_shell",  1, 1, 3 },

			// 中心に吸い込まれる粒子（毎フレーム2回）
			{ "core_charge_inward", 1, 2, 1 },

			// ぐるぐる回る細い帯（1/5）
			{ "core_charge_ribbon", 1, 1, 5 },

			// 放電フラッシュ（1/20）
			{ "core_charge_flash",  3, 1, 20 },
		};

		for (const auto& rule : kChargeRules) {
			if (rule.probability <= 1 || (std::rand() % rule.probability) == 0) {
				for (int i = 0; i < rule.repeat; ++i) {
					pm->Emit(rule.name, center, rule.emitCount);
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

	Vector3 pos = object_->GetTranslate();
	Vector3 scale = baseScale_;
	Vector3 col = colliderScale_;

	if (ImGui::DragFloat3("位置", &pos.x, 0.01f)) {
		object_->SetTranslate(pos);
	}
	if (ImGui::DragFloat3("拡縮", &scale.x, 0.01f)) {
		SetScale(scale);
	}
	if (ImGui::DragFloat3("当たり判定サイズ", &col.x, 0.01f, 0.01f, 50.0f)) {
		SetColliderScale(col);
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

	Vector3 dir = hitDir;
	if (MyMath::Length(dir) < 0.001f) {
		dir = { 0.0f, 0.0f, 1.0f };
	}
	dir = MyMath::Normalize(dir);

	deathDuration_ = 0.8f;
	deathVelocity_ = dir * 2.5f + Vector3{ 0.0f, 1.2f, 0.0f };
	deathRotateSpeed_ = { 0.0f, 2.0f, 0.0f };
}

void MidBossCore::SyncTransform() {
	if (!object_) return;
	object_->Update();  // 行列と定数バッファだけ更新
}