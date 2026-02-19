#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <cmath>
#include <algorithm>


#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

//=============================================================
// Ease名前空間
// イージング計算関数およびトゥイーン処理をまとめた名前空間。
//=============================================================
namespace Ease {

	// 0..1 を安全にクランプ
	inline float Clamp01(float v) { return std::max(0.0f, std::min(1.0f, v)); }

	// 種類
	enum class Type {
		Linear,
		InSine, OutSine, InOutSine,
		InQuad, OutQuad, InOutQuad,
		InCubic, OutCubic, InOutCubic,
		InQuart, OutQuart, InOutQuart,
		InQuint, OutQuint, InOutQuint,
		InExpo, OutExpo, InOutExpo,
		InCirc, OutCirc, InOutCirc,
		InBack, OutBack, InOutBack,
		OutElastic,
	};

	// ---- イージング本体（t は 0..1） ----
	inline float Linear(float t) { return t; }

	// Sine
	inline float InSine(float t) { return 1.0f - cosf((t * 3.1415926535f) * 0.5f); }
	inline float OutSine(float t) { return sinf((t * 3.1415926535f) * 0.5f); }
	inline float InOutSine(float t) {
		return -0.5f * (cosf(3.1415926535f * t) - 1.0f);
	}

	// Quad / Cubic / Quart / Quint
	inline float InQuad(float t) { return t * t; }
	inline float OutQuad(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }
	inline float InOutQuad(float t) { return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) * 0.5f; }

	inline float InCubic(float t) { return t * t * t; }
	inline float OutCubic(float t) { return 1.0f - powf(1.0f - t, 3.0f); }
	inline float InOutCubic(float t) {
		return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) * 0.5f;
	}

	inline float InQuart(float t) { return t * t * t * t; }
	inline float OutQuart(float t) { return 1.0f - powf(1.0f - t, 4.0f); }
	inline float InOutQuart(float t) {
		return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 4.0f) * 0.5f;
	}

	inline float InQuint(float t) { return t * t * t * t * t; }
	inline float OutQuint(float t) { return 1.0f - powf(1.0f - t, 5.0f); }
	inline float InOutQuint(float t) {
		return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 5.0f) * 0.5f;
	}

	// Expo
	inline float InExpo(float t) { return (t <= 0.0f) ? 0.0f : powf(2.0f, 10.0f * (t - 1.0f)); }
	inline float OutExpo(float t) { return (t >= 1.0f) ? 1.0f : 1.0f - powf(2.0f, -10.0f * t); }
	inline float InOutExpo(float t) {
		if (t <= 0.0f) return 0.0f;
		if (t >= 1.0f) return 1.0f;
		return t < 0.5f ? powf(2.0f, 20.0f * t - 10.0f) * 0.5f
			: (2.0f - powf(2.0f, -20.0f * t + 10.0f)) * 0.5f;
	}

	// Circ
	inline float InCirc(float t) { return 1.0f - sqrtf(1.0f - t * t); }
	inline float OutCirc(float t) { float f = t - 1.0f; return sqrtf(1.0f - f * f); }
	inline float InOutCirc(float t) {
		return (t < 0.5f)
			? 0.5f * (1.0f - sqrtf(1.0f - 4.0f * t * t))
			: 0.5f * (sqrtf(1.0f - powf(-2.0f * t + 2.0f, 2.0f)) + 1.0f);
	}

	// Back（“溜め/反動”）
	inline float InBack(float t) {
		const float c1 = 1.70158f, c3 = c1 + 1.0f;
		return c3 * t * t * t - c1 * t * t;
	}
	inline float OutBack(float t) {
		const float c1 = 1.70158f, c3 = c1 + 1.0f;
		float f = t - 1.0f;
		return 1.0f + c3 * powf(f, 3.0f) + c1 * powf(f, 2.0f);
	}
	inline float InOutBack(float t) {
		const float c1 = 1.70158f, c2 = c1 * 1.525f;
		return (t < 0.5f)
			? (powf(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) * 0.5f
			: (powf(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (2.0f * t - 2.0f) + c2) + 2.0f) * 0.5f;
	}
	// Elastic（“弾む”）
	inline float OutElastic(float t) {
		if (t <= 0.0f) return 0.0f;
		if (t >= 1.0f) return 1.0f;

		const float pi = 3.1415926535f;
		const float c4 = (2.0f * pi) / 3.0f;

		return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
	}

	// 種類→関数のディスパッチ
	inline float Eval(Type type, float t) {
		t = Clamp01(t);
		switch (type) {
		default:
		case Type::Linear:      return Linear(t);
		case Type::InSine:      return InSine(t);
		case Type::OutSine:     return OutSine(t);
		case Type::InOutSine:   return InOutSine(t);
		case Type::InQuad:      return InQuad(t);
		case Type::OutQuad:     return OutQuad(t);
		case Type::InOutQuad:   return InOutQuad(t);
		case Type::InCubic:     return InCubic(t);
		case Type::OutCubic:    return OutCubic(t);
		case Type::InOutCubic:  return InOutCubic(t);
		case Type::InQuart:     return InQuart(t);
		case Type::OutQuart:    return OutQuart(t);
		case Type::InOutQuart:  return InOutQuart(t);
		case Type::InQuint:     return InQuint(t);
		case Type::OutQuint:    return OutQuint(t);
		case Type::InOutQuint:  return InOutQuint(t);
		case Type::InExpo:      return InExpo(t);
		case Type::OutExpo:     return OutExpo(t);
		case Type::InOutExpo:   return InOutExpo(t);
		case Type::InCirc:      return InCirc(t);
		case Type::OutCirc:     return OutCirc(t);
		case Type::InOutCirc:   return InOutCirc(t);
		case Type::InBack:      return InBack(t);
		case Type::OutBack:     return OutBack(t);
		case Type::InOutBack:   return InOutBack(t);
		case Type::OutElastic:  return OutElastic(t);
		}
	}

	// 値 a→b を t(0..1) で補間（便利関数）
	inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

	// Tween構造体
	// 時間ベースのトゥイーン便利構造体：Update(dt) で値を返す
	struct Tween {
		float start = 0.0f;
		float end = 1.0f;
		float dur = 1.0f;     // 秒
		float t = 0.0f;     // 0..1
		Type  type = Type::Linear;

		void Reset(float s, float e, float durationSec, Type ty) {
			start = s; end = e; dur = std::max(0.0001f, durationSec); type = ty; t = 0.0f;
		}
		// dt: 経過秒。戻り値: 現在値
		float Update(float dt) {
			t = Clamp01(t + dt / dur);
			float k = Eval(type, t);
			return Lerp(start, end, k);
		}
		bool Finished() const { return t >= 1.0f; }
	};
}