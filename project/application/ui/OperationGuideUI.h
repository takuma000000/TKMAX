#pragma once
#include <memory>
#include <string>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "MyMath.h"
#include "Input.h"

namespace TKM {

	//=====================================================
	// OperationGuideUIクラス
	// 操作ガイドUIの管理クラスです。
	// 画面右側に、LB/RB/X/LSの操作アイコンを表示します。
	//=====================================================
	class OperationGuideUI {
	public:
		/// <summary>
		/// 右側の操作UIを初期化します。
		/// </summary>
		void Initialize(
			SpriteCommon* spriteCommon,
			DirectXCommon* dxCommon,
			BaseScene* parentScene,
			float screenW,
			float screenH
		);
		/// <summary>
		/// 右側の操作UIを更新します。
		/// </summary>
		void Update(float dt);
		/// <summary>
		/// 右側の操作UIを描画します。
		/// </summary>
		/// <param name="hudAlpha">UI全体アルファ</param>
		void Draw(float hudAlpha);
		/// <summary>
		/// ImGui調整を表示します。
		/// </summary>
		void DrawImGui();
		/// <summary>
		/// 画面サイズ変更時の再レイアウトを行います。
		/// </summary>
		void UpdateLayout(float screenW, float screenH);

		// Setter========================================
		/// <summary>
		/// 右側UI全体の余白を設定します。
		/// </summary>
		void SetRightUiMargin(float px);
		/// <summary>
		/// 右側UI全体の縦間隔を設定します。
		/// </summary>
		void SetRightUiSpacing(float px);
		// ==============================================

	private:
		/// <summary>
		/// スプライトを生成して、画像全体を使う設定にします。
		/// </summary>
		std::unique_ptr<Sprite> CreateSprite_(const std::string& texPath, const Vector2& anchor, Vector2* outTexSize);
		/// <summary>
		/// 指定スプライトへテクスチャを再適用します。
		/// </summary>
		void ApplySpriteTexture_(Sprite* sp, const std::string& texPath, Vector2* outTexSize);
		/// <summary>
		/// 現在の入力デバイスに応じてUI画像を切り替えます。
		/// </summary>
		void RefreshGuideTextures_();
		/// <summary>
		/// 各UIの描画サイズを再計算して適用します。
		/// </summary>
		void ApplyGuideSizes_();
		/// <summary>
		/// 各UIの基準位置を再計算して適用します。
		/// </summary>
		void ApplyGuidePositions_();
		/// <summary>
		/// 押下中に小刻みに揺らす処理です。
		/// </summary>
		void ApplyShake_(Sprite* sp, const Vector2& basePos, bool down, float& t);

		//=============================================================
		// 共通参照
		//=============================================================
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;
		float screenW_ = 0.0f; // 画面幅
		float screenH_ = 0.0f; // 画面高さ
		//=============================================================
		// スプライト
		//=============================================================
		std::unique_ptr<Sprite> uiLB_; // LBアイコン
		std::unique_ptr<Sprite> uiRB_; // RBアイコン
		std::unique_ptr<Sprite> uiX_; // Xアイコン
		std::unique_ptr<Sprite> uiLS_; // LSアイコン
		//=============================================================
		// テクスチャパス
		//=============================================================
		std::string lbTex_; // LBアイコンのテクスチャパス
		std::string rbTex_; // RBアイコンのテクスチャパス
		std::string xTex_; // Xアイコンのテクスチャパス
		std::string lsTex_; // LSアイコンのテクスチャパス
		// ゲームパッド用テクスチャパス
		std::string padLbTex_; // LBアイコンのテクスチャパス
		std::string padRbTex_; // RBアイコンのテクスチャパス
		std::string padXTex_; // Xアイコンのテクスチャパス
		// キーボード用テクスチャパス
		std::string keyLbTex_; // LBアイコンのテクスチャパス
		std::string keyRbTex_; // RBアイコンのテクスチャパス
		std::string keyXTex_; // Xアイコンのテクスチャパス
		//=============================================================
		// テクスチャサイズ
		//=============================================================
		Vector2 lbTexSize_{}; // LBアイコンのテクスチャサイズ
		Vector2 rbTexSize_{}; // RBアイコンのテクスチャサイズ
		Vector2 xTexSize_{}; // Xアイコンのテクスチャサイズ
		Vector2 lsTexSize_{}; // LSアイコンのテクスチャサイズ
		//=============================================================
		// 描画サイズ
		//=============================================================
		Vector2 lbDrawSize_{}; // LBアイコンの描画サイズ
		Vector2 rbDrawSize_{}; // RBアイコンの描画サイズ
		Vector2 xDrawSize_{}; // Xアイコンの描画サイズ
		Vector2 lsDrawSize_{}; // LSアイコンの描画サイズ
		//=============================================================
		// レイアウト
		//=============================================================
		float rightUiMargin_ = 20.0f; // 画面右端からUIまでの余白
		float rightUiSpacing_ = 10.0f; // LBとRBの間隔
		// 個別スケール（ゲームパッド）
		float padLbScale_ = 0.065f; // LBは小さめ
		float padRbScale_ = 0.114f; // RBは大きめ
		float padXScale_ = 0.066f; // Xはやや小さめ
		// 個別スケール（キーボード）
		float keyLbScale_ = 0.076f; // Lキーはやや大きめ
		float keyRbScale_ = 0.074f; // Kキーはやや小さめ
		float keyXScale_ = 0.084f; // Jキーはやや大きめ
		// 共通スケール
		float lsScale_ = 0.064f;
		// 個別オフセット（ゲームパッド）
		Vector2 padLbOffset_{ 0.0f, 0.0f };
		Vector2 padRbOffset_{ 0.0f, 0.0f };
		Vector2 padXOffset_{ 0.0f, 0.0f };
		// 個別オフセット（キーボード）
		Vector2 keyLbOffset_{ 4.0f, 0.0f };
		Vector2 keyRbOffset_{ 1.5f, 0.0f };
		Vector2 keyXOffset_{ 5.0f, 0.0f };
		// 共通オフセット
		Vector2 lsOffset_{ 1.0f, -37.5f };
		//=============================================================
		// 色
		//=============================================================
		Vector4 colLB_{ 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 colRB_{ 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 colX_{ 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 colLS_{ 1.0f, 1.0f, 1.0f, 1.0f };
		// 押下時の色（通常は白、押下時は赤っぽくする想定）
		Vector4 idleCol_{ 1.0f, 1.0f, 1.0f, 0.75f };
		Vector4 onCol_{ 1.0f, 0.25f, 0.25f, 1.0f };
		// アルファは押下状態に応じてidleColとonColをブレンドする想定。これとは別に全体のアルファも乗算する。
		float rightUiIdleAlpha_ = 0.45f; // 押下していないときのUI全体のアルファ
		float rightUiActiveAlpha_ = 1.0f; // 押下しているときのUI全体のアルファ
		//=============================================================
		// 基準位置
		//=============================================================
		Vector2 basePosLB_{};
		Vector2 basePosRB_{};
		Vector2 basePosX_{};
		Vector2 basePosLS_{};
		//=============================================================
		// 揺れ
		//=============================================================
		float shakeAmpPx_ = 3.0f; // 揺れの強さ（ピクセル）
		float shakeT_RB_ = 0.0f; // RBアイコンの揺れの経過時間
		float shakeT_LB_ = 0.0f; // LBアイコンの揺れの経過時間
		float shakeT_X_ = 0.0f; // Xアイコンの揺れの経過時間
		//=============================================================
		// Xアイコンのぬめっと移動
		//=============================================================
		Vector2 xCurrentOfs_{ 0.0f, 0.0f }; // 現在のオフセット（基準位置からの差分）
		Vector2 xTargetOfs_{ 0.0f, 0.0f }; // 目標オフセット（基準位置からの差分）
		float xMoveRangePx_ = 35.0f; // 押下時の最大移動量（ピクセル）
		float xFollowSpeed_ = 18.0f; // 目標オフセットに追従する速さ（大きいほど素早く追従する）
		float xReturnSpeed_ = 10.0f; // 押下からの復帰速度（大きいほど素早く戻る）
		bool prevXDown_ = false; // 前フレームのXボタン押下状態。これと現在の状態を比較して、押された瞬間を検出する。
		//=============================================================
		// LSアイコンのぬめっと移動
		//=============================================================
		Vector2 lsCurrentOfs_{ 0.0f, 0.0f }; // 現在のオフセット（基準位置からの差分）
		Vector2 lsTargetOfs_{ 0.0f, 0.0f }; // 目標オフセット（基準位置からの差分）
		float lsMoveRangePx_ = 18.0f; // 押下時の最大移動量（ピクセル）
		float lsFollowSpeed_ = 12.0f; // 目標オフセットに追従する速さ（大きいほど素早く追従する）
		float lsDeadzone_ = 0.20f; // スティックの入力を無視する範囲（0〜1）。これ以下の入力は押下とみなさない。
		//=============================================================
		// 入力状態
		//=============================================================
		bool isGamepadConnected_ = false; // ゲームパッドが接続されているかどうか。これに応じてUIの画像を切り替える。
		bool prevGamepadConnected_ = false; // 前フレームのゲームパッド接続状態。これと現在の状態を比較して、接続/切断の瞬間を検出する。
	};

} // namespace TKM