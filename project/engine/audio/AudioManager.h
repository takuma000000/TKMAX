#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <fstream>
#include <xaudio2.h>
#include <unordered_map>
#include <vector>
#pragma comment(lib,"xaudio2.lib")

//=============================================================
// WAV読み込み用構造体
//=============================================================

// チャンクヘッダ
struct ChunkHeader {
	char id_[4];     // チャンクID
	int32_t size_;   // チャンクサイズ
};

// RIFFヘッダ
struct RiffHeader {
	ChunkHeader chunk_; // "RIFF"
	char type_[4];      // "WAVE"
};

// Formatチャンク
struct FormatChunk {
	ChunkHeader chunk_; // "FMT"
	WAVEFORMATEX fmt_;  // 波形フォーマット
};

// 音声データ
struct SoundData {
	WAVEFORMATEX wfex_;       // 波形フォーマット
	std::vector<BYTE> buffer_;// 音声データ
	unsigned int bufferSize_; // バッファサイズ
};

namespace TKM {

	//=============================================================
	// AudioManagerクラス
	// 音声の読み込みと再生を管理するクラス
	//=============================================================
	class AudioManager {
	public:
		//=============================================================
		// 初期化・終了
		//=============================================================

		/// <summary>
		/// オーディオマネージャを初期化します。
		/// </summary>
		void Initialize();

		/// <summary>
		/// オーディオマネージャを終了します。
		/// </summary>
		void Finalize();

		//=============================================================
		// 読み込み・再生制御
		//=============================================================

		/// <summary>
		/// 音声データを読み込みます。
		/// </summary>
		bool LoadSound(const std::string& key, const std::string& filename);

		/// <summary>
		/// 音声データを再生します。
		/// </summary>
		void PlaySound(const std::string& key, float volume = 1.0f, bool loop = false);

		/// <summary>
		/// 音声データの再生を停止します。
		/// </summary>
		void StopSound(const std::string& key);

		/// <summary>
		/// 全ての音声の再生を停止します。
		/// </summary>
		void StopAllSounds();

		/// <summary>
		/// 音声データを一時停止します。
		/// </summary>
		void PauseSound(const std::string& key);

		/// <summary>
		/// 一時停止した音声データを再開します。
		/// </summary>
		void ResumeSound(const std::string& key);

		/// <summary>
		/// 音声データを解放します。
		/// </summary>
		void UnloadSound(const std::string& key);

		//=============================================================
		// シングルトン
		//=============================================================

		/// <summary>
		/// インスタンスを破棄します。
		/// </summary>
		static void DestroyInstance();

		/// <summary>
		/// インスタンスを取得します。
		/// </summary>
		static AudioManager* GetInstance();

		//=============================================================
		// Setter

		/// <summary>
		/// ゲーム全体音量を設定します。
		/// </summary>
		void SetGameVolume(float volume);

		//=============================================================

		//=============================================================
		// Getter

		/// <summary>
		/// ゲーム全体音量を取得します。
		/// </summary>
		float GetGameVolume() const;

		//=============================================================

	private:
		//=============================================================
		// XAudio2
		//=============================================================

		Microsoft::WRL::ComPtr<IXAudio2> xAudio2_; // XAudio2本体
		IXAudio2MasteringVoice* masterVoice_ = nullptr; // マスターボイス

		//=============================================================
		// 再生中音声
		//=============================================================

		struct PlayingVoice {
			std::string key_;              // 音声キー
			IXAudio2SourceVoice* voice_ = nullptr; // ソースボイス
			float baseVolume_ = 1.0f;      // 元音量
		};

		std::vector<PlayingVoice> playingVoices_; // 再生中音声一覧

		//=============================================================
		// 音声データ
		//=============================================================

		std::unordered_map<std::string, SoundData> soundMap_; // 音声データ管理

		//=============================================================
		// 状態
		//=============================================================

		bool initialized_ = false; // 初期化済みフラグ

#ifdef _DEBUG
		float gameVolume_ = 0.0f;  // デバッグビルド時はミュート
#else
		float gameVolume_ = 1.0f;  // リリースビルド時は通常音量
#endif

		//=============================================================
		// 内部読み込み処理
		//=============================================================

		/// <summary>
		/// Waveファイルを読み込みます。
		/// </summary>
		SoundData LoadWaveFile(const std::string& filename);

		//=============================================================
		// 禁止事項
		//=============================================================

		AudioManager() = default;
		~AudioManager() = default;
		AudioManager(AudioManager&) = delete;
		AudioManager& operator=(AudioManager&) = delete;
	};
}