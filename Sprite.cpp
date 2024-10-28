#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"

struct Matrix4x4 {
	float m[4][4];
};

Matrix4x4 MakeIdentity4x4() {

	Matrix4x4 identity;

	// 単位行列を生成する
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			identity.m[i][j] = (i == j) ? 1.0f : 0.0f;
		}
	}

	return identity;

}

void Sprite::Initialize(SpriteCommon* spriteCommon,DirectXCommon* dxCommon){
	//引数で受け取ったメンバ変数に記録する
	this->spriteCommon = spriteCommon;
	dxCommon_ = dxCommon;

	//VertexResourceを作る
	vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * 6);
	//IndexResourceを作る
	indexResource = dxCommon_->CreateBufferResource(sizeof(uint32_t) * 6);
	//VertexBufferViewを作成する
	//Resourceの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//使用するResourceのサイズは頂点6つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;
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
	//左下
	vertexData[0].position = { 0.0f,360.0f,0.0f,1.0f };
	vertexData[0].texcoord = { 0.0f,1.0f };
	vertexData[0].normal = { 0.0f,0.0f,-1.0f };
	//左上
	vertexData[1].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexData[1].texcoord = { 0.0f,0.0f };
	vertexData[1].normal = { 0.0f,0.0f,-1.0f };
	//右上
	vertexData[2].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexData[2].texcoord = { 1.0f,1.0f };
	vertexData[2].normal = { 0.0f,0.0f,-1.0f };
	//2枚目の三角形
	//左上
	vertexData[3].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexData[3].texcoord = { 0.0f,0.0f };
	vertexData[3].normal = { 0.0f,0.0f,-1.0f };
	//右上
	vertexData[4].position = { 640.0f,0.0f,0.0f,1.0f };
	vertexData[4].texcoord = { 1.0f,0.0f };
	vertexData[4].normal = { 0.0f,0.0f,-1.0f };
	//右下
	vertexData[5].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexData[5].texcoord = { 1.0f,1.0f };
	vertexData[5].normal = { 0.0f,0.0f,-1.0f };
	//
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 1;
	indexData[4] = 3;
	indexData[5] = 2;
	//マテリアうリソースを作る
	materialResource = dxCommon_->CreateBufferResource(sizeof(Material));
	//書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//今回は白を書き込んでみる
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = false;
	materialData->uvTransform = MakeIdentity4x4();
}
