#pragma once
namespace TKM{
	class DirectXCommon;
}


/// 画面ポストエフェクトの基底クラス
class BaseEffect {
public:
	virtual ~BaseEffect() = default;

	/// DX 共通を渡して初期化
	virtual void Initialize(TKM::DirectXCommon* dx) {
		dxCommon_ = dx;
	}

	/// 毎フレーム更新
	virtual void Update(float dt) = 0;

	/// フレーム末尾での描画処理（RenderTexture → Swapchain）
	/// ※ 実際に呼ぶ場所はメインループ側
	virtual void Draw() = 0;

protected:
	TKM::DirectXCommon* dxCommon_ = nullptr;
};