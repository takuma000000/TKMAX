#include "BossEnemy.h"
#include <cmath>

//=============================================================
// 初期化
//=============================================================
void BossEnemy::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	//=========================================================
	// 基底クラス初期化
	//=========================================================
	Enemy::Initialize(common, dxCommon);

	//=========================================================
	// 設定取得
	// 設定があればそれを、無ければデフォルトを使用
	//=========================================================
	const BossEnemyConfig& cfg = config_ ? *config_ : BossEnemyConfig{};

	//=========================================================
	// モデル設定
	//=========================================================
	SetModel(cfg.model_);
	SetTentacleModel(cfg.tentacleModel_);

	//=========================================================
	// 触手の初期ローカル変換
	//=========================================================
	SetTentacleLocal(
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f },
		{ 1.0f, 1.0f, 1.0f }
	);

	// 触手の基準値（後のアニメーション用）
	tentacleBasePos_ = { 0.0f, 0.0f, 0.0f };
	tentacleBaseRot_ = { 0.0f, 0.0f, 0.0f };
	tentacleBaseScale_ = { 1.0f, 1.0f, 1.0f };

	//=========================================================
	// ステータス設定
	//=========================================================
	SetHP(cfg.hp_);

	// baseScale_ を正しくするため、スケールは最初に一度だけ設定
	SetScale(cfg.scale_);

	// 当たり判定サイズも初期に一度だけ設定
	SetColliderScale(cfg.colliderScale_);

	// タイプ設定（ボス固定）
	SetType(EnemyType::Boss);
}

//=============================================================
// 更新
//=============================================================
void BossEnemy::Update(float dt) {
	// 死亡済みなら更新しない
	if (IsDead()) {
		return;
	}

	//=========================================================
	// イントロ用パニック触手
	// 登場演出などで激しく暴れる挙動
	//=========================================================
	if (introPanicActive_) {
		tentacleWiggleT_ += dt;

		// 強度クランプ（0～1）
		float s = introPanic01_;
		if (s < 0.0f) s = 0.0f;
		if (s > 1.0f) s = 1.0f;

		// 周波数・振幅設定（強度依存）
		float freq = 18.0f + s * 10.0f;
		float ampX = 0.22f * s;
		float ampY = 0.16f * s;
		float ampZ = 0.28f * s;
		float lift = 0.20f * s;

		Vector3 pos = tentacleBasePos_;
		Vector3 rot = tentacleBaseRot_;
		Vector3 scl = tentacleBaseScale_;

		// 回転揺れ
		rot.x += sinf(tentacleWiggleT_ * freq) * ampX;
		rot.y += cosf(tentacleWiggleT_ * (freq * 1.35f)) * ampY;
		rot.z += sinf(tentacleWiggleT_ * (freq * 1.8f)) * ampZ;

		// 位置揺れ
		pos.x += sinf(tentacleWiggleT_ * (freq * 0.7f)) * 0.18f * s;
		pos.y += fabsf(sinf(tentacleWiggleT_ * (freq * 0.9f))) * lift;

		// スケールの脈動
		float pulse = 1.0f + fabsf(sinf(tentacleWiggleT_ * (freq * 0.65f))) * (0.06f * s);
		scl.x *= pulse;
		scl.y *= 1.0f + 0.03f * s;
		scl.z *= pulse;

		SetTentacleLocal(pos, rot, scl);
	}
	//=========================================================
	// 通常チャージ触手
	// 攻撃チャージ中の軽い揺れ
	//=========================================================
	else if (tentacleChargeActive_) {
		tentacleWiggleT_ += dt;

		float s = tentacleCharge01_;
		if (s < 0.0f) s = 0.0f;
		if (s > 1.0f) s = 1.0f;

		float freq = 9.0f + s * 4.0f;
		float ampX = 0.18f * s;
		float ampY = 0.08f * s;
		float ampZ = 0.15f * s;
		float lift = 0.12f * s;

		Vector3 pos = tentacleBasePos_;
		Vector3 rot = tentacleBaseRot_;
		Vector3 scl = tentacleBaseScale_;

		// 回転揺れ
		rot.x += sinf(tentacleWiggleT_ * freq) * ampX;
		rot.y += cosf(tentacleWiggleT_ * (freq * 0.7f)) * ampY;
		rot.z += sinf(tentacleWiggleT_ * (freq * 1.2f)) * ampZ;

		// 上下揺れ
		pos.y += sinf(tentacleWiggleT_ * (freq * 0.5f)) * lift;

		// 軽いスケール脈動
		float pulse = 1.0f + sinf(tentacleWiggleT_ * (freq * 0.8f)) * (0.03f * s);
		scl.x *= pulse;
		scl.y *= pulse;
		scl.z *= pulse;

		SetTentacleLocal(pos, rot, scl);
	}
	//=========================================================
	// 通常状態（変形なし）
	//=========================================================
	else {
		SetTentacleLocal(tentacleBasePos_, tentacleBaseRot_, tentacleBaseScale_);
	}

	//=========================================================
	// 本体更新（基底クラス）
	//=========================================================
	Enemy::Update(dt);
}

//=============================================================
// ImGuiデバッグ表示
//=============================================================
void BossEnemy::ImGuiDebug() {
#ifdef USE_IMGUI
	ImGui::Begin("ボス");

	// 当たり判定サイズ調整
	Vector3 col_ = GetColliderScale();
	if (ImGui::DragFloat3("当たり判定サイズ", &col_.x, 0.01f, 0.01f, 999.0f)) {
		SetColliderScale(col_);
	}

	// HP表示
	int hp_ = GetHP();
	int maxHP_ = GetMaxHP();
	ImGui::Text("HP : %d / %d", hp_, maxHP_);

	// 状態表示
	ImGui::Text("isDead : %s", IsDead() ? "true" : "false");
	ImGui::Text("isDying : %s", IsDying() ? "true" : "false");

	ImGui::End();
#endif
}

//=============================================================
// 触手チャージ設定
//=============================================================
void BossEnemy::SetTentacleCharge(bool active, float charge01) {
	tentacleChargeActive_ = active; // チャージON/OFF
	tentacleCharge01_ = charge01;   // 0～1のチャージ量
}

//=============================================================
// イントロパニック設定
//=============================================================
void BossEnemy::SetIntroPanic(bool active, float panic01) {
	introPanicActive_ = active; // パニックON/OFF
	introPanic01_ = panic01;    // 0～1の慌て強度
}

//=============================================================
// 設定セット
//=============================================================
void BossEnemy::SetConfig(const BossEnemyConfig* config) {
	config_ = config;
}