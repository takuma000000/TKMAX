#include "BossBullet.h"
#include <cmath>

//=============================================================
// 初期化
//=============================================================
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
	//=========================================================
	// Object3d 初期化
	//=========================================================
	obj_ = std::make_unique<TKM::Object3d>();              // Object3d インスタンス生成
	obj_->Initialize(common, dx);                          // Object3d 初期化
	obj_->SetModel("sphere.obj");                          // モデル指定
	obj_->SetScale({ kDefaultScale_, kDefaultScale_, kDefaultScale_ }); // 初期スケール設定
	obj_->SetTranslate(pos);                               // 初期位置設定

	// カメラが有効なら描画用カメラを設定
	if (cam) {
		obj_->SetCamera(cam);
	}

	//=========================================================
	// 基本状態初期化
	//=========================================================
	cam_ = cam;
	prevPos_ = pos;
	trailPts_.clear();
	trailPts_.push_back(pos);
	trailDistAcc_ = 0.0f;

	//=========================================================
	// 弾パラメータ初期化
	//=========================================================
	dir_ = dir;          // 進行方向
	speed_ = speed;      // 速度
	damage_ = damage;    // ダメージ量
	life_ = lifeFrame;   // 寿命フレーム
}

//=============================================================
// 更新
//=============================================================
void BossBullet::Update() {
	// 既に死亡済みなら何もしない
	if (dead_) {
		return;
	}

	//=========================================================
	// トレイルフェードアウト中処理
	//=========================================================
	if (isTrailFading_) {
		// 先頭から少しずつ削ってトレイルを消していく
		if (!trailPts_.empty()) {
			trailPts_.erase(trailPts_.begin());
		}

		// トレイルが消え切ったら完全削除
		if (trailPts_.size() <= 1) {
			trailPts_.clear();
			dead_ = true;
		}

		return;
	}

	//=========================================================
	// フレーム進行
	//=========================================================
	++ageFrame_; // 経過フレーム加算

	Vector3 newPos_{};           // 更新後座標
	bool reachedCurveEnd_ = false; // 曲線移動終点到達フラグ

	//=========================================================
	// 移動更新
	//=========================================================
	if (useCurve_) {
		//=====================================================
		// 曲線移動
		//=====================================================
		++curveFrame_; // 曲線移動の進行フレームを加算

		// 進行率 t を 0.0～1.0 で計算
		float t_ = (curveTotalFrames_ > 0) ? (float)curveFrame_ / (float)curveTotalFrames_ : 1.0f;
		if (t_ > 1.0f) {
			t_ = 1.0f;
		}

		// 2次ベジェ曲線
		// B(t) = (1-t)^2 * P0 + 2(1-t)t * P1 + t^2 * P2
		const float u_ = 1.0f - t_;
		newPos_.x = (u_ * u_) * curveStart_.x + 2.0f * u_ * t_ * curveCtrl_.x + (t_ * t_) * curveEnd_.x;
		newPos_.y = (u_ * u_) * curveStart_.y + 2.0f * u_ * t_ * curveCtrl_.y + (t_ * t_) * curveEnd_.y;
		newPos_.z = (u_ * u_) * curveStart_.z + 2.0f * u_ * t_ * curveCtrl_.z + (t_ * t_) * curveEnd_.z;

		// 終点まで到達したらフラグを立てる
		if (t_ >= 1.0f) {
			reachedCurveEnd_ = true;
		}
	} else {
		//=====================================================
		// 直進移動
		//=====================================================
		Vector3 p_ = obj_->GetTranslate();
		newPos_.x = p_.x + dir_.x * speed_;
		newPos_.y = p_.y + dir_.y * speed_;
		newPos_.z = p_.z + dir_.z * speed_;
	}

	//=========================================================
	// 位置反映
	//=========================================================
	prevPos_ = obj_->GetTranslate(); // 更新前位置を保存
	obj_->SetTranslate(newPos_);     // 新しい位置を設定

	//=========================================================
	// 軌跡更新
	// ミサイル系エフェクトのときだけトレイルを更新する
	//=========================================================
	if (fxType_ == FxType::MissileEvil) {
		UpdateTrail(newPos_);
	}

	// ワールド行列などを再計算
	obj_->Update();

#ifdef USE_IMGUI
	//=========================================================
	// デバッグ表示
	//=========================================================
	{
		Vector3 center_ = obj_->GetTranslate();
		const float r_ = Radius(); // 簡易半径
		Vector3 size_{ r_ * 2.0f, r_ * 2.0f, r_ * 2.0f };

		//=====================================================
		// ボス弾当たり判定ワイヤーボックス描画
		//=====================================================
		auto* lr_ = TKM::LineRenderer::GetInstance();
		if (lr_) {
			TKM::LineRenderer::Color col_{ 1.0f, 0.8f, 0.2f, 1.0f }; // 黄っぽい色
			lr_->AddAABB(center_, size_, col_);
		}

		//=====================================================
		// SlashWave 用 X字判定ワイヤー表示
		//=====================================================
		if (fxType_ == FxType::SlashWave) {
			auto Cross_ = [](const Vector3& a, const Vector3& b) {
				// ベクトルの外積を計算する関数
				return Vector3{
					a.y * b.z - a.z * b.y,
					a.z * b.x - a.x * b.z,
					a.x * b.y - a.y * b.x
				};
				};

			Vector3 pos_ = obj_->GetTranslate(); // 現在位置
			Vector3 fwd_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f }); // 進行方向を正規化

			// fwd と平行だと right が作れないため、up を状況で切り替える
			Vector3 upA_{ 0.0f, 1.0f, 0.0f };
			if (std::fabs(fwd_.y) > 0.90f) {
				upA_ = { 0.0f, 0.0f, 1.0f };
			}

			// 直交座標系を作成
			Vector3 right_ = MyMath::SafeNormalize(Cross_(upA_, fwd_), { 1.0f, 0.0f, 0.0f });
			Vector3 up_ = MyMath::SafeNormalize(Cross_(fwd_, right_), { 0.0f, 1.0f, 0.0f });

			const float halfLen_ = 14.0f;                 // X の腕の長さ
			const float back_ = 8.0f;                     // 少し後ろにずらして残光感を出す
			const int   seg_ = 24;                        // 点密度
			const Vector3 segSize_{ 4.0f, 3.0f, 4.0f };  // 判定AABBサイズ

			Vector3 base_ = pos_ - fwd_ * back_; // X字中心位置

			const float c = 0.70710678f; // cos45
			const float s = 0.70710678f; // sin45
			Vector3 diag1_ = right_ * c + up_ * s; // 45度方向その1
			Vector3 diag2_ = right_ * c - up_ * s; // 45度方向その2

			auto* lr2_ = TKM::LineRenderer::GetInstance();
			if (lr2_) {
				TKM::LineRenderer::Color colA_{ 1.0f, 0.2f, 0.2f, 1.0f }; // 赤
				TKM::LineRenderer::Color colB_{ 0.2f, 0.9f, 1.0f, 1.0f }; // 水色

				for (int i = 0; i < seg_; ++i) {
					float t = (seg_ <= 1) ? 0.0f : (float)i / (float)(seg_ - 1); // 0..1
					float u = (t * 2.0f - 1.0f); // -1..+1
					float along = u * halfLen_;   // 中心からの距離

					// X字2本の線上にAABBを配置
					Vector3 p1 = base_ + diag1_ * along;
					Vector3 p2 = base_ + diag2_ * along;

					lr2_->AddAABB(p1, segSize_, colA_);
					lr2_->AddAABB(p2, segSize_, colB_);
				}
			}
		}
	}
#endif

	//=========================================================
	// Particle FX
	// 弾種ごとに見た目のエフェクトを分岐する
	//=========================================================
	{
		auto* pm_ = TKM::ParticleManager::GetInstance();
		if (pm_) {
			Vector3 fxPos_ = newPos_; // FX 発生位置は現在位置

			if (fxType_ == FxType::MissileEvil) {
				// ミサイルはパーティクルではなくトレイル描画に変更しているため、
				// ここでは何も出さない
			} else {
				//=====================================================
				// X字スラッシュエフェクト
				//=====================================================
				auto Cross_ = [](const Vector3& a, const Vector3& b) {
					return Vector3{
						a.y * b.z - a.z * b.y,
						a.z * b.x - a.x * b.z,
						a.x * b.y - a.y * b.x
					};
					};

				Vector3 fwd_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f });

				// fwd と平行だと right が作れないため、up を状況で切り替える
				Vector3 upA_{ 0.0f, 1.0f, 0.0f };
				if (std::fabs(fwd_.y) > 0.90f) {
					upA_ = { 0.0f, 0.0f, 1.0f };
				}

				Vector3 right_ = MyMath::SafeNormalize(Cross_(upA_, fwd_), { 1.0f, 0.0f, 0.0f });
				Vector3 up_ = MyMath::SafeNormalize(Cross_(fwd_, right_), { 0.0f, 1.0f, 0.0f });

				//=====================================================
				// Xスラッシュの見た目サイズ
				//=====================================================
				const float halfLen_ = 14.0f; // X の腕の長さ
				const float halfWide_ = 3.0f; // 太さ方向
				const int   seg_ = 22;        // 点密度
				const float back_ = 8.0f;     // 少し後ろにずらして残光感を出す

				Vector3 base_ = fxPos_ - fwd_ * back_;

				// 45度回転した2本の軸
				const float c = 0.70710678f; // cos45
				const float s = 0.70710678f; // sin45
				Vector3 diag1_ = right_ * c + up_ * s; // 右上方向
				Vector3 diag2_ = right_ * c - up_ * s; // 右下方向

				//=====================================================
				// X字の2本線を構成するパーティクル生成
				//=====================================================
				for (int i = 0; i < seg_; ++i) {
					float t = (seg_ <= 1) ? 0.0f : (float)i / (float)(seg_ - 1);
					float u = (t * 2.0f - 1.0f); // -1..+1
					float along = u * halfLen_;

					// 中央が太く、端が細くなるよう幅を調整
					float w = (1.0f - std::fabs(u)) * halfWide_;

					// 1本目（diag1）
					Vector3 p1 = base_ + diag1_ * along + diag2_ * w * 0.35f;
					Vector3 p2 = base_ + diag1_ * along - diag2_ * w * 0.35f;

					// 2本目（diag2）
					Vector3 q1 = base_ + diag2_ * along + diag1_ * w * 0.35f;
					Vector3 q2 = base_ + diag2_ * along - diag1_ * w * 0.35f;

					// Emit は Vector3& を取るため、一度変数に格納して渡す
					Vector3 a1 = p1;
					Vector3 a2 = p2;
					Vector3 b1 = q1;
					Vector3 b2 = q2;

					pm_->Emit("bossSlash_main", a1, 1);
					pm_->Emit("bossSlash_main", a2, 1);
					pm_->Emit("bossSlash_main", b1, 1);
					pm_->Emit("bossSlash_main", b2, 1);

					// 発光は間引きして片側のみ
					if ((i % 2) == 0) {
						Vector3 g1 = p1;
						Vector3 g2 = q1;
						pm_->Emit("bossSlash_glow", g1, 1);
						pm_->Emit("bossSlash_glow", g2, 1);
					}

					// 尾の表現も少し間引く
					if ((i % 3) == 0) {
						Vector3 t1 = p2;
						Vector3 t2 = q2;
						pm_->Emit("bossSlash_tail", t1, 1);
						pm_->Emit("bossSlash_tail", t2, 1);
					}
				}

				//=====================================================
				// 火花は X字の4端点からのみ出す
				//=====================================================
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

		// FX のタイミング制御用フレーム加算
		++fxFrame_;
	}

	//=========================================================
	// 曲線移動 → 直進移動への切り替え
	//=========================================================
	if (useCurve_ && reachedCurveEnd_) {
		Vector3 dirT_{};

		// 曲線終点での接線方向を end - ctrl で近似する
		dirT_.x = curveEnd_.x - curveCtrl_.x;
		dirT_.y = curveEnd_.y - curveCtrl_.y;
		dirT_.z = curveEnd_.z - curveCtrl_.z;

		const float len_ = std::sqrt(dirT_.x * dirT_.x + dirT_.y * dirT_.y + dirT_.z * dirT_.z);
		if (len_ > 0.0001f) {
			// 正規化
			dirT_.x /= len_;
			dirT_.y /= len_;
			dirT_.z /= len_;
		} else {
			// 接線が作れない場合はデフォルト方向を使う
			dirT_ = { 0.0f, 0.0f, 1.0f };
		}

		dir_ = dirT_;     // 以降の直進方向に採用
		useCurve_ = false; // 曲線移動終了

		// 曲線終了後の通り過ぎ用寿命
		life_ = 45;
		return;
	}

	//=========================================================
	// 寿命更新
	// 直進中のみ寿命を減らす
	//=========================================================
	if (!useCurve_) {
		if (--life_ <= 0) {
			dead_ = true;
		}
	}
}

//=============================================================
// 本体描画
//=============================================================
void BossBullet::Draw(TKM::DirectXCommon* dx) {
	if (!bodyHidden_ && !dead_) {
		obj_->Draw(dx);
	}
}

//=============================================================
// トレイル描画
//=============================================================
void BossBullet::DrawTrail(TKM::DirectXCommon* dx) {
	// ミサイルエフェクトのときだけトレイルを描画する
	if (!cam_ || fxType_ != FxType::MissileEvil) {
		return;
	}

	// トレイル点列が無ければ描画しない
	if (trailPts_.empty()) {
		return;
	}

	// 死亡済みならトレイルも描画しない
	if (dead_) {
		return;
	}

	std::vector<Vector3> drawPts_ = trailPts_;
	Vector3 currentPos_ = obj_->GetTranslate();

	// 現在位置が末尾点と異なるなら、現在位置も描画点列に加える
	if (drawPts_.empty() || MyMath::Length(currentPos_ - drawPts_.back()) > 0.0001f) {
		drawPts_.push_back(currentPos_);
	}

	// 線分として成立しないなら描画しない
	if (drawPts_.size() < 2) {
		return;
	}

	auto* rr_ = TKM::TrailRibbonRenderer::GetInstance();
	const auto& dbg_ = rr_->GetDebugParams();
	if (!dbg_.enable) {
		return;
	}

	rr_->DrawRibbon(
		dx,
		*cam_,
		drawPts_,
		trailHeadWidth_,
		trailTailWidth_,
		trailIntensity_,
		trailColor_,
		trailUvTiling_,
		trailUvScroll_
	);
}

//=============================================================
// 曲線移動設定
//=============================================================
void BossBullet::EnableCurveToTarget(const Vector3& start, const Vector3& end, float curveHeight, float speedPerFrame) {
	useCurve_ = true;
	curveStart_ = start;
	curveEnd_ = end;

	//=========================================================
	// 距離計算
	//=========================================================
	const Vector3 d_{ end.x - start.x, end.y - start.y, end.z - start.z };
	const float dist_ = std::sqrt(d_.x * d_.x + d_.y * d_.y + d_.z * d_.z);

	speed_ = speedPerFrame;

	// 0除算防止込みで総フレーム数を計算
	const float sp_ = (speedPerFrame > 0.0001f) ? speedPerFrame : 0.0001f;
	curveTotalFrames_ = (int)std::ceil(dist_ / sp_);
	if (curveTotalFrames_ < 1) {
		curveTotalFrames_ = 1;
	}
	curveFrame_ = 0;

	//=========================================================
	// 中点計算
	//=========================================================
	curveMid_ = {
		(start.x + end.x) * 0.5f,
		(start.y + end.y) * 0.5f,
		(start.z + end.z) * 0.5f
	};

	//=========================================================
	// XZ平面上の進行方向計算
	//=========================================================
	Vector3 dirXZ_{ d_.x, 0.0f, d_.z };
	const float lenXZ_ = std::sqrt(dirXZ_.x * dirXZ_.x + dirXZ_.z * dirXZ_.z);
	if (lenXZ_ > 0.0001f) {
		dirXZ_.x /= lenXZ_;
		dirXZ_.z /= lenXZ_;
	} else {
		dirXZ_ = { 0.0f, 0.0f, 1.0f };
	}

	// 左右方向ベクトル
	Vector3 perp_{ -dirXZ_.z, 0.0f, dirXZ_.x };

	//=========================================================
	// 曲がり量計算
	//=========================================================
	const float totalYaw_ = curveYawRad_ * (float)curveTotalFrames_;

	float s_ = totalYaw_ / (3.14159265f * 0.5f);
	if (s_ > 1.0f) s_ = 1.0f;
	if (s_ < -1.0f) s_ = -1.0f;

	// 距離に比例した横ずれ量
	const float sideOffset_ = dist_ * 0.35f * s_;

	//=========================================================
	// 制御点生成
	//=========================================================
	curveCtrl_ = curveMid_;
	curveCtrl_.x += perp_.x * sideOffset_;
	curveCtrl_.z += perp_.z * sideOffset_;
	curveCtrl_.y += curveHeight;

	//=========================================================
	// 寿命補正
	//=========================================================
	if (life_ < curveTotalFrames_) {
		life_ = curveTotalFrames_; // 最低でも到達までは生かす
	}
}

//=============================================================
// X字スラッシュ当たり判定
//=============================================================
bool BossBullet::HitTestSlashX(const Vector3& targetCenter, const Vector3& targetSize) const {
	// 死亡済みなら判定しない
	if (dead_) return false;

	// 斬撃以外の弾にはこの判定を使わない
	if (fxType_ != FxType::SlashWave) return false;

	AABB targetAABB(targetCenter, targetSize);

	//=========================================================
	// X字形状生成用の基準ベクトル計算
	//=========================================================
	auto Cross_ = [](const Vector3& a, const Vector3& b) {
		return Vector3{
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		};
		};

	Vector3 pos_ = obj_ ? obj_->GetTranslate() : Vector3{};
	Vector3 fwd_ = MyMath::SafeNormalize(dir_, { 0.0f, 0.0f, 1.0f });

	// fwd と平行だと right が作れないため、up を状況で切り替える
	Vector3 upA_{ 0.0f, 1.0f, 0.0f };
	if (std::fabs(fwd_.y) > 0.90f) {
		upA_ = { 0.0f, 0.0f, 1.0f };
	}

	Vector3 right_ = MyMath::SafeNormalize(Cross_(upA_, fwd_), { 1.0f, 0.0f, 0.0f });
	Vector3 up_ = MyMath::SafeNormalize(Cross_(fwd_, right_), { 0.0f, 1.0f, 0.0f });

	//=========================================================
	// FX と合わせた X字形状パラメータ
	//=========================================================
	const float halfLen_ = 14.0f; // X の腕の長さ
	const float back_ = 8.0f;     // 少し後ろにずらして残光感を出す
	const int   seg_ = 24;        // 点密度

	Vector3 base_ = pos_ - fwd_ * back_;

	const float c = 0.70710678f; // cos45
	const float s = 0.70710678f; // sin45
	Vector3 diag1_ = right_ * c + up_ * s;
	Vector3 diag2_ = right_ * c - up_ * s;

	const Vector3 segSize_{ 4.0f, 3.0f, 4.0f };

	//=========================================================
	// X字2本分のセグメントAABBで判定
	//=========================================================
	for (int i = 0; i < seg_; ++i) {
		float t = (seg_ <= 1) ? 0.0f : (float)i / (float)(seg_ - 1);
		float u = (t * 2.0f - 1.0f);
		float along = u * halfLen_;

		Vector3 p1 = base_ + diag1_ * along;
		Vector3 p2 = base_ + diag2_ * along;

		AABB segA(p1, segSize_);
		if (segA.IsCollidingWithAABB(targetAABB)) {
			return true;
		}

		AABB segB(p2, segSize_);
		if (segB.IsCollidingWithAABB(targetAABB)) {
			return true;
		}
	}

	return false;
}

//=============================================================
// 撃破処理
//=============================================================
void BossBullet::Kill() {
	bodyHidden_ = true;     // 本体を非表示にする
	isTrailFading_ = true; // トレイルをフェードアウトさせる
}

//=============================================================
// 制御点オフセット付き曲線移動設定
//=============================================================
void BossBullet::EnableCurveToTargetWithControlOffset(const Vector3& start, const Vector3& target, const Vector3& controlOffset, float speed) {
	useCurve_ = true;
	curveStart_ = start;
	curveEnd_ = target;
	curveControlOffset_ = controlOffset;
	speed_ = speed;

	const Vector3 d_{ target.x - start.x, target.y - start.y, target.z - start.z };
	const float dist_ = std::sqrt(d_.x * d_.x + d_.y * d_.y + d_.z * d_.z);

	const float sp_ = (speed > 0.0001f) ? speed : 0.0001f;
	curveTotalFrames_ = (int)std::ceil(dist_ / sp_);
	if (curveTotalFrames_ < 1) {
		curveTotalFrames_ = 1;
	}
	curveFrame_ = 0;

	curveMid_ = {
		(start.x + target.x) * 0.5f,
		(start.y + target.y) * 0.5f,
		(start.z + target.z) * 0.5f
	};

	curveCtrl_ = curveMid_;
	curveCtrl_.x += controlOffset.x;
	curveCtrl_.y += controlOffset.y;
	curveCtrl_.z += controlOffset.z;

	if (life_ < curveTotalFrames_) {
		life_ = curveTotalFrames_;
	}
}

//=============================================================
// カメラ設定
//=============================================================
void BossBullet::SetCamera(TKM::Camera* cam) {
	cam_ = cam;
	if (obj_) {
		obj_->SetCamera(cam);
	}
}

//=============================================================
// 曲がり量設定
//=============================================================
void BossBullet::SetCurveYaw(float yawRadPerFrame) {
	curveYawRad_ = yawRadPerFrame; // 1フレームあたりのヨー回転量
}

//=============================================================
// モデル設定
//=============================================================
void BossBullet::SetModel(const std::string& model) {
	if (obj_) {
		obj_->SetModel(model);
	}
}

//=============================================================
// スケール設定
//=============================================================
void BossBullet::SetScale(const Vector3& s) {
	if (obj_) {
		obj_->SetScale(s);
	}
}

//=============================================================
// FXタイプ設定
//=============================================================
void BossBullet::SetFxType(FxType t) {
	fxType_ = t;
}

//=============================================================
// 攻撃ID設定
//=============================================================
void BossBullet::SetAttackId(int id) {
	attackId_ = id;
}

//=============================================================
// トレイル更新
//=============================================================
void BossBullet::UpdateTrail(const Vector3& p) {
	// 初回はそのまま追加
	if (trailPts_.empty()) {
		trailPts_.push_back(p);
		trailDistAcc_ = 0.0f;
		return;
	}

	// 前回位置からの移動距離を蓄積
	const float moveDist_ = MyMath::Length(p - prevPos_);
	trailDistAcc_ += moveDist_;

	// 一定距離以上動いたら新しい点を追加
	if (trailDistAcc_ >= kTrailStep_) {
		trailPts_.push_back(p);
		trailDistAcc_ = 0.0f;

		// 上限数を超えたら古い点から削除
		while (trailPts_.size() > kTrailHardCap_) {
			trailPts_.erase(trailPts_.begin());
		}
	}
}