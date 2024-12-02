#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <fstream>
#include <xaudio2.h>
#include <unordered_map>
#pragma comment(lib,"xaudio2.lib")

//チャンクヘッダ
struct ChunkHeader {
	char id[4]; //チャンク毎のID
	int32_t size; //チャンクサイズ
};
//RIFFヘッダチャンク
struct RiffHeader {
	ChunkHeader chunk; //"RIFF"
	char type[4]; //"WAVE"
};
//FMTチャンク
struct FormatChunk {
	ChunkHeader chunk; //"FMT"
	WAVEFORMATEX fmt; //波形フォーマット
};
//音声データ
struct SoundData {
	//波形フォーマット
	WAVEFORMATEX wfex;
	//バッファの先頭アドレス
	BYTE* pBuffer;
	//バッファのサイズ
	unsigned int bufferSize;
};

class AudioManager
{
public:
	AudioManager();
	~AudioManager();

	// 初期化と終了
	void Initialize();
	void Finalize();

	// 音声データの読み込み
	bool LoadSound(const std::string& key, const std::string& filename);

	// 音声データの再生
	void PlaySound(const std::string& key);

	// 音声データの解放
	void UnloadSound(const std::string& key);

private:
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice = nullptr;

	// 音声データの管理マップ
	std::unordered_map<std::string, SoundData> soundMap;

	// WAVファイル読み込み
	SoundData LoadWaveFile(const std::string& filename);

};

