#include "AudioManager.h"
#include <cassert>
#include <fstream>

AudioManager* AudioManager::instance = nullptr; // 静的メンバ変数の初期化

void AudioManager::Initialize() {
	// XAudio2の初期化
	HRESULT hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hr));

	// マスターボイスの作成
	hr = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(hr));
}

void AudioManager::Finalize() {
	for (auto& [key, soundData] : soundMap) { // 登録されている音声データを解放
		delete[] soundData.pBuffer; // バッファの解放
	}
	soundMap.clear(); // マップのクリア

	if (masterVoice) { // マスターボイスの破棄
		masterVoice->DestroyVoice(); // マスターボイスの破棄
		masterVoice = nullptr; // ポインタをクリア
	}
	xAudio2.Reset(); // XAudio2オブジェクトの解放

	delete instance;
	instance = nullptr;
}

bool AudioManager::LoadSound(const std::string& key, const std::string& filename) {
	if (soundMap.find(key) != soundMap.end()) { // 既にロードされているか確認
		return false; // 既にロード済み
	}

	// ファイル名に "resources/" を追加
	std::string fullPath = "resources/" + filename;

	SoundData soundData = LoadWaveFile(fullPath); // WAVファイルの読み込み
	soundMap[key] = soundData; // マップに登録
	return true;
}


void AudioManager::PlaySound(const std::string& key) {
	auto it = soundMap.find(key);
	if (it == soundMap.end()) { // 音声キーが存在するか確認
		return; // 存在しない音声キー
	}

	SoundData& soundData = it->second; // 音声データの取得

	IXAudio2SourceVoice* sourceVoice = nullptr; // ソースボイスの作成
	HRESULT hr = xAudio2->CreateSourceVoice(&sourceVoice, &soundData.wfex); // ソースボイスの作成
	assert(SUCCEEDED(hr));

	// バッファの設定と再生
	XAUDIO2_BUFFER buffer = {};
	buffer.pAudioData = soundData.pBuffer;
	buffer.AudioBytes = soundData.bufferSize;
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	// バッファの送信
	hr = sourceVoice->SubmitSourceBuffer(&buffer);
	assert(SUCCEEDED(hr));
	// 再生開始
	hr = sourceVoice->Start();
	assert(SUCCEEDED(hr));
}


void AudioManager::UnloadSound(const std::string& key) {
	auto it = soundMap.find(key); // 音声データを検索
	if (it != soundMap.end()) { // 音声キーが存在するか確認
		delete[] it->second.pBuffer; // バッファの解放
		soundMap.erase(it); // マップから削除
	}
}

AudioManager* AudioManager::GetInstance(){
	if (instance == nullptr) { // インスタンスが存在しない場合に生成
		instance = new AudioManager;
	}
	return instance;
}

SoundData AudioManager::LoadWaveFile(const std::string& filename) {
	// ファイルをバイナリモードで開く
	std::ifstream file(filename, std::ios_base::binary);
	assert(file.is_open());

	// RIFFヘッダーの読み込み
	RiffHeader riff;
	file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
	assert(strncmp(riff.chunk.id, "RIFF", 4) == 0);
	assert(strncmp(riff.type, "WAVE", 4) == 0);

	// Formatチャンクの読み込み
	FormatChunk format = {};
	file.read(reinterpret_cast<char*>(&format.chunk), sizeof(ChunkHeader));
	assert(strncmp(format.chunk.id, "fmt ", 4) == 0);
	file.read(reinterpret_cast<char*>(&format.fmt), format.chunk.size);

	// PCM形式か確認
	assert(format.fmt.wFormatTag == WAVE_FORMAT_PCM);
	
	// Dataチャンクの探索と読み込み
	ChunkHeader data;
	while (true) { // dataチャンクを探す
		file.read(reinterpret_cast<char*>(&data), sizeof(data));
		if (file.eof() || !file) { // ファイルの終端に達した、またはエラー発生
			assert(false); // Dataチャンクが見つからなかった
		}
		if (strncmp(data.id, "data", 4) == 0) { // dataチャンクを発見
			break;
		}
		file.seekg(data.size, std::ios_base::cur);
	}

	char* pBuffer = new char[data.size]; // 音声データ用のバッファを確保
	file.read(pBuffer, data.size); // 音声データの読み込み
	file.close(); // ファイルを閉じる

	// SoundData構造体にデータをセットして返す
	SoundData soundData;
	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}