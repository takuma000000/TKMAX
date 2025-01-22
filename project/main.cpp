#include "MyGame.h"

Framework* gFramework = nullptr; // グローバル変数でFrameworkを保持

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	gFramework = new MyGame();

	gFramework->Run();

	delete gFramework;
	gFramework = nullptr;

	return 0;
}