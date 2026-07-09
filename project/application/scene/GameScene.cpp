#include "GameScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include <algorithm>
#include "HintLogExporter.h"
#include "HintPromptBuilder.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void GameScene::Initialize() {
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/enemy_typeB.png");
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/goal.png");

	player_ = std::make_unique<Player>();
	player_->Initialize(dxCommon_);

	roads_.clear();

	const int correctIndex = 2; // 3番目が当たり
	// 5択の択を生成
	for (int i = 0; i < 5; i++) {
		auto road = std::make_unique<Road>();
		Vector2 pos = {
			150.0f + i * 200.0f,
			500.0f
		};
		bool isCorrect = (i == correctIndex);
		road->Initialize(dxCommon_, pos, isCorrect);
		roads_.push_back(std::move(road));
	}
}

void GameScene::Finalize() {

}

void GameScene::Update() {
	TKM::Input::GetInstance()->Update();

	player_->Update();

	hitRoadIndex_ = -1;

	for (int i = 0; i < roads_.size(); i++) {
		bool isHit = CheckAABB(
			player_->GetPosition(),
			player_->GetSize(),
			roads_[i]->GetPosition(),
			roads_[i]->GetSize()
		);

		roads_[i]->SetHit(isHit);

		if (isHit) {
			hitRoadIndex_ = i;
		}

		roads_[i]->Update();
	}

	if (hitRoadIndex_ >= 0 && TKM::Input::GetInstance()->TriggerKey(DIK_E)) {
		selectedRoadIndex_ = hitRoadIndex_;
		isSelectedCorrect_ = roads_[hitRoadIndex_]->IsCorrect();

		hintLog_.AddSelectLog(selectedRoadIndex_, isSelectedCorrect_);

		HintLogExporter::SaveJson(
			hintLog_,
			"./resources/data/hint_log.json"
		);

		std::string json = hintLog_.MakeJson();

		hintClient_.Send(json);
	}

#ifdef USE_IMGUI
	ImGui::Begin("ヒントログ");

	if (hitRoadIndex_ >= 0) {
		ImGui::Text("現在は ''択%d'' の上にいます", hitRoadIndex_ + 1);
	} else {
		ImGui::Text("どの択にも触れていません");
	}

	ImGui::Separator();

	for (int i = 0; i < roads_.size(); i++) {
		ImGui::Text(
			"択%d : %s",
			i + 1,
			roads_[i]->IsHit() ? "〇" : "×"
		);
	}

	ImGui::Separator();

	if (selectedRoadIndex_ >= 0) {
		ImGui::Text("最後に選択した択 : %d", selectedRoadIndex_ + 1);
		ImGui::Text("結果 : %s", isSelectedCorrect_ ? "当たり" : "はずれ");
	} else {
		ImGui::Text("まだ選択していません");
	}

	ImGui::Separator();

	ImGui::Text("失敗回数 : %d", hintLog_.GetMissCount());
	ImGui::Text("成功回数 : %d", hintLog_.GetCorrectCount());
	ImGui::Text("総回数 : %d", hintLog_.GetTotalSelectCount());

	ImGui::Separator();

	std::string json = hintLog_.MakeJson();
	ImGui::TextWrapped("%s", json.c_str());

	ImGui::Separator();

	if (ImGui::CollapsingHeader("AIプロンプト")) {
		std::string prompt = HintPromptBuilder::BuildPrompt(hintLog_);
		ImGui::TextWrapped("%s", prompt.c_str());
	}

	ImGui::Separator();

	ImGui::Text("バックエンドからのヒント:");
	ImGui::TextWrapped("%s", hintClient_.GetLatestHint().c_str());

	ImGui::End();
#endif
}

void GameScene::Draw() {
	DrawSprite();
	Draw3D();
}

void GameScene::Draw3D() {}

void GameScene::DrawSprite() {
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	for (auto& road : roads_) {
		road->Draw();
	}


	player_->Draw();
}

void GameScene::DrawBack() {}

bool GameScene::CheckAABB(
	const Vector2& posA,
	const Vector2& sizeA,
	const Vector2& posB,
	const Vector2& sizeB
) {
	return
		posA.x < posB.x + sizeB.x &&
		posA.x + sizeA.x > posB.x &&
		posA.y < posB.y + sizeB.y &&
		posA.y + sizeA.y > posB.y;
}