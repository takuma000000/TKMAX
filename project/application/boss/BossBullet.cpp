#include "BossBullet.h"
#include <cmath>

void BossBullet::Initialize(
	TKM::Object3dCommon* common,
	TKM::DirectXCommon* dx,
	TKM::Camera* cam,
	const Vector3& pos,
	const Vector3& dir,
	float speed,
	int damage,
	int lifeFrame
) {
	obj_ = std::make_unique<TKM::Object3d>();
	obj_->Initialize(common, dx);
	obj_->SetModel("sphere.obj"); // モデル指定
	obj_->SetScale({ kDefaultScale_, kDefaultScale_, kDefaultScale_ }); // スケール
	obj_->SetTranslate(pos);
	if (cam) obj_->SetCamera(cam);

	dir_ = dir;
	speed_ = speed;
	damage_ = damage;
	life_ = lifeFrame;
}

void BossBullet::Update() {
	if (dead_) return;

	// フレームカウント
	++ageFrame_;
	// 寿命チェック
	Vector3 newPos_{};
	// 曲線移動用フラグ
	bool reachedCurveEnd_ = false;

	if (useCurve_) {
		// t: 0→1
		++curveFrame_;
		float t_ = (curveTotalFrames_ > 0) ? (float)curveFrame_ / (float)curveTotalFrames_ : 1.0f;
		if (t_ > 1.0f) t_ = 1.0f;

		// 2次ベジェ
		const float u_ = 1.0f - t_;
		newPos_.x = (u_ * u_) * curveStart_.x + 2.0f * u_ * t_ * curveCtrl_.x + (t_ * t_) * curveEnd_.x;
		newPos_.y = (u_ * u_) * curveStart_.y + 2.0f * u_ * t_ * curveCtrl_.y + (t_ * t_) * curveEnd_.y;
		newPos_.z = (u_ * u_) * curveStart_.z + 2.0f * u_ * t_ * curveCtrl_.z + (t_ * t_) * curveEnd_.z;

		if (t_ >= 1.0f) {
			reachedCurveEnd_ = true;
		}
	} else {
		// 直進
		Vector3 p_ = obj_->GetTranslate();
		newPos_.x = p_.x + dir_.x * speed_;
		newPos_.y = p_.y + dir_.y * speed_;
		newPos_.z = p_.z + dir_.z * speed_;
	}

	// まず位置更新（どのモードでもここを通る）
	obj_->SetTranslate(newPos_);
	obj_->Update();

#ifdef USE_IMGUI
	// ───────── ボス弾当たり判定ワイヤーボックス描画 ─────────
	{
		Vector3 center_ = obj_->GetTranslate();
		const float r_ = Radius(); // 簡易半径（kDefaultScale_）
		Vector3 size_{ r_ * 2.0f, r_ * 2.0f, r_ * 2.0f };

		auto* lr_ = TKM::LineRenderer::GetInstance();
		if (lr_) {
			TKM::LineRenderer::Color col_{ 1.0f, 0.8f, 0.2f, 1.0f }; // 黄っぽい
			lr_->AddAABB(center_, size_, col_);
		}
		// ───────── SlashWave の X字判定ワイヤー表示 ─────────
		if (fxType_ == FxType::SlashWave) {
			auto Cross_ = [](const Vector3& a, const Vector3& b) {
				return Vector3{
					a.y * b.z - a.z * b.y,
					a.z * b.x - a.x * b.z,
					a.x * b.y - a.y * b.x
				};
				};

			Vector3 pos_ = obj_->GetTranslate();
			Vector3 fwd_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f });

			Vector3 upA_{ 0.0f, 1.0f, 0.0f };
			if (std::fabs(fwd_.y) > 0.90f) { upA_ = { 0.0f, 0.0f, 1.0f }; }

			Vector3 right_ = MyMath::SafeNormalize(Cross_(upA_, fwd_), { 1.0f, 0.0f, 0.0f });
			Vector3 up_ = MyMath::SafeNormalize(Cross_(fwd_, right_), { 0.0f, 1.0f, 0.0f });

			const float halfLen_ = 14.0f;
			const float back_ = 8.0f;
			const int   seg_ = 24;
			const Vector3 segSize_{ 4.0f, 3.0f, 4.0f };

			Vector3 base_ = pos_ - fwd_ * back_;

			const float c = 0.70710678f;
			const float s = 0.70710678f;
			Vector3 diag1_ = right_ * c + up_ * s;
			Vector3 diag2_ = right_ * c - up_ * s;

			auto* lr2_ = TKM::LineRenderer::GetInstance();
			if (lr2_) {
				TKM::LineRenderer::Color colA_{ 1.0f, 0.2f, 0.2f, 1.0f }; // 赤
				TKM::LineRenderer::Color colB_{ 0.2f, 0.9f, 1.0f, 1.0f }; // 水色

				for (int i = 0; i < seg_; ++i) {
					float t = (seg_ <= 1) ? 0.0f : (float)i / (float)(seg_ - 1);
					float u = (t * 2.0f - 1.0f);
					float along = u * halfLen_;

					Vector3 p1 = base_ + diag1_ * along;
					Vector3 p2 = base_ + diag2_ * along;

					lr2_->AddAABB(p1, segSize_, colA_);
					lr2_->AddAABB(p2, segSize_, colB_);
				}
			}
		}
	}
#endif

	// ==========================
	// Particle FX（弾種で分岐）
	// ==========================
	{
		auto* pm_ = TKM::ParticleManager::GetInstance();
		if (pm_) {
			Vector3 fxPos_ = newPos_;

			if (fxType_ == FxType::MissileEvil) {
				// === ミサイル（今のまま）===
				pm_->Emit("bossEvil_core", fxPos_, 1);

				if ((fxFrame_ % 2) == 0) { pm_->Emit("bossEvil_smoke", fxPos_, 2); }
				if ((fxFrame_ % 4) == 0) { pm_->Emit("bossEvil_spark", fxPos_, 2); }
				if ((fxFrame_ % 6) == 0) { pm_->Emit("bossEvil_ring", fxPos_, 1); }

				pm_->Emit("bossEvil_trail", fxPos_, 2);

			} else {
				// === 斬撃（X字スラッシュ：2本を交差させる）===
				auto Cross_ = [](const Vector3& a, const Vector3& b) {
					return Vector3{
						a.y * b.z - a.z * b.y,
						a.z * b.x - a.x * b.z,
						a.x * b.y - a.y * b.x
					};
					};

				Vector3 fwd_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f });

				// fwd と平行だと right が死ぬので、up を状況で切り替える
				Vector3 upA_{ 0.0f, 1.0f, 0.0f };
				if (std::fabs(fwd_.y) > 0.90f) { upA_ = { 0.0f, 0.0f, 1.0f }; }

				Vector3 right_ = MyMath::SafeNormalize(Cross_(upA_, fwd_), { 1.0f, 0.0f, 0.0f });
				Vector3 up_ = MyMath::SafeNormalize(Cross_(fwd_, right_), { 0.0f, 1.0f, 0.0f });

				// ---- Xスラッシュの“サイズ” ----
				const float halfLen_ = 14.0f;  // X の腕の長さ
				const float halfWide_ = 3.0f;   // 太さ方向（見た目の幅）
				const int   seg_ = 22;     // 点密度（チマチマなら増やす）
				const float back_ = 8.0f;   // 中心を少し後ろにして“残光”っぽく

				Vector3 base_ = fxPos_ - fwd_ * back_;

				// 45度回転した2軸（Xを作るための斜め軸）
				const float c = 0.70710678f; // cos45
				const float s = 0.70710678f; // sin45
				Vector3 diag1_ = right_ * c + up_ * s;   // 右上方向
				Vector3 diag2_ = right_ * c - up_ * s;   // 右下方向

				// 2本の斬撃線を、中心で交差させて描く
				for (int i = 0; i < seg_; ++i) {
					float t = (seg_ <= 1) ? 0.0f : (float)i / (float)(seg_ - 1);
					float u = (t * 2.0f - 1.0f);         // -1..+1
					float along = u * halfLen_;

					// 線に“幅”を付ける（面っぽくする）
					float w = (1.0f - std::fabs(u)) * halfWide_; // 中央が太く、端が細い

					// 1本目（diag1）
					Vector3 p1 = base_ + diag1_ * along + diag2_ * w * 0.35f;
					Vector3 p2 = base_ + diag1_ * along - diag2_ * w * 0.35f;

					// 2本目（diag2）
					Vector3 q1 = base_ + diag2_ * along + diag1_ * w * 0.35f;
					Vector3 q2 = base_ + diag2_ * along - diag1_ * w * 0.35f;

					// Emitは Vector3& なので必ず変数で渡す
					Vector3 a1 = p1, a2 = p2, b1 = q1, b2 = q2;

					pm_->Emit("bossSlash_main", a1, 1);
					pm_->Emit("bossSlash_main", a2, 1);
					pm_->Emit("bossSlash_main", b1, 1);
					pm_->Emit("bossSlash_main", b2, 1);

					// 発光/残りは間引き（重い＋変にデカく見えるのを防ぐ）
					if ((i % 2) == 0) {
						Vector3 g1 = p1, g2 = q1;
						pm_->Emit("bossSlash_glow", g1, 1);
						pm_->Emit("bossSlash_glow", g2, 1);
					}
					if ((i % 3) == 0) {
						Vector3 t1 = p2, t2 = q2;
						pm_->Emit("bossSlash_tail", t1, 1);
						pm_->Emit("bossSlash_tail", t2, 1);
					}
				}

				// 火花：Xの4端点だけ（変な位置に散らない）
				if ((fxFrame_ % 3) == 0) {
					Vector3 tipA = base_ + diag1_ * halfLen_;
					Vector3 tipB = base_ - diag1_ * halfLen_;
					Vector3 tipC = base_ + diag2_ * halfLen_;
					Vector3 tipD = base_ - diag2_ * halfLen_;

					Vector3 s1 = tipA; pm_->Emit("bossSlash_spark", s1, 1);
					Vector3 s2 = tipB; pm_->Emit("bossSlash_spark", s2, 1);
					Vector3 s3 = tipC; pm_->Emit("bossSlash_spark", s3, 1);
					Vector3 s4 = tipD; pm_->Emit("bossSlash_spark", s4, 1);
				}
			}
		}
		++fxFrame_;
	}

	// 曲線→直進へ切り替え
	if (useCurve_ && reachedCurveEnd_) {
		// 終点の接線方向（end - ctrl）
		Vector3 dirT_{};
		dirT_.x = curveEnd_.x - curveCtrl_.x;
		dirT_.y = curveEnd_.y - curveCtrl_.y;
		dirT_.z = curveEnd_.z - curveCtrl_.z;

		const float len_ = std::sqrt(dirT_.x * dirT_.x + dirT_.y * dirT_.y + dirT_.z * dirT_.z);
		if (len_ > 0.0001f) {
			dirT_.x /= len_;
			dirT_.y /= len_;
			dirT_.z /= len_;
		} else {
			dirT_ = { 0.0f, 0.0f, 1.0f };
		}

		dir_ = dirT_;
		useCurve_ = false;

		// 通り過ぎ寿命
		life_ = 45;
		return;
	}

	// 寿命（直進中のみ減らす）
	if (!useCurve_) {
		if (--life_ <= 0) {
			dead_ = true;
		}
	}
}

void BossBullet::Draw(TKM::DirectXCommon* dx) {
	if (!dead_) obj_->Draw(dx);
}

void BossBullet::EnableCurveToTarget(const Vector3& start, const Vector3& end, float curveHeight, float speedPerFrame) {
	useCurve_ = true;
	curveStart_ = start;
	curveEnd_ = end;

	// 距離
	const Vector3 d_{ end.x - start.x, end.y - start.y, end.z - start.z };
	const float dist_ = std::sqrt(d_.x * d_.x + d_.y * d_.y + d_.z * d_.z);

	speed_ = speedPerFrame;
	const float sp_ = (speedPerFrame > 0.0001f) ? speedPerFrame : 0.0001f;
	curveTotalFrames_ = (int)std::ceil(dist_ / sp_);
	if (curveTotalFrames_ < 1) curveTotalFrames_ = 1;
	curveFrame_ = 0;

	// 中点
	curveMid_ = { (start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f, (start.z + end.z) * 0.5f };

	// 進行方向（XZ）
	Vector3 dirXZ_{ d_.x, 0.0f, d_.z };
	const float lenXZ_ = std::sqrt(dirXZ_.x * dirXZ_.x + dirXZ_.z * dirXZ_.z);
	if (lenXZ_ > 0.0001f) {
		dirXZ_.x /= lenXZ_;
		dirXZ_.z /= lenXZ_;
	} else {
		dirXZ_ = { 0.0f, 0.0f, 1.0f };
	}

	// 左右方向（up × dir）
	Vector3 perp_{ -dirXZ_.z, 0.0f, dirXZ_.x };

	// curveYawRad_ を「曲がり量」に変換（総回転量っぽく使う）
	const float totalYaw_ = curveYawRad_ * (float)curveTotalFrames_;

	// 0〜1に丸めた強さ（±は左右）
	float s_ = totalYaw_ / (3.14159265f * 0.5f); // 90度で1
	if (s_ > 1.0f) s_ = 1.0f;
	if (s_ < -1.0f) s_ = -1.0f;

	// 横ズレ量：距離に比例（見た目で分かるように）
	const float sideOffset_ = dist_ * 0.35f * s_; // ここ好みで 0.2〜0.6

	// 制御点：中点 + 横ズレ + 高さ
	curveCtrl_ = curveMid_;
	curveCtrl_.x += perp_.x * sideOffset_;
	curveCtrl_.z += perp_.z * sideOffset_;
	curveCtrl_.y += curveHeight;

	// 寿命調整
	if (life_ < curveTotalFrames_) {
		life_ = curveTotalFrames_; // 最低でも到達までは生かす
	}
}

bool BossBullet::HitTestSlashX(const Vector3& targetCenter, const Vector3& targetSize) const {
	if (dead_) return false;
	if (fxType_ != FxType::SlashWave) return false;

	AABB targetAABB(targetCenter, targetSize);

	// ---- X字の形（FXと同じ作り）----
	auto Cross_ = [](const Vector3& a, const Vector3& b) {
		return Vector3{
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		};
		};

	Vector3 pos_ = obj_ ? obj_->GetTranslate() : Vector3{};
	Vector3 fwd_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f });

	Vector3 upA_{ 0.0f, 1.0f, 0.0f };
	if (std::fabs(fwd_.y) > 0.90f) { upA_ = { 0.0f, 0.0f, 1.0f }; }

	Vector3 right_ = MyMath::SafeNormalize(Cross_(upA_, fwd_), { 1.0f, 0.0f, 0.0f });
	Vector3 up_ = MyMath::SafeNormalize(Cross_(fwd_, right_), { 0.0f, 1.0f, 0.0f });

	// FXで使ってる値と合わせる（見た目＝判定）
	const float halfLen_ = 14.0f;
	const float back_ = 8.0f;
	const int   seg_ = 24; // 点密度

	Vector3 base_ = pos_ - fwd_ * back_;

	const float c = 0.70710678f; // cos45
	const float s = 0.70710678f; // sin45
	Vector3 diag1_ = right_ * c + up_ * s;
	Vector3 diag2_ = right_ * c - up_ * s;

	const Vector3 segSize_{ 4.0f, 3.0f, 4.0f }; // セグメント当たりのAABBサイズ

	for (int i = 0; i < seg_; ++i) {
		float t = (seg_ <= 1) ? 0.0f : (float)i / (float)(seg_ - 1);
		float u = (t * 2.0f - 1.0f);   // -1..+1
		float along = u * halfLen_;

		// 2本分（X字）
		Vector3 p1 = base_ + diag1_ * along;
		Vector3 p2 = base_ + diag2_ * along;

		AABB segA(p1, segSize_);
		if (segA.IsCollidingWithAABB(targetAABB)) { return true; }

		AABB segB(p2, segSize_);
		if (segB.IsCollidingWithAABB(targetAABB)) { return true; }
	}
	return false;
}

void BossBullet::SetCamera(TKM::Camera* cam) {
	if (obj_) { obj_->SetCamera(cam); }
}

void BossBullet::SetCurveYaw(float yawRadPerFrame) {
	curveYawRad_ = yawRadPerFrame; // 1フレームあたりのヨー回転量をセット（曲がり量に影響）
}

void BossBullet::SetModel(const std::string& model) {
	if (obj_) { obj_->SetModel(model); } // モデルを変更するためのセッター（例: "sphere.obj" → "cube.obj" など）
}

void BossBullet::SetScale(const Vector3& s) {
	if (obj_) { obj_->SetScale(s); }
}

void BossBullet::SetFxType(FxType t) {
	fxType_ = t; // エフェクトタイプをセット（弾の見た目やパーティクルに影響）
}

void BossBullet::SetAttackId(int id) {
	attackId_ = id; // 攻撃IDをセット（斬撃の攻撃判定などで使用）
}