#include "MyGame.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	MyGame game;

	//ゲームの初期化
	game.Initialize();

	MSG msg{};
	//ウィンドウの×ボタンが押されるまでループ
	while (true) {//ゲームループ
		//毎フレーム更新
		game.Update();
		//終了リクエストが来たら抜ける
		if (game.IsEndRequest()) {
			//ゲームループを抜ける
			break;
		}
		//描画
		game.Draw();
	}//ゲームループ終わり
	//ゲームの終了
	game.Finalize();

	return 0;
}