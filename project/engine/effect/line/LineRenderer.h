#pragma once
#define NOMINMAX
#include <vector>
#include <wrl.h>
#include <d3d12.h>

#include "DirectXCommon.h"
#include "MyMath.h"

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