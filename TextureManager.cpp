#include "TextureManager.h"
#include "DirectXCommon.h"

TextureManager* TextureManager::instance = nullptr;

void TextureManager::Initialize(){
	//SRVの数と同数
	textureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

TextureManager* TextureManager::GetInstance(){
	if (instance == nullptr) {
		instance = new TextureManager;
	}
	return instance;
}

void TextureManager::Finalize(){
	delete instance;
	instance = nullptr;
}
