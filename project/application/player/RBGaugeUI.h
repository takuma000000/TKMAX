#pragma once
#include <memory>
#include <string>
#include <algorithm>
#include <cstdlib>
#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "WindowsAPI.h"
#include <vector>

namespace TKM {

	class RBGaugeUI {
	public:
		struct Desc {
			// 位置は「中央基準」で扱う（左右端→中央の計算が楽）
			Vector2 center_ = { WindowsAPI::kClientWidth_ * 0.5f, WindowsAPI::kClientHeight_ - 60.0f };
			Vector2 size_ = { 520.0f, 18.0f }; // 全体幅/高さ

			// shake
			float shakeTime_ = 0.12f; // 揺れの継続時間（秒）
			float shakePower_ = 4.0f; // 揺れの強さ（ピクセル）

			// lag（遅延バー）
			float lagSpeed_ = 900.0f; // 大きいほど速く追従（弾なので速めが気持ちいい）

			// テクスチャ
			std::string frameTex_ = "./resources/texture/gray.jpg"; // フレーム
			std::string fillTex_ = "./resources/texture/blue.dds"; // 塗り（通常）

			// 色（単純回避用）
			Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 0.3f };   // 通常
			Vector4 drainColor_ = { 0.25f, 0.95f, 1.0f, 1.0f };   // 減ってる最中（撃った直後）
			Vector4 refillColor_ = { 0.55f, 1.0f, 0.55f, 1.0f };   // 回復中
			Vector4 lagColor_ = { 0.65f, 0.65f, 0.65f, 1.0f };  // 遅延バー
		};

	public:

		/// <summary>
		/// 弾数 UI を初期化します。
		/// </summary>
		/// <param name="spriteCommon">スプライト共通管理クラス</param>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="parentScene">所属する親シーン</param>
		/// <param name="desc">弾数 UI の設定情報</param>
		void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, const Desc& desc);
		/// <summary>
		/// 弾数 UI の更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="ammo">現在の弾数</param>
		/// <param name="maxAmmo">最大弾数</param>
		/// <param name="refilling">リロード（補充）中の場合 true</param>
		void Update(float dt, int ammo, int maxAmmo, bool refilling, bool blink = false);
		/// <summary>
		/// 弾数 UI を描画します。
		/// </summary>
		void Draw();

		/// <summary>
		/// UI が表示状態かどうかを取得します。
		/// </summary>
		/// <returns>表示中の場合 true、それ以外は false</returns>
		bool IsVisible() const { return visible_; }

		// Getter=====================================
		/// <summary>
		/// 弾数 UI の設定情報を取得します。
		/// </summary>
		/// <returns></returns>
		const Desc& GetDesc() const { return desc_; }
		// ===========================================
		// Setter=====================================
		/// <summary>
		/// 可視状態を設定します。
		/// </summary>
		/// <param name="v">表示する場合 true、それ以外は false</param>
		void SetVisible(bool v);
		/// <summary>
		/// 設定情報を設定します。
		/// </summary>
		/// <param name="desc">設定する情報</param>
		void SetDesc(const Desc& desc);
		// ===========================================

	private:
		/// <summary>
		/// a〜b の範囲でランダムな浮動小数点数を返します。
		/// </summary>
		/// <param name="a">最小値</param>
		/// <param name="b">最大値</param>
		/// <returns>a〜b の範囲内のランダムな浮動小数点数</returns>
		float RandRange_(float a, float b);

		//======================================================================
		// 参照 / 設定
		//======================================================================
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;
		Desc desc_{}; // 設定情報
		bool visible_ = true; // 表示状態
		//======================================================================
		// 状態
		//======================================================================
		int lastAmmo_ = -1; // 前フレームの弾数（減ったかどうかの判定用。初期値は -1 で、最初の Update で現在弾数に置き換える）
		int prevAmmo_ = -1; // さらに前フレームの弾数（破片数の決定用）
		float lagAmmo_ = 0.0f; // 遅延バー用の弾数（float で、見た目上は小数点以下切り捨て。減ったときはすぐに現在弾数に合わせる。回復中はゆっくり現在弾数に近づける）
		float prevHalfW_ = 0.0f; // 前フレームの半幅（破片の発生位置決定用。Update の最後で現在の半幅に置き換える）
		//======================================================================
		// 揺れ
		//======================================================================
		float shakeTimer_ = 0.0f; // 揺れの残り時間（秒）。撃った瞬間に shakeTime_ をセットして、Update で減らしていく。0 以下になったら揺れ終了。
		//======================================================================
		// 直後だけ色を変える（撃った直後＝短時間）
		//======================================================================
		float drainTimer_ = 0.0f; // 撃った直後の残り時間（秒）。撃った瞬間に kDrainFlashSec_ をセットして、Update で減らしていく。0 以下になったら通常色。
		static constexpr float kDrainFlashSec_ = 0.10f; // 撃った直後の色変化の継続時間（秒）
		//======================================================================
		// パンチ（減った瞬間だけ縮んで戻る）
		//======================================================================
		float punchTimer_ = 0.0f; // パンチの残り時間（秒）。減った瞬間に kPunchSec_ をセットして、Update で減らしていく。0 以下になったらパンチ終了。
		static constexpr float kPunchSec_ = 0.08f; // パンチの継続時間（秒）
		//======================================================================
		// スプライト
		//======================================================================
		std::unique_ptr<Sprite> frame_; // フレーム
		std::unique_ptr<Sprite> fillL_; // 塗り（左半分）
		std::unique_ptr<Sprite> fillR_; // 塗り（右半分。分けるのは遅延バーを別の色にするため）
		std::unique_ptr<Sprite> lagL_; // 遅延バー（左半分。右半分は fillR_ と同じテクスチャで、色だけ変える）
		std::unique_ptr<Sprite> lagR_; // 遅延バー（右半分）
		//======================================================================
		// 破片（チップ）
		//======================================================================
		struct Chip {
			std::unique_ptr<Sprite> sp_; // 破片のスプライト
			Vector2 pos_{}; // 破片の位置（ワールド座標）
			Vector2 vel_{}; // 破片の速度
			float life_ = 0.0f; // 破片の残り寿命（秒）。撃った瞬間に maxLife_ をセットして、Update で減らしていく。0 以下になったら消える。
			float maxLife_ = 0.0f; // 破片の寿命（秒）。これもランダムにして、寿命が尽きると消えるようにする
			float size_ = 6.0f; // 破片のサイズ。これもランダムにして、Update でスプライトのサイズに反映させる
			bool active_ = false; // 破片が有効かどうか。撃った瞬間に true にして、寿命が尽きると false にする。Update で active_ な破片だけ更新・描画するようにする
		};
		std::vector<Chip> chips_; // 破片のプール。撃ったときにこの中から空いてるのを探して発生させる。数は適当に（多すぎると重くなるし、少なすぎると足りなくなる）。64 個もあればまず足りないと思う。
		// 破片パラメータ（好みで調整）
		static constexpr int   kChipPool_ = 64; // 破片のプール数
		static constexpr float kChipLife_ = 0.22f; // 破片の寿命（秒）
		static constexpr float kChipSpeed_ = 140.0f; // 破片の初速（ピクセル/秒）
		static constexpr float kChipSpread_ = 90.0f; // 破片の広がり角度（度）。0 なら真横、90 なら上下に広がる。180 以上は全方向に広がる
		static constexpr float kChipGravity_ = 520.0f; // 破片の重力加速度（ピクセル/秒^2）。0 なら重力なし。正の値で下方向にかかる
		static constexpr float kChipSizeMin_ = 4.0f; // 破片のサイズの最小値（ピクセル）。これもランダムにして、サイズがランダムになるようにする。あまり小さすぎると見えないし、あまり大きすぎると不自然なので、適当に調整する
		static constexpr float kChipSizeMax_ = 10.0f; // 破片のサイズの最大値（ピクセル）
		//======================================================================
		// 点滅
		//======================================================================
		float blinkT_ = 0.0f;          // 点滅用タイマー
		float blinkInterval_ = 0.10f;  // 何秒ごとにON/OFFするか
		float blinkLowMul_ = 0.25f;    // OFF側の暗さ（alpha倍率）

		/// <summary>
		/// 破片を発生させます。
		/// </summary>
		/// <param name="cx">発生中心の X 座標（ワールド座標）</param>
		/// <param name="y">発生位置の Y 座標（ワールド座標）</param>
		/// <param name="oldHalf">分割前の半径（または半幅）</param>
		/// <param name="newHalf">分割後の半径（または半幅）</param>
		void SpawnChips_(float cx, float y, float oldHalf, float newHalf);
		/// <summary>
		/// 破片の更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		void UpdateChips_(float dt);
		/// <summary>
		/// 破片を描画する。
		/// </summary>
		void DrawChips_();
	};
} // namespace TKM