#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"

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
	//
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 1;
	indexData[4] = 3;
	indexData[5] = 2;
}
