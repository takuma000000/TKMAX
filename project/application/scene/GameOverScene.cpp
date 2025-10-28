#include "GameOverScene.h"
#include "TextureManager.h"
#include "Input.h"
#include "SceneManager.h"
#include "engine/2d/SpriteCommon.h"
#include "engine/3d/Object3dCommon.h"
#include "application/scene/TitleScene.h"
#include <SkyBox.h>

// #include "TitleScene.h"
// #include "GameScene.h"

void GameOverScene::Initialize()
{
	ModelManager::GetInstance()->LoadModel("jett.obj", dxCommon_);
	TextureManager::GetInstance()->LoadTexture("./resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/over.png");

	// --- カメラ ---
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
	camera_->Update();

	// ライト
	dirLight_ = std::make_unique<DirectionalLight>();
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f,-1.0f,0.0f }, 1.0f);

	// パーティクル
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());

	// --- Player ---
	player_ = std::make_unique<Player>();
	player_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	player_->SetCamera(camera_.get());
	player_->SetPosition({ 0.0f, 0.0f, 0.0f }); // 初期位置
	player_->SetEnableJetSmoke(false); // ジェット噴射無効
	player_->SetPosition({ 0.0f, -1.6f, 5.0f });
	player_->SetRotation({ 0.25f, 0.0f, 1.35f });
	// 故障スポット（ローカル）：左右翼根元, 胴体下, 胴体横, 尾部 など
	faultLocal_ = {
		{  1.2f,  0.1f,  0.2f },  // 右翼根元
		{ -1.1f,  0.0f, -0.1f },  // 左翼根元
		{  0.2f, -0.6f,  0.0f },  // 胴体下
		{  0.0f,  0.2f, -1.0f },  // 胴体後方
		{ -0.3f,  0.4f,  1.1f },  // ノーズ側面
	};
	faultCD_.assign(faultLocal_.size(), 0.0f);
	faultNext_.resize(faultLocal_.size());
	for (auto& t : faultNext_) {
		t = 0.08f + (rand() % 60) / 1000.0f; // 0.08〜0.14s の初期待機
	}

	// --- skybox ---
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera_.get());

	// --- Iris（Title/GameScene と同一仕様）---
	iris_ = std::make_unique<Sprite>();
	iris_->Initialize(SpriteCommon::GetInstance(), dxCommon_, "./resources/circle2.png");
	iris_->SetAnchorPoint({ 0.5f, 0.5f });
	iris_->SetPosition({ WindowsAPI::kClientWidth * 0.5f, WindowsAPI::kClientHeight * 0.5f });

	// 画面対角から最大スケールを計算（対角×2.0f）
	const float diag = std::sqrt(
		float(WindowsAPI::kClientWidth) * float(WindowsAPI::kClientWidth) +
		float(WindowsAPI::kClientHeight) * float(WindowsAPI::kClientHeight)
	);
	irisMaxScale_ = diag * 2.0f;

	// 入場は「覆った状態 → 0」へ（OutBack, 0.8s）
	irisScale_ = irisMaxScale_;
	iris_->SetSize({ irisScale_, irisScale_ });
	irisOpenTween_.Reset(/*start*/ irisMaxScale_, /*end*/ 0.0f, /*sec*/ 0.8f, Ease::Type::OutBack);

	// --- 墜落用パーティクルグループ作成（circle.pngでOK） ---
	auto* PM = ParticleManager::GetInstance();
	PM->CreateParticleGroup("crashFlame", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	// 予備：火花（damageSpark）も使う
	PM->CreateParticleGroup("damageSpark", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	// --- 流星/降下ストリーク（縦に細長い線） ---
	PM->CreateParticleGroup("fallStreak", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);

	// スプライト生成
	overSprite_ = std::make_unique<Sprite>();
	overSprite_->Initialize(SpriteCommon::GetInstance(), dxCommon_, "./resources/over.png");
	overSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	overSprite_->SetPosition({ WindowsAPI::kClientWidth * 0.5f, WindowsAPI::kClientHeight * 0.5f });

	// アルファ0で開始（見えない状態）
	overSprite_->SetColor({ 1, 1, 1, 0 });

	// フェードイン（0→1, 0.7秒, OutQuad）& スケール（0.8→1.0, 0.7秒, OutBack）
	overAlphaTween_.Reset(0.0f, 1.0f, 0.7f, Ease::Type::OutQuad);
	overScaleTween_.Reset(0.8f, 1.0f, 0.7f, Ease::Type::OutBack);

	overActive_ = true;
}

void GameOverScene::Finalize()
{

}

void GameOverScene::Update()
{
	Input::GetInstance()->Update();

	if (player_) { player_->Update(); }
	if (camera_) { camera_->Update(); }
	if (dirLight_) { dirLight_->Update(); }
	ParticleManager::GetInstance()->Update();

	// ─── アイリス開き（入場） ───
	if (irisOpening_) {
		irisScale_ = irisOpenTween_.Update(0.016f);
		iris_->SetSize({ irisScale_, irisScale_ });
		iris_->Update();

		if (irisOpenTween_.Finished()) {
			irisOpening_ = false;
		}
	}
	// ─── Tキーでタイトルへ戻る（アイリス閉じ：InBack/0.8s） ───
	if (!irisClosing_ && Input::GetInstance()->TriggerKey(DIK_T)) {
		irisClosing_ = true;
		irisCloseTween_.Reset(/*start*/ 0.0f, /*end*/ irisMaxScale_, /*sec*/ 0.8f, Ease::Type::InBack);
	}

	if (irisClosing_) {
		float s = irisCloseTween_.Update(0.016f);
		iris_->SetSize({ s, s });
		iris_->Update();

		if (irisCloseTween_.Finished()) {
			sceneManager_->SetNextScene(new TitleScene(dxCommon_, srvManager_));
			return;
		}
	}

	// ─── スカイボックス回転 ───
	constexpr float kTwoPi = 6.2831853f;
	skyPitch_ -= skyRotSpeedX_;
	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;
	// Xだけ回す
	if (skybox_) {
		skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });
	}

	// --- 墜落中の失速スピン（常時回転）---
	if (tumbleActive_ && player_) {
		const float dt = 1.0f / 60.0f; // あなたのシーンは固定フレーム刻みでOK
		Vector3 r = player_->GetRotation();
		r.x += (tumbleSpeed_.x + ((rand() % 100 - 50) / 5000.0f)) * dt;
		r.y += tumbleSpeed_.y * dt;
		r.z += tumbleSpeed_.z * dt;
		player_->SetRotation(r);
	}

	// --- 常時：細い炎柱（複数点） ---
	{
		const int kSpotsPerFrame = 3;        // 毎フレ 2〜3点から
		const int kPerSpotCount = 3;        // 各点 2〜3粒（粒自体が太いので十分）
		const float radiusMin = 0.3f;        // 出火リングの内半径
		const float radiusMax = 1.1f;        // 出火リングの外半径

		Vector3 base = player_->GetPosition() + crashOffset_;
		for (int i = 0; i < kSpotsPerFrame; ++i) {
			float r = radiusMin + (rand() / float(RAND_MAX)) * (radiusMax - radiusMin);
			float th = (rand() / float(RAND_MAX)) * 6.2831853f; // 0..2π
			Vector3 spot = {
				base.x + r * cosf(th),
				base.y + ((rand() / float(RAND_MAX)) * 0.25f - 0.12f), // Yも微揺らし
				base.z + r * sinf(th)
			};
			ParticleManager::GetInstance()->Emit("crashFlame", spot, kPerSpotCount);
		}
	}

	// --- 故障スポットからの炎＆火花（中心固定をやめる） ---
	{
		// フレームごとの総量予算（重さ対策）
		int flameBudget = perFrameFlameBudget_;
		int sparkBudget = perFrameSparkBudget_;

		// 機体の回転を行列化（ローカル→ワールド）
		const Vector3 r = player_->GetRotation();
		Matrix4x4 R = MyMath::MakeRotateMatrix(r);
		const Vector3 basePos = player_->GetPosition();

		const float dt = 1.0f / 60.0f;
		for (size_t i = 0; i < faultLocal_.size(); ++i) {
			// クールダウン進行
			faultCD_[i] = std::max(0.0f, faultCD_[i] - dt);
			if (faultCD_[i] > 0.0f) continue;

			// ローカル点を回転→ワールドへ
			Vector3 w = MyMath::TransformNormal(faultLocal_[i], R) + basePos;

			// 炎と火花を小出し（“所々が壊れてる”見た目）
			int flameN = 2 + rand() % 2;  // 2〜3
			int sparkN = 1 + rand() % 2;  // 1〜2

			// 予算チェック（重ければスキップ）
			if (flameBudget > 0) {
				int n = std::min(flameN, flameBudget);
				ParticleManager::GetInstance()->Emit("crashFlame", w, n);
				flameBudget -= n;
			}
			if (sparkBudget > 0) {
				int n = std::min(sparkN, sparkBudget);
				ParticleManager::GetInstance()->Emit("damageSpark", w, n);
				sparkBudget -= n;
			}

			// 次回までの間隔をランダムに（0.06〜0.16s）
			faultCD_[i] = 0.06f + (rand() % 100) / 1000.0f;

			if (flameBudget <= 0 && sparkBudget <= 0) break; // そのフレームは打ち止め
		}
	}

	// --- たまに：ド派手バースト（炎＋火花） ---
	{
		const float dt = 1.0f / 60.0f;
		flameTimer_ += dt;
		if (flameTimer_ >= flameInterval_) {
			flameTimer_ = 0.0f;
			flameInterval_ = 0.35f + (rand() % 250) / 1000.0f; // 0.35〜0.60秒

			Vector3 base = player_->GetPosition() + crashOffset_;

			// バーストはリング上に6〜8点を一気に点火
			int burstSpots = 6 + rand() % 3; // 6〜8
			for (int i = 0; i < burstSpots; ++i) {
				float r = 0.25f + (rand() / float(RAND_MAX)) * 1.0f; // 0.25〜1.25
				float th = (2.0f * 3.1415926f / burstSpots) * i + (rand() / float(RAND_MAX)) * 0.5f;
				Vector3 p = {
					base.x + r * cosf(th),
					base.y + ((rand() / float(RAND_MAX)) * 0.3f - 0.15f),
					base.z + r * sinf(th)
				};

				// 炎 本体（派手に）
				ParticleManager::GetInstance()->Emit("crashFlame", p, 10);  // 1点10粒 × 6〜8点 = 60〜80粒

				// 火花を添える（ギラッと光を足す）
				ParticleManager::GetInstance()->Emit("damageSpark", p, 6);
			}
		}
	}

	// === GAME OVER 表示（フェードイン＋深紅の鼓動発光） ===
	if (overActive_ && overSprite_) {
		const float dt = 1.0f / 60.0f;
		overAlpha_ = overAlphaTween_.Update(dt);
		overScale_ = overScaleTween_.Update(dt);

		static float glowTimer = 0.0f;
		glowTimer += dt;

		// ── 鼓動テンポやや速め（3.8f）：心臓のようにドクドク動く
		float s = 0.5f + 0.5f * std::sin(glowTimer * 3.8f);
		float t01 = std::pow(s, 2.3f); // 明るい瞬間を鋭く（呼吸というより脈）

		// 深紅補間：ワインレッド→血の赤（R強ブースト、G少量、Bほぼ0）
		//   dark   : 黒と赤の中間（重く沈む）
		//   bright : 深紅〜血の赤（発光寄り）
		const Vector4 dark = { 0.35f, 0.00f, 0.00f, overAlpha_ }; // 黒寄りの赤
		const Vector4 bright = { 1.60f, 0.08f, 0.02f, overAlpha_ }; // 深紅（R1.6で強ブースト）

		Vector4 color = {
			dark.x + (bright.x - dark.x) * t01,
			dark.y + (bright.y - dark.y) * t01,
			dark.z + (bright.z - dark.z) * t01,
			overAlpha_
		};

		// 青殺し＋赤支配を確実に
		color.y *= 0.6f; // 緑をさらに抑える
		color.z *= 0.3f; // 青をほぼ潰す

		overSprite_->SetColor(color);

		// スケール反映
		const float baseW = 800.0f;
		const float baseH = 800.0f;
		overSprite_->SetSize({ baseW * overScale_, baseH * overScale_ });

		overSprite_->Update();
	}

	// === 画面上から下へ降るストリーク（流星風） ===
	{
		static int frameToggle = 0;
		frameToggle ^= 1;                  // 1フレームおきに生成（密度を下げる）
		if (frameToggle) { /* 今フレは生成しない */ } else {
			const int kSpawnPerFrame = 7;  // 12 → 7 に減らす（間隔を空ける）

			const Matrix4x4 camW = camera_->GetWorldMatrix();
			Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };
			Vector3 camRight = MyMath::Normalize({ camW.m[0][0], camW.m[0][1], camW.m[0][2] });
			Vector3 camUp = MyMath::Normalize({ camW.m[1][0], camW.m[1][1], camW.m[1][2] });
			Vector3 camFwd = MyMath::Normalize({ camW.m[2][0], camW.m[2][1], camW.m[2][2] });

			for (int i = 0; i < kSpawnPerFrame; ++i) {
				// 横幅を広げてバラけさせる（±26前後）
				float xSpread = ((rand() % 5200) - 2600) / 100.0f;  // 約 -26.0 ～ +26.0
				// 手前～やや奥までの帯
				float zDepth = 16.0f + (rand() % 1600) / 40.0f;    // 16 ～ 56
				// 画面上端よりさらに上から湧かせる（落ちてくる距離を確保）
				float yHeight = 10.0f + (rand() % 400) / 20.0f;     // 10 ～ 30

				Vector3 spawn = camPos + camRight * xSpread + camFwd * zDepth + camUp * yHeight;
				ParticleManager::GetInstance()->Emit("fallStreak", spawn, 1);
			}
		}
	}
}

void GameOverScene::Draw()
{
	// 3D
	Object3dCommon::GetInstance()->DrawSetCommon();
	if (player_) { player_->Draw(dxCommon_); }
	if (skybox_) { skybox_->Draw(); }

	// パーティクル描画
	ParticleManager::GetInstance()->Draw();

	// 2D（任意のオーバーレイ）
	SpriteCommon::GetInstance()->DrawSetCommon();
	if ((irisOpening_ || irisClosing_) && iris_) {
		iris_->Draw(); // 常に最前面
	}
	if (overSprite_) {
		overSprite_->Draw();
	}
}
