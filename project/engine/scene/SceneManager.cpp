#include "SceneManager.h"

namespace TKM {
	void SceneManager::Update() {
		if (quitRequested_) { return; } // アプリ終了要求がある場合は更新しない

		// 次シーン切り替え
		if (nextScene_) {
			if (scene_) { // 現在のシーンが存在する場合のみ終了処理
				scene_->Finalize(); //	現在のシーンの終了処理
				delete scene_; // 現在のシーンの解放
			}
			scene_ = nextScene_; // シーンを切り替え
			nextScene_ = nullptr; // 次シーンポインタをクリア
			scene_->SetSceneManager(this); // シーンマネージャーをセット
			scene_->Initialize(); // 新しいシーンの初期化
		}

		if (scene_) { // シーンが存在する場合のみ更新
			scene_->Update(); // シーンの更新
		}
	}

	void SceneManager::Draw() {
		if (quitRequested_) { return; } // アプリ終了要求がある場合は描画しない
		if (scene_) { scene_->Draw(); } // シーンが存在する場合のみ描画
	}

	SceneManager::~SceneManager() {
		if (scene_) { // シーンが存在する場合のみ解放
			scene_->Finalize();
			delete scene_;
			scene_ = nullptr; // ポインタを無効化
		}
	}
}