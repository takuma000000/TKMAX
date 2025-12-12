#pragma once
#define NOMINMAX
#include <vector>
#include <wrl.h>
#include <d3d12.h>

#include "DirectXCommon.h"
#include "MyMath.h"
#include "AABB.h"

class LineRenderer {
public:
	struct Color {
		float r, g, b, a;
	};

	static LineRenderer* GetInstance();

	void Initialize(DirectXCommon* dxCommon, size_t maxLines = 1024);
	void BeginFrame(); // 1フレーム目頭で呼ぶ（バッファをクリア）
	void AddLine(const Vector3& a, const Vector3& b, const Color& c);
	void Draw(const Matrix4x4& viewProj); // GameScene::Draw の中から呼ぶ

	/// <summary>
	/// AABB（軸平行境界ボックス）を追加する。
	/// </summary>
	/// <param name="center"></param>
	/// <param name="size"></param>
	/// <param name="color"></param>
	void AddAABB(const Vector3& center, const Vector3& size, const Color& color);
	/// <summary>
	/// AABB（軸平行境界ボックス）を追加し、指定したレイと交差していたら色を変える。
	/// </summary>
	/// <param name="center"></param>
	/// <param name="size"></param>
	/// <param name="rayOrigin"></param>
	/// <param name="rayDir"></param>
	/// <param name="normalColor"></param>
	/// <param name="hitColor"></param>
	void AddAABBWithRayHighlight(
		const Vector3& center,
		const Vector3& size,
		const Vector3& rayOrigin,
		const Vector3& rayDir,   // 正規化前でも OK
		const Color& normalColor,
		const Color& hitColor
	);
private:
	LineRenderer() = default;
	~LineRenderer() = default;

	struct Vertex {
		Vector3 pos;
		Color   col;
	};

	DirectXCommon* dx_ = nullptr;

	std::vector<Vertex> vertices_;
	size_t maxVertices_ = 0;  // maxLines * 2

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};

	// PSO / RootSignature とかもここに持つ（Object3d と共通でもOK）
	Microsoft::WRL::ComPtr<ID3D12PipelineState>       pso_;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>       rootSig_;

	void CreateBuffer();
	void CreatePipeline();
};