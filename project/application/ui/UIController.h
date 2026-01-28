#pragma once
#include <memory>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "MyMath.h"
#include "RBGaugeUI.h"
#include "Player.h"
#include <Input.h>

namespace TKM {
	class UIController {
	public:

		/// <summary>
		/// UI を初期化します。
		/// </summary>
		/// <param name="spriteCommon">スプライト共通管理クラス</param>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="parentScene">所属する親シーン</param>
		/// <param name="screenW">画面幅（ピクセル）</param>
		/// <param name="screenH">画面高さ（ピクセル）</param>
		void Initialize(
			SpriteCommon* spriteCommon,
			DirectXCommon* dxCommon,
			BaseScene* parentScene,
			float screenW,
			float screenH
		);
		/// <summary>
		/// UI の更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="player">参照対象となるプレイヤー</param>
		void Update(float dt, Player* player);
		/// <summary>
		/// UI を描画します。
		/// </summary>
		void Draw();

		/// <summary>
		/// 画面サイズ変更に応じてレイアウトを更新します。
		/// </summary>
		/// <param name="screenW">画面幅（ピクセル）</param>
		/// <param name="screenH">画面高さ（ピクセル）</param>
		void UpdateLayout(float screenW, float screenH);

	private:
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;

		std::unique_ptr<Sprite> uiLT_;
		std::unique_ptr<Sprite> uiLB_;
		std::unique_ptr<Sprite> uiRB_;
		std::unique_ptr<RBGaugeUI> rbGaugeUI_;
	};
}