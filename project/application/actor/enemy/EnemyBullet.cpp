#include "EnemyBullet.h"
#include <algorithm>

void EnemyBullet::Initialize(
	TKM::Object3dCommon* common,
	TKM::DirectXCommon* dxCommon,
	TKM::Camera* camera,
	const Vector3& position,
	const Vector3& velocity
) {
	// 弾本体の3Dオブジェクトを生成する
	object_ = std::make_unique<TKM::Object3d>();

	// 描画に必要な共通情報とDirectX情報を渡して初期化する
	object_->Initialize(common, dxCommon);

	// 弾の見た目として球モデルを設定する
	object_->SetModel("sphere.obj");

	// 生成位置を設定する
	object_->SetTranslate(position);

	// 通常弾の初期スケールを設定する
	object_->SetScale({ 0.6f, 0.6f, 0.6f });

	// カメラ参照を保持する
	camera_ = camera;

	// カメラが有効なら弾オブジェクトにも適用する
	if (camera_) {
		object_->SetCamera(camera_);
	}

	// 移動速度を設定する
	velocity_ = velocity;

	// 生存時間計測用タイマーを初期化する
	lifeTimer_ = 0.0f;

	// 生成直後は生存状態にする
	isDead_ = false;

	// 通常弾タイプとして初期化する
	type_ = Type::Normal;

	// 現在スケールを初期化する
	scaleNow_ = 0.6f;

	// 終了スケールも同じ値で初期化する
	scaleEnd_ = 0.6f;

	// チャージ用タイマーを初期化する
	chargeTimer_ = 0.0f;

	// 通常弾なのでチャージ時間は0で初期化する
	chargeDuration_ = 0.0f;

	// 通常弾のダメージを設定する
	damage_ = 1;

	// 生成直後は表示状態にする
	visible_ = true;
}

void EnemyBullet::Update(float dt) {
	// オブジェクト未生成、または死亡済みなら更新しない
	if (!object_ || isDead_) {
		return;
	}

	// 現在位置を取得する
	Vector3 pos_ = object_->GetTranslate();

	//=========================================================
	// 特殊コアのチャージ中更新
	//=========================================================
	if (type_ == Type::SpecialCoreCharging) {

		// チャージ経過時間を進める
		chargeTimer_ += dt;

		// チャージ進行率を初期値1.0で用意する
		float t_ = 1.0f;

		// チャージ時間が極端に小さくない場合だけ割合を計算する
		if (chargeDuration_ > 0.0001f) {
			t_ = chargeTimer_ / chargeDuration_;
		}

		// 進行率を0.0～1.0に収める
		t_ = std::clamp(t_, 0.0f, 1.0f);

		// 現在スケールから終了スケールまで線形補間で拡大・縮小する
		const float sc_ = scaleNow_ + (scaleEnd_ - scaleNow_) * t_;

		// 見た目スケールを更新する
		object_->SetScale({ sc_, sc_, sc_ });

		// 当たり判定用半径も見た目に合わせて更新する
		radius_ = sc_ * 0.75f;

		// 反映したTransformで更新する
		object_->Update();

		// チャージ中は移動処理へ進まずここで終了
		return;
	}

	//=========================================================
	// 通常弾 / 発射後の特殊コア更新
	//=========================================================

	// 速度に応じて位置を進める
	pos_ += velocity_ * (dt * 60.0f);

	// 新しい位置を反映する
	object_->SetTranslate(pos_);

	// Transformを更新する
	object_->Update();

	// 生存時間を進める
	lifeTimer_ += dt;

	// 寿命を超えたら死亡扱いにする
	if (lifeTimer_ >= lifeTime_) {
		isDead_ = true;
	}
}

void EnemyBullet::Draw(TKM::DirectXCommon* dx) {
	// オブジェクト未生成、死亡済み、非表示なら描画しない
	if (!object_ || isDead_ || !visible_) {
		return;
	}

	// 弾本体を描画する
	object_->Draw(dx);
}

Vector3 EnemyBullet::GetWorldPosition() const {
	// オブジェクト未生成なら安全のため原点を返す
	if (!object_) {
		return { 0.0f, 0.0f, 0.0f };
	}

	// 現在のワールド座標を返す
	return object_->GetTranslate();
}

void EnemyBullet::InitializeSpecialCore(
	TKM::Object3dCommon* common,
	TKM::DirectXCommon* dxCommon,
	TKM::Camera* camera,
	const Vector3& position,
	float startScale,
	float endScale,
	float radius,
	float chargeDuration,
	int damage
) {
	// 特殊コア本体の3Dオブジェクトを生成する
	object_ = std::make_unique<TKM::Object3d>();

	// 描画に必要な共通情報とDirectX情報を渡して初期化する
	object_->Initialize(common, dxCommon);

	// 見た目として球モデルを設定する
	object_->SetModel("sphere.obj");

	// 生成位置を設定する
	object_->SetTranslate(position);

	// チャージ開始時の初期スケールを設定する
	object_->SetScale({ startScale, startScale, startScale });

	// カメラ参照を保持する
	camera_ = camera;

	// カメラが有効なら弾オブジェクトにも適用する
	if (camera_) {
		object_->SetCamera(camera_);
	}

	// チャージ中は動かないので速度をゼロにする
	velocity_ = { 0.0f, 0.0f, 0.0f };

	// 当たり判定半径を設定する
	radius_ = radius;

	// 生存時間タイマーを初期化する
	lifeTimer_ = 0.0f;

	// 発射後も含めた寿命を設定する
	lifeTime_ = 8.0f;

	// 生成直後は生存状態にする
	isDead_ = false;

	// 特殊コアのチャージ中タイプとして初期化する
	type_ = Type::SpecialCoreCharging;

	// チャージ開始スケールを保持する
	scaleNow_ = startScale;

	// チャージ完了時スケールを保持する
	scaleEnd_ = endScale;

	// チャージタイマーを初期化する
	chargeTimer_ = 0.0f;

	// チャージにかける時間を設定する
	chargeDuration_ = chargeDuration;

	// ダメージ値を設定する
	damage_ = damage;

	// チャージ中は非表示で開始する
	visible_ = false;
}

void EnemyBullet::LaunchSpecialCore(const Vector3& velocity) {
	// 発射後の移動速度を設定する
	velocity_ = velocity;

	// 状態を「発射済み特殊コア」に切り替える
	type_ = Type::SpecialCoreLaunched;
}

void EnemyBullet::SetVisible(bool visible) {
	// 表示・非表示状態を切り替える
	visible_ = visible;
}

void EnemyBullet::SetPosition(const Vector3& position) {
	// オブジェクト未生成なら何もしない
	if (!object_) { return; }

	// 位置を直接設定する
	object_->SetTranslate(position);
}

void EnemyBullet::SetScale(float uniformScale) {
	// 現在スケール値を保持する
	scaleNow_ = uniformScale;

	// オブジェクト未生成なら見た目更新はしない
	if (!object_) { return; }

	// XYZ共通の等倍スケールとして反映する
	object_->SetScale({ uniformScale, uniformScale, uniformScale });
}

void EnemyBullet::SetColor(const Vector4& color) {
	// オブジェクト未生成なら何もしない
	if (!object_) { return; }

	// 色を設定する
	object_->SetColor(color);
}