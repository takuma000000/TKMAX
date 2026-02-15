#define NOMINMAX
#include "Enemy.h"
#include "ModelManager.h"
#include <algorithm>
#include <cstdlib> 
#include <AABB.h>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void Enemy::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon);

	// 触手
	tentacle_ = std::make_unique<TKM::Object3d>();
	tentacle_->Initialize(common, dxCommon);

	// カメラ設定（既存に合わせる）
	if (camera_) {
		object_->SetCamera(camera_);
		tentacle_->SetCamera(camera_);
	}

	// 親子付け：触手を傘の子にする
	tentacle_->SetParent(object_.get());

	// 触手のローカル（傘からの相対）初期値
	tentacle_->SetTranslate(tentacleLocalPos_);
	tentacle_->SetRotate(tentacleLocalRot_);
	tentacle_->SetScale(tentacleLocalScale_);

	baseScale_ = object_->GetScale();
	startX_ = object_->GetTranslate().x;
}

void Enemy::Update(float dt) {

	// 60fps基準の値をそのまま使えるようにする係数
	// dt=1/60 のとき factor=1.0 になる
	const float factor_ = dt * 60.0f;

	// =========================================================
	// Data-driven：死亡リアクション / 行動 の関数テーブル
	// （switch を排除）
	// =========================================================
	struct DeathCtx {
		float dt_;
		float t_; // 0..1
		Vector3 pos_;
		Vector3 rot_;
		Vector3 scale_;
	};

	struct MoveCtx {
		float dt_;
		float factor_;
		Vector3 pos_;
	};

	struct Local {

		// ---------- Death reactions ----------
		static void Death_BlowAway(Enemy* self, DeathCtx& c) {
			float speed_ = 1.0f - c.t_;
			c.pos_ += self->deathVelocity_ * speed_ * c.dt_;

			c.rot_.x += self->deathRotateSpeed_.x * c.dt_;
			c.rot_.y += self->deathRotateSpeed_.y * c.dt_;
			c.rot_.z += self->deathRotateSpeed_.z * c.dt_;

			float s_ = 1.0f - c.t_;
			c.scale_ = { self->baseScale_.x * s_, self->baseScale_.y * s_, self->baseScale_.z * s_ };
		}

		static void Death_RiseAbsorb(Enemy* self, DeathCtx& c) {
			c.pos_ += self->deathVelocity_ * c.dt_;

			c.rot_.y += self->deathRotateSpeed_.y * c.dt_;

			float s = 1.0f - c.t_;
			c.scale_ = {
				self->baseScale_.x * s * 0.5f,
				self->baseScale_.y * (1.0f - c.t_ * 0.2f),
				self->baseScale_.z * s * 0.5f
			};
		}

		static void Death_Collapse(Enemy* self, DeathCtx& c) {
			c.pos_ += self->deathVelocity_ * c.dt_;

			c.rot_.x += self->deathRotateSpeed_.x * c.dt_;

			float s_ = 1.0f - c.t_;
			c.scale_ = {
				self->baseScale_.x,
				self->baseScale_.y * s_ * 0.2f,
				self->baseScale_.z
			};
		}

		static void Death_BossFinal(Enemy* self, DeathCtx& c) {
			const float launchStartT_ = 0.5f;

			if (c.t_ < launchStartT_) {
				float shakeAmp_ = 0.25f;
				float shakeFreq_ = 18.0f;

				c.pos_.x += sinf(self->deathTimer_ * shakeFreq_) * shakeAmp_;
				c.pos_.y += cosf(self->deathTimer_ * shakeFreq_ * 0.7f) * shakeAmp_ * 0.6f;

				float pulse_ = 1.0f + 0.10f * sinf(self->deathTimer_ * 10.0f);
				c.scale_ = {
					self->baseScale_.x * pulse_,
					self->baseScale_.y * pulse_,
					self->baseScale_.z * pulse_,
				};

				TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
				if (std::rand() % 3 != 0) {
					Vector3 center_ = self->GetWorldPosition();
					Vector3 off_ = {
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.x,
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.y,
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.z
					};
					Vector3 emitPos_ = center_ + off_ * 0.5f;
					pm_->Emit("bossDeath_bomb", emitPos_, 1);
				}

			} else {

				if (!self->bossFinalLaunchStarted_) {
					self->bossFinalLaunchStarted_ = true;
					self->bossFinalLaunchStartPos_ = c.pos_;

					TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
					Vector3 center_ = self->GetWorldPosition();
					pm_->Emit("bossDeath_ring", center_, 2);
					pm_->Emit("bossDeath_bomb", center_, 10);
					pm_->Emit("bossDeath_smoke", center_, 24);
				}

				float u_ = (c.t_ - launchStartT_) / (1.0f - launchStartT_);
				if (u_ < 0.0f) u_ = 0.0f;
				if (u_ > 1.0f) u_ = 1.0f;

				float k_ = u_ * u_ * u_;

				Vector3 upDir_ = { 0.0f, 1.0f, 0.0f };
				Vector3 forwardDir_ = { 0.0f, 0.0f, 1.0f };

				float upDist_ = 15.0f;
				float depthDist_ = 40.0f;

				c.pos_ = self->bossFinalLaunchStartPos_
					+ upDir_ * (upDist_ * k_)
					+ forwardDir_ * (depthDist_ * k_);

				c.rot_.x += 2.5f * c.dt_;
				c.rot_.y += 3.0f * c.dt_;
				c.rot_.z += 1.5f * c.dt_;

				float s_ = 1.0f - 0.3f * k_;
				if (s_ < 0.1f) s_ = 0.1f;
				c.scale_ = {
					self->baseScale_.x * s_,
					self->baseScale_.y * s_,
					self->baseScale_.z * s_,
				};
			}
		}

		// ---------- Movement behaviors ----------
		static void Move_StraightStop(Enemy* self, MoveCtx& c) {
			if (!self->stopMove_) {
				c.pos_ += self->velocity_ * c.factor_;
				if (c.pos_.z <= self->stopZ_) { c.pos_.z = self->stopZ_; self->stopMove_ = true; }
			}
		}

		static void Move_SineX(Enemy* self, MoveCtx& c) {
			self->t_ += 0.05f * c.factor_;
			c.pos_.z += self->velocity_.z * c.factor_;
			c.pos_.x = self->startX_ + std::sinf(self->sinePhase_ + self->t_ * self->sineFreq_) * self->sineAmpX_;
			if (c.pos_.z <= self->stopZ_) { c.pos_.z = self->stopZ_; }
		}

		static void Move_StrafeLtoR(Enemy* self, MoveCtx& c) {
			c.pos_.z += self->velocity_.z * c.factor_;
			self->strafePosX_ += self->strafeSpeed_ * self->strafeDir_ * c.factor_;
			if (self->strafePosX_ > self->strafeRight_) { self->strafePosX_ = self->strafeRight_; self->strafeDir_ = -1; }
			if (self->strafePosX_ < self->strafeLeft_) { self->strafePosX_ = self->strafeLeft_;  self->strafeDir_ = +1; }
			c.pos_.x = self->strafePosX_;
			if (c.pos_.z <= self->stopZ_) { c.pos_.z = self->stopZ_; }
		}

		static void Move_ChasePlayer(Enemy* self, MoveCtx& c) {
			c.pos_.z += self->velocity_.z * c.factor_;
			if (self->playerGetter_) {
				Vector3 toP_ = self->playerGetter_() - c.pos_;
				Vector3 desire_ = { toP_.x, toP_.y, 0.0f };
				float len_ = MyMath::Length(desire_);
				if (len_ > 0.001f) {
					Vector3 dir = MyMath::Normalize(desire_);
					c.pos_.x += dir.x * self->chaseSpeed_ * c.factor_;
					c.pos_.y += dir.y * self->chaseSpeed_ * c.factor_;
				}
			}
			if (c.pos_.z <= self->stopZ_) { c.pos_.z = self->stopZ_; }
		}

		static void Move_PounceFromAbove(Enemy* self, MoveCtx& c) {
			if (!self->pounceStarted_) { return; }

			TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();

			if (!self->pounceDiving_) {
				self->pounceTime_ += c.dt_;
				float t_ = self->pounceTime_ / self->pounceDuration_;
				if (t_ > 1.0f) t_ = 1.0f;

				auto EaseOutQuad_ = [](float x) {
					return 1.0f - (1.0f - x) * (1.0f - x);
					};
				float u_ = EaseOutQuad_(t_);

				Vector3 pos1_ = MyMath::Vector3Lerp(self->pounceStart_, self->pounceApex_, u_);
				Vector3 pos2_ = MyMath::Vector3Lerp(self->pounceApex_, self->pounceTarget_, u_);
				Vector3 newPos_ = MyMath::Vector3Lerp(pos1_, pos2_, u_);

				c.pos_ = newPos_;

				{
					Vector3 emitPos_ = c.pos_;
					pm_->Emit("enemyPounceTrail", emitPos_, 2);
					pm_->Emit("enemyPounceSpark", emitPos_, 3);
				}

				if (t_ >= 1.0f) {
					Vector3 dir_ = self->pounceTarget_ - self->pounceStart_;
					float len_ = MyMath::Length(dir_);
					if (len_ > 0.001f) {
						dir_ = MyMath::Normalize(dir_);
					} else {
						dir_ = { 0.0f, -0.1f, -1.0f };
					}

					dir_.y -= 0.2f;
					dir_ = MyMath::Normalize(dir_);

					float diveSpeed_ = 0.7f;
					self->velocity_ = dir_ * diveSpeed_;

					self->pounceDiving_ = true;
				}

			} else {

				c.pos_ += self->velocity_ * c.factor_;

				Vector3 emitPos_ = c.pos_;
				pm_->Emit("enemyPounceTrail", emitPos_, 2);
				pm_->Emit("enemyPounceSpark", emitPos_, 2);
			}
		}

		static void Move_FreeRoam(Enemy* self, MoveCtx& c) {
			auto random01_ = []() {
				return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
				};

			float distToTarget_ = MyMath::Length(self->roamTarget_ - c.pos_);
			if (!self->hasRoamTarget_ || distToTarget_ < 0.5f) {
				self->hasRoamTarget_ = true;

				Vector3 target_;
				target_.x = self->roamMin_.x + (self->roamMax_.x - self->roamMin_.x) * random01_();
				target_.y = self->roamMin_.y + (self->roamMax_.y - self->roamMin_.y) * random01_();
				target_.z = self->roamMin_.z + (self->roamMax_.z - self->roamMin_.z) * random01_();

				if (self->isAngry_ && self->playerGetter_) {
					Vector3 p = self->playerGetter_();
					target_.x = (target_.x * 0.4f) + (p.x * 0.6f);
					target_.x = std::max(self->roamMin_.x, std::min(self->roamMax_.x, target_.x));
				}

				self->roamTarget_ = target_;
			}

			Vector3 toT_ = self->roamTarget_ - c.pos_;
			float len_ = MyMath::Length(toT_);
			if (len_ > 0.001f) {
				Vector3 dir_ = toT_ / len_;
				float speed_ = self->isAngry_ ? self->roamSpeedAngry_ : self->roamSpeedNormal_;
				c.pos_ += dir_ * speed_ * c.factor_;
			}

			c.pos_.x = std::max(self->roamMin_.x, std::min(self->roamMax_.x, c.pos_.x));
			c.pos_.y = std::max(self->roamMin_.y, std::min(self->roamMax_.y, c.pos_.y));
			c.pos_.z = std::max(self->roamMin_.z, std::min(self->roamMax_.z, c.pos_.z));
		}
	};

	// Death table（enum順：BlowAway, RiseAbsorb, Collapse, BossFinal）
	using DeathFn = void(*)(Enemy*, DeathCtx&);
	static const DeathFn kDeathTable_[] = {
		&Local::Death_BlowAway,
		&Local::Death_RiseAbsorb,
		&Local::Death_Collapse,
		&Local::Death_BossFinal,
	};

	// Move table（enum順：StraightStop, SineX, StrafeLtoR, ChasePlayer, PounceFromAbove, FreeRoam）
	using MoveFn = void(*)(Enemy*, MoveCtx&);
	static const MoveFn kMoveTable_[] = {
		&Local::Move_StraightStop,
		&Local::Move_SineX,
		&Local::Move_StrafeLtoR,
		&Local::Move_ChasePlayer,
		&Local::Move_PounceFromAbove,
		&Local::Move_FreeRoam,
	};

	// =========================================================
	// 死亡演出（Data-driven）
	// =========================================================
	if (isDying_) {
		deathTimer_ += dt;
		float t_ = std::min(deathTimer_ / deathDuration_, 1.0f);

		DeathCtx c_{};
		c_.dt_ = dt;
		c_.t_ = t_;
		c_.pos_ = object_->GetTranslate();
		c_.rot_ = object_->GetRotate();
		c_.scale_ = baseScale_;

		const int di_ = static_cast<int>(deathReaction_);
		if (0 <= di_ && di_ < static_cast<int>(std::size(kDeathTable_))) {
			kDeathTable_[di_](this, c_);
		}

		object_->SetTranslate(c_.pos_);
		object_->SetRotate(c_.rot_);
		object_->SetScale(c_.scale_);

		if (deathReaction_ == EnemyDeathReaction::BossFinal) {
			if (t_ < 0.7f) {
				deathAlpha_ = 1.0f;
			} else {
				float u_ = (t_ - 0.7f) / 0.3f;
				if (u_ > 1.0f) u_ = 1.0f;
				deathAlpha_ = 1.0f - u_;
			}
		} else {
			deathAlpha_ = 1.0f - t_;
		}

		object_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });
		object_->Update();

		if (deathTimer_ >= deathDuration_) {
			TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
			Vector3 emitPos_ = GetWorldPosition();

			// ここもテーブル化できるけど、今回は「主要switch排除」が目的なので
			// いったん必要最小限：deathReaction_ ごとに出すものを if でまとめる
			// ※完全排除したいなら「Emitテーブル」も作る（言ってくれ）
			if (deathReaction_ == EnemyDeathReaction::BlowAway) {
				pm_->Emit("enemyDeath_core", emitPos_, 1);
				pm_->Emit("enemyDeath_shard", emitPos_, 20);
				pm_->Emit("enemyDeath_smoke", emitPos_, 4);
			} else if (deathReaction_ == EnemyDeathReaction::RiseAbsorb) {
				pm_->Emit("enemyDeath_core", emitPos_, 1);
				pm_->Emit("enemyDeath_shard", emitPos_, 14);
				pm_->Emit("enemyDeath_smoke", emitPos_, 6);
			} else if (deathReaction_ == EnemyDeathReaction::Collapse) {
				pm_->Emit("enemyDeath_shard", emitPos_, 10);
				pm_->Emit("enemyDeath_smoke", emitPos_, 3);
			} else if (deathReaction_ == EnemyDeathReaction::BossFinal) {
				if (!bossFinalBigBurstDone_) {
					pm_->Emit("bossClear_core", emitPos_, 1);
					pm_->Emit("bossClear_ring", emitPos_, 3);
					pm_->Emit("bossClear_spark", emitPos_, 80);
					pm_->Emit("bossClear_debris", emitPos_, 60);
				}
			}

			isDead_ = true;
		}

		// 死亡中も触手を更新して、親の動きに追従させる
		if (tentacle_) {
			tentacle_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });

			if (type_ != EnemyType::Boss) {
				tentacleLocalRot_.y += 0.1f * factor_;
			}

			tentacle_->SetTranslate(tentacleLocalPos_);
			tentacle_->SetRotate(tentacleLocalRot_);
			tentacle_->SetScale(tentacleLocalScale_);

			tentacle_->Update();
		}

		return;
	}

	// =========================================================
	// 怒りタイマー更新
	// =========================================================
	if (isAngry_) {
		angryTimer_ += dt;
		if (angryTimer_ >= angryDuration_) {
			isAngry_ = false;
		}
	}

	// =========================================================
	// 行動（Data-driven）
	// =========================================================
	MoveCtx m_{};
	m_.dt_ = dt;
	m_.factor_ = factor_;
	m_.pos_ = object_->GetTranslate();

	if (!freezeMove_) {
		const int bi_ = static_cast<int>(behavior_);
		if (0 <= bi_ && bi_ < static_cast<int>(std::size(kMoveTable_))) {
			kMoveTable_[bi_](this, m_);
		}
	}

	object_->SetTranslate(m_.pos_);

	if (!isDying_) {
		if (m_.pos_.z < -30.0f) {
			escaped_ = true;
			isDead_ = true;
		}
	}

#ifdef USE_IMGUI
	// AABB 表示（そのまま）
	{
		Vector3 center_ = GetWorldPosition();
		Vector3 size_ = colliderScale_;

		auto* lr_ = TKM::LineRenderer::GetInstance();

		TKM::LineRenderer::Color normal_{ 0.0f, 1.0f, 0.0f, 1.0f };
		TKM::LineRenderer::Color hit_{ 1.0f, 0.0f, 0.0f, 1.0f };

		if (reticle_) {
			Vector3 rayOrigin_;
			if (playerGetter_) {
				rayOrigin_ = playerGetter_();
			} else {
				rayOrigin_ = reticle_->GetCenterWorldPos();
			}

			Vector3 rayDir_ = reticle_->GetAimDirection();
			lr_->AddAABBWithRayHighlight(center_, size_, rayOrigin_, rayDir_, normal_, hit_);
		} else {
			lr_->AddAABB(center_, size_, normal_);
		}
	}
#endif

	// ロック脈動（そのまま）
	if (isLocked_ && lockPulseEnabled_) {
		pulseT_ += 0.12f * factor_;
		float s_ = 1.0f + 0.15f * sinf(pulseT_);
		object_->SetScale({ baseScale_.x * s_, baseScale_.y * s_, baseScale_.z * s_ });
	} else {
		object_->SetScale(baseScale_);
	}

	// 射撃（そのまま）
	if (canShoot_ && !isDying_) {
		shootTimer_ += factor_; // フレーム加算→dt換算
		if (shootTimer_ >= shootInterval_) {
			shootTimer_ = 0.0f;
		}
	}

	object_->Update();

	// 触手も同じアルファで更新
	tentacle_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });

	// --- テンタクル回転（Y軸くるくる） ---
	// ※ボスは回転させない
	if (type_ != EnemyType::Boss) {
		tentacleLocalRot_.y += 0.1f * factor_; // 回転速度調整
	}

	// 取り付け位置を毎フレ反映したいなら（調整中なら便利）
	tentacle_->SetTranslate(tentacleLocalPos_);
	tentacle_->SetRotate(tentacleLocalRot_);
	tentacle_->SetScale(tentacleLocalScale_);

	tentacle_->Update();
}

void Enemy::Draw(TKM::DirectXCommon* dxCommon) {
	if (!object_) return;
	object_->Draw(dxCommon); // 傘
	if (!tentacle_) return;
	tentacle_->Draw(dxCommon); // 触手
}

void Enemy::SetCamera(TKM::Camera* camera) {
	camera_ = camera;
	if (object_) { object_->SetCamera(camera); }
	if (tentacle_) { tentacle_->SetCamera(camera); }
}
void Enemy::SetPosition(const Vector3& pos) {
	if (object_) { object_->SetTranslate(pos); }
}
void Enemy::SetParentScene(TKM::BaseScene* scene) {
	parentScene_ = scene;
	if (object_) { object_->SetParentScene(scene); }
	if (tentacle_) { tentacle_->SetParentScene(scene); }
}
void Enemy::SetTentacleModel(const std::string& modelName) {
	if (tentacle_) tentacle_->SetModel(modelName); // モデル設定
}
void Enemy::SetTentacleLocal(const Vector3& pos, const Vector3& rot, const Vector3& scale) {
	tentacleLocalPos_ = pos; // ローカル位置設定
	tentacleLocalRot_ = rot; // ローカル回転設定
	tentacleLocalScale_ = scale; // ローカルスケール設定
}
Vector3 Enemy::GetWorldPosition() const {
	return object_->GetTranslate(); // ワールド位置を返す
}

void Enemy::ImGuiDebug() {
#ifdef USE_IMGUI
	if (!object_) return;

	ImGui::Begin("Enemy");

	Vector3 pos_ = object_->GetTranslate();
	Vector3 rot_ = object_->GetRotate();
	Vector3 scale_ = object_->GetScale();

	if (ImGui::DragFloat3("位置", &pos_.x, 0.01f)) {
		object_->SetTranslate(pos_);
	}
	if (ImGui::DragFloat3("回転", &rot_.x, 0.01f)) {
		object_->SetRotate(rot_);
	}
	if (ImGui::DragFloat3("拡縮", &scale_.x, 0.01f)) {
		SetScale(scale_);   // モデルと当たり判定両方に反映される
	}

	// 当たり判定スケール編集
	Vector3 col_ = colliderScale_;
	if (ImGui::DragFloat3("当たり判定サイズ", &col_.x, 0.01f, 0.01f, 999.0f)) {
		SetColliderScale(col_);
	}

	ImGui::Text("HP: %d / %d", hp_, maxHP_);
	ImGui::Text("生死: %s", isDead_ ? "死" : "生");

	ImGui::End();
#endif
}

void Enemy::OnHitWithDamage(int damage) {
	// すでに死んでる or 死亡演出中なら無視
	if (isDead_ || isDying_) {
		return;
	}

	hp_ -= damage;
	if (hp_ <= 0) {
		hp_ = 0;
		// ボスなら専用死亡演出、それ以外は従来通り
		if (type_ == EnemyType::Boss) {
			StartBossDeathReaction({ 0.0f, 0.0f, 1.0f });
		} else {
			StartDeathReaction({ 0.0f, 0.0f, 1.0f });
		}
	}
}

void Enemy::StartDeathReaction(const Vector3& hitDir) {
	if (isDying_) {
		return;
	}

	// ボスなら共通処理は使わず専用リアクションへ
	if (type_ == EnemyType::Boss) {
		StartBossDeathReaction(hitDir); // ボス専用死亡リアクション
		return;
	}

	freezeMove_ = true; // 死んだ瞬間に動き停止
	defeated_ = true; // 敵撃破フラグをtrueに
	isDying_ = true; // 死亡演出中フラグを立てるtrueに
	deathTimer_ = 0.0f; // タイマーリセット
	deathAlpha_ = 1.0f; // アルファ初期値

	// 0,1,2 のどれかをランダムに選ぶ
	int r_ = std::rand() % 3;

	// 共通で使うノックバック方向
	Vector3 dir_ = hitDir;
	if (MyMath::Length(dir_) < 0.001f) {
		dir_ = { 0.0f, 0.0f, 1.0f };
	}
	dir_ = MyMath::Normalize(dir_);

	auto Pick0_BlowAway_ = [&]() {
		deathReaction_ = EnemyDeathReaction::BlowAway;
		deathDuration_ = 3.0f;
		deathVelocity_ = dir_ * 4.0f;
		deathRotateSpeed_ = { 1.5f, 2.0f, 0.8f };
		};

	auto Pick1_RiseAbsorb_ = [&]() {
		deathReaction_ = EnemyDeathReaction::RiseAbsorb;
		deathDuration_ = 1.2f;
		deathVelocity_ = { 0.0f, 3.0f, 0.0f };
		deathRotateSpeed_ = { 0.0f, 2.0f, 0.0f };
		};

	auto Pick2_Collapse_ = [&]() {
		deathReaction_ = EnemyDeathReaction::Collapse;
		deathDuration_ = 0.9f;
		deathVelocity_ = { dir_.x * 1.5f, -3.0f, dir_.z * 1.5f };
		deathRotateSpeed_ = { 3.0f, 0.5f, 0.0f };
		};

	if (r_ == 0) { Pick0_BlowAway_(); } else if (r_ == 1) { Pick1_RiseAbsorb_(); } else { Pick2_Collapse_(); }
}

void Enemy::SyncTransform() {
	if (!object_) return;
	if (!tentacle_) return;
	object_->Update();  // 行列と定数バッファだけ更新
	tentacle_->Update(); // 行列と定数バッファだけ更新
}

void Enemy::StartBossDeathReaction(const Vector3& hitDir) {
	if (isDying_) { // すでに死亡演出中なら無視
		return;
	}

	freezeMove_ = true; // 死んだ瞬間に動き停止
	defeated_ = true;   // 敵撃破フラグをtrueに
	isDying_ = true;    // 死亡演出中フラグを立てる
	deathTimer_ = 0.0f; // タイマーリセット
	deathAlpha_ = 1.0f; // アルファ初期値

	// ボス専用リアクション
	deathReaction_ = EnemyDeathReaction::BossFinal;

	// ボスはしっかり見せたいので少し長め
	deathDuration_ = 5.0f;

	// 位置＆回転は BossFinal ブロック側で制御するのでここでは 0
	deathVelocity_ = { 0.0f, 0.0f, 0.0f };
	deathRotateSpeed_ = { 0.0f, 0.0f, 0.0f };

	// BossFinal 用（ぶっ飛び演出の開始をリセット）
	bossFinalLaunchStarted_ = false;
	bossFinalLaunchStartPos_ = object_->GetTranslate();

	// BossFinal 用一時変数リセット
	bossFinalLaunchStarted_ = false;
	bossFinalBigBurstDone_ = false;
	bossFinalCameraInited_ = false;
}