#pragma once
#include <memory>

#include "IntroSequence.h"
#include "Easing.h"
#include "IrisUtil.h"
#include "Input.h"
#include "Player.h"
#include "Sprite.h"
#include "camera/Camera.h"

namespace TKM {
	class GameFlowController {
	public:
		enum class TransitionRequest {
			None,
			ToTitle,
			ToGameOver,
		};

	public:
		void Initialize(DirectXCommon* dxCommon);

		// Intro更新（敵初期化要求もここで作る）
		void Update(float dt, Camera* camera, bool enemiesInitialized, bool& outRequestInitEnemies);

		// 死亡 / タイトル戻り（Tキー） / アイリス閉じ進行
		TransitionRequest UpdateTransitions(float dt, Player* player);

		// 描画（IntroSequence側に描かせる）
		void Draw() const;

		// 状態
		bool IsGameplayLocked() const;
		bool IsIrisClosing() const { return irisClosing_; }
		bool IsExternalIrisDraw() const { return externalIrisDraw_; }

		// Setter=====================================
		/// <summary>
		/// 外部からIris描画を制御するか？
		/// </summary>
		/// <param name="enable"></param>
		void SetExternalIrisDraw(bool enable);
		// ===========================================
		// Getter=====================================
		/// <summary>
		/// Irisスプライトの取得。
		/// </summary>
		/// <returns></returns>
		TKM::Sprite* GetIrisSprite() const;
		/// <summary>
		/// Iris最大スケールの取得。
		/// </summary>
		/// <returns></returns>
		float GetIrisMaxScale() const;
		// ===========================================
	private:
		// Intro
		std::unique_ptr<IntroSequence> intro_ = nullptr;

		// Lock
		bool gameplayLocked_ = true;

		// Iris close
		static constexpr float kIrisDurationSec_ = 0.8f;
		bool        irisClosing_ = false;
		Ease::Tween irisCloseTween_;
		bool        irisToTitle_ = false;

		// Death → transition
		bool  playerDeathStarted_ = false;
		float playerDeathElapsed_ = 0.0f;

		// 固定dtで閉じ進行（いまの実装に合わせる）
		static constexpr float kFixedDt_ = 0.016f;

		bool externalIrisDraw_ = false;
	};
} // namespace TKM