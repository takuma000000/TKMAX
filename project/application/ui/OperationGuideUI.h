#pragma once
#include <memory>
#include <string>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "MyMath.h"
#include "Input.h"

namespace TKM {

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

		/// <summary>
		/// 右側UI全体の余白を設定します。
		/// </summary>
		void SetRightUiMargin(float px);

		/// <summary>
		/// 右側UI全体の縦間隔を設定します。
		/// </summary>
		void SetRightUiSpacing(float px);

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

	private:
		//=============================================================
		// 共通参照
		//=============================================================
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;
		float screenW_ = 0.0f;
		float screenH_ = 0.0f;

		//=============================================================
		// スプライト
		//=============================================================
		std::unique_ptr<Sprite> uiLB_;
		std::unique_ptr<Sprite> uiRB_;
		std::unique_ptr<Sprite> uiX_;
		std::unique_ptr<Sprite> uiLS_;

		//=============================================================
		// テクスチャパス
		//=============================================================
		std::string lbTex_;
		std::string rbTex_;
		std::string xTex_;
		std::string lsTex_;

		std::string padLbTex_;
		std::string padRbTex_;
		std::string padXTex_;

		std::string keyLbTex_;
		std::string keyRbTex_;
		std::string keyXTex_;

		//=============================================================
		// テクスチャサイズ
		//=============================================================
		Vector2 lbTexSize_{};
		Vector2 rbTexSize_{};
		Vector2 xTexSize_{};
		Vector2 lsTexSize_{};

		//=============================================================
		// 描画サイズ
		//=============================================================
		Vector2 lbDrawSize_{};
		Vector2 rbDrawSize_{};
		Vector2 xDrawSize_{};
		Vector2 lsDrawSize_{};

		//=============================================================
		// レイアウト
		//=============================================================
		float rightUiMargin_ = 20.0f;
		float rightUiSpacing_ = 10.0f;

		// 個別スケール（ゲームパッド）
		float padLbScale_ = 0.065f;
		float padRbScale_ = 0.114f;
		float padXScale_ = 0.066f;

		// 個別スケール（キーボード）
		float keyLbScale_ = 0.076f;
		float keyRbScale_ = 0.074f;
		float keyXScale_ = 0.084f;

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

		Vector4 idleCol_{ 1.0f, 1.0f, 1.0f, 0.75f };
		Vector4 onCol_{ 1.0f, 0.25f, 0.25f, 1.0f };

		float rightUiIdleAlpha_ = 0.45f;
		float rightUiActiveAlpha_ = 1.0f;

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
		float shakeAmpPx_ = 3.0f;
		float shakeT_RB_ = 0.0f;
		float shakeT_LB_ = 0.0f;
		float shakeT_X_ = 0.0f;

		//=============================================================
		// Xアイコンのぬめっと移動
		//=============================================================
		Vector2 xCurrentOfs_{ 0.0f, 0.0f };
		Vector2 xTargetOfs_{ 0.0f, 0.0f };
		float xMoveRangePx_ = 35.0f;
		float xFollowSpeed_ = 18.0f;
		float xReturnSpeed_ = 10.0f;
		bool prevXDown_ = false;

		//=============================================================
		// LSアイコンのぬめっと移動
		//=============================================================
		Vector2 lsCurrentOfs_{ 0.0f, 0.0f };
		Vector2 lsTargetOfs_{ 0.0f, 0.0f };
		float lsMoveRangePx_ = 18.0f;
		float lsFollowSpeed_ = 12.0f;
		float lsDeadzone_ = 0.20f;

		//=============================================================
		// 入力状態
		//=============================================================
		bool isGamepadConnected_ = false;
		bool prevGamepadConnected_ = false;
	};

} // namespace TKM