#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <fstream>
#include <xaudio2.h>
#include <unordered_map>
#include <vector>
#pragma comment(lib,"xaudio2.lib")

//チャンクヘッダ
struct ChunkHeader {
	char id_[4]; //チャンク毎のID
	int32_t size_; //チャンクサイズ
};
//RIFFヘッダチャンク
struct RiffHeader {
	ChunkHeader chunk_; //"RIFF"
	char type_[4]; //"WAVE"
};
//FMTチャンク
struct FormatChunk {
	ChunkHeader chunk_; //"FMT"
	WAVEFORMATEX fmt_; //波形フォーマット
};
//音声データ
struct SoundData {
	WAVEFORMATEX wfex_; // 波形フォーマット
	std::vector<BYTE> buffer_; // 音声データのバッファ
	unsigned int bufferSize_; // 音声データのサイズ
};

//=============================================================
// AudioManagerクラス
// 音声データの読み込み・再生・管理を行うクラス。
//=============================================================
namespace TKM {
	class AudioManager {
	public:
		/// <summary>
		/// オーディオマネージャを初期化します。
		/// </summary>
		void Initialize();
		/// <summary>
		/// オーディオマネージャを終了処理します。
		/// </summary>
		void Finalize();
		/// <summary>
		/// 音声データを読み込みます。
		/// </summary>
		/// <param name="key"></param>
		/// <param name="filename"></param>
		/// <returns></returns>
		bool LoadSound(const std::string& key, const std::string& filename);
		/// <summary>
		/// 音声データを再生します。
		/// </summary>
		/// <param name="key"></param>
		/// <param name="volume"></param>
		/// <param name="loop"></param>
		void PlaySound(const std::string& key, float volume = 1.0f, bool loop = false);

		/// <summary>
		/// 音声データの再生を停止します。
		/// </summary>
		/// <param name="key"></param>
		void StopSound(const std::string& key);
		/// <summary>
		/// 全ての音声データの再生を停止します。
		/// </summary>
		void StopAllSounds();

		/// <summary>
		/// 音声データを一時停止します。
		/// </summary>
		/// <param name="key"></param>
		void PauseSound(const std::string& key);
		/// <summary>
		/// 一時停止中の音声データを再開します。
		/// </summary>
		/// <param name="key"></param>
		void ResumeSound(const std::string& key);

		/// <summary>
		/// 音声データを解放します。
		/// </summary>
		/// <param name="key"></param>
		void UnloadSound(const std::string& key);

		/// <summary>
		/// シングルトンインスタンスを破棄します。
		/// </summary>
		static void DestroyInstance();

		//シングルトンインスタンスの取得
		/// <summary>シングルトンインスタンスを取得します。</summary>
		static AudioManager* GetInstance();

		// Setter=================================
	
		/// <summary>
		/// ゲーム全体の音量を設定します。0.0f（無音）から1.0f（最大音量）までの範囲で指定します。
		/// </summary>
		/// <param name="volume"></param>
		void SetGameVolume(float volume);
		// =======================================
		// Getter=================================
		/// <summary>
		/// ゲーム全体の音量を取得します。0.0f（無音）から1.0f（最大音量）までの範囲で返します。
		/// </summary>
		/// <returns></returns>
		float GetGameVolume() const;
		// =======================================

	private:
		Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
		IXAudio2MasteringVoice* masterVoice_ = nullptr;

		// 再生中の音声の管理構造体とリスト
		struct PlayingVoice {
			std::string key_; // 再生中の音声のキー
			IXAudio2SourceVoice* voice_ = nullptr; // 再生中の音声のソースボイス
			float baseVolume_ = 1.0f; // PlaySoundで指定された元の音量
		};
		std::vector<PlayingVoice> playingVoices_; // 再生中の音声のリスト

		// 音声データの管理マップ
		std::unordered_map<std::string, SoundData> soundMap_;

		bool initialized_ = false; // 初期化済みフラグ

		float gameVolume_ = 1.0f; // ゲーム全体音量

		/// <summary>
		///		
		/// </summary>
		/// <param name="filename"></param>
		/// <returns></returns>
		SoundData LoadWaveFile(const std::string& filename);

		////シングルトン-----------------------------------------------
		//コンストラクタ、デストラクタの隠蔽
		AudioManager() = default;
		~AudioManager() = default;
		//コピーインストラクタの封印
		AudioManager(AudioManager&) = delete;
		//コピー代入演算子の封印
		AudioManager& operator=(AudioManager&) = delete;
		////---------------------------------------------------------
	};
}