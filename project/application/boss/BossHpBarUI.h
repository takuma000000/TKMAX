#pragma once
#include <memory>
#include <vector>
#include <string>
#include <cstdlib>
#include <algorithm>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "BossEnemy.h"
#include "WindowsAPI.h"

namespace TKM {

	class BossHpBarUI {
	public:
		struct Desc {
			Vector2 pos = { 60.0f, 40.0f };
			Vector2 size = { 600.0f, 22.0f };

			float shakeTime = 0.18f;
			float shakePower = 9.0f;

			float lagSpeed = 160.0f; // 遅延バー追従速度（大きいほどすぐ追いつく）
			int segmentCount = 60;   // 砕けブロック数

			float shardLife = 0.45f;
			float shardSpeedMin = 140.0f;
			float shardSpeedMax = 320.0f;
			float shardRotSpeed = 10.0f;

			// 追加要素：色演出
			Vector4 baseColor = { 0.25f, 1.0f, 0.9f, 1.0f };   // 通常
			Vector4 drainColor = { 1.0f, 0.75f, 0.15f, 1.0f };  // 減ってる最中
			Vector4 flashColor = { 1.0f, 0.25f, 0.25f, 1.0f };  // 被弾直後
			Vector4 lowHpColor = { 1.0f, 0.20f, 0.90f, 1.0f };  // 低HP域
			float lowHpStartRate = 0.35f;                       // ここから低HP色へ寄せる

			// 減った区間が光って消える
			float drainGlowTime = 0.22f;

			std::string frameTex = "./resources/uvChecker.png";      // 仮（差し替えOK）
			std::string fillTex = "./resources/gradationLine.png";  // 仮（差し替えOK）
			std::string shardTex = "./resources/damageSpark.png";    // 仮（差し替えOK）
		};

	public:
		void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc);
		void SetVisible(bool v) {
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

		// BossManager::Update() から呼ぶ
		void Update(float dt, BossEnemy* boss);
		// GameSceneのSpriteパスから呼ぶ
		void Draw();

		Desc& GetDesc() { return desc_; }

	private:
		struct Shard {
			std::unique_ptr<Sprite> sp;
			Vector2 vel{};
			float rot = 0.0f;
			float rotVel = 0.0f;
			float t = 0.0f;
			float life = 0.5f;
			bool alive = false;
		};

	private:
		float Rand01_() { return float(std::rand()) / float(RAND_MAX); }
		float RandRange_(float a, float b) { return a + (b - a) * Rand01_(); }

		void SpawnShards_(int segBegin, int segEnd);

		static Vector4 LerpColor_(const Vector4& a, const Vector4& b, float t) {
			t = std::clamp(t, 0.0f, 1.0f);
			return {
				a.x + (b.x - a.x) * t,
				a.y + (b.y - a.y) * t,
				a.z + (b.z - a.z) * t,
				a.w + (b.w - a.w) * t
			};
		}

	private:
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;

		Desc desc_{};
		bool visible_ = true;

		// HP管理
		int maxHp_ = 1;
		int hp_ = 1;
		int lastHp_ = 0;
		bool initialized_ = false;

		// 遅延バー
		float lagHp_ = 0.0f;

		// 揺れ
		float shakeTimer_ = 0.0f;

		// 追加要素：ヒットパルス＆時間
		float hitPulse_ = 0.0f;
		float drainGlowTimer_ = 0.0f;
		float time_ = 0.0f;

		// スプライト
		std::unique_ptr<Sprite> frame_;     // フレーム
		std::unique_ptr<Sprite> fill_;      // 本体バー
		std::unique_ptr<Sprite> lagFill_;   // 遅れて減るバー
		std::unique_ptr<Sprite> drainGlow_; // 減った区間の残像

		// 破片
		std::vector<Shard> shards_;
	};

} // namespace TKM