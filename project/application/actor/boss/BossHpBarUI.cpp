#include "BossHpBarUI.h"
#include <cmath>

namespace TKM {

	//=============================================================
	// 初期化
	//=============================================================
	void BossHpBarUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		desc_ = desc;

		//=========================================================
		// フレーム
		//=========================================================
		frame_ = std::make_unique<Sprite>();
		frame_->Initialize(spriteCommon_, dxCommon_, desc_.frameTex_);
		frame_->SetParentScene(parentScene_);
		frame_->SetAutoAdjustTextureSize(false); // 枠サイズは自前で制御
		frame_->SetAnchorPoint({ 0.0f, 0.0f });
		frame_->SetPosition(desc_.pos_);
		frame_->SetSize({ desc_.size_.x + 10.0f, desc_.size_.y + 10.0f });
		frame_->SetColor({ 1.0f, 1.0f, 1.0f, 0.5f });

		//=========================================================
		// 遅延バー（後ろ）
		//=========================================================
		lagFill_ = std::make_unique<Sprite>();
		lagFill_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
		lagFill_->SetParentScene(parentScene_);
		lagFill_->SetAutoAdjustTextureSize(false);
		lagFill_->SetAnchorPoint({ 0.0f, 0.0f });
		lagFill_->SetPosition({ desc_.pos_.x + 5.0f, desc_.pos_.y + 5.0f });
		lagFill_->SetSize(desc_.size_);
		lagFill_->SetColor({ 1.0f, 0.35f, 0.35f, 1.0f }); // ダメージ寄りの色

		//=========================================================
		// 本体バー（前）
		//=========================================================
		fill_ = std::make_unique<Sprite>();
		fill_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
		fill_->SetParentScene(parentScene_);
		fill_->SetAutoAdjustTextureSize(false);
		fill_->SetAnchorPoint({ 0.0f, 0.0f });
		fill_->SetPosition({ desc_.pos_.x + 5.0f, desc_.pos_.y + 5.0f });
		fill_->SetSize(desc_.size_);
		fill_->SetColor(desc_.baseColor_);

		//=========================================================
		// 減少区間の残像
		// lagFill_ - fill_ の差分だけ光って消える帯
		//=========================================================
		drainGlow_ = std::make_unique<Sprite>();
		drainGlow_->Initialize(spriteCommon_, dxCommon_, desc_.fillTex_);
		drainGlow_->SetParentScene(parentScene_);
		drainGlow_->SetAutoAdjustTextureSize(false);
		drainGlow_->SetAnchorPoint({ 0.0f, 0.0f });
		drainGlow_->SetPosition({ desc_.pos_.x + 5.0f, desc_.pos_.y + 5.0f });
		drainGlow_->SetSize({ 0.0f, desc_.size_.y });
		drainGlow_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

		//=========================================================
		// 破片プール
		//=========================================================
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

		//=========================================================
		// 状態初期化
		//=========================================================
		initialized_ = false;      // HP初期値事故防止のため false スタート
		lastHp_ = 0;               // 前フレームのHP
		lagHp_ = 0.0f;             // 遅延バー用HP
		hitPulse_ = 0.0f;          // 被弾パルスタイマー
		drainGlowTimer_ = 0.0f;    // 残像タイマー
		time_ = 0.0f;              // 経過時間
	}

	//=============================================================
	// 乱数ヘルパ
	//=============================================================
	float BossHpBarUI::RandRange_(float a, float b) {
		return a + (b - a) * MyMath::Rand01();
	}

	//=============================================================
	// 破片生成
	//=============================================================
	void BossHpBarUI::SpawnShards_(int segBegin, int segEnd) {
		segBegin = std::max(segBegin, 0);
		segEnd = std::min(segEnd, desc_.segmentCount_);

		// 右端から「減った分」を砕く
		const float segW_ = desc_.size_.x / float(desc_.segmentCount_);
		for (int i = segBegin; i < segEnd; ++i) {
			//=====================================================
			// 空いている破片スロット検索
			//=====================================================
			Shard* slot_ = nullptr;
			for (auto& s : shards_) {
				if (!s.alive_) {
					slot_ = &s;
					break;
				}
			}
			if (!slot_) {
				break;
			}

			//=====================================================
			// 生成位置
			//=====================================================
			float x_ = (desc_.pos_.x + 5.0f) + segW_ * (float(i) + 0.5f);
			float y_ = (desc_.pos_.y + 5.0f) + desc_.size_.y * 0.5f;

			//=====================================================
			// 破片初期化
			//=====================================================
			slot_->alive_ = true;
			slot_->t_ = 0.0f;
			slot_->life_ = desc_.shardLife_;

			// ランダムな角度・速度で飛ばす
			float spd_ = RandRange_(desc_.shardSpeedMin_, desc_.shardSpeedMax_);
			float ang_ = RandRange_(-1.3f, 1.3f);
			slot_->vel_ = { std::cos(ang_) * spd_, std::sin(ang_) * spd_ - 120.0f };

			// 回転速度もランダム
			slot_->rot_ = 0.0f;
			slot_->rotVel_ = RandRange_(-desc_.shardRotSpeed_, desc_.shardRotSpeed_);

			// スプライト反映
			slot_->sp_->SetPosition({ x_, y_ });
			slot_->sp_->SetColor({ 1,1,1,1 });
		}
	}

	//=============================================================
	// 色補間
	//=============================================================
	Vector4 BossHpBarUI::LerpColor_(const Vector4& a, const Vector4& b, float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return {
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t,
			a.w + (b.w - a.w) * t
		};
	}

	//=============================================================
	// 更新
	//=============================================================
	void BossHpBarUI::Update(float dt, BossEnemy* boss) {
		//=========================================================
		// 更新前チェック
		//=========================================================
		if (!visible_) { return; }      // 非表示なら更新しない
		if (!boss) { return; }          // ボス不在なら更新しない
		if (boss->IsDead()) { return; } // ボス死亡中なら更新しない

		// ボスのHPを参照してUIを更新
		time_ += dt;

		//=========================================================
		// HP取得
		//=========================================================
		maxHp_ = std::max(1, boss->GetMaxHP());              // 最大HPは 1 以上
		hp_ = std::clamp(boss->GetHP(), 0, maxHp_);         // 現在HPを 0..maxHp_ にクランプ

		//=========================================================
		// 初回同期
		// lastHp_ の事故を防ぐため、初回は現在HPに合わせる
		//=========================================================
		if (!initialized_ || lastHp_ < 0 || lastHp_ > maxHp_) {
			lastHp_ = hp_;
			lagHp_ = float(hp_);
			initialized_ = true;
		}

		//=========================================================
		// 被弾検出
		// HPが減った瞬間だけ演出を発火する
		//=========================================================
		if (hp_ < lastHp_) {
			shakeTimer_ = desc_.shakeTime_;              // シェイク開始
			hitPulse_ = 0.12f;                           // パルス開始
			drainGlowTimer_ = desc_.drainGlowTime_;      // 残像開始

			float oldRate_ = float(lastHp_) / float(maxHp_); // 前フレームHP率
			float newRate_ = float(hp_) / float(maxHp_);     // 今フレームHP率

			int oldSeg_ = int(std::ceil(oldRate_ * desc_.segmentCount_));   // 前フレームのセグメント数
			int newSeg_ = int(std::floor(newRate_ * desc_.segmentCount_));  // 今フレームのセグメント数

			// セグメント数を有効範囲に収める
			oldSeg_ = std::clamp(oldSeg_, 0, desc_.segmentCount_);
			newSeg_ = std::clamp(newSeg_, 0, desc_.segmentCount_);

			// ダメージがあるのに差分0なら最低1セグメント砕く
			if (hp_ < lastHp_ && oldSeg_ <= newSeg_) {
				oldSeg_ = std::min(desc_.segmentCount_, newSeg_ + 1);
			}

			// 破片生成
			SpawnShards_(newSeg_, oldSeg_);

			// HP減少時はここで lastHp_ を更新
			lastHp_ = hp_;
		} else if (hp_ > lastHp_) {
			//=====================================================
			// HP回復時
			// 回復演出は特に入れず、そのまま即同期する
			//=====================================================
			lastHp_ = hp_;
			lagHp_ = float(hp_);
		}

		//=========================================================
		// 被弾パルス（スカッシュ）
		//=========================================================
		float pulse_ = 1.0f;
		if (hitPulse_ > 0.0f) {
			hitPulse_ -= dt;
			if (hitPulse_ < 0.0f) {
				hitPulse_ = 0.0f;
			}

			float t_ = hitPulse_ / 0.12f;     // 1..0
			pulse_ = 1.0f + (t_ * 0.30f);     // 1.0..1.3
		}

		//=========================================================
		// 本体バー
		// HPは即時反映
		//=========================================================
		float rate_ = float(hp_) / float(maxHp_);
		float w_ = desc_.size_.x * rate_;
		fill_->SetSize({ w_, desc_.size_.y * pulse_ });

		//=========================================================
		// 遅延バー
		// HP減少時はゆっくり追従、回復時は即時反映
		//=========================================================
		const float hpPerPixel_ = float(maxHp_) / std::max(1.0f, desc_.size_.x);
		const float lagHpStep_ = desc_.lagSpeed_ * dt * hpPerPixel_;

		if (lagHp_ > float(hp_)) {
			lagHp_ = std::max(float(hp_), lagHp_ - lagHpStep_);
		} else {
			lagHp_ = float(hp_);
		}

		float lagRate_ = lagHp_ / float(maxHp_);
		float lw_ = desc_.size_.x * lagRate_;

		// 本体より少し控えめにパルスを乗せる
		lagFill_->SetSize({
			lw_,
			desc_.size_.y * (1.0f + (pulse_ - 1.0f) * 0.5f)
			});

		//=========================================================
		// シェイク
		//=========================================================
		Vector2 basePos_ = desc_.pos_;
		Vector2 fillPos_ = { desc_.pos_.x + 5.0f, desc_.pos_.y + 5.0f };
		Vector2 off_{ 0.0f, 0.0f };

		if (shakeTimer_ > 0.0f) {
			shakeTimer_ -= dt;
			if (shakeTimer_ < 0.0f) {
				shakeTimer_ = 0.0f;
			}

			off_ = {
				RandRange_(-desc_.shakePower_, desc_.shakePower_),
				RandRange_(-desc_.shakePower_, desc_.shakePower_)
			};
		}

		//=========================================================
		// 位置反映
		//=========================================================
		frame_->SetPosition({ basePos_.x + off_.x, basePos_.y + off_.y });
		fill_->SetPosition({ fillPos_.x + off_.x, fillPos_.y + off_.y });
		lagFill_->SetPosition({ fillPos_.x + off_.x, fillPos_.y + off_.y });

		//=========================================================
		// 色変化
		//=========================================================
		bool draining_ = (lagHp_ > float(hp_));                // HP減少中か
		float hpRate_ = float(hp_) / float(maxHp_);            // 現在HP率
		float lowT_ = 0.0f;                                    // 低HP域ブレンド率

		// lowHpStartRate_ を下回るほど lowHpColor_ に寄せる
		if (hpRate_ < desc_.lowHpStartRate_) {
			lowT_ = (desc_.lowHpStartRate_ - hpRate_) / std::max(0.0001f, desc_.lowHpStartRate_);
		}

		// ベース色 → 低HP色
		Vector4 col_ = LerpColor_(desc_.baseColor_, desc_.lowHpColor_, lowT_);

		// HP減少中はさらに drainColor_ を周期的に混ぜる
		if (draining_) {
			float flicker_ = 0.5f + 0.5f * std::sin(time_ * 32.0f); // 0..1
			float t_ = 0.55f + 0.20f * flicker_;                    // 0.55..0.75
			col_ = LerpColor_(col_, desc_.drainColor_, t_);
		}

		// 被弾直後は flashColor_ に寄せる
		if (hitPulse_ > 0.0f) {
			float t_ = std::clamp(hitPulse_ / 0.12f, 0.0f, 1.0f);
			col_ = LerpColor_(col_, desc_.flashColor_, t_);
		}

		fill_->SetColor(col_);

		//=========================================================
		// 減った区間の残像
		// 遅延バーと本体バーの差分だけ表示する
		//=========================================================
		if (drainGlow_) {
			if (drainGlowTimer_ > 0.0f) {
				drainGlowTimer_ -= dt;
				if (drainGlowTimer_ < 0.0f) {
					drainGlowTimer_ = 0.0f;
				}

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

		//=========================================================
		// スプライト更新
		//=========================================================
		if (frame_) { frame_->Update(); }
		if (lagFill_) { lagFill_->Update(); }
		if (drainGlow_) { drainGlow_->Update(); }
		if (fill_) { fill_->Update(); }

		//=========================================================
		// 破片更新
		//=========================================================
		for (auto& s : shards_) {
			if (!s.alive_) {
				continue;
			}

			s.t_ += dt;

			// 寿命切れなら非表示
			if (s.t_ >= s.life_) {
				s.alive_ = false;
				continue;
			}

			// 位置更新
			Vector2 p_ = s.sp_->GetPosition();
			p_.x += s.vel_.x * dt;
			p_.y += s.vel_.y * dt;

			// 重力っぽい加速度
			s.vel_.y += 520.0f * dt;

			// 回転更新
			s.rot_ += s.rotVel_ * dt;

			// フェード
			float a_ = 1.0f - (s.t_ / s.life_);
			s.sp_->SetColor({ 1,1,1,a_ });
			s.sp_->SetPosition(p_);
			s.sp_->SetRotation(s.rot_);
			s.sp_->Update();
		}
	}

	//=============================================================
	// 描画
	//=============================================================
	void BossHpBarUI::Draw() {
		if (!visible_) {
			return;
		}

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

	//=============================================================
	// 表示切り替え
	//=============================================================
	void BossHpBarUI::SetVisible(bool v) {
		visible_ = v;

		// ボス戦開始などで再表示したとき、HP同期を取り直す
		if (v) {
			initialized_ = false;
			lastHp_ = 0;
			lagHp_ = 0.0f;
			drainGlowTimer_ = 0.0f;
			hitPulse_ = 0.0f;
		}
	}

} // namespace TKM