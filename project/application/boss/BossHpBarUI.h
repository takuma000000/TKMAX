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
#include "MyMath.h"

namespace TKM {
	//=============================================================
	// BossHpBarUIクラス
	// ボスHPバーUIの管理を行うクラス。
	//=============================================================
	class BossHpBarUI {
	public:
		struct Desc {
			Vector2 pos_ = { 60.0f, 40.0f }; // 画面上の表示位置（左上基準）
			Vector2 size_ = { 600.0f, 22.0f }; // HPバーのサイズ
			float shakeTime_ = 0.18f; // 画面震え時間
			float shakePower_ = 9.0f; // 画面震え強さ

			float lagSpeed_ = 160.0f; // 遅延バー追従速度（大きいほどすぐ追いつく）
			int segmentCount_ = 60;   // 砕けブロック数
			float shardLife_ = 0.45f; // 砕けブロック寿命
			float shardSpeedMin_ = 140.0f; // 砕けブロック速度最小値
			float shardSpeedMax_ = 320.0f; // 砕けブロック速度最大値
			float shardRotSpeed_ = 10.0f;

			// 追加要素：色演出
			Vector4 baseColor_ = { 0.25f, 1.0f, 0.9f, 1.0f };   // 通常
			Vector4 drainColor_ = { 1.0f, 0.75f, 0.15f, 1.0f };  // 減ってる最中
			Vector4 flashColor_ = { 1.0f, 0.25f, 0.25f, 1.0f };  // 被弾直後
			Vector4 lowHpColor_ = { 1.0f, 0.20f, 0.90f, 1.0f };  // 低HP域
			float lowHpStartRate_ = 0.35f;                       // ここから低HP色へ寄せる

			// 減った区間が光って消える
			float drainGlowTime_ = 0.22f;

			std::string frameTex_ = "./resources/uvChecker.png";      // 仮（差し替えOK）
			std::string fillTex_ = "./resources/gradationLine.png";  // 仮（差し替えOK）
			std::string shardTex_ = "./resources/damageSpark.png";    // 仮（差し替えOK）
		};

		/// <summary>
		/// ボスHPバーUIを初期化します。
		/// </summary>
		/// <param name="spriteCommon">スプライト共通管理クラス</param>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="parentScene">所属する親シーン</param>
		/// <param name="desc">UIの設定（テクスチャ、色、サイズなど）</param>
		void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc);
		/// <summary>
		/// ボスHPバーUIを更新します。
		/// BossManager::Update() から呼び出されます。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="boss">HP参照対象となるボス（nullptr の場合は更新しません）</param>
		void Update(float dt, BossEnemy* boss);
		/// <summary>
		/// ボスHPバーUIを描画します。
		/// GameScene の Sprite パスから呼び出されます。
		/// </summary>
		void Draw();

		// Getter=====================================
		/// <summary>
		/// </summary>設定情報の取得。
		/// </summary>
		/// <returns></returns>
		Desc& GetDesc() { return desc_; }
		// ===========================================
		// Settet=====================================
		/// <summary>
		/// UIの表示/非表示を設定します。
		/// </summary>
		/// <param name="v">表示する場合 true、非表示の場合 false</param>
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
		// ===========================================

	private:
		struct Shard {
			std::unique_ptr<Sprite> sp_;
			Vector2 vel_{};
			float rot_ = 0.0f;
			float rotVel_ = 0.0f;
			float t_ = 0.0f;
			float life_ = 0.5f;
			bool alive_ = false;
		};

		/// <summary>
		/// 指定した範囲内の乱数を生成します。
		/// </summary>
		/// <param name="a">最小値</param>
		/// <param name="b">最大値</param>
		/// <returns>a 以上 b 以下の乱数値</returns>
		float RandRange_(float a, float b) {
			return a + (b - a) * MyMath::Rand01();
		}

		/// <summary>
		/// 破片（シャード）を指定セグメント範囲で生成します。
		/// </summary>
		/// <param name="segBegin">生成開始セグメント番号</param>
		/// <param name="segEnd">生成終了セグメント番号</param>
		void SpawnShards_(int segBegin, int segEnd);

		/// <summary>
		/// 2つの色を線形補間します。
		/// </summary>
		/// <param name="a">開始色</param>
		/// <param name="b">終了色</param>
		/// <param name="t">補間係数（0.0〜1.0）</param>
		/// <returns>補間後の色</returns>
		static Vector4 LerpColor_(const Vector4& a, const Vector4& b, float t) {
			t = std::clamp(t, 0.0f, 1.0f);
			return {
				a.x + (b.x - a.x) * t,
				a.y + (b.y - a.y) * t,
				a.z + (b.z - a.z) * t,
				a.w + (b.w - a.w) * t
			};
		}

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