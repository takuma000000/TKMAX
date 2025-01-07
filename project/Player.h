#pragma once
#include <memory>
#include "Object3d.h"
#include "Vector3.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "Sprite.h"
#include "Camera.h"
#include "Input.h"
#include <windows.h>

#include <vector>
#include "PlayerBullet.h"

class Player {
public:

	/// <summary>
	/// 
	/// </summary>
	/// <param name="object3dCommon"></param>
	/// <param name="dxCommon"></param>
	/// <param name="camera"></param>
	/// <param name="input"></param>
	/// <param name="spriteCommon"></param>
	void Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon, Camera* camera, Input* input, SpriteCommon* spriteCommon);
	/// <summary>
	/// 
	/// </summary>
	void Update();
	/// <summary>
	/// 
	/// </summary>
	void Draw();

	/// <summary>
	/// 
	/// </summary>
	void FireBullet();

	/// <summary>
	/// 
	/// </summary>
	/// <returns></returns>
	Vector3 GetTargetDirection() const; // 新規追加

	// スケール、回転、位置の設定
	/// <summary>
	void SetScale(const Vector3& scale) { transform_.scale = scale; }
	/// <summary>
	/// 
	/// </summary>
	/// <param name="rotate"></param>
	void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
	/// <summary>
	/// 
	/// </summary>
	/// <param name="translate"></param>
	void SetTranslate(const Vector3& translate) { transform_.translate = translate; }

	/// <summary>
	/// 
	/// </summary>
	/// <returns></returns>
	const Vector3& GetScale() const { return transform_.scale; }
	/// <summary>
	/// 
	/// </summary>
	/// <returns></returns>
	const Vector3& GetRotate() const { return transform_.rotate; }	
	/// <summary>
	/// 
	/// </summary>
	/// <returns></returns>
	const Vector3& GetTranslate() const { return transform_.translate; }

	/// <summary>
	/// 
	/// </summary>
	/// <param name="camera"></param>
	void SetCamera(Camera* camera);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="screenPos"></param>
	/// <returns></returns>
	Vector3 ScreenToWorld(const POINT& screenPos) const;

	/// <summary>
	/// 
	/// </summary>
	void DrawImGui();

	/// <summary>
	/// 
	/// </summary>
	/// <param name="spriteCommon"></param>
	void SetSpriteCommon(SpriteCommon* spriteCommon); // Sprite共通部の設定

	/// <summary>
	/// 
	/// </summary>
	/// <returns></returns>
	int GetBulletCount() const { return bulletCount_; }

	// 弾の取得
	const std::vector<std::unique_ptr<PlayerBullet>>& GetBullets() const { return bullets_; }

	/// <summary>
	/// 
	/// </summary>
	void ResetBulletCount() { bulletCount_ = 10; }

	/// <summary>
	/// 
	/// </summary>
	/// <returns></returns>
	bool IsOverTimerExpired() const { return overTimer_ == 0.0f; }

private:

	/// <summary>
	/// 
	/// </summary>
	Transform transform_;

	/// <summary>
	/// 
	/// </summary>
	std::unique_ptr<Object3d> object3d_ = nullptr;

	/// <summary>
	/// 
	/// </summary>
	DirectXCommon* dxCommon_ = nullptr;
	Object3dCommon* obj3dCo_ = nullptr;
	Camera* camera_ = nullptr;
	Input* input_ = nullptr;
	WindowsAPI* windo = nullptr;

	/// <summary>
	/// 
	/// </summary>
	std::vector<std::unique_ptr<PlayerBullet>> bullets_;

	/// <summary>
	/// 
	/// </summary>
	std::unique_ptr<Sprite> mouseSprite_;            // マウス位置を示すSprite
	/// <summary>
	/// 
	/// </summary>
	SpriteCommon* spriteCommon_ = nullptr;           // Sprite共通部

	/// <summary>
	/// 
	/// </summary>
	int bulletCount_ = 10; // 弾の初期残数
	/// <summary>
	/// 
	/// </summary>
	float overTimer_ = -1.0f; // -1: 未使用状態, 0以上: カウントダウン中

};
