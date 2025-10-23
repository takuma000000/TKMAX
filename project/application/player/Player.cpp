#include "Player.h"
#include <engine/effect/particle/ParticleManager.h>

void Player::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	common_ = common; // Object3d共通
	dxCommon_ = dxCommon; // DirectX共通

	// 3Dオブジェクト作成
	object_ = std::make_unique<Object3d>();
	object_->Initialize(common_, dxCommon_);
	object_->SetModel("jett.obj");
	object_->SetEnvironment("./resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");

	TextureManager::GetInstance()->LoadTexture("./resources/circle.png"); // 煙用テクスチャ

	// パーティクルグループ作成
	ParticleManager::GetInstance()->CreateParticleGroup(
		"jetSmoke", "./resources/circle.png", ParticleManager::ParticleType::NORMAL); // 煙

	// パーティクルグループ作成
	ParticleManager::GetInstance()->CreateParticleGroup("trail_rb", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	ParticleManager::GetInstance()->CreateParticleGroup("trail_lb", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	ParticleManager::GetInstance()->CreateParticleGroup("trail_rt", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	ParticleManager::GetInstance()->CreateParticleGroup("trail_lt", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);

	// ---- ジェット煙エミッター初期化 ----
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

		bool hold = (input->GetRightTrigger() > 128) && (canUseSpecial_ || debugUnlimitedSpecial_);

		// ターゲットが切り替わったら前のロックを解除
		if (lastLockedEnemy_ && lastLockedEnemy_ != cur) {
			lastLockedEnemy_->SetLocked(false);
		}

		if (cur && hold) { // ロック中
			cur->SetLocked(true);
			lastLockedEnemy_ = cur;
		} else { // ロック解除
			if (cur) cur->SetLocked(false);
			lastLockedEnemy_ = nullptr;
		}
	}

	HandleShooting(); // 先にプレイヤーの操作より下に置くと自然

	for (auto it = bullets_.begin(); it != bullets_.end(); ) { // 弾更新と削除
		(*it)->Update();
		if ((*it)->IsDead()) { // 弾が死んでたら削除
			it = bullets_.erase(it);
		} else { // 生存してたら次へ
			++it;
		}
	}

	// ---- ジェット煙 ----
	Vector3 jetPos = object_->GetTranslate();
	jetPos.z -= 2.0f;  // 機体のケツあたり
	jetEmitter_.SetPosition(jetPos);  // 新しく追加する関数
	jetEmitter_.Update();             // 1フレームごとに放出チェック


	ParticleManager::GetInstance()->Update(); // パーティクルマネージャー更新

	object_->Update(); // プレイヤー本体更新
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
	ImGui::Checkbox("Unlimited RT (Debug)", &debugUnlimitedSpecial_);


	ImGui::End();
}

void Player::RemoveEnemyIfDead()
{
	if (enemy_ && enemy_->IsDead()) { // 敵が死んでたら参照をクリア
		enemy_ = nullptr;
	}
}


void Player::Draw(DirectXCommon* dxCommon) {
	object_->Draw(dxCommon); // プレイヤー本体描画

	for (auto& bullet : bullets_) {
		bullet->Draw(dxCommon); // 弾描画
	}
}

void Player::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos); // 位置設定
}

void Player::SetParentScene(BaseScene* scene) {
	parentScene_ = scene; // 親シーン設定
}

void Player::StartCameraShake(int frameCount) {
	cameraShakeFrame_ = frameCount; // シェイクフレーム数セット
}

void Player::HandleGamePadMove() {
	Input* input = Input::GetInstance(); // 入力取得
	const float moveSpeed = 0.25f; // 移動速度調整用
	const SHORT deadZone = 8000; // デッドゾーン

	SHORT lx = input->GetLeftStickX(); // 左スティックX
	SHORT ly = input->GetLeftStickY(); // 左スティックY
	float stickX = abs(lx) > deadZone ? (lx / 32768.0f) : 0.0f; // デッドゾーン処理
	float stickY = abs(ly) > deadZone ? (ly / 32768.0f) : 0.0f; // デッドゾーン処理

	Vector3 pos = object_->GetTranslate(); // 現在位置取得

	// XYのみ移動（Zはレール固定）
	pos.x += stickX * moveSpeed;
	pos.y -= -stickY * moveSpeed;
	pos.z = 0.0f; // レール固定

	// 範囲クランプ
	pos.x = std::clamp(pos.x, moveMin_.x, moveMax_.x);
	pos.y = std::clamp(pos.y, moveMin_.y, moveMax_.y);

	object_->SetTranslate(pos); // 位置設定

	// バンク角（ロール）をスティックに応じてスムージング
	float targetBank = -stickX * 0.35f; // 左で左に傾く
	// 簡易クリティックダンピング
	float k = 0.25f, d = 0.45f;
	bankVel_ += (targetBank - bankAngle_) * k - bankVel_ * d; // 速度更新
	bankAngle_ += bankVel_; // 角度更新
	Vector3 rot = object_->GetRotate(); // 現在回転取得
	rot.z = bankAngle_; // ロール
	object_->SetRotate(rot); // 回転設定
}


void Player::HandleCameraControl() {
	if (!camera) return;

	Input* input = Input::GetInstance();

	const float sensitivity = 0.02f; // 回転感度（調整してOK）
	const SHORT deadZone = 8000; // デッドゾーン

	// 右スティックの入力取得
	SHORT rx = input->GetRightStickX();
	SHORT ry = input->GetRightStickY();

	// デッドゾーン処理
	float rotX = abs(ry) > deadZone ? -(ry / 32768.0f) * sensitivity : 0.0f;
	float rotY = abs(rx) > deadZone ? (rx / 32768.0f) * sensitivity : 0.0f;

	// カメラ回転更新
	Vector3 rotation = camera->GetRotate();

	// 回転加算
	rotation.x += rotX;
	rotation.y += rotY;

	// X軸回転に制限をかける（真上向いたり真下向いたりしないように）
	const float limitX = 1.5f; // 約85度
	rotation.x = std::clamp(rotation.x, -limitX, limitX);

	camera->SetRotate(rotation); // カメラ回転設定
}

void Player::HandleFollowCamera() {
	if (!camera) return;

	Vector3 playerPos = object_->GetTranslate(); // プレイヤー位置取得
	Vector3 camRot = camera->GetRotate(); // カメラ回転取得
	float distance = 40.0f; // カメラとプレイヤーの距離
	float height = 4.0f; // カメラの高さ

	float angleY = camRot.y; // カメラのY軸回転（ラジアン）
	Vector3 offset = { // カメラのオフセット計算
		sinf(angleY) * -distance, // Xオフセット
		height, // Yオフセット（高さ固定）
		cosf(angleY) * -distance // Zオフセット
	};

	// --- シェイクオフセット加算 ---
	if (cameraShakeFrame_ > 0) { // シェイク中
		// ランダムなオフセットを生成
		cameraShakeOffset_.x = (rand() % 100 - 50) / 500.0f; 
		cameraShakeOffset_.y = (rand() % 100 - 50) / 500.0f;
		cameraShakeOffset_.z = (rand() % 100 - 50) / 500.0f;
		cameraShakeFrame_--;
	} else {
		cameraShakeOffset_ = { 0, 0, 0 }; // シェイク終了
	}

	Vector3 cameraPos = playerPos + offset + cameraShakeOffset_; // 最終的なカメラ位置
	camera->SetTranslate(cameraPos); // カメラ位置設定
}


void Player::HandleShooting() {
	Input* input = Input::GetInstance();

	// ▼ LT：完全追従弾（強ホーミング）
	bool ltPressed = (input->GetLeftTrigger() > 128);
	if (ltPressed && !ltHeld_) { // 押した瞬間だけ発射
		auto bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize(common_, dxCommon_);

		Vector3 startPos = object_->GetTranslate();
		bullet->SetPosition(startPos);


		bullet->SetEnemy(enemy_);               // ターゲット
		bullet->SetHoming(true, 0.6f);          // 完全追従ON（速度は好みで）
		bullet->SetCamera(camera); // カメラ設定
		bullet->SetPlayer(this); // プレイヤー設定

		// 初速は一応ターゲット方向、enemy_がいなければ前方
		if (enemy_ && !enemy_->IsDead()) { // ターゲットがいるなら
			Vector3 dir = enemy_->GetWorldPosition() - startPos;
			float len = MyMath::Length(dir);
			bullet->SetVelocity((len > 0.01f ? MyMath::Normalize(dir) : Vector3{ 0,0,1 }) * 0.6f);
		} else { // ターゲットがいないなら前方
			bullet->SetVelocity({ 0,0,0.6f });
		}

		// LT専用の軌道
		bullet->SetTrailGroup("trail_lt");

		bullets_.push_back(std::move(bullet)); // 弾リストに追加
	}
	ltHeld_ = ltPressed; // 離したら解放（次の押下で1発だけ出る）

	// ▼ RB：通常弾
	if (input->TriggerButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
		auto bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize(common_, dxCommon_);

		Vector3 startPos = object_->GetTranslate(); // 発射位置
		bullet->SetPosition(startPos); // 弾位置設定

		if (enemy_) { // ターゲットがいるならそっち向ける
			Vector3 enemyPos = enemy_->GetWorldPosition();
			Vector3 dir = enemyPos - startPos;
			float length = MyMath::Length(dir);

			if (length < 0.01f) { // 長さがほぼ0なら
				dir = { 0, 0, 1 };
			} else { // 正常な場合
				dir = MyMath::Normalize(dir);
			}
			bullet->SetVelocity(dir * 0.5f);
		} else { // ターゲットがいないなら前方
			bullet->SetVelocity({ 0, 0, 0.5f });
		}
	
		bullet->SetCamera(camera); // カメラ設定
		bullet->SetEnemy(enemy_); // ターゲット設定
		bullet->SetPlayer(this); // プレイヤー設定

		// RB専用の軌跡
		bullet->SetTrailGroup("trail_rb");

		bullets_.push_back(std::move(bullet)); // 弾リストに追加
	}

	// ▼ LB：全敵必中弾
	if (input->TriggerButton(XINPUT_GAMEPAD_LEFT_SHOULDER) && allEnemies_) {
		for (auto& enemy : *allEnemies_) { // 全敵ループ
			if (enemy->IsDead()) continue;
			// 敵ごとに弾を生成
			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize(common_, dxCommon_);
	
			Vector3 startPos = object_->GetTranslate(); // 発射位置
			Vector3 enemyPos = enemy->GetWorldPosition(); // 敵位置
			Vector3 dir = MyMath::Normalize(enemyPos - startPos); // 方向計算

			// 弾設定
			bullet->SetPosition(startPos); // 弾位置設定
			bullet->SetVelocity(dir * 0.5f); // 速度設定
			bullet->SetCamera(camera); // カメラ設定
			bullet->SetEnemy(enemy.get()); // 敵設定
			bullet->SetPlayer(this); // プレイヤー設定

			// LB専用の軌跡
			bullet->SetTrailGroup("trail_lb");

			bullets_.push_back(std::move(bullet)); // 弾リストに追加
		}
	}

	// RT：一撃必殺（最も近い敵に必中弾）
	const bool pressed = (input->GetRightTrigger() > 128);

	// 押している間：ホールド状態にする（発射はしない）
	if (pressed && (canUseSpecial_ || debugUnlimitedSpecial_) && enemy_ && !enemy_->IsDead()) {
		rtHeld_ = true; // ロックの見た目は Update() 側でON
	}

	// 離した瞬間：発射
	if (!pressed && rtHeld_) {
		if ((canUseSpecial_ || debugUnlimitedSpecial_) && enemy_ && !enemy_->IsDead()) {
			auto bullet = std::make_unique<PlayerBullet>();
			bullet->Initialize(common_, dxCommon_);

			Vector3 startPos = object_->GetTranslate(); // 発射位置
			Vector3 enemyPos = enemy_->GetWorldPosition(); // 敵位置
			Vector3 dir = MyMath::Normalize(enemyPos - startPos); // 方向計算

			// 弾設定
			bullet->SetPosition(startPos); // 弾位置設定
			bullet->SetVelocity(dir * 0.5f); // 速度設定
			bullet->SetCamera(camera); // カメラ設定
			bullet->SetEnemy(enemy_); // 敵設定
			bullet->SetPlayer(this); // プレイヤー設定
			bullet->SetSpecialAttack(true); // 一撃必殺フラグON

			// RT専用の軌跡
			bullet->SetTrailGroup("trail_rt");

			bullets_.push_back(std::move(bullet)); // 弾リストに追加

			// 見た目のロックは解除
			enemy_->SetLocked(false);
			if (!debugUnlimitedSpecial_) { // 一撃必殺使用済みにする
				canUseSpecial_ = false;
			}
		}
		rtHeld_ = false; // 次に備えて解除
	}
}





