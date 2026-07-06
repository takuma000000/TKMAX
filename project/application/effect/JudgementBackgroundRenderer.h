#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;
	class Camera;

	// =============================================================
	// JudgementBackgroundRendererクラス
	// 判定背景の描画を行うクラス。
	// =============================================================
	class JudgementBackgroundRenderer {
	public:
		
		/// <summary>
		/// 初期化処理を行います。
		/// </summary>
		/// <param name="dxCommon">DirectX共通管理クラス</param>
		void Initialize(DirectXCommon* dxCommon);
		/// <summary>
		/// 毎フレームの更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		void Update(float dt);
		/// <summary>
		/// 描画処理を行います。
		/// </summary>
		/// <param name="dxCommon">DirectX共通管理クラス</param>
		/// <param name="camera">描画および判定に使用するカメラ</param>
		/// <param name="center">判定背景の中心座標（ワールド座標）</param>
		void Draw(DirectXCommon* dxCommon, const Camera& camera, const Vector3& center);

		/// <summary>
		/// 判定背景の描画を事前に行い、GPUのキャッシュを温めます。
		/// </summary>
		/// <param name="dxCommon">DirectX共通管理クラス</param>
		/// <param name="camera">描画および判定に使用するカメラ</param>
		/// <param name="center">判定背景の中心座標（ワールド座標）</param>
		void WarmUpDraw(DirectXCommon* dxCommon, const Camera& camera, const Vector3& center);

		// Setter============================================
		/// <summary>
		/// 判定背景のアクティブ状態を設定します。
		/// </summary>
		/// <param name="active">アクティブにする場合 true、それ以外は false</param>
		void SetActive(bool active) { active_ = active; }
		// ==================================================

		/// <summary>
		/// 判定背景がアクティブかどうかを取得します。
		/// </summary>
		/// <returns>アクティブな場合 true、それ以外は false</returns>
		bool IsActive() const { return active_; }

	private:
		// 判定背景の頂点構造体
		struct Vertex {
			Vector3 pos;
			Vector2 uv;
		};
		// 判定背景の定数バッファ構造体
		struct ConstBuffer {
			Matrix4x4 viewProj;
			Vector3 centerWS;
			float time;

			Vector3 camRight;
			float intensity;

			Vector3 camUp;
			float width;

			Vector3 camFwd;
			float height;
		};

		/// <summary>
		/// パイプラインステートとルートシグネチャを作成します。
		/// </summary>
		void CreatePipeline_();
		/// <summary>
		/// 頂点バッファと定数バッファを作成します。
		/// </summary>
		void CreateResources_();

	private:
		// =============================================================
		// メンバ変数
		// =============================================================
		DirectXCommon* dxCommon_ = nullptr;

		bool active_ = false;
		float time_ = 0.0f;
		float fade_ = 0.0f;

		float width_ = 95.0f;
		float height_ = 58.0f;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
		D3D12_VERTEX_BUFFER_VIEW vbView_{};

		Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
		ConstBuffer* constMap_ = nullptr;

		bool warmedUp_ = false; // GPUキャッシュを温めたかどうか

		/// <summary>
		/// 内部描画処理を行います。
		/// </summary>
		/// <param name="dxCommon">DirectX共通管理クラス</param>
		/// <param name="camera">描画および判定に使用するカメラ</param>
		/// <param name="center">判定背景の中心座標（ワールド座標）</param>
		/// <param name="intensity">描画の強度（0.0f 〜 1.0f）</param>	
		void DrawInternal_(DirectXCommon* dxCommon, const Camera& camera, const Vector3& center, float intensity);
	};
}