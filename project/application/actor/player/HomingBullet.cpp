#define NOMINMAX
#include "HomingBullet.h"
#include <algorithm>
#include <ParticleManager.h>
#include "AABB.h"
#include "Player.h"
#include "Enemy.h"
#include "BarrierCore.h"
#include "TrailRibbonRenderer.h"

namespace {
	// 進行方向ベクトルをオイラー角へ変換する補助関数
	static Vector3 DirToEuler_(const Vector3& dir) {
		// const引数は直接いじれないのでコピーして使う
		Vector3 d = dir;

		// ベクトル長を求める
		float len = MyMath::Length(d);

		// 長さが極端に小さい場合は向きを決められないのでゼロ回転を返す
		if (len < 0.0001f) { return { 0,0,0 }; }

		// 向きだけを使いたいので正規化する
		d = d / len;

		// XZ平面上の向きからヨー角を求める
		float yaw = std::atan2(d.x, d.z);

		// Y成分からピッチ角を求める
		float pitch = -std::asin(d.y);

		// ロールは使わないので0固定で返す
		return { pitch, yaw, 0.0f };
	}
}

void HomingBullet::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	// ホーミング弾用の3Dオブジェクトを生成する
	object_ = std::make_unique<TKM::Object3d>();

	// 描画に必要な共通情報を渡して初期化する
	object_->Initialize(common, dxCommon);

	// 仮の見た目として球モデルを設定する
	object_->SetModel("sphere.obj");

	// デフォルトスケールを設定する
	object_->SetScale({ kDefaultScale_, kDefaultScale_, kDefaultScale_ });

	// 初期位置を前回位置として保持しておく
	prevPos_ = object_->GetTranslate();

	// 寿命タイマーを初期化する
	lifeTimer_ = 0.0f;

	// トレイル履歴を空にする
	trailPts_.clear();

	// トレイル距離の蓄積をリセットする
	trailDistAcc_ = 0.0f;
}

void HomingBullet::SetPosition(const Vector3& pos) {
	// 現在位置を更新する
	object_->SetTranslate(pos);

	// 前回位置も同じ値にして瞬間移動時のズレを防ぐ
	prevPos_ = pos;

	// 古いトレイルは不自然になるので全消去する
	trailPts_.clear();

	// 新しい位置をトレイルの始点として入れる
	trailPts_.push_back(pos);

	// トレイル距離蓄積をリセットする
	trailDistAcc_ = 0.0f;

	// トレイルフェード状態を解除する
	isTrailFading_ = false;

	// トレイルフェードタイマーをリセットする
	trailFadeTimer_ = 0.0f;
}

void HomingBullet::SetCamera(TKM::Camera* camera) {
	// カメラ参照を保持する
	camera_ = camera;

	// オブジェクトがあれば同じカメラを設定する
	if (object_) {
		object_->SetCamera(camera);
	}
}

void HomingBullet::SetEnemy(Enemy* enemy) {
	// 追尾対象の敵を設定する
	enemy_ = enemy;
}

void HomingBullet::SetPlayer(Player* player) {
	// プレイヤー参照を設定する
	player_ = player;
}

void HomingBullet::SetCore(BarrierCore* core) {
	// 追尾対象のコアを設定する
	core_ = core;
}

void HomingBullet::StartArc(
	const Vector3& start,
	const Vector3& control1,
	const Vector3& control2,
	const Vector3& end,
	float duration
) {
	// 3次ベジェの開始点を設定する
	p0_ = start;

	// 3次ベジェの第1制御点を設定する
	p1_ = control1;

	// 3次ベジェの第2制御点を設定する
	p2_ = control2;

	// 3次ベジェの終点を設定する
	p3_ = end;

	// 弾道進行率をリセットする
	arcT_ = 0.0f;

	// 継続時間が極端に小さくならないように下限付きで設定する
	arcDuration_ = std::max(0.001f, duration);

	// 山なり弾道を有効化する
	isArcActive_ = true;

	// 開始点へ弾を配置する
	SetPosition(start);
}

void HomingBullet::UpdateTrail_(const Vector3& p) {
	// まだトレイル点が無ければ最初の1点として追加して終わる
	if (trailPts_.empty()) {
		trailPts_.push_back(p);
		trailDistAcc_ = 0.0f;
		return;
	}

	// 前回位置からの移動距離を求める
	float moveDist = MyMath::Length(p - prevPos_);

	// 移動距離を蓄積する
	trailDistAcc_ += moveDist;

	// 一定距離以上進んだら新しいトレイル点を追加する
	if (trailDistAcc_ >= kTrailStep_) {
		trailPts_.push_back(p);

		// 蓄積距離をリセットする
		trailDistAcc_ = 0.0f;

		// 点が多すぎる場合は古い点から削除して数を抑える
		while (trailPts_.size() > kTrailHardCap_) {
			trailPts_.erase(trailPts_.begin());
		}
	}
}

void HomingBullet::Update() {
	// オブジェクト未生成、または死亡済みなら更新しない
	if (!object_ || isDead_) { return; }

	//=========================================================
	// トレイルフェード中の更新
	//=========================================================
	if (isTrailFading_) {
		// フェードタイマーを進める
		trailFadeTimer_ += dt_;

		// 一定間隔ごとにトレイル先頭を削除して薄れていく見た目にする
		while (trailFadeTimer_ >= trailFadeInterval_) {
			trailFadeTimer_ -= trailFadeInterval_;

			if (!trailPts_.empty()) {
				trailPts_.erase(trailPts_.begin());
			}
		}

		// 2点未満になるとリボンが成立しないので弾自体も消す
		if (trailPts_.size() < 2) {
			isDead_ = true;
		}
		return;
	}

	// 現在位置を取得する
	Vector3 oldPos = object_->GetTranslate();

	// 現在位置を前回位置として保持する
	prevPos_ = oldPos;

	// 寿命タイマーを進める
	lifeTimer_ += dt_;

	// 寿命が尽きたら死亡扱いにする
	if (lifeTimer_ >= kLifeTime_) {
		isDead_ = true;
		return;
	}

	//=========================================================
	// 山なり弾道の更新
	//=========================================================
	if (isArcActive_) {
		// ベジェ弾道上の進行率を進める
		arcT_ += dt_ / arcDuration_;

		// 0.0～1.0に収める
		float t = std::clamp(arcT_, 0.0f, 1.0f);

		// 現在の進行率でベジェ曲線上の位置を計算する
		Vector3 pos = MyMath::Bezier3(p0_, p1_, p2_, p3_, t);

		// 終盤だけ現在のターゲット位置へ少しずつ寄せる
		if (t >= 0.65f) {
			bool hasTarget = false;
			Vector3 targetPos = pos;

			// 生きているコアが優先ターゲット
			if (core_ && !core_->IsDead()) {
				targetPos = core_->GetWorldPosition();
				hasTarget = true;
			}
			// コアが無ければ生きている敵をターゲットにする
			else if (enemy_ && !enemy_->IsDead()) {
				targetPos = enemy_->GetWorldPosition();
				targetPos.y += 1.5f;
				hasTarget = true;
			}

			// ターゲットがいれば終盤ほど強く寄せる
			if (hasTarget) {
				float followT = (t - 0.65f) / (1.0f - 0.65f);
				followT = std::clamp(followT, 0.0f, 1.0f);
				pos = pos + (targetPos - pos) * followT;
			}
		}

		// 計算した位置を反映する
		object_->SetTranslate(pos);

		//=========================================================
		// LB弾用トレイル演出（キラキラ・稲光）
		//=========================================================
		{
			auto* pm = TKM::ParticleManager::GetInstance();

			// トレイル点が2つ以上ある時だけ演出を出す
			if (trailPts_.size() >= 2) {

				// 指定範囲のランダム値を返すラムダ
				auto frand = [](float a, float b) {
					return a + (b - a) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
					};

				// すべての点に出すと重いので間引きながら走査する
				const int step = 2;

				for (size_t i = 0; i < trailPts_.size(); i += step) {
					// このトレイル点の中心位置
					Vector3 centerPos = trailPts_[i];

					// デフォルトの進行方向
					Vector3 dir = { 0,0,1 };

					// 次の点があれば点列方向から進行方向を計算する
					if (i + 1 < trailPts_.size()) {
						dir = trailPts_[i + 1] - trailPts_[i];
						float len = MyMath::Length(dir);

						if (len > 0.0001f) dir = dir / len;
					}

					//----------------------------
					// トレイル横方向を求める
					//----------------------------
					Vector3 side = { 1,0,0 };

					// カメラがあるなら視線との関係から横方向を決める
					if (camera_) {
						Vector3 camVec = camera_->GetTranslate() - centerPos;
						float camLen = MyMath::Length(camVec);

						if (camLen > 0.0001f) {
							camVec /= camLen;
							Vector3 s = MyMath::Cross(camVec, dir);
							float sLen = MyMath::Length(s);

							if (sLen > 0.0001f) side = s / sLen;
						}
					}

					// 左右どちら側に出すかをランダムで決める
					float sign = (rand() % 2 == 0) ? -1.0f : 1.0f;

					// 中心からの基本横オフセット
					float edgeBase = 0.72f;

					// 少し外へはみ出すための追加量
					float overhang = frand(0.05f, 0.30f);

					// キラキラ配置位置を求める
					Vector3 edgePos = centerPos + side * sign * (edgeBase + overhang);

					//----------------------------
					// キラキラ演出
					//----------------------------
					if ((rand() % 100) < 80) {
						pm->Emit("trail_lb_glitter", edgePos, 2);

						// さらに少しだけ厚みを出すため追加発生
						if ((rand() % 100) < 55) {
							pm->Emit("trail_lb_glitter", edgePos, 2);
						}
					}

					//----------------------------
					// 稲光演出
					//----------------------------
					if ((rand() % 100) < 45) {
						// 稲光はキラキラより少し広い位置に出す
						Vector3 boltPos = centerPos + side * sign * frand(0.65f, 1.15f);

						pm->Emit("trail_lb_bolt_main", boltPos, 2);
						pm->Emit("trail_lb_bolt_core", boltPos, 1);

						// 追加でコアを重ねて密度を出す
						if ((rand() % 100) < 40) {
							pm->Emit("trail_lb_bolt_core", boltPos, 1);
						}
					}
				}
			}
		}

		//=========================================================
		// 向きを弾道に合わせる
		//=========================================================
		float t2 = std::min(1.0f, t + 0.01f);

		// 少し先の位置を求めて進行方向を作る
		Vector3 nextPos = MyMath::Bezier3(p0_, p1_, p2_, p3_, t2);

		// 終盤だけこちらも現在のターゲットへ寄せる
		if (t2 >= 0.65f) {
			bool hasTarget2 = false;
			Vector3 targetPos2 = nextPos;

			if (core_ && !core_->IsDead()) {
				targetPos2 = core_->GetWorldPosition();
				hasTarget2 = true;
			} else if (enemy_ && !enemy_->IsDead()) {
				targetPos2 = enemy_->GetWorldPosition();
				targetPos2.y += 1.5f;
				hasTarget2 = true;
			}

			if (hasTarget2) {
				float followT2 = (t2 - 0.65f) / (1.0f - 0.65f);
				followT2 = std::clamp(followT2, 0.0f, 1.0f);
				nextPos = nextPos + (targetPos2 - nextPos) * followT2;
			}
		}

		// 現在位置から少し先の位置への差分で進行方向を求める
		Vector3 dir = nextPos - pos;

		// 十分な長さがある場合だけ向きを更新する
		if (MyMath::Length(dir) > 0.0001f) {
			object_->SetRotate(DirToEuler_(dir));
		}

		// トレイル点を更新する
		UpdateTrail_(pos);

		//=========================================================
		// Wave1バリアとの当たり判定
		//=========================================================
		if (player_ && player_->IsWave1BarrierActive()) {

			// 現在の弾位置を取得する
			const Vector3 bulletPos_ = object_->GetTranslate();

			// 現在の弾スケールを取得する
			const Vector3 bulletScale_ = object_->GetScale();

			// 弾のAABBを作成する
			AABB bulletBox_(bulletPos_, bulletScale_);

			// バリアのAABBを作成する
			AABB barrierBox_(player_->GetWave1BarrierCenter(), player_->GetWave1BarrierSize());

			bool barrierHit_ = false;

			// 前フレーム位置から現在位置までの線分で貫通判定する
			if (barrierBox_.IsIntersectSegment(prevPos_, bulletPos_)) {
				barrierHit_ = true;
			}

			// 線分で当たっていなければ通常AABB同士でも判定する
			if (!barrierHit_ && bulletBox_.IsCollidingWithAABB(barrierBox_)) {
				barrierHit_ = true;
			}

			// バリアに当たった場合の処理
			if (barrierHit_) {
				isHit_ = true;
				isDead_ = true;

				TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
				if (pm) {
					pm->Emit("enemyHit_flash", bulletPos_, 1);
					pm->Emit("enemyHit_ring", bulletPos_, 1);
					pm->Emit("enemyHit_spark", bulletPos_, 12);
				}

				// プレイヤー側へヒット通知と演出要求を送る
				if (player_) {
					player_->AddWave1BarrierHit(bulletPos_);
					player_->RequestWave1BarrierFlash(bulletPos_);
					player_->StartCameraShake(6);
				}
				return;
			}
		}

		// 現在の弾位置を取得する
		Vector3 bulletPos = object_->GetTranslate();

		// 現在の弾スケールを取得する
		Vector3 bulletScale = object_->GetScale();

		//=========================================================
		// 線分＋AABB の swept 判定共通ラムダ
		//=========================================================
		auto CheckSweptHitAABB = [&](const Vector3& targetPos, const Vector3& targetSize) -> bool {
			// 弾側AABBを作成する
			AABB bulletBox(bulletPos, bulletScale);

			// ターゲット側AABBを作成する
			AABB targetBox(targetPos, targetSize);

			// 線分で貫通判定する
			if (targetBox.IsIntersectSegment(prevPos_, bulletPos)) {
				return true;
			}

			// 現在位置のAABB同士でも判定する
			if (bulletBox.IsCollidingWithAABB(targetBox)) {
				return true;
			}
			return false;
			};

		//=========================================================
		// 敵との当たり判定
		//=========================================================
		if (enemy_ && !enemy_->IsDead()) {
			// 敵の現在位置を取得する
			Vector3 enemyPos = enemy_->GetWorldPosition();

			// 敵のコライダーサイズを取得する
			Vector3 enemySize = enemy_->GetColliderScale();

			// 線分またはAABBで当たったらヒット扱い
			if (CheckSweptHitAABB(enemyPos, enemySize)) {
				isHit_ = true;
				isTrailFading_ = true;
				trailFadeTimer_ = 0.0f;

				TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
				Vector3 hitPos = bulletPos;
				pm->Emit("lt_nova_core", hitPos, 1);
				pm->Emit("lt_nova_wave", hitPos, 3);
				pm->Emit("lt_nova_burst", hitPos, 40);
				pm->Emit("lt_nova_debris", hitPos, 120);
				pm->Emit("lt_nova_crack", hitPos, 80);

				// 敵へダメージを与える
				enemy_->OnHitWithDamage(kEnemyDamage_);

				// プレイヤーがいれば強めのカメラシェイクをかける
				if (player_) {
					player_->StartCameraShake(40);
				}
				return;
			}
		}

		//=========================================================
		// コアとの当たり判定
		//=========================================================
		if (core_ && !core_->IsDead()) {
			// コアの現在位置を取得する
			Vector3 corePos = core_->GetWorldPosition();

			// コアのコライダーサイズを取得する
			Vector3 coreSize = core_->GetColliderScale();

			// 線分またはAABBで当たったらヒット扱い
			if (CheckSweptHitAABB(corePos, coreSize)) {
				isHit_ = true;
				isTrailFading_ = true;
				trailFadeTimer_ = 0.0f;

				TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
				Vector3 hitPos = bulletPos;
				pm->Emit("lt_nova_core", hitPos, 1);
				pm->Emit("lt_nova_wave", hitPos, 3);
				pm->Emit("lt_nova_burst", hitPos, 40);
				pm->Emit("lt_nova_debris", hitPos, 120);
				pm->Emit("lt_nova_crack", hitPos, 80);

				// コアへダメージを与える
				core_->OnHitWithDamage(kCoreDamage_);

				// プレイヤーがいれば強めのカメラシェイクをかける
				if (player_) {
					player_->StartCameraShake(40);
				}
				return;
			}
		}

		// 弾道終端まで到達したら本体移動を止め、トレイルだけフェードさせる
		if (t >= 1.0f) {
			isArcActive_ = false;
			isTrailFading_ = true;
			trailFadeTimer_ = 0.0f;
		}
	}

	// 最後にオブジェクトの更新を反映する
	object_->Update();
}

void HomingBullet::Draw(TKM::DirectXCommon* dxCommon) {
	// 本体未生成、死亡済み、またはトレイルフェード中なら本体描画しない
	if (!object_ || isDead_ || isTrailFading_) { return; }

	// 本体を描画する
	object_->Draw(dxCommon);
}

void HomingBullet::DrawTrail(TKM::DirectXCommon* dxCommon) {
	// カメラ未設定、トレイル無し、死亡済みなら描画しない
	if (!camera_ || trailPts_.empty() || isDead_) { return; }

	auto* rr = TKM::TrailRibbonRenderer::GetInstance();

	// デバッグ描画パラメータを取得する
	const auto& p = rr->GetDebugParams();

	// デバッグで無効なら描画しない
	if (!p.enable) { return; }

	// 描画用にトレイル点列をコピーする
	std::vector<Vector3> drawPts = trailPts_;

	// 現在の弾位置を取得する
	Vector3 currentPos = object_->GetTranslate();

	// 最後の点と十分離れている場合だけ現在位置を末尾に足す
	if (drawPts.empty() || MyMath::Length(currentPos - drawPts.back()) > 0.0001f) {
		drawPts.push_back(currentPos);
	}

	// 2点未満ではリボンを作れないので終了する
	if (drawPts.size() < 2) { return; }

	// リボン状トレイルを描画する
	rr->DrawRibbon(
		dxCommon,
		*camera_,
		drawPts,
		p.headWidth,
		p.tailWidth,
		p.intensity,
		p.color,
		p.uvTiling,
		p.uvScroll
	);
}

void HomingBullet::ResetForReuse() {
	// 死亡・ヒット状態を解除する
	isDead_ = false;
	isHit_ = false;

	// 寿命をリセットする
	lifeTimer_ = 0.0f;

	// 山なり弾道状態をリセットする
	isArcActive_ = false;
	arcT_ = 0.0f;
	arcDuration_ = 0.0f;

	// ベジェ制御点を初期化する
	p0_ = { 0.0f, 0.0f, 0.0f };
	p1_ = { 0.0f, 0.0f, 0.0f };
	p2_ = { 0.0f, 0.0f, 0.0f };
	p3_ = { 0.0f, 0.0f, 0.0f };

	// トレイル状態を初期化する
	trailPts_.clear();
	trailDistAcc_ = 0.0f;
	isTrailFading_ = false;
	trailFadeTimer_ = 0.0f;

	// ターゲット参照を初期化する
	enemy_ = nullptr;
	core_ = nullptr;

	// 位置情報を現在位置基準で揃える
	if (object_) {
		prevPos_ = object_->GetTranslate();
		object_->SetScale({ kDefaultScale_, kDefaultScale_, kDefaultScale_ });
		object_->SetRotate({ 0.0f, 0.0f, 0.0f });
	}
}