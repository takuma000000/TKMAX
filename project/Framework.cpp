#include "Framework.h"

void Framework::Run()
{
	//ゲームの初期化
	Initialize();

	//ウィンドウの×ボタンが押されるまでループ
	while (true) {//ゲームループ
		//毎フレーム更新
		Update();
		//終了リクエストが来たら抜ける
		if (IsEndRequest()) {
			//ゲームループを抜ける
			break;
		}
		//描画
		Draw();
	}//ゲームループ終わり
	//ゲームの終了
	Finalize();
}
