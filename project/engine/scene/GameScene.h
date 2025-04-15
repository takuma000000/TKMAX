#pragma once
#include "BaseScene.h"

#include <memory>
#include "engine/audio/AudioManager.h"
#include "TextureManager.h"
#include "DirectXCommon.h"
#include "srvManager.h"
#include "engine/2d/Sprite.h"
#include "SpriteCommon.h"
#include "Object3d.h"
#include "Camera.h"
#include "Object3dCommon.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "DirectionalLight.h"

#include "engine/func/math/Vector3.h"

class GameScene : public BaseScene
{
public:
	GameScene(DirectXCommon* dxCommon, SrvManager* srvManager) : dxCommon(dxCommon), srvManager(srvManager) {}
	~GameScene() = default;

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:// ──────────────────── 初期化処理 ────────────────────

   // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   // ゲーム内のサウンドをロード＆再生
   // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void InitializeAudio();

	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	// 必要なテクスチャをロード
	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void LoadTextures();

	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	// スプライトを作成し、初期化
	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void InitializeSprite();

	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	// 必要な3Dモデルをロード
	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void LoadModels();

	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	// 3Dオブジェクトを作成し、初期化
	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void InitializeObjects();

	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	// カメラを作成し、各オブジェクトに適用
	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void InitializeCamera();

private: // ──────────────────── 更新処理 ────────────────────

	/**
	* @brief 指定した Object3d の Transform を更新する
	* @param obj 更新対象の Object3d
	* @param translate 新しい座標
	* @param rotate 回転角の加算値
	* @param scale スケール値
	*
	* ───────────────────────────────────────────
	* obj の移動・回転・スケールをまとめて更新する
	* obj->SetTranslate(translate);
	* obj->SetRotate(obj->GetRotate() + rotate);
	* obj->SetScale(scale);
	* ───────────────────────────────────────────
	*/
	void UpdateObjectTransform(std::unique_ptr<Object3d>& obj, const Vector3& translate, const Vector3& rotate, const Vector3& scale);

	//メモリ使用量
	void UpdateMemory();

private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

	std::unique_ptr<Sprite> sprite = nullptr;
	std::unique_ptr<Camera> camera = nullptr;

	std::unique_ptr<Object3d> object3d = nullptr;
	std::unique_ptr<Object3d> anotherObject3d = nullptr;
	//地面
	std::unique_ptr<Object3d> ground_ = nullptr;

	std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;// ディレクショナルライト

	static const int kMemoryHistorySize = 100;
	std::array<float, kMemoryHistorySize> memoryHistory_{}; // 過去のメモリ使用履歴（MB）
	int memoryHistoryIndex_ = 0;
};

