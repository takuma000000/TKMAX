#pragma once
#include <string>
#include "MyMath.h"

//=============================================================
// BarrierConfigクラス
// バリア戦の設定を管理するクラス。	
// JSONファイルからバリアの挙動やコアの配置、死亡演出などのパラメータを読み込み、
// ゲーム内で参照できるようにします。
//=============================================================
class BarrierConfig {
public:
	struct BarrierShader {
		float fresnelPower_ = 2.2f;                     // フレネルの強さ
		float baseStrength_ = 0.02f;                    // バリア面全体のベース発光
		float rimStrength_ = 1.0f;                      // 輪郭部分の発光
		float alphaBase_ = 0.08f;                       // バリア面全体の透明度
		float alphaRim_ = 0.35f;                        // 輪郭部分の透明度

		Vector3 tint_ = { 1.0f, 0.72f, 0.95f };         // バリアの色味

		float hexScale_ = 8.0f;                         // 六角模様の密度
		float hexLineWidth_ = 0.03f;                    // 六角模様の線幅
		float hexGlowStrength_ = 2.4f;                  // 六角模様の発光
		float hexAlpha_ = 0.85f;                        // 六角模様の透明度

		float breakEdgeWidth_ = 0.08f;                  // 破壊境界線の幅
		float breakGlowStrength_ = 2.8f;                // 破壊境界線の発光
		float breakNoiseScale_ = 14.0f;                 // 破壊ノイズの細かさ
		Vector3 breakOrigin_ = { 0.0f, 0.0f, 0.0f };    // 破壊開始位置
	};

	struct Barrier {
		bool followCore_ = true;                        // 特殊コア位置に追従するか
		Vector3 offset_ = { 0.0f, 0.0f, 0.0f };         // バリア中心位置のオフセット
		float radius_ = 1.0f;                           // バリアの基本半径
		Vector3 shapeScale_ = { 23.0f, 23.0f, 11.0f };  // バリアの形状スケール
		Vector4 color_ = { 0.0f, 0.0f, 0.0f, 1.0f };    // Object3d側の色
		float breakDuration_ = 2.0f;                    // 破壊演出時間
		BarrierShader shader_{};                        // シェーダー用設定
	};

	struct CorePlacement {
		float barrierOuterRadius_ = 18.0f;
		float outerMargin_ = 6.0f;
		float startAngleDeg_ = -90.0f;
		float zOffset_ = -13.0f;
	};

	struct CoreDeath {
		float duration_ = 0.8f;
		float hitDirSpeed_ = 2.5f;
		Vector3 upVelocity_ = { 0.0f, 1.2f, 0.0f };
		Vector3 rotateSpeed_ = { 0.0f, 2.0f, 0.0f };
	};

	struct Core {
		std::string model_ = "barrierCore.obj";
		int count_ = 5;
		int hp_ = 3;
		Vector3 scale_ = { 1.8f, 1.8f, 1.8f };
		Vector3 colliderScale_ = { 3.1f, 3.1f, 3.1f };
		CorePlacement placement_;
		CoreDeath death_;
	};

	bool Load(const char* path);

	const Barrier& GetBarrier() const { return barrier_; }
	const Core& GetCore() const { return core_; }

private:
	Barrier barrier_;
	Core core_;
};