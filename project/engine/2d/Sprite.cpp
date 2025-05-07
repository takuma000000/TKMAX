#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "MyMath.h"
#include "TextureManager.h"
#include "externals/imgui/imgui.h"
#include "BaseScene.h"

Sprite::Sprite()
{
	++activeCount_;
}

Sprite::~Sprite()
{
	--activeCount_;
}

void Sprite::SetParentScene(BaseScene* parentScene)
{
	parentScene_ = parentScene;
}

void Sprite::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, std::string textureFilePath) {
	//引数で受け取ったメンバ変数に記録する
	this->spriteCommon = spriteCommon;
	dxCommon_ = dxCommon;
	this->textureFilePath = textureFilePath;

	//VertexResourceを作る
	vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * 4);
	//IndexResourceを作る
	indexResource = dxCommon_->CreateBufferResource(sizeof(uint32_t) * 6);
	//VertexBufferViewを作成する
	//Resourceの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//使用するResourceのサイズは頂点4つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 4;
	//1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);
	//リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	//使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	//インデックスはuint32_tとする
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	//
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	//マテリアルリソースを作る
	materialResource = dxCommon->CreateBufferResource(sizeof(Material));
	//書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//色は白に設定
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//SprightはLightingしないのでfalseを設定する
	materialData->enableLighting = false;
	materialData->uvTransform = MyMath::MakeIdentity4x4();

	//座標変換行列リソースを作る
	transformationMatrixResource = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	//書き込むためのアドレスを取得
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	//単位行列を書き込んでおく
	transformationMatrixData->wvp = MyMath::MakeIdentity4x4();
	transformationMatrixData->World = MyMath::MakeIdentity4x4();

	transformSprite = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	cameraTransform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,-10.0f} };

	//単位行列を書き込んでおく
	textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
}

void Sprite::Update() {

	float left = 0.0f - anchorPoint.x;
	float right = 1.0f - anchorPoint.x;
	float top = 0.0f - anchorPoint.y;
	float bottom = 1.0f - anchorPoint.y;

	//左右反転
	if (isFlipX_) {
		left = -left;
		right = -right;
	}
	//上下反転
	if (isFlipY_) {
		top = -top;
		bottom = -bottom;
	}

	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetadata(textureFilePath);
	float tex_left = textureLeftTop.x / metadata.width;
	float tex_right = (textureLeftTop.x + textureSize.x) / metadata.width;
	float tex_top = textureLeftTop.y / metadata.height;
	float tex_bottom = (textureLeftTop.y + textureSize.y) / metadata.height;

	//左下
	vertexData[0].position = { left,bottom,0.0f,1.0f };
	vertexData[0].texcoord = { tex_left,tex_bottom };
	vertexData[0].normal = { 0.0f,0.0f,-1.0f };
	//左上
	vertexData[1].position = { left,top,0.0f,1.0f };
	vertexData[1].texcoord = { tex_left,tex_top };
	vertexData[1].normal = { 0.0f,0.0f,-1.0f };
	//右下
	vertexData[2].position = { right,bottom,0.0f,1.0f };
	vertexData[2].texcoord = { tex_right,tex_bottom };
	vertexData[2].normal = { 0.0f,0.0f,-1.0f };
	//右上
	vertexData[3].position = { right,top,0.0f,1.0f };
	vertexData[3].texcoord = { tex_right,tex_top };
	vertexData[3].normal = { 0.0f,0.0f,-1.0f };

	//
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 1;
	indexData[4] = 3;
	indexData[5] = 2;

	Matrix4x4 worldMatrixSprite = MyMath::MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
	Matrix4x4 viewMatrixSprite = MyMath::MakeIdentity4x4();
	Matrix4x4 projectionMatrixSprite = MyMath::MakeOrthographicMatrix(0.0f, 0.0f, float(WindowsAPI::kClientWidth), float(WindowsAPI::kClientHeight), 0.0f, 100.0f);

	transformationMatrixData->wvp = MyMath::Multiply(worldMatrixSprite, MyMath::Multiply(viewMatrixSprite, projectionMatrixSprite));
	transformationMatrixData->World = worldMatrixSprite;

	//反映処理
	transformSprite.translate = { position.x,position.y,0.0f };
	transformSprite.rotate = { 0.0f,0.0f,rotation };
	transformSprite.scale = { size.x,size.y,1.0f };

	//テクスチャサイズをイメージに合わせる
	AdjustTextureSize();
}

void Sprite::Draw() {

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->GetCurrentRTVHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDSVHandle();
	dxCommon_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	if (parentScene_) {
		parentScene_->AddDrawCallCount();
	}

	//VertexBufferViewを設定
	//Spriteの描画。変更が必要なものだけ変更する
	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);//VBVを設定
	//TransformationMatrixCBufferの場所を設定
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
	//描画
	//dxCommon_->GetCommandList()->DrawInstanced(6, 1, 0, 0);

	//IndexBufferViewを設定
	//IBVを設定
	dxCommon_->GetCommandList()->IASetIndexBuffer(&indexBufferView);

	//マテリアルCBufferの場所を設定
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	//座標変換行列CBufferの場所を設定
	Transform uvTransformSprite{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
	};

	Matrix4x4 uvTransformMatrix = MyMath::MakeScaleMatrix(uvTransformSprite.scale);
	uvTransformMatrix = MyMath::Multiply(uvTransformMatrix, MyMath::MakeRotateZMatrix(uvTransformSprite.rotate.z));
	uvTransformMatrix = MyMath::Multiply(uvTransformMatrix, MyMath::MakeTranslateMatrix(uvTransformSprite.translate));
	materialData->uvTransform = uvTransformMatrix;

	//SRVのDescriptorTableの先頭を設定
	// 修正後: インデックス指定でSRV取得（こっちにする）
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex));
	//描画。6個のインデックスを使用し1つのインスタンスを描画。その他は当面0で良い
	dxCommon_->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::ImGuiDebug() {

	// ImGui ウィジェット: スプライトの座標操作
	ImGui::Begin("Sprite");

	// スプライトの座標を操作するスライダー
	ImGui::SliderFloat2("Sprite Position", &position.x, 0.0f, 500.0f, "%.1f");

	ImGui::End();
}

void Sprite::AdjustTextureSize()
{
	//テクスチャデータを取得
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetadata(textureFilePath);

	textureSize.x = static_cast<float>(metadata.width);
	textureSize.y = static_cast<float>(metadata.height);
	//画像サイズをテクスチャサイズに合わせる
	size = textureSize;
}
