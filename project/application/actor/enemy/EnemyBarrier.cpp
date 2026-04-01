#include "EnemyBarrier.h"
#include "Player.h"
#include "TextureManager.h"
#include "Model.h"
#include "LineRenderer.h"

void EnemyBarrier::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	barrierCommon_ = TKM::BarrierCommon::GetInstance();

	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon_);
	object_->SetModel("sphere.obj");

	if (camera_) {
		object_->SetCamera(camera_);
	}

	// ===== Material =====
	materialResource_ = dxCommon_->CreateBufferResource(sizeof(Vector4));
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	// ===== WVP =====
	wvpResource_ = dxCommon_->CreateBufferResource(sizeof(Matrix4x4));
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));

	UpdateVisual_();

	barrierShaderParamResource_ = dxCommon_->CreateBufferResource(sizeof(BarrierShaderParam));
	barrierShaderParamResource_->Map(0, nullptr, reinterpret_cast<void**>(&barrierShaderParamData_));

	barrierShaderParamData_->fresnelPower = shaderFresnelPower_;
	barrierShaderParamData_->baseStrength = shaderBaseStrength_;
	barrierShaderParamData_->rimStrength = shaderRimStrength_;
	barrierShaderParamData_->alphaBase = shaderAlphaBase_;
	barrierShaderParamData_->alphaRim = shaderAlphaRim_;
	barrierShaderParamData_->tint = shaderTint_;
	barrierShaderParamData_->hexScale = shaderHexScale_;
	barrierShaderParamData_->hexLineWidth = shaderHexLineWidth_;
	barrierShaderParamData_->hexGlowStrength = shaderHexGlowStrength_;
	barrierShaderParamData_->hexAlpha = shaderHexAlpha_;
	barrierShaderParamData_->breakProgress = 0.0f;
	barrierShaderParamData_->breakEdgeWidth = shaderBreakEdgeWidth_;
	barrierShaderParamData_->breakGlowStrength = shaderBreakGlowStrength_;
	barrierShaderParamData_->breakNoiseScale = shaderBreakNoiseScale_;
	barrierShaderParamData_->breakOrigin = shaderBreakOrigin_;
	barrierShaderParamData_->padding1 = 0.0f;
}

void EnemyBarrier::Update(float dt) {
	if (isBreaking_) {
		breakTimer_ += dt;
		float t = breakTimer_ / breakDuration_;
		if (t < 0.0f) { t = 0.0f; }
		if (t > 1.0f) { t = 1.0f; }

		shaderBreakProgress_ = t;

		if (breakTimer_ >= breakDuration_) {
			isBreaking_ = false;
			visible_ = false;
		}
	}

	// キランタイマー更新
	if (hitFlashTimer_ >= 0.0f) {
		hitFlashTimer_ += dt;
		if (hitFlashTimer_ > 0.3f) { // 0.3秒で消える
			hitFlashTimer_ = -1.0f;
		}
	}

	UpdateVisual_();
	SyncToPlayer();

#ifdef USE_IMGUI
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
	if ((!active_ && !isBreaking_) || !visible_ || !object_ || !barrierCommon_) {
		return;
	}

	auto* model = object_->GetModel();
	if (!model) {
		return;
	}

	barrierCommon_->DrawSetCommon();

	auto* cmd = dxCommon->GetCommandList();

	// Object3d 側の CBV をそのまま使う
	cmd->SetGraphicsRootConstantBufferView(0, object_->GetMaterialGPUVirtualAddress());
	cmd->SetGraphicsRootConstantBufferView(1, object_->GetWVPGPUVirtualAddress());

	// テクスチャ
	cmd->SetGraphicsRootDescriptorTable(
		2,
		TKM::TextureManager::GetInstance()->GetSrvHandleGPU(model->GetTexturePath())
	);

	// ライト類
	cmd->SetGraphicsRootConstantBufferView(3, object_->GetDirectionalLightGPUVirtualAddress());
	cmd->SetGraphicsRootConstantBufferView(4, object_->GetCameraGPUVirtualAddress());
	cmd->SetGraphicsRootConstantBufferView(5, object_->GetPointLightGPUVirtualAddress());
	cmd->SetGraphicsRootConstantBufferView(6, object_->GetSpotLightGPUVirtualAddress());
	cmd->SetGraphicsRootConstantBufferView(8, object_->GetEnvironmentGPUVirtualAddress());
	cmd->SetGraphicsRootConstantBufferView(9, barrierShaderParamResource_->GetGPUVirtualAddress());

	// Geometryだけ描く（Model 側の白マテリアル上書きなし）
	model->DrawWithoutMaterialOverride();
}

void EnemyBarrier::SetCamera(TKM::Camera* camera) {
	camera_ = camera;
	if (object_) {
		object_->SetCamera(camera_);
	}
}

void EnemyBarrier::SetPlayer(Player* player) {
	player_ = player;
}

void EnemyBarrier::SetActive(bool active) {
	active_ = active;

	// 無効になった時はPlayer側のヒット情報も整理させる
	SyncToPlayer();
}

void EnemyBarrier::SetCenter(const Vector3& center) {
	center_ = center;
	UpdateVisual_();
}

void EnemyBarrier::SetRadius(float radius) {
	radius_ = radius;
	UpdateVisual_();
}

void EnemyBarrier::SetColor(const Vector4& color) {
	color_ = color;
	UpdateVisual_();
}

void EnemyBarrier::SetShapeScale(const Vector3& shapeScale) {
	shapeScale_ = shapeScale;
	UpdateVisual_();
}

Vector3 EnemyBarrier::GetAABBSize() const {
	return {
		radius_ * 2.0f * shapeScale_.x,
		radius_ * 2.0f * shapeScale_.y,
		radius_ * 2.0f * shapeScale_.z
	};
}

Vector3 EnemyBarrier::GetEllipsoidRadius() const {
	return {
		radius_ * shapeScale_.x,
		radius_ * shapeScale_.y,
		radius_ * shapeScale_.z
	};
}

void EnemyBarrier::SyncToPlayer() {
	if (!player_) {
		return;
	}

	player_->SetWave1BarrierInfo(
		active_,
		center_,
		GetEllipsoidRadius()
	);
}

void EnemyBarrier::OnHit(const Vector3& pos) {
	hitFlashTimer_ = 0.0f;
	hitFlashPos_ = pos;
}

void EnemyBarrier::StartBreak() {
	active_ = false;
	isBreaking_ = true;
	breakTimer_ = 0.0f;
	shaderBreakProgress_ = 0.0f;
	shaderBreakOrigin_ = center_;
	visible_ = true;

	shaderBreakEdgeWidth_ = 0.035f;
	shaderBreakGlowStrength_ = 3.3f;
	shaderBreakNoiseScale_ = 16.0f;

	SyncToPlayer();
}

void EnemyBarrier::UpdateVisual_() {
	if (!object_) {
		return;
	}

	object_->SetTranslate(center_);
	object_->SetScale({
		radius_ * shapeScale_.x,
		radius_ * shapeScale_.y,
		radius_ * shapeScale_.z
		});
	object_->SetColor(color_);
	object_->Update();

	if (barrierShaderParamData_) {
		barrierShaderParamData_->fresnelPower = shaderFresnelPower_;
		barrierShaderParamData_->baseStrength = shaderBaseStrength_;
		barrierShaderParamData_->rimStrength = shaderRimStrength_;
		barrierShaderParamData_->alphaBase = shaderAlphaBase_;
		barrierShaderParamData_->alphaRim = shaderAlphaRim_;
		barrierShaderParamData_->tint = shaderTint_;
		barrierShaderParamData_->hexScale = shaderHexScale_;
		barrierShaderParamData_->hexLineWidth = shaderHexLineWidth_;
		barrierShaderParamData_->hexGlowStrength = shaderHexGlowStrength_;
		barrierShaderParamData_->hexAlpha = shaderHexAlpha_;
		barrierShaderParamData_->breakProgress = shaderBreakProgress_;
		barrierShaderParamData_->breakEdgeWidth = shaderBreakEdgeWidth_;
		barrierShaderParamData_->breakGlowStrength = shaderBreakGlowStrength_;
		barrierShaderParamData_->breakNoiseScale = shaderBreakNoiseScale_;
		barrierShaderParamData_->breakOrigin = shaderBreakOrigin_;
		barrierShaderParamData_->hitFlashTime = hitFlashTimer_;
		barrierShaderParamData_->hitFlashPos = hitFlashPos_;
	}
}