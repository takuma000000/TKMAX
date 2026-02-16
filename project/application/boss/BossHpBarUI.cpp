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
		frame_->Initialize(spriteCommon_, dxCommon_, desc_.frameTex_);
		frame_->SetParentScene(parentScene_);
		frame_->SetAutoAdjustTextureSize(false); // 枠も自前制御
		frame_->SetAnchorPoint({ 0.0f, 0.0f });
		frame_->SetPosition(desc_.pos_);
		frame_->SetSize({ desc_.size_.x + 10.0f, desc_.size_.y + 10.0f });
		frame_->SetColor({ 1.0f, 1.0f, 1.0f, 0.5f });

		// 遅延バー（後ろ）
		lagFill_ = std::make_unique<Sprite>();
		lagFill_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
		lagFill_->SetParentScene(parentScene_);
		lagFill_->SetAutoAdjustTextureSize(false);
		lagFill_->SetAnchorPoint({ 0.0f, 0.0f });
		lagFill_->SetPosition({ desc_.pos_.x + 5.0f, desc_.pos_.y + 5.0f });
		lagFill_->SetSize(desc_.size_);
		lagFill_->SetColor({ 1.0f, 0.35f, 0.35f, 1.0f }); // ダメージ色

		// 本体バー（前）
		fill_ = std::make_unique<Sprite>();
		fill_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
		fill_->SetParentScene(parentScene_);
		fill_->SetAutoAdjustTextureSize(false);
		fill_->SetAnchorPoint({ 0.0f, 0.0f });
		fill_->SetPosition({ desc_.pos_.x + 5.0f, desc_.pos_.y + 5.0f });
		fill_->SetSize(desc_.size_);
		fill_->SetColor(desc_.baseColor_);

		// 減った区間の残像（lag - fill の差分だけ光って消える）
		drainGlow_ = std::make_unique<Sprite>();
		drainGlow_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
		drainGlow_->SetParentScene(parentScene_);
		drainGlow_->SetAutoAdjustTextureSize(false);
		drainGlow_->SetAnchorPoint({ 0.0f, 0.0f });
		drainGlow_->SetPosition({ desc_.pos_.x + 5.0f, desc_.pos_.y + 5.0f });
		drainGlow_->SetSize({ 0.0f, desc_.size_.y });
		drainGlow_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

		// 破片プール
		shards_.resize(desc_.segmentCount_);
		for (auto& s_ : shards_) {
			s_.sp_ = std::make_unique<Sprite>();
			s_.sp_->Initialize(spriteCommon_, dxCommon_, desc_.shardTex_);
			s_.sp_->SetParentScene(parentScene_);
			s_.sp_->SetAutoAdjustTextureSize(false);
			s_.sp_->SetAnchorPoint({ 0.5f, 0.5f });
			s_.sp_->SetSize({ 18.0f, 18.0f });
			s_.alive_ = false;
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
		segEnd = std::min(segEnd, desc_.segmentCount_);

		// 右端から“減った分”を砕く
		const float segW_ = desc_.size_.x / float(desc_.segmentCount_);
		for (int i = segBegin; i < segEnd; ++i) {
			// 空いてる破片スロットを探す
			Shard* slot_ = nullptr;
			for (auto& s : shards_) {
				if (!s.alive_) { slot_ = &s; break; }
			}
			if (!slot_) { break; }

			float x_ = (desc_.pos_.x + 5.0f) + segW_ * (float(i) + 0.5f);
			float y_ = (desc_.pos_.y + 5.0f) + desc_.size_.y * 0.5f;

			slot_->alive_ = true;
			slot_->t_ = 0.0f;
			slot_->life_ = desc_.shardLife_;

			float spd_ = RandRange_(desc_.shardSpeedMin_, desc_.shardSpeedMax_);
			float ang_ = RandRange_(-1.3f, 1.3f);
			slot_->vel_ = { std::cos(ang_) * spd_, std::sin(ang_) * spd_ - 120.0f };

			slot_->rot_ = 0.0f;
			slot_->rotVel_ = RandRange_(-desc_.shardRotSpeed_, desc_.shardRotSpeed_);

			slot_->sp_->SetPosition({ x_, y_ });
			slot_->sp_->SetColor({ 1,1,1,1 });
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
			shakeTimer_ = desc_.shakeTime_;
			hitPulse_ = 0.12f;
			drainGlowTimer_ = desc_.drainGlowTime_;

			float oldRate_ = float(lastHp_) / float(maxHp_);
			float newRate_ = float(hp_) / float(maxHp_);

			int oldSeg_ = int(std::ceil(oldRate_ * desc_.segmentCount_));
			int newSeg_ = int(std::floor(newRate_ * desc_.segmentCount_));

			oldSeg_ = std::clamp(oldSeg_, 0, desc_.segmentCount_);
			newSeg_ = std::clamp(newSeg_, 0, desc_.segmentCount_);

			// ダメージがあるのに差分0なら最低1セグ砕く
			if (hp_ < lastHp_ && oldSeg_ <= newSeg_) {
				oldSeg_ = std::min(desc_.segmentCount_, newSeg_ + 1);
			}
			// 破片生成
			SpawnShards_(newSeg_, oldSeg_);

			lastHp_ = hp_;
		} else if (hp_ > lastHp_) {
			// 回復/リセット時：検知ズレ防止で同期だけ取る
			lastHp_ = hp_;
			lagHp_ = float(hp_);
		}

		// ヒットパルス（スカッシュ）
		float pulse_ = 1.0f;
		if (hitPulse_ > 0.0f) {
			hitPulse_ -= dt;
			if (hitPulse_ < 0.0f) { hitPulse_ = 0.0f; }
			float t_ = hitPulse_ / 0.12f;  // 1..0
			pulse_ = 1.0f + (t_ * 0.30f);
		}

		// 本体バー：即時反映
		float rate_ = float(hp_) / float(maxHp_);
		float w_ = desc_.size_.x * rate_;
		fill_->SetSize({ w_, desc_.size_.y * pulse_ });

		// lagSpeed_ を「px/秒」として使う
		const float hpPerPixel_ = float(maxHp_) / std::max(1.0f, desc_.size_.x);
		const float lagHpStep_ = desc_.lagSpeed_ * dt * hpPerPixel_;
		if (lagHp_ > float(hp_)) {
			lagHp_ = std::max(float(hp_), lagHp_ - lagHpStep_);
		} else {
			lagHp_ = float(hp_);
		}
		float lagRate_ = lagHp_ / float(maxHp_);
		float lw_ = desc_.size_.x * lagRate_;
		lagFill_->SetSize({ lw_, desc_.size_.y * (1.0f + (pulse_ - 1.0f) * 0.5f) });

		// シェイク（位置を揺らす）
		Vector2 basePos_ = desc_.pos_;
		Vector2 fillPos_ = { desc_.pos_.x + 5.0f, desc_.pos_.y + 5.0f };
		Vector2 off_{ 0.0f, 0.0f };

		if (shakeTimer_ > 0.0f) {
			shakeTimer_ -= dt;
			if (shakeTimer_ < 0.0f) { shakeTimer_ = 0.0f; }
			off_ = {
				RandRange_(-desc_.shakePower_, desc_.shakePower_),
				RandRange_(-desc_.shakePower_, desc_.shakePower_)
			};
		}

		frame_->SetPosition({ basePos_.x + off_.x, basePos_.y + off_.y });
		fill_->SetPosition({ fillPos_.x + off_.x, fillPos_.y + off_.y });
		lagFill_->SetPosition({ fillPos_.x + off_.x, fillPos_.y + off_.y });

		// -----------------------------
		// 色変化：単純回避
		// -----------------------------
		bool draining_ = (lagHp_ > float(hp_));
		float hpRate_ = float(hp_) / float(maxHp_);

		float lowT_ = 0.0f;
		if (hpRate_ < desc_.lowHpStartRate_) {
			lowT_ = (desc_.lowHpStartRate_ - hpRate_) / std::max(0.0001f, desc_.lowHpStartRate_);
		}

		Vector4 col_ = LerpColor_(desc_.baseColor_, desc_.lowHpColor_, lowT_);

		if (draining_) {
			float flicker_ = 0.5f + 0.5f * std::sin(time_ * 32.0f);
			float t_ = 0.55f + 0.20f * flicker_;
			col_ = LerpColor_(col_, desc_.drainColor_, t_);
		}

		if (hitPulse_ > 0.0f) {
			float t_ = std::clamp(hitPulse_ / 0.12f, 0.0f, 1.0f);
			col_ = LerpColor_(col_, desc_.flashColor_, t_);
		}

		fill_->SetColor(col_);

		// -----------------------------
		// 減った区間の残像（lw - w）
		// -----------------------------
		if (drainGlow_) {
			if (drainGlowTimer_ > 0.0f) {
				drainGlowTimer_ -= dt;
				if (drainGlowTimer_ < 0.0f) { drainGlowTimer_ = 0.0f; }

				float a_ = drainGlowTimer_ / std::max(0.0001f, desc_.drainGlowTime_);
				float glowW_ = std::max(0.0f, lw_ - w_);

				drainGlow_->SetSize({ glowW_, desc_.size_.y });
				drainGlow_->SetPosition({ (fillPos_.x + off_.x) + w_, (fillPos_.y + off_.y) });
				drainGlow_->SetColor({ 1.0f, 0.55f, 0.55f, 0.65f * a_ });
			} else {
				drainGlow_->SetSize({ 0.0f, desc_.size_.y });
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
			if (!s.alive_) { continue; }

			s.t_ += dt;
			if (s.t_ >= s.life_) {
				s.alive_ = false;
				continue;
			}

			Vector2 p_ = s.sp_->GetPosition();
			p_.x += s.vel_.x * dt;
			p_.y += s.vel_.y * dt;

			// 重力っぽく
			s.vel_.y += 520.0f * dt;

			s.rot_ += s.rotVel_ * dt;

			// フェード
			float a_ = 1.0f - (s.t_ / s.life_);
			s.sp_->SetColor({ 1,1,1,a_ });

			s.sp_->SetPosition(p_);
			s.sp_->SetRotation(s.rot_);
			s.sp_->Update();
		}
	}

	void BossHpBarUI::Draw() {
		if (!visible_) { return; }
		if (frame_) { frame_->Draw(); }
		if (lagFill_) { lagFill_->Draw(); }
		if (drainGlow_) { drainGlow_->Draw(); }
		if (fill_) { fill_->Draw(); }

		for (auto& s : shards_) {
			if (s.alive_ && s.sp_) {
				s.sp_->Draw();
			}
		}
	}
} // namespace TKM