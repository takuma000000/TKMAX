#include "Player.h"
#include <engine/effect/particle/ParticleManager.h>


void Player::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	common_ = common;
	dxCommon_ = dxCommon;

	object_ = std::make_unique<Object3d>();
	object_->Initialize(common_, dxCommon_);
	object_->SetModel("jett.obj");
	object_->SetEnvironment("./resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");

	TextureManager::GetInstance()->LoadTexture("./resources/circle.png");

	// パーティクルグループ作成
	ParticleManager::GetInstance()->CreateParticleGroup(
		"jetSmoke", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);

	Vector3 jetPos = object_->GetTranslate();
	jetPos.z -= 2.0f; // 機体の後方
	jetEmitter_.Initialize("jetSmoke", jetPos); // ← 一度だけ初期化
}

void Player::Update() {
	HandleGamePadMove(); // ゲームパッドのスティック入力で移動
	HandleFollowCamera(); // カメラの追従処理
	RemoveEnemyIfDead(); // 敵が死んでたら参照をクリア

	// RTホールド中はターゲットをロック表示（切り替わり時は前の敵を解除）
	{
		Input* input = Input::GetInstance();
		Enemy* cur = (enemy_ && !enemy_->IsDead()) ? enemy_ : nullptr;

		bool hold = (input->GetRightTrigger() > 128 && canUseSpecial_);

		// ターゲットが切り替わったら前のロックを解除
		if (lastLockedEnemy_ && lastLockedEnemy_ != cur) {
			lastLockedEnemy_->SetLocked(false);
		}

		if (cur && hold) {
			cur->SetLocked(true);
			lastLockedEnemy_ = cur;
		} else {
			if (cur) cur->SetLocked(false);
			lastLockedEnemy_ = nullptr;
		}
	}

	HandleShooting(); // 先にプレイヤーの操作より下に置くと自然
	for (auto it = bullets_.begin(); it != bullets_.end(); ) {
		(*it)->Update();
		if ((*it)->IsDead()) {
			it = bullets_.erase(it);
		} else {
			++it;
		}
	}

	// ---- ジェット煙 ----
	Vector3 jetPos = object_->GetTranslate();
	jetPos.z -= 2.0f;  // 機体のケツあたり
	jetEmitter_.SetPosition(jetPos);  // 新しく追加する関数
	jetEmitter_.Update();             // 1フレームごとに放出チェック

	ParticleManager::GetInstance()->Update();

	object_->Update();
}

void Player::ImGuiDebug() {
	if (!object_) return;

	ImGui::Begin("Player");

	Vector3 pos = object_->GetTranslate();
	Vector3 rot = object_->GetRotate();
	Vector3 scale = object_->GetScale();

	if (ImGui::DragFloat3("Position", &pos.x, 0.01f)) {
		object_->SetTranslate(pos);
	}
	if (ImGui::DragFloat3("Rotation", &rot.x, 0.01f)) {
		object_->SetRotate(rot);
	}
	if (ImGui::DragFloat3("Scale", &scale.x, 0.01f)) {
		object_->SetScale(scale);
	}

	ImGui::Text("Special Attack: %s", canUseSpecial_ ? "READY" : "NOT READY"); // 一撃必殺の使用可能状態を表示


	ImGui::End();
}

void Player::RemoveEnemyIfDead()
{
	if (enemy_ && enemy_->IsDead()) {
		enemy_ = nullptr;
	}
}


void Player::Draw(DirectXCommon* dxCommon) {
	object_->Draw(dxCommon);

	for (auto& bullet : bullets_) {
		bullet->Draw(dxCommon);
	}
}

void Player::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos);
}

void Player::SetParentScene(BaseScene* scene) {
	// 必要な処理を書く、例：
	parentScene_ = scene;
}

void Player::StartCameraShake(int frameCount) {
	cameraShakeFrame_ = frameCount;
}

void Player::HandleGamePadMove() {
	Input* input = Input::GetInstance();
	const float moveSpeed = 0.25f;
	const SHORT deadZone = 8000;

	SHORT lx = input->GetLeftStickX();
	SHORT ly = input->GetLeftStickY();
	float stickX = abs(lx) > deadZone ? (lx / 32768.0f) : 0.0f;
	float stickY = abs(ly) > deadZone ? (ly / 32768.0f) : 0.0f;

	Vector3 pos = object_->GetTranslate();
	// XYのみ移動（Zはレール固定）
	pos.x += stickX * moveSpeed;
	pos.y -= -stickY * moveSpeed;
	pos.z = 0.0f; // レール固定

	// 範囲クランプ
	pos.x = std::clamp(pos.x, moveMin_.x, moveMax_.x);
	pos.y = std::clamp(pos.y, moveMin_.y, moveMax_.y);

	object_->SetTranslate(pos);

	// バンク角（ロール）をスティックに応じてスムージング
	float targetBank = -stickX * 0.35f; // 左で左に傾く
	// 簡易クリティックダンピング
	float k = 0.25f, d = 0.45f;
	bankVel_ += (targetBank - bankAngle_) * k - bankVel_ * d;
	bankAngle_ += bankVel_;
	Vector3 rot = object_->GetRotate();
	rot.z = bankAngle_; // ロール
	object_->SetRotate(rot);
}


void Player::HandleCameraControl() {
	if (!camera) return;

	Input* input = Input::GetInstance();

	const float sensitivity = 0.02f; // 回転感度（調整してOK）
	const SHORT deadZone = 8000;

	SHORT rx = input->GetRightStickX();
	SHORT ry = input->GetRightStickY();

	// デッドゾーン処理
	float rotX = abs(ry) > deadZone ? -(ry / 32768.0f) * sensitivity : 0.0f;
	float rotY = abs(rx) > deadZone ? (rx / 32768.0f) * sensitivity : 0.0f;

	Vector3 rotation = camera->GetRotate();
	rotation.x += rotX;
	rotation.y += rotY;

	// X軸回転に制限をかける（真上向いたり真下向いたりしないように）
	const float limitX = 1.5f; // ≒ 85度くらい
	rotation.x = std::clamp(rotation.x, -limitX, limitX);

	camera->SetRotate(rotation);
}

void Player::HandleFollowCamera() {
	if (!camera) return;

	Vector3 playerPos = object_->GetTranslate();
	Vector3 camRot = camera->GetRotate();
	float distance = 40.0f;
	float height = 4.0f;

	float angleY = camRot.y;
	Vector3 offset = {
		sinf(angleY) * -distance,
		height,
		cosf(angleY) * -distance
	};

	// --- シェイクオフセット加算 ---
	if (cameraShakeFrame_ > 0) {
		cameraShakeOffset_.x = (rand() % 100 - 50) / 500.0f; // ±0.1くらい
		cameraShakeOffset_.y = (rand() % 100 - 50) / 500.0f;
		cameraShakeOffset_.z = (rand() % 100 - 50) / 500.0f;
		cameraShakeFrame_--;
	} else {
		cameraShakeOffset_ = { 0, 0, 0 };
	}

	Vector3 cameraPos = playerPos + offset + cameraShakeOffset_;
	camera->SetTranslate(cameraPos);
}


void Player::HandleShooting() {
	Input* input = Input::GetInstance();

	// ▼ LT：完全追従弾（強ホーミング）
	bool ltPressed = (input->GetLeftTrigger() > 128);
	if (ltPressed && !ltHeld_) {
		auto bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize(common_, dxCommon_);

		Vector3 startPos = object_->GetTranslate();
		bullet->SetPosition(startPos);

		// 追従対象：現在のenemy_（GameScene側で最も近い敵が設定されている想定）
		bullet->SetEnemy(enemy_);               // ← ターゲット
		bullet->SetHoming(true, 0.6f);          // ← 完全追従ON（速度は好みで）
		bullet->SetCamera(camera);
		bullet->SetPlayer(this);

		// 初速は一応ターゲット方向、enemy_がいなければ前方
		if (enemy_ && !enemy_->IsDead()) {
			Vector3 dir = enemy_->GetWorldPosition() - startPos;
			float len = MyMath::Length(dir);
			bullet->SetVelocity((len > 0.01f ? MyMath::Normalize(dir) : Vector3{ 0,0,1 }) * 0.6f);
		} else {
			bullet->SetVelocity({ 0,0,0.6f });
		}

		bullets_.push_back(std::move(bullet));
	}
	if (!ltPressed) {
		ltHeld_ = false; // 離したら解放（次の押下で1発だけ出る）
	} else {
		ltHeld_ = true;
	}

	// RBボタン：通常弾
	if (input->TriggerButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
		auto bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize(common_, dxCommon_);

		Vector3 startPos = object_->GetTranslate();
		bullet->SetPosition(startPos);

		if (enemy_) {
			Vector3 enemyPos = enemy_->GetWorldPosition();
			Vector3 dir = enemyPos - startPos;
			float length = MyMath::Length(dir);

			if (length < 0.01f) {
				dir = { 0, 0, 1 }; // fallback
			} else {
				dir = MyMath::Normalize(dir);
			}

			bullet->SetVelocity(dir * 0.5f);
		} else {
			bullet->SetVelocity({ 0, 0, 0.5f });
		}

		bullet->SetCamera(camera);
		bullet->SetEnemy(enemy_);
		bullet->SetPlayer(this);
		bullets_.push_back(std::move(bullet));
	}

	// LBボタン：全敵必中弾
	if (input->TriggerButton(XINPUT_GAMEPAD_LEFT_SHOULDER) && allEnemies_) {
		for (auto& enemy : *allEnemies_) {
			if (enemy->IsDead()) continue;

			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize(common_, dxCommon_);

			Vector3 startPos = object_->GetTranslate();
			Vector3 enemyPos = enemy->GetWorldPosition();
			Vector3 dir = MyMath::Normalize(enemyPos - startPos);

			bullet->SetPosition(startPos);
			bullet->SetVelocity(dir * 0.5f);
			bullet->SetCamera(camera);
			bullet->SetEnemy(enemy.get());
			bullet->SetPlayer(this);

			bullets_.push_back(std::move(bullet));
		}
	}

	// RTボタン：一撃必殺（最も近い敵に必中弾）
	const bool pressed = (input->GetRightTrigger() > 128);

	// 押している間：ホールド状態にする（発射はしない）
	if (pressed && canUseSpecial_ && enemy_ && !enemy_->IsDead()) {
		rtHeld_ = true;
		// ロックの見た目は Update() 側で既にONにしている
	}

	// 離した瞬間：発射
	if (!pressed && rtHeld_) {
		if (canUseSpecial_ && enemy_ && !enemy_->IsDead()) {
			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize(common_, dxCommon_);

			Vector3 startPos = object_->GetTranslate();
			Vector3 enemyPos = enemy_->GetWorldPosition();
			Vector3 dir = MyMath::Normalize(enemyPos - startPos);

			bullet->SetPosition(startPos);
			bullet->SetVelocity(dir * 0.5f);
			bullet->SetCamera(camera);
			bullet->SetEnemy(enemy_);
			bullet->SetPlayer(this);
			bullet->SetSpecialAttack(true);

			bullets_.push_back(std::move(bullet));

			// 見た目のロックは解除
			enemy_->SetLocked(false);
			canUseSpecial_ = false;
		}
		rtHeld_ = false; // 次に備えて解除
	}
}





