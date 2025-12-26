#define NOMINMAX
#include "Enemy.h"
#include "ModelManager.h"
#include <algorithm>
#include <cstdlib> 
#include <AABB.h>

void Enemy::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	object_ = std::make_unique<TKM::Object3d>(); // Object3d のインスタンスを生成
	object_->Initialize(common, dxCommon); // 初期化
	object_->SetModel("enemy.obj"); // モデル名は適宜変更

	// カメラ設定
	if (camera) {
		object_->SetCamera(camera);
	}

	baseScale_ = object_->GetScale(); // 元のスケールを保持
	startX_ = object_->GetTranslate().x; // サイン波の基準用
}

void Enemy::Update(float dt) {

	// 60fps基準の値をそのまま使えるようにする係数
	// dt=1/60 のとき factor=1.0 になる
	const float factor = dt * 60.0f;

	// =========================================================
	// Data-driven：死亡リアクション / 行動 の関数テーブル
	// （switch を排除）
	// =========================================================
	struct DeathCtx {
		float dt;
		float t; // 0..1
		Vector3 pos;
		Vector3 rot;
		Vector3 scale;
	};

	struct MoveCtx {
		float dt;
		float factor;
		Vector3 pos;
	};

	struct Local {

		// ---------- Death reactions ----------
		static void Death_BlowAway(Enemy* self, DeathCtx& c) {
			float speed = 1.0f - c.t;
			c.pos += self->deathVelocity_ * speed * c.dt;

			c.rot.x += self->deathRotateSpeed_.x * c.dt;
			c.rot.y += self->deathRotateSpeed_.y * c.dt;
			c.rot.z += self->deathRotateSpeed_.z * c.dt;

			float s = 1.0f - c.t;
			c.scale = { self->baseScale_.x * s, self->baseScale_.y * s, self->baseScale_.z * s };
		}

		static void Death_RiseAbsorb(Enemy* self, DeathCtx& c) {
			c.pos += self->deathVelocity_ * c.dt;

			c.rot.y += self->deathRotateSpeed_.y * c.dt;

			float s = 1.0f - c.t;
			c.scale = {
				self->baseScale_.x * s * 0.5f,
				self->baseScale_.y * (1.0f - c.t * 0.2f),
				self->baseScale_.z * s * 0.5f
			};
		}

		static void Death_Collapse(Enemy* self, DeathCtx& c) {
			c.pos += self->deathVelocity_ * c.dt;

			c.rot.x += self->deathRotateSpeed_.x * c.dt;

			float s = 1.0f - c.t;
			c.scale = {
				self->baseScale_.x,
				self->baseScale_.y * s * 0.2f,
				self->baseScale_.z
			};
		}

		static void Death_BossFinal(Enemy* self, DeathCtx& c) {
			const float launchStartT = 0.5f;

			if (c.t < launchStartT) {
				float shakeAmp = 0.25f;
				float shakeFreq = 18.0f;

				c.pos.x += sinf(self->deathTimer_ * shakeFreq) * shakeAmp;
				c.pos.y += cosf(self->deathTimer_ * shakeFreq * 0.7f) * shakeAmp * 0.6f;

				float pulse = 1.0f + 0.10f * sinf(self->deathTimer_ * 10.0f);
				c.scale = {
					self->baseScale_.x * pulse,
					self->baseScale_.y * pulse,
					self->baseScale_.z * pulse,
				};

				TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
				if (std::rand() % 3 != 0) {
					Vector3 center = self->GetWorldPosition();
					Vector3 off = {
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.x,
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.y,
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.z
					};
					Vector3 emitPos = center + off * 0.5f;
					pm->Emit("bossDeath_bomb", emitPos, 1);
				}

			} else {

				if (!self->bossFinalLaunchStarted_) {
					self->bossFinalLaunchStarted_ = true;
					self->bossFinalLaunchStartPos_ = c.pos;

					TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
					Vector3 center = self->GetWorldPosition();
					pm->Emit("bossDeath_ring", center, 2);
					pm->Emit("bossDeath_bomb", center, 10);
					pm->Emit("bossDeath_smoke", center, 24);
				}

				float u = (c.t - launchStartT) / (1.0f - launchStartT);
				if (u < 0.0f) u = 0.0f;
				if (u > 1.0f) u = 1.0f;

				float k = u * u * u;

				Vector3 upDir = { 0.0f, 1.0f, 0.0f };
				Vector3 forwardDir = { 0.0f, 0.0f, 1.0f };

				float upDist = 15.0f;
				float depthDist = 40.0f;

				c.pos = self->bossFinalLaunchStartPos_
					+ upDir * (upDist * k)
					+ forwardDir * (depthDist * k);

				c.rot.x += 2.5f * c.dt;
				c.rot.y += 3.0f * c.dt;
				c.rot.z += 1.5f * c.dt;

				float s = 1.0f - 0.3f * k;
				if (s < 0.1f) s = 0.1f;
				c.scale = {
					self->baseScale_.x * s,
					self->baseScale_.y * s,
					self->baseScale_.z * s,
				};
			}
		}

		// ---------- Movement behaviors ----------
		static void Move_StraightStop(Enemy* self, MoveCtx& c) {
			if (!self->stopMove_) {
				c.pos += self->velocity_ * c.factor;
				if (c.pos.z <= self->stopZ_) { c.pos.z = self->stopZ_; self->stopMove_ = true; }
			}
		}

		static void Move_SineX(Enemy* self, MoveCtx& c) {
			self->t_ += 0.05f * c.factor;
			c.pos.z += self->velocity_.z * c.factor;
			c.pos.x = self->startX_ + std::sinf(self->sinePhase_ + self->t_ * self->sineFreq_) * self->sineAmpX_;
			if (c.pos.z <= self->stopZ_) { c.pos.z = self->stopZ_; }
		}

		static void Move_StrafeLtoR(Enemy* self, MoveCtx& c) {
			c.pos.z += self->velocity_.z * c.factor;
			self->strafePosX_ += self->strafeSpeed_ * self->strafeDir_ * c.factor;
			if (self->strafePosX_ > self->strafeRight_) { self->strafePosX_ = self->strafeRight_; self->strafeDir_ = -1; }
			if (self->strafePosX_ < self->strafeLeft_) { self->strafePosX_ = self->strafeLeft_;  self->strafeDir_ = +1; }
			c.pos.x = self->strafePosX_;
			if (c.pos.z <= self->stopZ_) { c.pos.z = self->stopZ_; }
		}

		static void Move_ChasePlayer(Enemy* self, MoveCtx& c) {
			c.pos.z += self->velocity_.z * c.factor;
			if (self->playerGetter_) {
				Vector3 toP = self->playerGetter_() - c.pos;
				Vector3 desire = { toP.x, toP.y, 0.0f };
				float len = MyMath::Length(desire);
				if (len > 0.001f) {
					Vector3 dir = MyMath::Normalize(desire);
					c.pos.x += dir.x * self->chaseSpeed_ * c.factor;
					c.pos.y += dir.y * self->chaseSpeed_ * c.factor;
				}
			}
			if (c.pos.z <= self->stopZ_) { c.pos.z = self->stopZ_; }
		}

		static void Move_PounceFromAbove(Enemy* self, MoveCtx& c) {
			if (!self->pounceStarted_) { return; }

			TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();

			if (!self->pounceDiving_) {
				self->pounceTime_ += c.dt;
				float t = self->pounceTime_ / self->pounceDuration_;
				if (t > 1.0f) t = 1.0f;

				auto EaseOutQuad = [](float x) {
					return 1.0f - (1.0f - x) * (1.0f - x);
					};
				float u = EaseOutQuad(t);

				Vector3 pos1 = MyMath::Vector3Lerp(self->pounceStart_, self->pounceApex_, u);
				Vector3 pos2 = MyMath::Vector3Lerp(self->pounceApex_, self->pounceTarget_, u);
				Vector3 newPos = MyMath::Vector3Lerp(pos1, pos2, u);

				c.pos = newPos;

				{
					Vector3 emitPos = c.pos;
					pm->Emit("enemyPounceTrail", emitPos, 2);
					pm->Emit("enemyPounceSpark", emitPos, 3);
				}

				if (t >= 1.0f) {
					Vector3 dir = self->pounceTarget_ - self->pounceStart_;
					float len = MyMath::Length(dir);
					if (len > 0.001f) {
						dir = MyMath::Normalize(dir);
					} else {
						dir = { 0.0f, -0.1f, -1.0f };
					}

					dir.y -= 0.2f;
					dir = MyMath::Normalize(dir);

					float diveSpeed = 0.7f;
					self->velocity_ = dir * diveSpeed;

					self->pounceDiving_ = true;
				}

			} else {

				c.pos += self->velocity_ * c.factor;

				Vector3 emitPos = c.pos;
				pm->Emit("enemyPounceTrail", emitPos, 2);
				pm->Emit("enemyPounceSpark", emitPos, 2);
			}
		}

		static void Move_FreeRoam(Enemy* self, MoveCtx& c) {
			auto random01 = []() {
				return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
				};

			float distToTarget = MyMath::Length(self->roamTarget_ - c.pos);
			if (!self->hasRoamTarget_ || distToTarget < 0.5f) {
				self->hasRoamTarget_ = true;

				Vector3 target;
				target.x = self->roamMin_.x + (self->roamMax_.x - self->roamMin_.x) * random01();
				target.y = self->roamMin_.y + (self->roamMax_.y - self->roamMin_.y) * random01();
				target.z = self->roamMin_.z + (self->roamMax_.z - self->roamMin_.z) * random01();

				if (self->isAngry_ && self->playerGetter_) {
					Vector3 p = self->playerGetter_();
					target.x = (target.x * 0.4f) + (p.x * 0.6f);
					target.x = std::max(self->roamMin_.x, std::min(self->roamMax_.x, target.x));
				}

				self->roamTarget_ = target;
			}

			Vector3 toT = self->roamTarget_ - c.pos;
			float len = MyMath::Length(toT);
			if (len > 0.001f) {
				Vector3 dir = toT / len;
				float speed = self->isAngry_ ? self->roamSpeedAngry_ : self->roamSpeedNormal_;
				c.pos += dir * speed * c.factor;
			}

			c.pos.x = std::max(self->roamMin_.x, std::min(self->roamMax_.x, c.pos.x));
			c.pos.y = std::max(self->roamMin_.y, std::min(self->roamMax_.y, c.pos.y));
			c.pos.z = std::max(self->roamMin_.z, std::min(self->roamMax_.z, c.pos.z));
		}
	};

	// Death table（enum順：BlowAway, RiseAbsorb, Collapse, BossFinal）
	using DeathFn = void(*)(Enemy*, DeathCtx&);
	static const DeathFn kDeathTable[] = {
		&Local::Death_BlowAway,
		&Local::Death_RiseAbsorb,
		&Local::Death_Collapse,
		&Local::Death_BossFinal,
	};

	// Move table（enum順：StraightStop, SineX, StrafeLtoR, ChasePlayer, PounceFromAbove, FreeRoam）
	using MoveFn = void(*)(Enemy*, MoveCtx&);
	static const MoveFn kMoveTable[] = {
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
		float t = std::min(deathTimer_ / deathDuration_, 1.0f);

		DeathCtx c{};
		c.dt = dt;
		c.t = t;
		c.pos = object_->GetTranslate();
		c.rot = object_->GetRotate();
		c.scale = baseScale_;

		const int di = static_cast<int>(deathReaction_);
		if (0 <= di && di < static_cast<int>(std::size(kDeathTable))) {
			kDeathTable[di](this, c);
		}

		object_->SetTranslate(c.pos);
		object_->SetRotate(c.rot);
		object_->SetScale(c.scale);

		if (deathReaction_ == EnemyDeathReaction::BossFinal) {
			if (t < 0.7f) {
				deathAlpha_ = 1.0f;
			} else {
				float u = (t - 0.7f) / 0.3f;
				if (u > 1.0f) u = 1.0f;
				deathAlpha_ = 1.0f - u;
			}
		} else {
			deathAlpha_ = 1.0f - t;
		}

		object_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });
		object_->Update();

		if (deathTimer_ >= deathDuration_) {
			TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
			Vector3 emitPos = GetWorldPosition();

			// ここもテーブル化できるけど、今回は「主要switch排除」が目的なので
			// いったん必要最小限：deathReaction_ ごとに出すものを if でまとめる
			// ※完全排除したいなら「Emitテーブル」も作る（言ってくれ）
			if (deathReaction_ == EnemyDeathReaction::BlowAway) {
				pm->Emit("enemyDeath_core", emitPos, 1);
				pm->Emit("enemyDeath_shard", emitPos, 20);
				pm->Emit("enemyDeath_smoke", emitPos, 4);
			} else if (deathReaction_ == EnemyDeathReaction::RiseAbsorb) {
				pm->Emit("enemyDeath_core", emitPos, 1);
				pm->Emit("enemyDeath_shard", emitPos, 14);
				pm->Emit("enemyDeath_smoke", emitPos, 6);
			} else if (deathReaction_ == EnemyDeathReaction::Collapse) {
				pm->Emit("enemyDeath_shard", emitPos, 10);
				pm->Emit("enemyDeath_smoke", emitPos, 3);
			} else if (deathReaction_ == EnemyDeathReaction::BossFinal) {
				if (!bossFinalBigBurstDone_) {
					pm->Emit("bossClear_core", emitPos, 1);
					pm->Emit("bossClear_ring", emitPos, 3);
					pm->Emit("bossClear_spark", emitPos, 80);
					pm->Emit("bossClear_debris", emitPos, 60);
				}
			}

			isDead_ = true;
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
	MoveCtx m{};
	m.dt = dt;
	m.factor = factor;
	m.pos = object_->GetTranslate();

	if (!freezeMove_) {
		const int bi = static_cast<int>(behavior_);
		if (0 <= bi && bi < static_cast<int>(std::size(kMoveTable))) {
			kMoveTable[bi](this, m);
		}
	}

	object_->SetTranslate(m.pos);

	if (!isDying_) {
		if (m.pos.z < -30.0f) {
			escaped_ = true;
			isDead_ = true;
		}
	}

	// AABB 表示（そのまま）
	{
		Vector3 center = GetWorldPosition();
		Vector3 size = colliderScale_;

		auto* lr = TKM::LineRenderer::GetInstance();

		TKM::LineRenderer::Color normal{ 0.0f, 1.0f, 0.0f, 1.0f };
		TKM::LineRenderer::Color hit{ 1.0f, 0.0f, 0.0f, 1.0f };

		if (reticle_) {
			Vector3 rayOrigin;
			if (playerGetter_) {
				rayOrigin = playerGetter_();
			} else {
				rayOrigin = reticle_->GetCenterWorldPos();
			}

			Vector3 rayDir = reticle_->GetAimDirection();
			lr->AddAABBWithRayHighlight(center, size, rayOrigin, rayDir, normal, hit);
		} else {
			lr->AddAABB(center, size, normal);
		}
	}

	// ロック脈動（そのまま）
	if (isLocked_ && lockPulseEnabled_) {
		pulseT_ += 0.12f * factor;
		float s = 1.0f + 0.15f * sinf(pulseT_);
		object_->SetScale({ baseScale_.x * s, baseScale_.y * s, baseScale_.z * s });
	} else {
		object_->SetScale(baseScale_);
	}

	// 射撃（そのまま）
	if (canShoot_ && !isDying_) {
		shootTimer_ += factor; // フレーム加算→dt換算
		if (shootTimer_ >= shootInterval_) {
			shootTimer_ = 0.0f;
		}
	}

	object_->Update();
}

void Enemy::Draw(TKM::DirectXCommon* dxCommon) {
	if (!object_) return;
	object_->Draw(dxCommon); // Object3d の描画
}

void Enemy::SetCamera(TKM::Camera* camera) {
	this->camera = camera; // メンバ変数に保存
	if (object_) {
		object_->SetCamera(camera); // Object3d に反映
	}
}
void Enemy::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos); // 位置設定
}
void Enemy::SetParentScene(TKM::BaseScene* scene) {
	parentScene_ = scene; // メンバ変数に保存
}
Vector3 Enemy::GetWorldPosition() const {
	return object_->GetTranslate(); // ワールド位置を返す
}

void Enemy::ImGuiDebug() {
#ifdef USE_IMGUI
	if (!object_) return;

	ImGui::Begin("Enemy");

	Vector3 pos = object_->GetTranslate();
	Vector3 rot = object_->GetRotate();
	Vector3 scale = object_->GetScale();

	if (ImGui::DragFloat3("位置", &pos.x, 0.01f)) {
		object_->SetTranslate(pos);
	}
	if (ImGui::DragFloat3("回転", &rot.x, 0.01f)) {
		object_->SetRotate(rot);
	}
	if (ImGui::DragFloat3("拡縮", &scale.x, 0.01f)) {
		SetScale(scale);   // モデルと当たり判定両方に反映される
	}

	// 当たり判定スケール編集
	Vector3 col = colliderScale_;
	if (ImGui::DragFloat3("当たり判定サイズ", &col.x, 0.01f, 0.01f, 999.0f)) {
		SetColliderScale(col);
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
	int r = std::rand() % 3;

	// 共通で使うノックバック方向
	Vector3 dir = hitDir;
	if (MyMath::Length(dir) < 0.001f) {
		dir = { 0.0f, 0.0f, 1.0f };
	}
	dir = MyMath::Normalize(dir);

	auto Pick0_BlowAway = [&]() {
		deathReaction_ = EnemyDeathReaction::BlowAway;
		deathDuration_ = 3.0f;
		deathVelocity_ = dir * 4.0f;
		deathRotateSpeed_ = { 1.5f, 2.0f, 0.8f };
		};

	auto Pick1_RiseAbsorb = [&]() {
		deathReaction_ = EnemyDeathReaction::RiseAbsorb;
		deathDuration_ = 1.2f;
		deathVelocity_ = { 0.0f, 3.0f, 0.0f };
		deathRotateSpeed_ = { 0.0f, 2.0f, 0.0f };
		};

	auto Pick2_Collapse = [&]() {
		deathReaction_ = EnemyDeathReaction::Collapse;
		deathDuration_ = 0.9f;
		deathVelocity_ = { dir.x * 1.5f, -3.0f, dir.z * 1.5f };
		deathRotateSpeed_ = { 3.0f, 0.5f, 0.0f };
		};

	if (r == 0) { Pick0_BlowAway(); } else if (r == 1) { Pick1_RiseAbsorb(); } else { Pick2_Collapse(); }
}

void Enemy::SyncTransform() {
	if (!object_) return;
	object_->Update();  // 行列と定数バッファだけ更新
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