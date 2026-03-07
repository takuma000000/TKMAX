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

		// 状態初期化
		initialized_ = false; // HP初期値事故防止のためfalseスタート
		lastHp_ = 0; // 前フレームのHP
		lagHp_ = 0.0f; // 遅延バーのHP（floatで滑らかに）
		hitPulse_ = 0.0f; // 被弾パルス（スカッシュ）タイマー
		drainGlowTimer_ = 0.0f; // 減少区間の残像タイマー
		time_ = 0.0f; // 経過時間（全体の時間管理用、シャードの動きなどで使用）
	}

	float BossHpBarUI::RandRange_(float a, float b) {
		return a + (b - a) * MyMath::Rand01();
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

			// 生成位置はセグメントの中央あたり
			float x_ = (desc_.pos_.x + 5.0f) + segW_ * (float(i) + 0.5f);
			float y_ = (desc_.pos_.y + 5.0f) + desc_.size_.y * 0.5f;
			// 生成
			slot_->alive_ = true;
			slot_->t_ = 0.0f;
			slot_->life_ = desc_.shardLife_;
			// 速度はランダムな角度で、速さもランダム
			float spd_ = RandRange_(desc_.shardSpeedMin_, desc_.shardSpeedMax_);
			float ang_ = RandRange_(-1.3f, 1.3f);
			slot_->vel_ = { std::cos(ang_) * spd_, std::sin(ang_) * spd_ - 120.0f };
			// 回転速度もランダム
			slot_->rot_ = 0.0f;
			slot_->rotVel_ = RandRange_(-desc_.shardRotSpeed_, desc_.shardRotSpeed_);
			// スプライトの初期状態
			slot_->sp_->SetPosition({ x_, y_ });
			slot_->sp_->SetColor({ 1,1,1,1 });
		}
	}

	Vector4 BossHpBarUI::LerpColor_(const Vector4& a, const Vector4& b, float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return {
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t,
			a.w + (b.w - a.w) * t
		};
	}

	void BossHpBarUI::Update(float dt, BossEnemy* boss) {
		if (!visible_) { return; } // 非表示なら更新しない
		if (!boss) { return; } // ボスがいないなら更新しない
		if (boss->IsDead()) { return; } // ボスが死んでるなら更新しない
		// ボスのHPを参照してUIを更新
		time_ += dt;

		maxHp_ = std::max(1, boss->GetMaxHP()); // 最大HPは1以上（0だと割り算で死ぬ）
		hp_ = std::clamp(boss->GetHP(), 0, maxHp_); // 現HPは0..maxHp_の範囲にクランプ

		// 初回同期（lastHp_初期値事故防止）
		if (!initialized_ || lastHp_ < 0 || lastHp_ > maxHp_) {
			lastHp_ = hp_; // 初期値事故防止のため、初回は強制的に現在HPに合わせる
			lagHp_ = float(hp_); // 遅延バーも合わせる
			initialized_ = true; // 初期化完了
		}

		// 被弾検出：hpが減った瞬間だけ演出
		if (hp_ < lastHp_) {
			shakeTimer_ = desc_.shakeTime_; // 画面震えタイマー
			hitPulse_ = 0.12f; // 被弾パルスタイマー
			drainGlowTimer_ = desc_.drainGlowTime_; // 減少区間の残像タイマー

			float oldRate_ = float(lastHp_) / float(maxHp_); // 前フレームのHP率
			float newRate_ = float(hp_) / float(maxHp_); // 今フレームのHP率

			int oldSeg_ = int(std::ceil(oldRate_ * desc_.segmentCount_)); // 前フレームのセグメント数（ceilで切り上げ：HPが減ってるのにセグメント数が同じになるのを防止）
			int newSeg_ = int(std::floor(newRate_ * desc_.segmentCount_)); // 今フレームのセグメント数（floorで切り捨て：HPが減ってるのにセグメント数が同じになるのを防止）
			
			// セグメント数は0..segmentCount_の範囲にクランプ
			oldSeg_ = std::clamp(oldSeg_, 0, desc_.segmentCount_);
			newSeg_ = std::clamp(newSeg_, 0, desc_.segmentCount_);

			// ダメージがあるのに差分0なら最低1セグ砕く
			if (hp_ < lastHp_ && oldSeg_ <= newSeg_) {
				oldSeg_ = std::min(desc_.segmentCount_, newSeg_ + 1);
			}
			// 破片生成
			SpawnShards_(newSeg_, oldSeg_);
			// HP減少時は lastHp_ を更新しておく（増加時は後でまとめて更新）
			lastHp_ = hp_;
		} else if (hp_ > lastHp_) { // HPが増えたときは、HP増加の演出は特にないので、ここで
			lastHp_ = hp_; // HP増加を検出したら lastHp_ を更新
			lagHp_ = float(hp_); // 遅延バーも即座に合わせる（HP回復は遅延させない）
		}

		// ヒットパルス（スカッシュ）
		float pulse_ = 1.0f;
		if (hitPulse_ > 0.0f) { // 被弾パルスが残ってるときだけ
			hitPulse_ -= dt; // タイマーを減らす
			if (hitPulse_ < 0.0f) { hitPulse_ = 0.0f; } // クランプ
			float t_ = hitPulse_ / 0.12f;  // 1..0
			pulse_ = 1.0f + (t_ * 0.30f); // タイマーに応じて1.0..1.3のスケールを計算
		}

		// 本体バー：即時反映
		float rate_ = float(hp_) / float(maxHp_);
		float w_ = desc_.size_.x * rate_; // パルスに応じて高さを少し変える（スカッシュ）
		fill_->SetSize({ w_, desc_.size_.y * pulse_ }); // パルスに応じて高さを少し変える（スカッシュ）

		// lagSpeed_ を「px/秒」として使う
		const float hpPerPixel_ = float(maxHp_) / std::max(1.0f, desc_.size_.x);
		// 遅延バー：HPが減ってるときはゆっくり追従、増えてるときは即座に合わせる
		const float lagHpStep_ = desc_.lagSpeed_ * dt * hpPerPixel_;

		// 遅延バーのHPを更新
		if (lagHp_ > float(hp_)) {
			lagHp_ = std::max(float(hp_), lagHp_ - lagHpStep_); // HPが減ってるときはゆっくり追従（下限は現在HP）
		} else {
			lagHp_ = float(hp_); // HPが増えてるときは即座に合わせる
		}

		// 遅延バーのサイズを更新
		float lagRate_ = lagHp_ / float(maxHp_);
		// パルスに応じて高さを少し変える（スカッシュ）+ HP減少時は遅延バーも少し大きくして派手に見せる
		float lw_ = desc_.size_.x * lagRate_;
		lagFill_->SetSize({ lw_, desc_.size_.y * (1.0f + (pulse_ - 1.0f) * 0.5f) }); // パルスに応じて高さを少し変える（スカッシュ）+ HP減少時は遅延バーも少し大きくして派手に見せる

		// シェイク（位置を揺らす）
		Vector2 basePos_ = desc_.pos_;
		Vector2 fillPos_ = { desc_.pos_.x + 5.0f, desc_.pos_.y + 5.0f };
		Vector2 off_{ 0.0f, 0.0f };

		if (shakeTimer_ > 0.0f) { // シェイクタイマーが残ってるときだけ
			shakeTimer_ -= dt; // タイマーを減らす

			// タイマーに応じて0..shakePower_のランダムなオフセットを計算
			if (shakeTimer_ < 0.0f) { shakeTimer_ = 0.0f; }

			// シェイクのオフセットは、タイマーが減るにつれて小さくなるようにする（線形減衰）
			off_ = {
				RandRange_(-desc_.shakePower_, desc_.shakePower_),
				RandRange_(-desc_.shakePower_, desc_.shakePower_)
			};
		}

		// 位置にオフセットを加算して反映
		frame_->SetPosition({ basePos_.x + off_.x, basePos_.y + off_.y });
		fill_->SetPosition({ fillPos_.x + off_.x, fillPos_.y + off_.y });
		lagFill_->SetPosition({ fillPos_.x + off_.x, fillPos_.y + off_.y });

		// -----------------------------
		// 色変化：単純回避
		// -----------------------------
		bool draining_ = (lagHp_ > float(hp_)); // HPが減ってる最中は true
		float hpRate_ = float(hp_) / float(maxHp_); // 現在HP率（0..1）
		// 低HP域の割合（0..1）。hpRate_ が lowHpStartRate_ を下回るほど 1 に近づく
		float lowT_ = 0.0f;

		// 低HP域の割合（0..1）。hpRate_ が lowHpStartRate_ を下回るほど 1 に近づく
		if (hpRate_ < desc_.lowHpStartRate_) {
			lowT_ = (desc_.lowHpStartRate_ - hpRate_) / std::max(0.0001f, desc_.lowHpStartRate_); // hpRate_ が lowHpStartRate_ を下回るほど 0..1 に近づく
		}
		// ベース色から、低HP色へ寄せる
		Vector4 col_ = LerpColor_(desc_.baseColor_, desc_.lowHpColor_, lowT_);

		// HPが減ってる最中は、さらに drainColor_ へ寄せる（周期的に明滅させる）
		if (draining_) {
			float flicker_ = 0.5f + 0.5f * std::sin(time_ * 32.0f); // 0..1の周期的な値（sin波で明滅）
			float t_ = 0.55f + 0.20f * flicker_; // flicker_ に応じて 0.55..0.75 を行き来する値（drainColor_ への寄せ具合）
			col_ = LerpColor_(col_, desc_.drainColor_, t_); // さらに drainColor_ へ寄せる
		}

		if (hitPulse_ > 0.0f) { // 被弾パルスが残ってるときは、さらに flashColor_ へ寄せる（被弾直後の一瞬だけ明るく赤くする）
			float t_ = std::clamp(hitPulse_ / 0.12f, 0.0f, 1.0f); // hitPulse_ に応じて 1..0 の値（被弾直後は1、時間経過で0に近づく）
			col_ = LerpColor_(col_, desc_.flashColor_, t_); // flashColor_ へ寄せる
		}
		// 色を反映
		fill_->SetColor(col_);

		// -----------------------------
		// 減った区間の残像（lw - w）
		// -----------------------------
		if (drainGlow_) { // 安全にアクセス
			if (drainGlowTimer_ > 0.0f) { // タイマーが残ってるときだけ表示
				drainGlowTimer_ -= dt;

				// タイマーが減るにつれて、残像の幅を減らす（遅延バーと本体バーの差分だけ残像が出る）
				if (drainGlowTimer_ < 0.0f) { drainGlowTimer_ = 0.0f; }

				// 残像の幅は、遅延バーと本体バーの差分（lw - w）に、タイマーに応じた割合を掛ける
				float a_ = drainGlowTimer_ / std::max(0.0001f, desc_.drainGlowTime_);
				// HP減少の最中は、遅延バーと本体バーの差分（lw - w）に応じて残像が出る。タイマーが減るにつれて残像も減る。
				float glowW_ = std::max(0.0f, lw_ - w_);

				drainGlow_->SetSize({ glowW_, desc_.size_.y }); // 幅は遅延バーと本体バーの差分
				drainGlow_->SetPosition({ (fillPos_.x + off_.x) + w_, (fillPos_.y + off_.y) }); // 位置は本体バーの右端（オフセットも加算）
				drainGlow_->SetColor({ 1.0f, 0.55f, 0.55f, 0.65f * a_ }); // 色は薄い赤で、タイマーに応じて徐々に透明になる
			} else {
				drainGlow_->SetSize({ 0.0f, desc_.size_.y }); // タイマーが切れたら残像は消す
				drainGlow_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f }); // タイマーが切れたら残像は完全に透明にする
			}
		}

		// 更新
		if (frame_) { frame_->Update(); }
		if (lagFill_) { lagFill_->Update(); }
		if (drainGlow_) { drainGlow_->Update(); }
		if (fill_) { fill_->Update(); }

		// 破片更新
		for (auto& s : shards_) {
			// 生存してない破片は更新しない
			if (!s.alive_) { continue; }

			s.t_ += dt; // 経過時間を更新

			// 寿命切れなら非表示にして更新しない
			if (s.t_ >= s.life_) {
				s.alive_ = false; // 非表示にする
				continue;
			}
			// 位置を更新
			Vector2 p_ = s.sp_->GetPosition();
			p_.x += s.vel_.x * dt;
			p_.y += s.vel_.y * dt;

			// 重力っぽく
			s.vel_.y += 520.0f * dt;
			// 回転を更新
			s.rot_ += s.rotVel_ * dt;

			// フェード
			float a_ = 1.0f - (s.t_ / s.life_);
			s.sp_->SetColor({ 1,1,1,a_ }); // 時間経過に応じて透明になる
			s.sp_->SetPosition(p_); // 更新した位置を反映
			s.sp_->SetRotation(s.rot_); // 更新した回転を反映
			s.sp_->Update(); // スプライトの更新
		}
	}

	void BossHpBarUI::Draw() {
		if (!visible_) { return; }
		if (frame_) { frame_->Draw(); }
		if (lagFill_) { lagFill_->Draw(); }
		if (drainGlow_) { drainGlow_->Draw(); }
		if (fill_) { fill_->Draw(); }

		for (auto& s : shards_) {
			if (s.alive_ && s.sp_) { // 生存してる破片だけ描画
				s.sp_->Draw();
			}
		}
	}
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