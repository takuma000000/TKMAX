#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include <dxcapi.h>
#include <string>
#include <chrono>
#include "WindowsAPI.h"
#include "SrvManager.h"
#include "DirectXTex.h"
#include "MyMath.h"
#include "SystemIncludes.h"

// PostEffect
namespace TKM {
	class RadialBlurEffect;
	class VignettingEffect;
	class WaterRippleEffect;
	class FogEffect;
	class AuraEffect;
	class NoiseEffect;
	class MotionBlurEffect;
	class SpeedLineEffect;
}

//=============================================================
// DirectXCommonクラス
// DirectX12の初期化・描画・リソース管理を行うクラス。
//=============================================================
namespace TKM {
	class DirectXCommon {
	public:
		// OutlineParameter構造体
		struct OutlineParameter {
			Matrix4x4 projectionInverse;
		};

		// VignettingCB構造体
		struct VignettingCB {
			Vector4 color;     // 枠色
			float   intensity; // 強度
			float   radius;    // どこから暗くするか
			float   softness;  // ふちのボケ具合
			float   padding;   // 16byte アライメント
		};
		// WaterRippleCB構造体
		struct WaterRippleCB {
			Vector2 center;    // 波紋中心 (UV)
			float   radius;    // 現在の半径
			float   amplitude; // ズレの強さ
			float   frequency; // 波の細かさ
			float   width;     // 帯の幅
			float  padding;   // アライメント
			Vector3 color;   // 波紋色
			float   colorIntensity; // 波紋色の強さ
		};
		// FogCB構造体
		struct FogCB {
			Vector3 FogColor;     // 霧の色
			float   FogDensity;   // 全体の濃さ
			float   FogStart;     // 霧開始の高さ (0〜1)
			float   FogEnd;       // 霧最大の高さ (0〜1)
			float   NoiseScale;   // ノイズの細かさ
			float   NoiseStrength;// 濃さのムラの強さ
			float   Time;         // 経過時間
			float   worldScale;   // 霧パターンの「世界空間スケール」
			Vector3 worldPos;     // カメラ or プレイヤーのワールド座標
			float   padding2;     // 16byte整列用
		};
		// AuraCB構造体
		struct AuraCB {
			Vector2 CenterUV; // 中心UV
			float   Time; 	  // 経過時間
			float   Radius;   // スケール
			float   Intensity; // 明るさ
			float   UseRing;   // リングを使うかどうか
			float   RingRadius; // リングの半径
			float   RingWidth; // リングの幅
			Vector3 ColorA; // 色A
			float   _pad0; 	  // 16byte整列
			Vector3 ColorB; // 色B
			float   Mix; 	// 色の混ぜ具合 (0=A, 1=B)
			Vector2 TopUV;      // ボス頭のUV
			Vector2 BottomUV;   // ボス足のUV
			float   Aspect;        // 画面横/縦
			float   Taper;         // 上に行くほど細く (例 0.65)
			float   NoiseScale;    // 炎の細かさ
			float   NoiseSpeed;    // 炎の速さ
			float   FlameStrength; // 炎の立ち上がり強さ
			float   EdgePower;     // 外周のキレ
			float   VerticalFade;  // 上下のフェード
			float   _pad1;         // 16byte整列
		};
		// AuraVolumeCB構造体
		struct AuraVolumeCB {
			Matrix4x4 ViewProj;

			Vector3   CenterWS;
			float     Radius;

			float     Height;
			uint32_t  SliceCount;
			float     Time;
			float     _pad0;

			Vector3   Color;
			float     Intensity;

			float     NoiseScale;
			float     NoiseSpeed;
			float     RimPower;
			float     AlphaBase;
		};
		// FogVolumeCB構造体（空間霧用）
		struct FogVolumeCB {
			Matrix4x4 ViewProj;

			Vector3 CenterWS; float _pad0;

			Vector3 HalfSizeWS; float Density;

			Vector3 CamRightWS; float _pad1;
			Vector3 CamUpWS;    float _pad2;
			Vector3 CamFwdWS;   float _pad3;

			uint32_t SliceCount;
			float Time;
			float NoiseScale;
			float NoiseSpeed;

			Vector3 FogColor;
			float Softness;

			float FogStart;
			float FogEnd;

			float NoiseStrength;
			float WorldScale;

			Vector3 WorldPos;
			float _padX;

		};
		// SmokeVolumeCB構造体（立体煙用）
		struct SmokeVolumeCB {
			Matrix4x4 ViewProj;

			Vector3 CenterWS; float _pad0;
			Vector3 HalfSizeWS; float Density;

			Vector3 CamRightWS; float _pad1;
			Vector3 CamUpWS;    float _pad2;
			Vector3 CamFwdWS;   float _pad3;

			uint32_t SliceCount;
			float Time;
			float BaseScale;
			float FlowSpeed;

			float DetailScale;
			float DetailStrength;
			float Threshold;
			float Softness;

			Vector3 SmokeColor;
			float AlphaMax;

			float RiseSpeed;
			float _padX[3];
		};

		// BeamCB構造体（レーザービーム用）
		struct LaserBeamCB {
			Matrix4x4 ViewProj;

			Vector3 StartWS;
			float Radius;

			Vector3 EndWS;
			float Intensity;

			Vector3 CamRightWS;
			float _pad0;
			Vector3 CamUpWS;
			float _pad1;
			Vector3 CamFwdWS;
			float _pad2;

			uint32_t SliceCount;
			float Time;
			float CoreSharpness;
			float EdgeSoftness;

			Vector3 Color;
			float NoiseScale;

			float NoiseSpeed;
			uint32_t Telegraph;
			float _pad3;
			float _pad4;
		};
		// NoiseCB構造体
		struct NoiseCB {
			float   Time;
			float   Intensity;
			float   LineDensity;
			float   LineSpeed;

			float   BlockScale;
			float   BlockShift;
			float   RGBShift;
			float   Flash;

			Vector2 Resolution;
			float   _pad0;
			float   _pad1;
		};
		// MotionBlurCB構造体
		struct MotionBlurCB {
			float strength;   // モーションブラーの強さ
			float padding[3]; // 16バイトアライメントのためのパディング
		};
		// SpeedLineCB構造体
		struct SpeedLineCB {
			Vector2 direction; // スピード線の方向 (正規化されたUV)
			float intensity;   // スピード線の強さ (0.0f で見えない、1.0f で最大)
			float time;        // 経過時間

			float density;     // スピード線の密度
			float speed;       // スピード線の移動速度
			float width;       // スピード線の幅
			float padding;     // 16バイトアライメントのためのパディング
		};

		// -------------------- 初期化 --------------------
		/// <summary>
		/// DirectXCommonの初期化を行う関数
		/// </summary>
		/// <param name="windowsAPI"></param>
		void Initialize(WindowsAPI* windowsAPI);
		/// <summary>
		/// DirectXCommonの終了処理を行う関数
		/// </summary>
		void InitializeDevice();
		/// <summary>
		/// DirectXCommonの終了処理を行う関数
		/// </summary>
		void InitializeCommand();
		/// <summary>
		/// DirectXCommonの終了処理を行う関数
		/// </summary>
		void InitializeRTV();
		/// <summary>
		/// DirectXCommonの終了処理を行う関数
		/// </summary>
		void InitializeDSV();
		/// <summary>
		/// DirectXCommonの終了処理を行う関数
		/// </summary>
		void InitializeFence();
		/// <summary>
		/// DirectXCommonの終了処理を行う関数
		/// </summary>
		void InitializeViewport();
		/// <summary>
		/// DirectXCommonの終了処理を行う関数
		/// </summary>
		void InitializeScissorRect();
		// -------------------- 描画 --------------------
		/// <summary>
		/// DirectXCommonの描画前処理を行う関数
		/// </summary>
		void PreDraw();
		/// <summary>
		/// DirectXCommonの描画後処理を行う関数
		/// </summary>
		void PostDraw();
		// -------------------- デスクリプタヒープ生成 --------------------
		/// <summary>
		/// スワップチェーンの生成を行う関数
		/// </summary>
		void GenerateSwapChain();
		/// <summary>
		/// レンダーターゲットビューの生成を行う関数
		/// </summary>
		void GenerateZBuffer();
		/// <summary>
		/// デスクリプタヒープの生成を行う関数
		/// </summary>
		void GenerateDescpitorHeap();
		/// <summary>
		/// DXCの初期化を行う関数
		/// </summary>
		void GenerateDXC();
		// -------------------- リソース生成 --------------------
		/// <summary>
		/// バッファリソースの生成を行う関数
		/// </summary>
		/// <param name="sizeInBytes"></param>
		/// <returns></returns>
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);
		/// <summary>
		/// テクスチャリソースの生成を行う関数
		/// </summary>
		/// <param name="metadata"></param>
		/// <returns></returns>
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);
		/// <summary>
		/// テクスチャデータのアップロードを行う関数
		/// </summary>
		/// <param name="texture"></param>
		/// <param name="mipImages"></param>
		/// <returns></returns>
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);
		/// <summary>
		/// デスクリプタヒープの生成を行う関数
		/// </summary>
		/// <param name="heapType"></param>
		/// <param name="numDescriptors"></param>
		/// <param name="shaderVisible"></param>
		/// <returns></returns>
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
		/// <summary>
		/// レンダーターゲット用テクスチャリソースの生成を行う関数
		/// </summary>
		/// <param name="device"></param>
		/// <param name="width"></param>
		/// <param name="height"></param>
		/// <param name="format"></param>
		/// <param name="clearColor"></param>
		/// <returns></returns>
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device,
			int width, int height, DXGI_FORMAT format, const Vector4& clearColor);
		/// <summary>
		/// RenderTexture 用 RTV 作成
		/// </summary>
		void CreateRenderTextureRTV();
		/// <summary>
		/// ポストエフェクトチェーン用：RadialBlur適用
		/// </summary>
		/// <param name="inputTex"></param>
		/// <param name="inputSrvIndex"></param>
		/// <param name="outputTex"></param>
		/// <param name="outputRtv"></param>
		void ApplyRadialBlur(
			ID3D12Resource* inputTex,
			uint32_t        inputSrvIndex,
			ID3D12Resource* outputTex,
			D3D12_CPU_DESCRIPTOR_HANDLE outputRtv);
		/// <summary>
		/// ポストエフェクトチェーン用：WaterRipple適用
		/// </summary>
		/// <param name="inputTex"></param>
		/// <param name="inputSrvIndex"></param>
		/// <param name="outputTex"></param>
		/// <param name="outputRtv"></param>
		void ApplyWaterRipple(
			ID3D12Resource* inputTex,
			uint32_t        inputSrvIndex,
			ID3D12Resource* outputTex,
			D3D12_CPU_DESCRIPTOR_HANDLE outputRtv);
		/// <summary>
		/// ポストエフェクトチェーン用：Vignetting適用
		/// </summary>
		/// <param name="inputTex"></param>
		/// <param name="inputSrvIndex"></param>
		/// <param name="outputTex"></param>
		/// <param name="outputRtv"></param>
		void ApplyVignetting(
			ID3D12Resource* inputTex,
			uint32_t        inputSrvIndex,
			ID3D12Resource* outputTex,
			D3D12_CPU_DESCRIPTOR_HANDLE outputRtv);
		/// <summary>
		/// ポストエフェクトチェーン用：Fog適用
		/// </summary>
		/// <param name="inputTex"></param>
		/// <param name="inputSrvIndex"></param>
		/// <param name="outputTex"></param>
		/// <param name="outputRtv"></param>
		void ApplyFog(
			ID3D12Resource* inputTex,
			uint32_t        inputSrvIndex,
			ID3D12Resource* outputTex,
			D3D12_CPU_DESCRIPTOR_HANDLE outputRtv);

		/// <summary>
		/// テクスチャ → Swapchain へコピー描画
		/// </summary>
		/// <param name="inputTex"></param>
		/// <param name="inputSrvIndex"></param>
		void DrawTextureToSwapchain(
			ID3D12Resource* inputTex,
			uint32_t        inputSrvIndex);
		/// <summary>
		/// RenderTexture 用 SRV 作成
		/// </summary>
		void BeginDrawToSwapchain();
		/// <summary>
		/// RenderTexture 用 SRV 作成
		/// </summary>
		void InitializeCopyImagePipeline();
		/// <summary>
		/// RenderTexture → Swapchain へコピー描画
		/// </summary>
		void DrawRenderTextureToSwapchain();
		/// <summary>
		/// RadialBlur パイプラインの初期化
		/// </summary>
		void InitializeRadialBlurPipeline();
		/// <summary>
		/// Vignetting パイプラインの初期化
		/// </summary>
		void InitializeVignettingPipeline();
		/// <summary>
		/// WaterRipple パイプラインの初期化
		/// </summary>
		void InitializeWaterRipplePipeline();
		/// <summary>
		/// Fog パイプラインの初期化
		/// </summary>
		void InitializeFogPipeline();
		/// <summary>
		/// Aura パイプラインの初期化
		/// </summary>
		void InitializeAuraPipeline();
		/// <summary>
		/// AuraVolume パイプラインの初期化
		/// </summary>
		void InitializeAuraVolumePipeline();
		/// <summary>
		/// FogVolume パイプラインの初期化
		/// </summary>
		void InitializeFogVolumePipeline();
		/// <summary>
		/// SmokeVolume パイプラインの初期化
		/// </summary>
		void InitializeSmokeVolumePipeline();
		/// <summary>
		/// LaserBeam パイプラインの初期化
		/// </summary>
		void InitializeLaserBeamPipeline();
		/// <summary>
		/// Noise パイプラインの初期化
		/// </summary>
		void InitializeMotionBlurPipeline();
		/// <summary>
		/// SpeedLine パイプラインの初期化
		/// </summary>
		void InitializeSpeedLinePipeline();
		/// <summary>
		/// ポストエフェクトチェーン用：Aura適用
		/// </summary>
		/// <param name="inputTex"></param>
		/// <param name="inputSrvIndex"></param>
		/// <param name="outputTex"></param>
		/// <param name="outputRtv"></param>
		void ApplyAura(
			ID3D12Resource* inputTex,
			uint32_t        inputSrvIndex,
			ID3D12Resource* outputTex,
			D3D12_CPU_DESCRIPTOR_HANDLE outputRtv);
		/// <summary>
		/// ポストエフェクトチェーン用：MotionBlur適用
		/// </summary>
		/// <param name="inputTex"></param>
		/// <param name="inputSrvIndex"></param>
		/// <param name="outputTex"></param>
		/// <param name="outputRtv"></param>
		void ApplyMotionBlur(
			ID3D12Resource* inputTex,
			uint32_t        inputSrvIndex,
			ID3D12Resource* outputTex,
			D3D12_CPU_DESCRIPTOR_HANDLE outputRtv
		);
		/// <summary>
		/// ポストエフェクトチェーン用：SpeedLine適用
		/// </summary>
		/// <param name="inputTex"></param>
		/// <param name="inputSrvIndex"></param>
		/// <param name="outputTex"></param>
		/// <param name="outputRtv"></param>
		void ApplySpeedLine(
			ID3D12Resource* inputTex,
			uint32_t inputSrvIndex,
			ID3D12Resource* outputTex,
			D3D12_CPU_DESCRIPTOR_HANDLE outputRtv
		);
		/// <summary>
		/// 前フレームのテクスチャに、現在のフレームの内容をコピーする関数
		/// </summary>
		/// <param name="inputTex"></param>
		void CopyCurrentFrameToPreviousFrame(ID3D12Resource* inputTex);
		/// <summary>
		/// オーラボリュームの描画
		/// </summary>
		/// <param name="viewProj"></param>
		/// <param name="centerWS"></param>
		/// <param name="radius"></param>
		/// <param name="height"></param>
		/// <param name="sliceCount"></param>
		/// <param name="time"></param>
		/// <param name="color"></param>
		/// <param name="intensity"></param>
		/// <param name="noiseScale"></param>
		/// <param name="noiseSpeed"></param>
		/// <param name="rimPower"></param>
		/// <param name="alphaBase"></param>
		void DrawAuraVolume(
			const Matrix4x4& viewProj,
			const Vector3& centerWS,
			float radius,
			float height,
			uint32_t sliceCount,
			float time,
			const Vector3& color,
			float intensity,
			float noiseScale,
			float noiseSpeed,
			float rimPower,
			float alphaBase);
		/// <summary>
		/// フォグボリュームの描画
		/// </summary>
		/// <param name="viewProj"></param>
		/// <param name="centerWS"></param>
		/// <param name="halfSizeWS"></param>
		/// <param name="camRightWS"></param>
		/// <param name="camUpWS"></param>
		/// <param name="camFwdWS"></param>
		/// <param name="sliceCount"></param>
		/// <param name="time"></param>
		/// <param name="fogColor"></param>
		/// <param name="density"></param>
		/// <param name="noiseScale"></param>
		/// <param name="noiseSpeed"></param>
		/// <param name="softness"></param>
		/// <param name="fogStart"></param>
		/// <param name="fogEnd"></param>
		/// <param name="noiseStrength"></param>
		/// <param name="worldScale"></param>
		/// <param name="worldPos"></param>
		void DrawFogVolume(
			const Matrix4x4& viewProj,
			const Vector3& centerWS,
			const Vector3& halfSizeWS,
			const Vector3& camRightWS,
			const Vector3& camUpWS,
			const Vector3& camFwdWS,
			uint32_t sliceCount,
			float time,
			const Vector3& fogColor,
			float density,
			float noiseScale,
			float noiseSpeed,
			float softness,
			float fogStart,
			float fogEnd,
			float noiseStrength,
			float worldScale,
			const Vector3& worldPos);
		/// <summary>
		/// スモークボリュームの描画
		/// </summary>
		/// <param name="viewProj"></param>
		/// <param name="centerWS"></param>
		/// <param name="halfSizeWS"></param>
		/// <param name="camRightWS"></param>
		/// <param name="camUpWS"></param>
		/// <param name="camFwdWS"></param>
		/// <param name="sliceCount"></param>
		/// <param name="time"></param>
		/// <param name="smokeColor"></param>
		/// <param name="density"></param>
		/// <param name="baseScale"></param>
		/// <param name="detailScale"></param>
		/// <param name="detailStrength"></param>
		/// <param name="threshold"></param>
		/// <param name="softness"></param>
		/// <param name="flowSpeed"></param>
		/// <param name="riseSpeed"></param>
		/// <param name="alphaMax"></param>
		/// <param name="worldScale"></param>
		/// <param name="worldPos"></param>
		void DrawSmokeVolume(
			const Matrix4x4& viewProj,
			const Vector3& centerWS,
			const Vector3& halfSizeWS,
			const Vector3& camRightWS,
			const Vector3& camUpWS,
			const Vector3& camFwdWS,
			uint32_t sliceCount,
			float time,
			const Vector3& smokeColor,
			float density,
			float baseScale,
			float detailScale,
			float detailStrength,
			float threshold,
			float softness,
			float flowSpeed,
			float riseSpeed,
			float alphaMax,
			float worldScale,
			const Vector3& worldPos);
		/// <summary>
		/// レーザービームボリュームの描画
		/// </summary>
		/// <param name="viewProj"></param>
		/// <param name="startWS"></param>
		/// <param name="endWS"></param>
		/// <param name="radius"></param>
		/// <param name="camRightWS"></param>
		/// <param name="camUpWS"></param>
		/// <param name="camFwdWS"></param>
		/// <param name="sliceCount"></param>
		/// <param name="time"></param>
		/// <param name="color"></param>
		/// <param name="intensity"></param>
		/// <param name="coreSharpness"></param>
		/// <param name="edgeSoftness"></param>
		/// <param name="noiseScale"></param>
		/// <param name="noiseSpeed"></param>
		/// <param name="telegraph"></param>
		void DrawLaserBeamVolume(
			const Matrix4x4& viewProj,
			const Vector3& startWS,
			const Vector3& endWS,
			float radius,
			const Vector3& camRightWS,
			const Vector3& camUpWS,
			const Vector3& camFwdWS,
			uint32_t sliceCount,
			float time,
			const Vector3& color,
			float intensity,
			float coreSharpness,
			float edgeSoftness,
			float noiseScale,
			float noiseSpeed,
			uint32_t telegraph);
		/// <summary>
		/// ポストエフェクトなしで RenderTexture → Swapchain へ描画
		/// </summary>
		void DrawPostEffectToSwapchain();
		/// <summary>
		/// 
		/// </summary>
		void InitializeNoisePipeline();
		/// <summary>
		/// 
		/// </summary>
		/// <param name="inputTex"></param>
		/// <param name="inputSrvIndex"></param>
		/// <param name="outputTex"></param>
		/// <param name="outputRtv"></param>
		void ApplyNoise(
			ID3D12Resource* inputTex,
			uint32_t        inputSrvIndex,
			ID3D12Resource* outputTex,
			D3D12_CPU_DESCRIPTOR_HANDLE outputRtv);
		/// <summary>
		/// シェーダのコンパイルを行う関数
		/// </summary>
		/// <param name="filePath"></param>
		/// <param name="profile"></param>
		/// <returns></returns>
		Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);

		// Getter==================================================================
		/// <summary>
		/// D3D12デバイスのゲッター
		/// </summary>
		/// <returns></returns>
		ID3D12Device* GetDevice() const { return device_.Get(); }
		/// <summary>
		/// コマンドキューのゲッター
		/// </summary>
		/// <returns></returns>
		ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
		/// <summary>
		/// ビューポートのゲッター
		/// </summary>
		/// <returns></returns>
		D3D12_VIEWPORT GetViewport() const { return viewport_; }
		/// <summary>
		/// シザー矩形のゲッター
		/// </summary>
		/// <returns></returns>
		D3D12_RECT GetRect() const { return scissorRect_; }
		/// <summary>
		/// デスクリプタサイズのゲッター
		/// </summary>
		/// <returns></returns>
		uint32_t GetDescriptorSizeRTV() const { return descriptorSizeRTV_; }
		/// <summary>
		/// デスクリプタサイズのゲッター
		/// </summary>
		/// <returns></returns>
		uint32_t GetDescriptorSizeDSV() const { return descriptorSizeDSV_; }
		/// <summary>
		/// デスクリプタサイズのゲッター
		/// </summary>
		/// <returns></returns>
		size_t GetBackBufferCount() const { return backBufferChange_; }
		/// <summary>
		/// DSVハンドルのゲッター
		/// </summary>
		/// <returns></returns>
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const {
			return dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
		}
		/// <summary>
		/// 現在のRTVハンドルのゲッター
		/// </summary>
		/// <returns></returns>
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const {
			UINT index = swapChain_->GetCurrentBackBufferIndex();
			return rtvHandles_[index];
		}
		/// <summary>
		/// RenderTexture 用 SRV インデックスのゲッター
		/// </summary>
		/// <returns></returns>
		uint32_t GetRenderTextureSrvIndex() const { return renderTextureSrvIndex_; }
		/// <summary>
		/// RadialBlurEffect を取得
		/// </summary>
		/// <returns></returns>
		TKM::RadialBlurEffect* GetRadialBlurEffect() const { return radialBlurEffect_; }
		/// <summary>
		/// VignettingEffect を取得
		/// </summary>
		/// <returns></returns>
		TKM::AuraEffect* GetAuraEffect() const { return auraEffect_; }
		/// <summary>
		/// WaterRippleEffect を取得
		/// </summary>
		/// <returns></returns>
		TKM::WaterRippleEffect* GetWaterRippleEffect() const { return rippleEffect_; }
		// ========================================================================
		// Setter==================================================================
		/// <summary>
		/// SrvManager をセット
		/// </summary>
		/// <param name="srvManager"></param>
		void SetSrvManager(SrvManager* srvManager) { srvManager_ = srvManager; }
		/// <summary>
		/// RadialBlurEffect をセット（必要なら）
		/// </summary>
		/// <param name="effect"></param>
		void SetRadialBlurEffect(TKM::RadialBlurEffect* effect) { radialBlurEffect_ = effect; }
		/// <summary>
		/// VignettingEffect をセット（必要なら）
		/// </summary>
		/// <param name="effect"></param>
		void SetVignettingEffect(TKM::VignettingEffect* effect) { vignettingEffect_ = effect; }
		/// <summary>
		/// Vignetting 用 パラメータセット
		/// </summary>
		/// <param name="color"></param>
		/// <param name="intensity"></param>
		/// <param name="radius"></param>
		/// <param name="softness"></param>
		void SetVignettingParam(const Vector4& color, float intensity, float radius, float softness);
		/// <summary>
		/// WaterRippleEffect をセット（必要なら）
		/// </summary>
		/// <param name="effect"></param>
		void SetWaterRippleEffect(TKM::WaterRippleEffect* effect) { rippleEffect_ = effect; }
		/// <summary>
		/// WaterRipple 用 パラメータセット
		/// </summary>
		/// <param name="centerUV"></param>
		/// <param name="radius"></param>
		/// <param name="amplitude"></param>
		/// <param name="frequency"></param>
		/// <param name="width"></param>
		void SetWaterRippleParam(const Vector2& centerUV, float radius, float amplitude, float frequency, float width, const Vector3& color, float colorIntensity);
		/// <summary>
		/// FogEffect をセット（必要なら）
		/// </summary>
		/// <param name="effect"></param>
		void SetFogEffect(TKM::FogEffect* effect) { fogEffect_ = effect; }
		/// <summary>
		/// Fog 用 パラメータセット
		/// </summary>
		/// <param name="color"></param>
		/// <param name="density"></param>
		/// <param name="start"></param>
		/// <param name="end"></param>
		/// <param name="noiseScale"></param>
		/// <param name="noiseStrength"></param>
		/// <param name="time"></param>
		/// <param name="worldPos"></param>
		/// <param name="worldScale"></param>
		void SetFogParam(const Vector3& color, float density,
			float start, float end,
			float noiseScale, float noiseStrength,
			float time,
			const Vector3& worldPos, float worldScale);
		/// <summary>
		/// Aura 用 パラメータセット
		/// </summary>
		/// <param name="centerUV"></param>
		/// <param name="topUV"></param>
		/// <param name="bottomUV"></param>
		/// <param name="aspect"></param>
		/// <param name="time"></param>
		/// <param name="radius"></param>
		/// <param name="intensity"></param>
		/// <param name="useRing"></param>
		/// <param name="ringRadius"></param>
		/// <param name="ringWidth"></param>
		/// <param name="colorA"></param>
		/// <param name="colorB"></param>
		/// <param name="mix"></param>
		/// <param name="taper"></param>
		/// <param name="noiseScale"></param>
		/// <param name="noiseSpeed"></param>
		/// <param name="flameStrength"></param>
		/// <param name="edgePower"></param>
		/// <param name="verticalFade"></param>
		void SetAuraParam(
			const Vector2& centerUV,
			const Vector2& topUV,
			const Vector2& bottomUV,
			float aspect,
			float time,
			float radius,
			float intensity,
			float useRing,
			float ringRadius,
			float ringWidth,
			const Vector3& colorA,
			const Vector3& colorB,
			float mix,
			float taper,
			float noiseScale,
			float noiseSpeed,
			float flameStrength,
			float edgePower,
			float verticalFade);
		/// <summary>
		/// AuraEffect をセット（必要なら）
		/// </summary>
		/// <param name="effect"></param>
		void SetAuraEffect(TKM::AuraEffect* effect) { auraEffect_ = effect; }
		/// <summary>
		/// NoiseEffect をセット（必要なら）
		/// </summary>
		/// <param name="effect"></param>
		void SetNoiseEffect(TKM::NoiseEffect* effect) { noiseEffect_ = effect; }
		/// <summary>
		/// Noise 用 パラメータセット
		/// </summary>
		/// <param name="time"></param>
		/// <param name="intensity"></param>
		/// <param name="lineDensity"></param>
		/// <param name="lineSpeed"></param>
		/// <param name="blockScale"></param>
		/// <param name="blockShift"></param>
		/// <param name="rgbShift"></param>
		/// <param name="flash"></param>
		/// <param name="resolution"></param>
		void SetNoiseParam(
			float time,
			float intensity,
			float lineDensity,
			float lineSpeed,
			float blockScale,
			float blockShift,
			float rgbShift,
			float flash,
			const Vector2& resolution);
		/// <summary>
		/// MotionBlurEffect をセット（必要なら）
		/// </summary>
		/// <param name="strength"></param>
		void SetMotionBlurParam(float strength);
		/// <summary>
		/// MotionBlurEffect をセット（必要なら）
		/// </summary>
		/// <param name="effect"></param>
		void SetMotionBlurEffect(TKM::MotionBlurEffect* effect) { motionBlurEffect_ = effect; }
		/// <summary>
		/// SpeedLineEffect をセット（必要なら）
		/// </summary>
		/// <param name="effect"></param>
		void SetSpeedLineEffect(TKM::SpeedLineEffect* effect);
		/// <summary>
		/// SpeedLine 用 パラメータセット
		/// </summary>
		void SetSpeedLineParam(
			const Vector2& direction,
			float intensity,
			float time,
			float density,
			float speed,
			float width
		);
		// ========================================================================
	private:
		//======================================================================
		// 固定FPS制御
		//======================================================================
		/// <summary>
		/// 固定FPS制御の初期化を行う関数
		/// </summary>
		void InitializeFixFPS();
		/// <summary>
		/// 固定FPS制御の更新を行う関数
		/// </summary>
		void UpdateFixFPS();
		// -------------------- 時間計測 --------------------
		std::chrono::steady_clock::time_point reference_;
		//======================================================================
		// DirectX関連コア（デバイス / ファクトリ / スワップチェーン）
		//======================================================================
		// -------------------- DirectX関連 --------------------
		Microsoft::WRL::ComPtr<ID3D12Device>       device_;       // D3D12デバイス
		Microsoft::WRL::ComPtr<IDXGIFactory7>      dxgiFactory_;  // DXGIファクトリ
		Microsoft::WRL::ComPtr<IDXGISwapChain4>    swapChain_;    // スワップチェーン
		Microsoft::WRL::ComPtr<ID3D12Debug1>       debugController_; // デバッグコントローラ
		Microsoft::WRL::ComPtr<IDXGIAdapter4>      useAdapter_;   // 使用アダプタ

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      commandAllocator_;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   commandList_;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>          commandQueue_;
		//======================================================================
		// ヒープ / フェンス / レンダーターゲット / 深度
		//======================================================================
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
		Microsoft::WRL::ComPtr<ID3D12Fence>          fence_;

		Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources_;

		DXGI_SWAP_CHAIN_DESC1          swapChainDesc_{};
		D3D12_RENDER_TARGET_VIEW_DESC  rtvDesc_{};
		D3D12_DEPTH_STENCIL_VIEW_DESC  dsvDesc_{};
		D3D12_VIEWPORT                 viewport_{};
		D3D12_RECT                     scissorRect_{};
		D3D12_RESOURCE_BARRIER         barrier_{};
		// -------------------- 描画状態 --------------------
		D3D12_RESOURCE_STATES renderTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		// -------------------- デスクリプタサイズ --------------------
		uint32_t descriptorSizeRTV_ = 0;
		uint32_t descriptorSizeDSV_ = 0;
		uint32_t descriptorSizeSRV_ = 0;
		// -------------------- ImGui/RTV --------------------
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
		D3D12_CPU_DESCRIPTOR_HANDLE                  rtvStartHandle_{};
		D3D12_CPU_DESCRIPTOR_HANDLE                  rtvHandles_[4]{};
		//======================================================================
		// DXC（シェーダコンパイラ）
		//======================================================================
		// -------------------- DXC --------------------
		Microsoft::WRL::ComPtr<IDxcUtils>          dxcUtils_;
		Microsoft::WRL::ComPtr<IDxcCompiler3>      dxcCompiler_;
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
		//======================================================================
		// RootSignature / PipelineState
		//======================================================================
		// -------------------- Root & PSO --------------------
		Microsoft::WRL::ComPtr<ID3D12RootSignature>  rootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>  graphicsPipelineState_;
		Microsoft::WRL::ComPtr<ID3D12RootSignature>  copyImageRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>  copyImagePipelineState_;
		bool                                          copyImageInitialized_ = false;

		// RadialBlur 用 PSO
		Microsoft::WRL::ComPtr<ID3D12PipelineState> radialBlurPipelineState_;
		bool                                         radialBlurInitialized_ = false;
		TKM::RadialBlurEffect* radialBlurEffect_ = nullptr; // 現在シーンの RadialBlurEffect（なければ nullptr）

		// Vignetting 用 PSO
		Microsoft::WRL::ComPtr<ID3D12RootSignature>  vignettingRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>  vignettingPipelineState_;
		bool                                         vignettingInitialized_ = false;
		TKM::VignettingEffect* vignettingEffect_ = nullptr; // 現在シーンの VignettingEffect（なければ nullptr）
		Microsoft::WRL::ComPtr<ID3D12Resource> vignettingConstantBuffer_; // Vignetting 用 定数バッファ
		void* vignettingMappedData_ = nullptr; // Vignetting 用 定数バッファマッピングデータポインタ

		// WaterRipple 用 PSO
		Microsoft::WRL::ComPtr<ID3D12RootSignature>  rippleRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>  ripplePipelineState_;
		bool                                         rippleInitialized_ = false;
		TKM::WaterRippleEffect* rippleEffect_ = nullptr; // 現在シーンの WaterRippleEffect
		Microsoft::WRL::ComPtr<ID3D12Resource> rippleConstantBuffer_; // Ripple 用 定数バッファ
		void* rippleMappedData_ = nullptr; // Ripple 用 定数バッファマッピングデータポインタ

		// Fog 用 PSO
		TKM::FogEffect* fogEffect_ = nullptr;
		bool fogInitialized_ = false;
		Microsoft::WRL::ComPtr<ID3D12RootSignature>  fogRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>  fogPipelineState_;
		Microsoft::WRL::ComPtr<ID3D12Resource>       fogConstantBuffer_;
		void* fogMappedData_ = nullptr;

		// Aura 用 PSO
		bool auraInitialized_ = false;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> auraRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> auraPipelineState_;
		Microsoft::WRL::ComPtr<ID3D12Resource> auraConstantBuffer_;
		void* auraMappedData_ = nullptr;
		TKM::AuraEffect* auraEffect_ = nullptr;

		// AuraVolume 用 PSO
		bool auraVolumeInitialized_ = false;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> auraVolumeRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> auraVolumePipelineState_;
		Microsoft::WRL::ComPtr<ID3D12Resource> auraVolumeConstantBuffer_;
		void* auraVolumeMappedData_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> auraVolumeVB_;
		D3D12_VERTEX_BUFFER_VIEW auraVolumeVBView_{};

		// FogVolume 用 PSO
		bool fogVolumeInitialized_ = false;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> fogVolumeRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> fogVolumePipelineState_;
		Microsoft::WRL::ComPtr<ID3D12Resource> fogVolumeConstantBuffer_;
		void* fogVolumeMappedData_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> fogVolumeVB_;
		D3D12_VERTEX_BUFFER_VIEW fogVolumeVBView_{};

		// SmokeVolume 用 PSO
		bool smokeVolumeInitialized_ = false;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> smokeVolumeRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> smokeVolumePipelineState_;
		Microsoft::WRL::ComPtr<ID3D12Resource> smokeVolumeVertexBuffer_;
		D3D12_VERTEX_BUFFER_VIEW smokeVolumeVBView_{};

		Microsoft::WRL::ComPtr<ID3D12Resource> smokeVolumeConstantBuffer_;
		SmokeVolumeCB* smokeVolumeCB_ = nullptr;

		// Noise 用 PSO
		Microsoft::WRL::ComPtr<ID3D12RootSignature> noiseRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> noisePipelineState_;
		bool noiseInitialized_ = false;
		TKM::NoiseEffect* noiseEffect_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> noiseConstantBuffer_;
		void* noiseMappedData_ = nullptr;

		// MotionBlur 用 PSO
		TKM::MotionBlurEffect* motionBlurEffect_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> previousFrameTextureResource_ = nullptr;
		uint32_t previousFrameSrvIndex_ = 0;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> motionBlurRootSignature_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> motionBlurPipelineState_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> motionBlurConstantBuffer_ = nullptr;
		void* motionBlurMappedData_ = nullptr;
		bool motionBlurInitialized_ = false;
		bool previousFrameReady_ = false;
		D3D12_RESOURCE_STATES previousFrameState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;

		// SpeedLine 用 PSO
		TKM::SpeedLineEffect* speedLineEffect_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> speedLineRootSignature_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> speedLinePipelineState_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> speedLineConstantBuffer_ = nullptr;
		void* speedLineMappedData_ = nullptr;
		bool speedLineInitialized_ = false;

		// LaserBeamVolume 用 PSO
		bool laserBeamInitialized_ = false;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> laserBeamRootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>  laserBeamPipelineState_;
		Microsoft::WRL::ComPtr<ID3D12Resource>       laserBeamConstantBuffer_;
		void* laserBeamMappedData_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> laserBeamVB_;
		D3D12_VERTEX_BUFFER_VIEW laserBeamVBView_{};

		//======================================================================
		// 定数バッファ / ポストエフェクト関連リソース
		//======================================================================
		// -------------------- ConstantBuffer --------------------
		Microsoft::WRL::ComPtr<ID3D12Resource> outlineConstantBuffer_;
		OutlineParameter* outlineMappedData_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> thresholdBuffer_;
		// ThresholdParam構造体
		struct ThresholdParam { float threshold_; float padding_[3]; }; // 16バイトアライメントのためにパディングを追加

		ThresholdParam* thresholdMappedData_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> renderTextureResource_;
		// RenderTexture 用 SRV のインデックス
		uint32_t renderTextureSrvIndex_ = 0;

		// ポストエフェクト用 ping-pong テクスチャ
		Microsoft::WRL::ComPtr<ID3D12Resource> postEffectTextureResource_;
		uint32_t postEffectSrvIndex_ = 0;
		//======================================================================
		// フェンス / シンクロ
		//======================================================================
		uint64_t fenceValue_ = 0;
		HANDLE   fenceEvent_ = nullptr;
		UINT     fenceVal_ = 0;
		//======================================================================
		// 外部参照 / 定数
		//======================================================================
		// -------------------- 外部参照 --------------------
		WindowsAPI* windowsAPI_ = nullptr;
		SrvManager* srvManager_ = nullptr;
		// -------------------- 定数 --------------------
		uint32_t               backBufferChange_ = 2;
		static constexpr uint32_t kRenderTextureRTVIndex_ = 2;
		static constexpr uint32_t kDepthSRVIndex_ = 11;
	};
}