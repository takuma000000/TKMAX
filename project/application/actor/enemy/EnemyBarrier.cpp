#include "EnemyBarrier.h"
#include "Player.h"
#include "TextureManager.h"
#include "Model.h"
#include "LineRenderer.h"

void EnemyBarrier::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	// DirectX共通の参照を保持する
	dxCommon_ = dxCommon;

	// バリア描画共通クラスのインスタンスを取得する
	barrierCommon_ = TKM::BarrierCommon::GetInstance();

	// バリア本体の3Dオブジェクトを生成する
	object_ = std::make_unique<TKM::Object3d>();

	// バリア本体の描画に必要な情報を渡して初期化する
	object_->Initialize(common, dxCommon_);

	// バリアの見た目として球モデルを設定する
	object_->SetModel("sphere.obj");

	// カメラが設定済みならバリア本体にも適用する
	if (camera_) {
		object_->SetCamera(camera_);
	}

	//=========================================================
	// Material用定数バッファの作成
	//=========================================================

	// マテリアル情報を書き込むためのバッファを作成する
	materialResource_ = dxCommon_->CreateBufferResource(sizeof(Vector4));

	// CPU側から書き込めるようにマップする
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	//=========================================================
	// WVP用定数バッファの作成
	//=========================================================

	// ワールド・ビュー・プロジェクション用バッファを作成する
	wvpResource_ = dxCommon_->CreateBufferResource(sizeof(Matrix4x4));

	// CPU側から書き込めるようにマップする
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));

	// 初期位置・初期スケール・初期色などの見た目を反映する
	UpdateVisual_();

	//=========================================================
	// バリア専用シェーダーパラメータ用定数バッファの作成
	//=========================================================

	// バリアシェーダー用の各種パラメータを書き込むバッファを作成する
	barrierShaderParamResource_ = dxCommon_->CreateBufferResource(sizeof(BarrierShaderParam));

	// CPU側から書き込めるようにマップする
	barrierShaderParamResource_->Map(0, nullptr, reinterpret_cast<void**>(&barrierShaderParamData_));

	// シェーダーへ渡すフレネル強度を初期化する
	barrierShaderParamData_->fresnelPower = shaderFresnelPower_;

	// ベース発光強度を初期化する
	barrierShaderParamData_->baseStrength = shaderBaseStrength_;

	// リム発光強度を初期化する
	barrierShaderParamData_->rimStrength = shaderRimStrength_;

	// ベースアルファを初期化する
	barrierShaderParamData_->alphaBase = shaderAlphaBase_;

	// リムアルファを初期化する
	barrierShaderParamData_->alphaRim = shaderAlphaRim_;

	// 色味を初期化する
	barrierShaderParamData_->tint = shaderTint_;

	// 六角模様のスケールを初期化する
	barrierShaderParamData_->hexScale = shaderHexScale_;

	// 六角模様の線幅を初期化する
	barrierShaderParamData_->hexLineWidth = shaderHexLineWidth_;

	// 六角模様の発光強度を初期化する
	barrierShaderParamData_->hexGlowStrength = shaderHexGlowStrength_;

	// 六角模様のアルファを初期化する
	barrierShaderParamData_->hexAlpha = shaderHexAlpha_;

	// 破壊進行度は初期状態では0にする
	barrierShaderParamData_->breakProgress = 0.0f;

	// 破壊エッジ幅を初期化する
	barrierShaderParamData_->breakEdgeWidth = shaderBreakEdgeWidth_;

	// 破壊時の発光強度を初期化する
	barrierShaderParamData_->breakGlowStrength = shaderBreakGlowStrength_;

	// 破壊ノイズスケールを初期化する
	barrierShaderParamData_->breakNoiseScale = shaderBreakNoiseScale_;

	// 破壊開始原点を初期化する
	barrierShaderParamData_->breakOrigin = shaderBreakOrigin_;

	// パディングを初期化する
	barrierShaderParamData_->padding1 = 0.0f;
}

void EnemyBarrier::Update(float dt) {
	//=========================================================
	// 破壊演出中の更新
	//=========================================================
	if (isBreaking_) {
		// 破壊演出タイマーを進める
		breakTimer_ += dt;

		// 破壊進行度を0.0～1.0で計算する
		float t = breakTimer_ / breakDuration_;
		if (t < 0.0f) { t = 0.0f; }
		if (t > 1.0f) { t = 1.0f; }

		// シェーダーへ渡す破壊進行度を更新する
		shaderBreakProgress_ = t;

		// 破壊演出が終わったら非表示にする
		if (breakTimer_ >= breakDuration_) {
			isBreaking_ = false;
			visible_ = false;
		}
	}

	//=========================================================
	// 被弾フラッシュ（キラン）の更新
	//=========================================================

	// フラッシュ中なら経過時間を進める
	if (hitFlashTimer_ >= 0.0f) {
		hitFlashTimer_ += dt;

		// 一定時間経過でフラッシュを終了する
		if (hitFlashTimer_ > 0.3f) {
			hitFlashTimer_ = -1.0f;
		}
	}

	// 現在のTransformやシェーダーパラメータを見た目へ反映する
	UpdateVisual_();

	// プレイヤー側へバリアの有効状態とサイズ情報を同期する
	SyncToPlayer();

#ifdef USE_IMGUI
	//=========================================================
	// デバッグ用の当たり判定可視化
	//=========================================================
	if ((active_ || isBreaking_) && visible_) {
		TKM::LineRenderer::GetInstance()->AddEllipsoid(
			center_,
			{
				radius_ * shapeScale_.x,
				radius_ * shapeScale_.y,
				radius_ * shapeScale_.z
			},
			TKM::LineRenderer::Color{ 0.0f, 0.0f, 1.0f, 1.0f },
			32
		);
	}
#endif
}

void EnemyBarrier::Draw(TKM::DirectXCommon* dxCommon) {
	//=========================================================
	// 描画条件チェック
	//=========================================================

	// 無効かつ破壊中でもない、または非表示、または必要オブジェクトが無ければ描画しない
	if ((!active_ && !isBreaking_) || !visible_ || !object_ || !barrierCommon_) {
		return;
	}

	// モデル取得に失敗したら描画しない
	auto* model = object_->GetModel();
	if (!model) {
		return;
	}

	// バリア描画用の共通設定を適用する
	barrierCommon_->DrawSetCommon();

	// コマンドリストを取得する
	auto* cmd = dxCommon->GetCommandList();

	//=========================================================
	// Object3d側の定数バッファを設定
	//=========================================================

	// マテリアルCBVを設定する
	cmd->SetGraphicsRootConstantBufferView(0, object_->GetMaterialGPUVirtualAddress());

	// WVP行列CBVを設定する
	cmd->SetGraphicsRootConstantBufferView(1, object_->GetWVPGPUVirtualAddress());

	//=========================================================
	// テクスチャ設定
	//=========================================================

	// モデルが持つテクスチャを設定する
	cmd->SetGraphicsRootDescriptorTable(
		2,
		TKM::TextureManager::GetInstance()->GetSrvHandleGPU(model->GetTexturePath())
	);

	//=========================================================
	// ライト・カメラ・環境情報の設定
	//=========================================================

	// 平行光源を設定する
	cmd->SetGraphicsRootConstantBufferView(3, object_->GetDirectionalLightGPUVirtualAddress());

	// カメラ情報を設定する
	cmd->SetGraphicsRootConstantBufferView(4, object_->GetCameraGPUVirtualAddress());

	// 点光源を設定する
	cmd->SetGraphicsRootConstantBufferView(5, object_->GetPointLightGPUVirtualAddress());

	// スポットライトを設定する
	cmd->SetGraphicsRootConstantBufferView(6, object_->GetSpotLightGPUVirtualAddress());

	// 環境情報を設定する
	cmd->SetGraphicsRootConstantBufferView(8, object_->GetEnvironmentGPUVirtualAddress());

	// バリア専用シェーダーパラメータを設定する
	cmd->SetGraphicsRootConstantBufferView(9, barrierShaderParamResource_->GetGPUVirtualAddress());

	// モデル側のマテリアル上書きなしでジオメトリだけ描画する
	model->DrawWithoutMaterialOverride();
}

//=============================================================
// 設定適用
//=============================================================
void EnemyBarrier::ApplyConfig(const BarrierConfig::Barrier& config) {
	//=========================================================
	// バリア本体設定
	// JSONで管理している半径・形状スケール・色を反映する。
	//=========================================================
	radius_ = config.radius_;
	shapeScale_ = config.shapeScale_;
	color_ = config.color_;

	//=========================================================
	// 破壊演出設定
	// バリアが割れて消えるまでの時間をJSONから反映する。
	//=========================================================
	breakDuration_ = config.breakDuration_;

	//=========================================================
	// シェーダー設定
	// 見た目に関わる値はコード固定にせず、JSONのshader項目から反映する。
	//=========================================================
	shaderFresnelPower_ = config.shader_.fresnelPower_;
	shaderBaseStrength_ = config.shader_.baseStrength_;
	shaderRimStrength_ = config.shader_.rimStrength_;
	shaderAlphaBase_ = config.shader_.alphaBase_;
	shaderAlphaRim_ = config.shader_.alphaRim_;

	shaderTint_ = config.shader_.tint_;

	shaderHexScale_ = config.shader_.hexScale_;
	shaderHexLineWidth_ = config.shader_.hexLineWidth_;
	shaderHexGlowStrength_ = config.shader_.hexGlowStrength_;
	shaderHexAlpha_ = config.shader_.hexAlpha_;

	shaderBreakEdgeWidth_ = config.shader_.breakEdgeWidth_;
	shaderBreakGlowStrength_ = config.shader_.breakGlowStrength_;
	shaderBreakNoiseScale_ = config.shader_.breakNoiseScale_;
	shaderBreakOrigin_ = config.shader_.breakOrigin_;

	//=========================================================
	// 見た目反映
	// 反映後すぐ描画・シェーダー定数バッファへ流す。
	//=========================================================
	UpdateVisual_();
}

void EnemyBarrier::SetCamera(TKM::Camera* camera) {
	// カメラ参照を保持する
	camera_ = camera;

	// オブジェクトが存在するなら現在のカメラを反映する
	if (object_) {
		object_->SetCamera(camera_);
	}
}

void EnemyBarrier::SetPlayer(Player* player) {
	// プレイヤー参照を保持する
	player_ = player;
}

void EnemyBarrier::SetActive(bool active) {
	// バリアの有効状態を更新する
	active_ = active;

	// 無効化時も含めて、プレイヤー側へ現在の状態を同期する
	SyncToPlayer();
}

void EnemyBarrier::SetCenter(const Vector3& center) {
	// バリア中心位置を更新する
	center_ = center;

	// 見た目へ反映する
	UpdateVisual_();
}

void EnemyBarrier::SetRadius(float radius) {
	// バリア半径を更新する
	radius_ = radius;

	// 見た目へ反映する
	UpdateVisual_();
}

void EnemyBarrier::SetColor(const Vector4& color) {
	// バリア色を更新する
	color_ = color;

	// 見た目へ反映する
	UpdateVisual_();
}

void EnemyBarrier::SetShapeScale(const Vector3& shapeScale) {
	// 軸ごとの形状スケールを更新する
	shapeScale_ = shapeScale;

	// 見た目へ反映する
	UpdateVisual_();
}

Vector3 EnemyBarrier::GetAABBSize() const {
	// AABB用サイズを直径ベースで返す
	return {
		radius_ * 2.0f * shapeScale_.x,
		radius_ * 2.0f * shapeScale_.y,
		radius_ * 2.0f * shapeScale_.z
	};
}

Vector3 EnemyBarrier::GetEllipsoidRadius() const {
	// 楕円体当たり判定用の半径を各軸ごとに返す
	return {
		radius_ * shapeScale_.x,
		radius_ * shapeScale_.y,
		radius_ * shapeScale_.z
	};
}

void EnemyBarrier::SyncToPlayer() {
	// プレイヤーが未設定なら何もしない
	if (!player_) {
		return;
	}

	// プレイヤー側へバリアの有効状態・中心位置・半径を渡す
	player_->SetWave1BarrierInfo(
		active_,
		center_,
		GetEllipsoidRadius()
	);
}

void EnemyBarrier::OnHit(const Vector3& pos) {
	// 被弾フラッシュを開始する
	hitFlashTimer_ = 0.0f;

	// 被弾位置を記録する
	hitFlashPos_ = pos;
}

void EnemyBarrier::StartBreak() {
	// 通常の当たり判定状態は無効にする
	active_ = false;

	// 破壊演出状態に入る
	isBreaking_ = true;

	// 破壊タイマーをリセットする
	breakTimer_ = 0.0f;

	// 破壊進行度を初期化する
	shaderBreakProgress_ = 0.0f;

	// 破壊開始位置を現在中心にする
	shaderBreakOrigin_ = center_;

	// 破壊演出中は表示を維持する
	visible_ = true;

	// プレイヤー側へ現在の無効状態を同期する
	SyncToPlayer();
}

void EnemyBarrier::UpdateVisual_() {
	// オブジェクト未生成なら更新しない
	if (!object_) {
		return;
	}

	//=========================================================
	// Transform更新
	//=========================================================

	// 現在の中心位置を反映する
	object_->SetTranslate(center_);

	// 半径と形状スケールから見た目サイズを反映する
	object_->SetScale({
		radius_ * shapeScale_.x,
		radius_ * shapeScale_.y,
		radius_ * shapeScale_.z
		});

	// 現在色を反映する
	object_->SetColor(color_);

	// オブジェクトのTransform更新を反映する
	object_->Update();

	//=========================================================
	// シェーダーパラメータ更新
	//=========================================================
	if (barrierShaderParamData_) {
		// フレネル強度を反映する
		barrierShaderParamData_->fresnelPower = shaderFresnelPower_;

		// ベース発光強度を反映する
		barrierShaderParamData_->baseStrength = shaderBaseStrength_;

		// リム発光強度を反映する
		barrierShaderParamData_->rimStrength = shaderRimStrength_;

		// ベースアルファを反映する
		barrierShaderParamData_->alphaBase = shaderAlphaBase_;

		// リムアルファを反映する
		barrierShaderParamData_->alphaRim = shaderAlphaRim_;

		// 色味を反映する
		barrierShaderParamData_->tint = shaderTint_;

		// 六角模様スケールを反映する
		barrierShaderParamData_->hexScale = shaderHexScale_;

		// 六角模様の線幅を反映する
		barrierShaderParamData_->hexLineWidth = shaderHexLineWidth_;

		// 六角模様の発光強度を反映する
		barrierShaderParamData_->hexGlowStrength = shaderHexGlowStrength_;

		// 六角模様アルファを反映する
		barrierShaderParamData_->hexAlpha = shaderHexAlpha_;

		// 破壊進行度を反映する
		barrierShaderParamData_->breakProgress = shaderBreakProgress_;

		// 破壊エッジ幅を反映する
		barrierShaderParamData_->breakEdgeWidth = shaderBreakEdgeWidth_;

		// 破壊発光強度を反映する
		barrierShaderParamData_->breakGlowStrength = shaderBreakGlowStrength_;

		// 破壊ノイズスケールを反映する
		barrierShaderParamData_->breakNoiseScale = shaderBreakNoiseScale_;

		// 破壊開始原点を反映する
		barrierShaderParamData_->breakOrigin = shaderBreakOrigin_;

		// 被弾フラッシュ時間を反映する
		barrierShaderParamData_->hitFlashTime = hitFlashTimer_;

		// 被弾フラッシュ位置を反映する
		barrierShaderParamData_->hitFlashPos = hitFlashPos_;
	}
}