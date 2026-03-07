#define NOMINMAX
#include "MidBossCore.h"
#include "ModelManager.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

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

void MidBossCore::SetParentScene(TKM::BaseScene* scene) {
	parent_ = scene; // Object3d の親シーンも設定
}

void MidBossCore::SetColliderScale(const Vector3& s) {
	colliderScale_ = s; // 当たり判定のサイズを変更（エフェクトや音などがあればここで）
}

void MidBossCore::SetReticle(Reticle* r) {
	reticle_ = r; // 当たり判定の可視化にレティクルの情報を使うために保持
}

void MidBossCore::SetPlayer(std::function<Vector3()> getter) {
	playerGetter_ = std::move(getter); // プレイヤー位置取得関数を保持
}

void MidBossCore::SetHP(int hp) {
	hp_ = hp; maxHP_ = hp; // HP変化に応じたエフェクトや音などがあればここで
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
		deathTimer_ += fixedDt_; // t は 0〜1 で変化する値。1 になったら演出完了
		float t = std::min(deathTimer_ / deathDuration_, 1.0f); // 0〜1 に正規化
		// 演出内容：上にふわっと上がって縮む感じ + 回転 + 徐々に透明に
		Vector3 pos_ = object_->GetTranslate();
		Vector3 rot_ = object_->GetRotate();
		Vector3 scale_ = baseScale_;

		// シンプルに上にふわっと上がって縮む感じ
		pos_ += deathVelocity_ * fixedDt_;
		rot_.y += deathRotateSpeed_.y * fixedDt_;
		// t が 0→1 で変化する値を使って、スケールを徐々に小さくする
		float s = 1.0f - t;
		// baseScale_ に s を掛けることで、t が 0→1 でスケールが元の大きさ→0 に変化する
		scale_ = { baseScale_.x * s, baseScale_.y * s, baseScale_.z * s };
		// 変化をオブジェクトに反映
		object_->SetTranslate(pos_);
		object_->SetRotate(rot_);
		object_->SetScale(scale_);
		// t が 0→1 で変化する値を使って、徐々に透明にする
		deathAlpha_ = 1.0f - t;
		// 透明度をオブジェクトに反映（モデルのマテリアルが頂点カラーを乗算するタイプである必要あり）
		object_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });
		// 演出完了後はオブジェクトを消す
		object_->Update();

		// deathTimer_ が deathDuration_ を超えたら演出完了とみなす
		if (deathTimer_ >= deathDuration_) {
			// 消える瞬間にエフェクト
			TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
			Vector3 emitPos_ = GetWorldPosition(); // 核の位置からエフェクトを出す
			pm_->Emit("enemyDeath_core", emitPos_, 1); // 爆発の中心エフェクト
			pm_->Emit("enemyDeath_smoke", emitPos_, 4); // 煙は複数出す
			// ここでオブジェクトを完全に消す（描画も更新もしない）
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

		// ルールに従ってパーティクルを放出
		for (const auto& rule : kChargeRules_) {
			// rule.probability_ に従って、一定確率で放出するか決める
			if (rule.probability_ <= 1 || (std::rand() % rule.probability_) == 0) {
				// rule.repeat_ に従って、同フレームで複数回 Emit する
				for (int i = 0; i < rule.repeat_; ++i) {
					pm_->Emit(rule.name_, center_, rule.emitCount_); // rule.emitCount_ は、同時に放出するパーティクルの数（例：フラッシュは3つ同時に出す）
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
	hp_ -= damage; // ダメージを減算

	// ダメージを受けたときのエフェクトや音などがあればここで
	if (hp_ <= 0) {
		hp_ = 0; // HPが0以下になったら死亡状態に移行
		StartDeathReaction({ 0.0f, 0.0f, 1.0f }); // デフォルトの被弾方向（例：正面からの攻撃）で死亡リアクションを開始
	}
}

void MidBossCore::StartDeathReaction(const Vector3& hitDir) {
	if (isDying_) return;
	// 死亡演出開始
	isDying_ = true;
	deathTimer_ = 0.0f;
	deathAlpha_ = 1.0f;

	Vector3 dir_ = hitDir;
	// hitDir がほぼゼロベクトルだった場合の安全策（正面方向に飛ばす）
	if (MyMath::Length(dir_) < 0.001f) {
		dir_ = { 0.0f, 0.0f, 1.0f };
	}
	dir_ = MyMath::Normalize(dir_); // 正規化して方向ベクトルにする
	// 死亡演出のパラメータを設定（例：被弾方向に少し飛ばしつつ、上にもふわっと上がる感じ）
	deathDuration_ = 0.8f;
	deathVelocity_ = dir_ * 2.5f + Vector3{ 0.0f, 1.2f, 0.0f };
	deathRotateSpeed_ = { 0.0f, 2.0f, 0.0f };
}

void MidBossCore::SyncTransform() {
	if (!object_) return;
	object_->Update();  // 行列と定数バッファだけ更新
}