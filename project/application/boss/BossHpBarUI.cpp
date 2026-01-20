#include "BossHpBarUI.h"
#include <cmath>

namespace TKM {

	void BossHpBarUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		desc_ = desc;

		// --- frame ---
		frame_ = std::make_unique<Sprite>();
		frame_->Initialize(spriteCommon_, dxCommon_, desc_.frameTex);
		frame_->SetParentScene(parentScene_);
		frame_->SetAutoAdjustTextureSize(false); // 枠も自前制御（"そのままサイズ"事故防止）
		frame_->SetAnchorPoint({ 0.0f, 0.0f });
		frame_->SetPosition(desc_.pos);
		frame_->SetSize({ desc_.size.x + 10.0f, desc_.size.y + 10.0f });
		// uvCheckerはデバッグ用の当て布なので薄く
		frame_->SetColor({ 1.0f, 1.0f, 1.0f, 0.18f });

		// 遅延バー（後ろ）
		lagFill_ = std::make_unique<Sprite>();
		lagFill_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex);
		lagFill_->SetParentScene(parentScene_);
		lagFill_->SetAutoAdjustTextureSize(false);
		lagFill_->SetAnchorPoint({ 0.0f, 0.0f });
		lagFill_->SetPosition({ desc_.pos.x + 5.0f, desc_.pos.y + 5.0f });
		lagFill_->SetSize(desc_.size);
		lagFill_->SetColor({ 1.0f, 0.35f, 0.35f, 1.0f }); // ダメージ色

		// 本体バー（前）
		fill_ = std::make_unique<Sprite>();
		fill_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex);
		fill_->SetParentScene(parentScene_);
		fill_->SetAutoAdjustTextureSize(false);
		fill_->SetAnchorPoint({ 0.0f, 0.0f });
		fill_->SetPosition({ desc_.pos.x + 5.0f, desc_.pos.y + 5.0f });
		fill_->SetSize(desc_.size);
		fill_->SetColor(desc_.baseColor);

		// 減った区間の残像（lag - fill の差分だけ光って消える）
		drainGlow_ = std::make_unique<Sprite>();
		drainGlow_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex);
		drainGlow_->SetParentScene(parentScene_);
		drainGlow_->SetAutoAdjustTextureSize(false);
		drainGlow_->SetAnchorPoint({ 0.0f, 0.0f });
		drainGlow_->SetPosition({ desc_.pos.x + 5.0f, desc_.pos.y + 5.0f });
		drainGlow_->SetSize({ 0.0f, desc_.size.y });
		drainGlow_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

		// 破片プール
		shards_.resize(desc_.segmentCount);
		for (auto& s : shards_) {
			s.sp = std::make_unique<Sprite>();
			s.sp->Initialize(spriteCommon_, dxCommon_, desc_.shardTex);
			s.sp->SetParentScene(parentScene_);
			s.sp->SetAutoAdjustTextureSize(false);
			s.sp->SetAnchorPoint({ 0.5f, 0.5f });
			s.sp->SetSize({ 18.0f, 18.0f });
			s.alive = false;
		}

		initialized_ = false;
		lastHp_ = 0;
		lagHp_ = 0.0f;
		hitPulse_ = 0.0f;
		drainGlowTimer_ = 0.0f;
		time_ = 0.0f;
	}

	void BossHpBarUI::SpawnShards_(int segBegin, int segEnd) {
		segBegin = std::max(segBegin, 0);
		segEnd = std::min(segEnd, desc_.segmentCount);

		// 右端から“減った分”を砕く
		const float segW = desc_.size.x / float(desc_.segmentCount);
		for (int i = segBegin; i < segEnd; ++i) {
			// 空いてる破片スロットを探す
			Shard* slot = nullptr;
			for (auto& s : shards_) {
				if (!s.alive) { slot = &s; break; }
			}
			if (!slot) { break; }

			float x = (desc_.pos.x + 5.0f) + segW * (float(i) + 0.5f);
			float y = (desc_.pos.y + 5.0f) + desc_.size.y * 0.5f;

			slot->alive = true;
			slot->t = 0.0f;
			slot->life = desc_.shardLife;

			float spd = RandRange_(desc_.shardSpeedMin, desc_.shardSpeedMax);
			float ang = RandRange_(-1.3f, 1.3f);
			slot->vel = { std::cos(ang) * spd, std::sin(ang) * spd - 120.0f };

			slot->rot = 0.0f;
			slot->rotVel = RandRange_(-desc_.shardRotSpeed, desc_.shardRotSpeed);

			slot->sp->SetPosition({ x, y });
			slot->sp->SetColor({ 1,1,1,1 });
		}
	}

	void BossHpBarUI::Update(float dt, BossEnemy* boss) {
		if (!visible_) { return; }
		if (!boss) { return; }
		if (boss->IsDead()) { return; }

		time_ += dt;

		maxHp_ = std::max(1, boss->GetMaxHP());
		hp_ = std::clamp(boss->GetHP(), 0, maxHp_);

		// 初回同期（lastHp_初期値事故防止）
		if (!initialized_ || lastHp_ < 0 || lastHp_ > maxHp_) {
			lastHp_ = hp_;
			lagHp_ = float(hp_);
			initialized_ = true;
		}

		// 被弾検出：hpが減った瞬間だけ演出
		if (hp_ < lastHp_) {
			shakeTimer_ = desc_.shakeTime;
			hitPulse_ = 0.12f;
			drainGlowTimer_ = desc_.drainGlowTime;

			int oldSeg = int((float(lastHp_) / float(maxHp_)) * desc_.segmentCount);
			int newSeg = int((float(hp_) / float(maxHp_)) * desc_.segmentCount);
			SpawnShards_(newSeg, oldSeg);

			lastHp_ = hp_;
		} else if (hp_ > lastHp_) {
			// 回復/リセット時：検知ズレ防止で同期だけ取る
			lastHp_ = hp_;
			lagHp_ = float(hp_);
		}

		// ヒットパルス（スカッシュ）
		float pulse = 1.0f;
		if (hitPulse_ > 0.0f) {
			hitPulse_ -= dt;
			if (hitPulse_ < 0.0f) { hitPulse_ = 0.0f; }
			float t = hitPulse_ / 0.12f;  // 1..0
			pulse = 1.0f + (t * 0.30f);
		}

		// 本体バー：即時反映
		float rate = float(hp_) / float(maxHp_);
		float w = desc_.size.x * rate;
		fill_->SetSize({ w, desc_.size.y * pulse });

		// 遅延バー：あとから追従
		if (lagHp_ > float(hp_)) {
			lagHp_ = std::max(float(hp_), lagHp_ - desc_.lagSpeed * dt);
		} else {
			lagHp_ = float(hp_);
		}
		float lagRate = lagHp_ / float(maxHp_);
		float lw = desc_.size.x * lagRate;
		lagFill_->SetSize({ lw, desc_.size.y * (1.0f + (pulse - 1.0f) * 0.5f) });

		// シェイク（位置を揺らす）
		Vector2 basePos = desc_.pos;
		Vector2 fillPos = { desc_.pos.x + 5.0f, desc_.pos.y + 5.0f };
		Vector2 off{ 0.0f, 0.0f };

		if (shakeTimer_ > 0.0f) {
			shakeTimer_ -= dt;
			if (shakeTimer_ < 0.0f) { shakeTimer_ = 0.0f; }
			off = {
				RandRange_(-desc_.shakePower, desc_.shakePower),
				RandRange_(-desc_.shakePower, desc_.shakePower)
			};
		}

		frame_->SetPosition({ basePos.x + off.x, basePos.y + off.y });
		fill_->SetPosition({ fillPos.x + off.x, fillPos.y + off.y });
		lagFill_->SetPosition({ fillPos.x + off.x, fillPos.y + off.y });

		// -----------------------------
		// 色変化：単純回避
		// -----------------------------
		bool draining = (lagHp_ > float(hp_));
		float hpRate = float(hp_) / float(maxHp_);

		float lowT = 0.0f;
		if (hpRate < desc_.lowHpStartRate) {
			lowT = (desc_.lowHpStartRate - hpRate) / std::max(0.0001f, desc_.lowHpStartRate);
		}

		Vector4 col = LerpColor_(desc_.baseColor, desc_.lowHpColor, lowT);

		if (draining) {
			float flicker = 0.5f + 0.5f * std::sin(time_ * 32.0f);
			float t = 0.55f + 0.20f * flicker;
			col = LerpColor_(col, desc_.drainColor, t);
		}

		if (hitPulse_ > 0.0f) {
			float t = std::clamp(hitPulse_ / 0.12f, 0.0f, 1.0f);
			col = LerpColor_(col, desc_.flashColor, t);
		}

		fill_->SetColor(col);

		// -----------------------------
		// 減った区間の残像（lw - w）
		// -----------------------------
		if (drainGlow_) {
			if (drainGlowTimer_ > 0.0f) {
				drainGlowTimer_ -= dt;
				if (drainGlowTimer_ < 0.0f) { drainGlowTimer_ = 0.0f; }

				float a = drainGlowTimer_ / std::max(0.0001f, desc_.drainGlowTime);
				float glowW = std::max(0.0f, lw - w);

				drainGlow_->SetSize({ glowW, desc_.size.y });
				drainGlow_->SetPosition({ (fillPos.x + off.x) + w, (fillPos.y + off.y) });
				drainGlow_->SetColor({ 1.0f, 0.55f, 0.55f, 0.65f * a });
			} else {
				drainGlow_->SetSize({ 0.0f, desc_.size.y });
				drainGlow_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
			}
		}

		// 反映（必須）
		if (frame_) { frame_->Update(); }
		if (lagFill_) { lagFill_->Update(); }
		if (drainGlow_) { drainGlow_->Update(); }
		if (fill_) { fill_->Update(); }

		// 破片更新
		for (auto& s : shards_) {
			if (!s.alive) { continue; }

			s.t += dt;
			if (s.t >= s.life) {
				s.alive = false;
				continue;
			}

			Vector2 p = s.sp->GetPosition();
			p.x += s.vel.x * dt;
			p.y += s.vel.y * dt;

			// 重力っぽく
			s.vel.y += 520.0f * dt;

			s.rot += s.rotVel * dt;

			// フェード
			float a = 1.0f - (s.t / s.life);
			s.sp->SetColor({ 1,1,1,a });

			s.sp->SetPosition(p);
			s.sp->SetRotation(s.rot);
			s.sp->Update();
		}
	}

	void BossHpBarUI::Draw() {
		if (!visible_) { return; }
		if (frame_) { frame_->Draw(); }
		if (lagFill_) { lagFill_->Draw(); }
		if (drainGlow_) { drainGlow_->Draw(); }
		if (fill_) { fill_->Draw(); }

		for (auto& s : shards_) {
			if (s.alive && s.sp) {
				s.sp->Draw();
			}
		}
	}

} // namespace TKM