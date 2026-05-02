#include "SceneManager.h"

namespace TKM {
	void SceneManager::Update() {
		if (quitRequested_) { return; } // アプリ終了要求がある場合は更新しない

		// 次シーン切り替え
		if (nextScene_) {
			if (scene_) { // 現在のシーンが存在する場合のみ終了処理
				scene_->Finalize(); // 現在のシーンの終了処理
			}
			scene_ = std::move(nextScene_); // 次のシーンに切り替え

			scene_->SetSceneManager(this); // シーンマネージャーをセット
			scene_->Start(); // 新しいシーンの初期化
		}

		if (scene_) { // シーンが存在する場合のみ更新
			scene_->Update(); // シーンの更新
		}
	}

	void SceneManager::Draw() {
		if (quitRequested_) { return; } // アプリ終了要求がある場合は描画しない
		if (scene_) { scene_->Draw(); } // シーンが存在する場合のみ描画
	}

	void SceneManager::Draw3D() {
		if (quitRequested_) { return; } // アプリ終了要求がある場合は描画しない
		if (!scene_) { return; } // シーンが存在しない場合は描画しない

		scene_->DrawBack(); // 背景スプライト（3Dより先）
		scene_->Draw3D();         // 3D描画
	}

	void SceneManager::DrawSprite() {
		if (quitRequested_) { return; } // アプリ終了要求がある場合は描画しない
		if (scene_) { scene_->DrawSprite(); } // シーンが存在する場合のみスプライト描画
	}

	SceneManager::~SceneManager() {
		if (scene_) { // シーンが存在する場合のみ終了処理
			scene_->Finalize(); // 現在のシーンの終了処理
		}
	}

	void SceneManager::ChangeScene(const std::string& sceneName) {
		if (!sceneFactory_) { // シーンファクトリーがセットされていない場合は切り替えできない
			return;
		}

		// シーンファクトリーを使って新しいシーンを生成し、次のシーンとしてセット
		SetNextScene(sceneFactory_->CreateScene(sceneName));
	}
}