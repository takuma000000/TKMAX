#pragma once
#include "Framework.h"
#include <memory> // 追加

class Player; // 前方宣言

// GE3クラス化(MyClass)
#include "Input.h"
#include "SpriteCommon.h"
#include "engine/2d/Sprite.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "Camera.h"
#include "ImGuiManager.h"
#include "AudioManager.h"

#include <vector>

//WinterGame_クラス
#include "Skydome.h"
#include "Player.h"
#include "Enemy.h" // 敵クラスをインクルード
#include "PlayerBullet.h"
#include "Title.h" // Title クラス

enum class GamePhase {
	Title,
	Explanation,
	GameScene,
	Clear,
	Over,
};


class MyGame : public Framework
{
public://メンバ関数
	//初期化
	void Initialize() override;
	//終了
	void Finalize() override;
	//毎フレーム更新
	void Update() override;
	//描画
	void Draw() override;

	void ResetGame();

private://メンバ変数

	//Audio
	std::unique_ptr<AudioManager> audio = nullptr;
	//ポインタ...Input
	std::unique_ptr<Input> input = nullptr;
	//ポインタ...SpriteCommon
	std::unique_ptr<SpriteCommon> spriteCommon = nullptr;
	//ポインタ...Sprite
	std::unique_ptr<Sprite> sprite = nullptr;
	//ポインタ...Object3dCommon
	std::unique_ptr<Object3dCommon> object3dCommon = nullptr;
	//ポインタ...Camera
	std::unique_ptr<Camera> camera = nullptr;
	//ポインタ...ImGuiManager
	std::unique_ptr<ImGuiManager>  imguiManager = nullptr;
	//ポインタ...Skydome
	std::unique_ptr<Skydome> skydome = nullptr;
	//ポインタ...Player
	std::unique_ptr<Player> player = nullptr;

	std::vector<Enemy*> enemies; // 敵のリストを追加


	//Object3dの初期化
	Object3d* object3d = new Object3d();
	//AnotherObject3d ( もう一つのObject3d )
	Object3d* anotherObject3d = new Object3d();

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

private:
	//フラグ
	bool endRequest_ = false; // 終了フラグ

	GamePhase currentPhase_ = GamePhase::Title; // 現在のフェーズ

	// Title クラスのポインタを追加
	Title* title = nullptr;
	// Claer クラスのポインタを追加
	std::unique_ptr<Sprite> clearSprite_; // CLEAR画面用スプライト
	// Over クラスのポインタを追加
	std::unique_ptr<Sprite> overSprite_; // GameOver画面用スプライト

};

