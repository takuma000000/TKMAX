#include "SceneManager.h"

void SceneManager::Update()
{
	//TODO : シーン切り替え機構
	//次のシーンが予約されていたら
	if (nextScene_) {
		//今のシーンを解放
		if (scene_) {
			scene_->Finalize();
			delete scene_;
		}
		//次のシーンを実行中にする
		scene_ = nextScene_;
		//次のシーンを予約から解除
		nextScene_ = nullptr;
		//シーンマネージャーを設定
		scene_->SetSceneManager(this);
		//次のシーンを初期化
		scene_->Initialize();
	}

	//実行中シーンを更新
	if (scene_) {
		scene_->Update();
	} else {
		// Handle the error or initialize scene_
	}
}

void SceneManager::Draw()
{
	//実行中シーンを描画
	scene_->Draw();
}

SceneManager::~SceneManager()
{
	if (scene_) {
		scene_->Finalize();
		delete scene_;
		scene_ = nullptr; // ポインタを無効化
	}

}
