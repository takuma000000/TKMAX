#pragma once
#include <memory>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "MyMath.h"

class Player;

namespace TKM {

	class DirectXCommon;

	//=============================================================
	// DodgeUIクラス
	// 回避クールタイムゲージを表示するUI。
	//=============================================================
	class DodgeUI {
	public:
		/// <summary>
		/// 回避UIを初期化します。
		/// </summary>
		void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon);
		/// <summary>
		/// 回避UIを更新します。
		/// </summary>
		void Update(Player* player, float screenW, float screenH);
		/// <summary>
		/// 回避UIを描画します。
		/// </summary>
		void Draw(float hudAlpha);

	private:

		//=============================================================
		// ゲージ描画用スプライト
		//=============================================================
		std::unique_ptr<Sprite> gaugeBack_; // ゲージの背景スプライト
		std::unique_ptr<Sprite> gaugeFill_; // ゲージの塗りつぶしスプライト
		//=============================================================
		// ゲージの詳細設定
		//=============================================================
		// サイズ
		Vector2 gaugeSize_{ 90.0f, 8.0f };      // ゲージのサイズ
		Vector2 gaugeBackSize_{ 98.0f, 14.0f }; // ゲージ背景のサイズ
		// 色
		Vector4 backColor_{ 0.0f, 0.0f, 0.0f, 0.7f }; // ゲージの背景色
		Vector4 fillColor_{ 0.2f, 0.8f, 1.0f, 0.95f };  // ゲージの塗りつぶし色
		//=============================================================
		// ゲージの状態
		//=============================================================
		bool visible_ = false; // ゲージが表示されているかどうか
		float rate_ = 0.0f;    // ゲージの溜まり具合（0.0f～1.0f）
	};

}