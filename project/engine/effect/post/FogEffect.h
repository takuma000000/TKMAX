#pragma once
#include "BaseEffect.h"
#include "MyMath.h"

class FogEffect : public BaseEffect {
public:
	void Initialize(TKM::DirectXCommon* dx) override {
		BaseEffect::Initialize(dx);
	}

	void Update(float dt) override;
	void Draw() override {}  // 描画は DirectXCommon 側のチェーンでやる

	bool IsActive() const { return active_; }
	void SetActive(bool a) { active_ = a; }

#ifdef USE_IMGUI
	void ImGuiDebug();
#endif

	// Setter========================================
	void SetWorldPos(const Vector3& pos) { worldPos_ = pos; }
	void SetWorldScale(float s) { worldScale_ = s; }
	// ==============================================

private:
	bool   active_ = true;             // 霧は最初から有効でOK
	float  density_ = 0.495f;            // 画面全体の濃さ
	float  start_ = 0.17f;              // 全画面に霧をかけたいので 0〜1 のまま
	float  end_ = 0.376f;
	float  noiseScale_ = 10.0f;         // 塊の大きさ）
	float  noiseStrength_ = 0.817f;      // ムラの強さ
	float  time_ = 0.0f; // 時間経過用
	float timeScale_ = 1.0f;      // 霧アニメ速度（Time倍率）
	Vector2 driftSpeedXZ_ = { 0.0f, 0.0f };   // 霧の自動移動速度（ワールド単位/秒）
	Vector2 driftOffsetXZ_ = { 0.0f, 0.0f };  // 蓄積オフセット
	bool   freezeTime_ = false;    // 時間停止（形だけ止めたい時）
	Vector3 color_ = { 0.9f, 0.9f, 1.0f }; // 霧の色

	Vector3 worldPos_ = { 0.0f, 0.0f, 0.0f }; // 霧の基準となるワールド座標
	float   worldScale_ = 0.02f;                // どれくらい動きに反応するか
};