#pragma once
#include <memory>
#include <string>
#include <array>
#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "WindowsAPI.h"

namespace TKM {
	//=============================================================
	// LBGaugeUIクラス
	// LBゲージUIの管理を行うクラス。
	//=============================================================
	class LBGaugeUI {
	public:
		struct Desc {
			// RBと同じ思想：中心座標で扱う
			Vector2 center_ = { WindowsAPI::kClientWidth_ * 0.5f, WindowsAPI::kClientHeight_ - 40.0f };
			Vector2 size_ = { 520.0f, 18.0f };

			int segments_ = 5; // 分割数
			float pad_ = 2.2f;     // 内側余白
			float gap_ = 7.7f;     // セグメント間の隙間

			// テクスチャ
			std::string frameTex_ = "./resources/texture/gray.jpg"; // フレームのテクスチャ（背景）
			std::string fillTex_ = "./resources/texture/gauge_green.jpg"; // 塗りつぶしのテクスチャ

			// 色
			Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };   // 通常
			Vector4 drainColor_ = { 0.25f, 0.95f, 1.0f, 1.0f }; // 消費直後
			Vector4 refillColor_ = { 0.55f, 1.0f, 0.55f, 1.0f };// 回復直後
		};

		/// <summary>
		/// LBGaugeUIを初期化します。
		/// </summary>
		/// <param name="spriteCommon">SpriteCommonのインスタンス</param>
		/// <param name="dxCommon">DirectXCommonのインスタンス</param>
		/// <param name="parentScene">このLBGaugeUIを使用するシーン</param>
		/// <param name="desc">LBGaugeUIの表示設定</param>
		void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc);
		/// <summary>
		/// LBGaugeUIを終了処理します。
		/// </summary>
		/// <param name="dt"></param>
		/// <param name="ammo">現在のLB残弾数</param>
		/// <param name="maxAmmo">LBの最大残弾数</param>
		/// <param name="blink">点滅させるかどうか（LBを押しているときなど）</param>
		void Update(float dt, int ammo, int maxAmmo, bool blink = false);
		/// <summary>
		/// LBGaugeUIを描画します。
		/// </summary>
		void Draw();

		/// <summary>
		/// LBGaugeUIのImGui表示を行います。
		/// </summary>
		/// <param name="ammo"></param>
		bool IsVisible() const { return visible_; }

		// Getter=========================================
		/// <summary>
		/// LBGaugeUIの表示設定を取得します。
		/// </summary>
		/// <returns></returns>
		const Desc& GetDesc() const { return desc_; }
		// ===============================================
		// Setter=========================================
		/// <summary>
		/// LBGaugeUIの表示/非表示を設定します。
		/// </summary>
		/// <param name="v">表示するならtrue、非表示にするならfalse</param>
		void SetVisible(bool v) { visible_ = v; }
		/// <summary>
		/// LBGaugeUIの表示設定を更新します。これを呼ぶとレイアウトが再計算されます。
		/// </summary>
		/// <param name="desc">新しい表示設定</param>
		void SetDesc(const Desc& desc);
		// ===============================================

	private:

		/// <summary>
		/// LBGaugeUIのレイアウトを更新します。表示設定（Desc）をもとに、スプライトの位置やサイズを計算して保存します。
		/// </summary>
		void ApplyLayout_();
		/// <summary>
		/// LBGaugeUIのアニメーションを更新します。消費・回復のパルスや、押下時の波などを計算して保存します。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="pressed">LBが押されているかどうか</param>
		void ApplyAnim_(float dt, bool pressed);

		//==============================================
		// 共通参照
		//==============================================
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;
		//==============================================
		// 設定・表示状態
		//==============================================
		Desc desc_{}; // 表示設定
		bool visible_ = true; // 表示状態
		//==============================================
		// 弾数状態
		//==============================================
		int currentAmmo_ = 0; // 現在の弾数
		int lastAmmo_ = -1; // 前フレームの弾数（変化を検出するため）
		int shownAmmo_ = 0;            // 今 “見せてる” 個数
		int refillTarget_ = 0;         // 最終的に見せたい個数
		bool refillAnimating_ = false; // 回復アニメ中かどうか
		//==============================================
		// タイマー
		//==============================================
		float drainTimer_ = 0.0f; // 消費フラッシュのタイマー（0.0f〜kFlashSec_）
		float refillTimer_ = 0.0f; // 回復フラッシュのタイマー（0.0f〜kFlashSec_）
		float shakeTimer_ = 0.0f; // 押下シェイクのタイマー（0.0f〜kShakeSec_）
		float punchTimer_ = 0.0f; // 消費パンチのタイマー（0.0f〜kPunchSec_）
		float refillPunchTimer_ = 0.0f; // 回復パンチのタイマー（0.0f〜kRefillPunchSec_）
		float refillStepTimer_ = 0.0f; // 回復アニメの段階タイマー（0.0f〜kRefillStepSec_）
		//==============================================
		// 定数
		//==============================================
		static constexpr float kFlashSec_ = 0.12f; // 消費・回復フラッシュの秒数
		static constexpr float kShakeSec_ = 0.12f; // 押下シェイクの秒数
		static constexpr float kPunchSec_ = 0.10f; // 消費パンチの秒数
		static constexpr float kRefillPunchSec_ = 0.10f; // 回復パンチの秒数
		static constexpr float kRefillStepSec_ = 0.045f; // 回復アニメの1段階あたりの秒数
		//==============================================
		// スプライト
		//==============================================
		std::unique_ptr<Sprite> frame_; // フレーム全体のスプライト
		std::array<std::unique_ptr<Sprite>, 5> seg_{}; // セグメントのスプライト（最大5個。desc_.segments_で実際の使用数が決まる）
		//==============================================
		// レイアウト基準
		//==============================================
		Vector2 baseFramePos_{}; // フレームの基準位置（center_を元に計算される）
		Vector2 baseFrameSize_{}; // フレームの基準サイズ（size_を元に計算される）
		std::array<Vector2, 5> baseSegPos_{}; // セグメントの基準位置（desc_.pad_やdesc_.gap_を元に計算される）
		std::array<Vector2, 5> baseSegSize_{}; // セグメントの基準サイズ（desc_.pad_を元に計算される）
		//==============================================
		// 押下パルス
		//==============================================
		float pressPulseT_ = 0.0f;      // 押下中の波
		float pressPulseAmp_ = 0.0f;    // 0..1（押してない時は0へ戻す）
		//==============================================
		// シェイク
		//==============================================
		float shakeAmpPx_ = 2.0f;       // 揺れ幅（小さめが良い）
		//==============================================
		// 消費パンチ
		//==============================================
		float punchAmp_ = 0.10f;        // 拡縮量（10%）
		//==============================================
		// 回復パンチ
		//==============================================
		float refillPunchAmp_ = 0.07f;  // 回復は少し弱め
	};
}