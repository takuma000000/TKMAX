#pragma once
#include "Vector3.h"
#include <cmath>
#include <algorithm>

//=============================================================
// AABBクラス
// 軸平行境界ボックス（AABB）の当たり判定を扱うクラス。
//=============================================================
class AABB {
public:
	AABB() = default;
	AABB(const Vector3& center, const Vector3& size) : center(center), halfSize(size * 0.5f) {}

	/// <summary>
	/// AABBの中心とサイズを設定する
	/// </summary>
	/// <param name="center"></param>
	/// <param name="size"></param>
	void Set(const Vector3& center, const Vector3& size) {
		this->center = center;
		this->halfSize = size * 0.5f;
	}

	/// <summary>
	/// 点との当たり判定
	/// </summary>
	/// <param name="point"></param>
	/// <returns></returns>
	bool IsCollidingWithPoint(const Vector3& point) const {
		return std::abs(point.x - center.x) <= halfSize.x &&
			std::abs(point.y - center.y) <= halfSize.y &&
			std::abs(point.z - center.z) <= halfSize.z;
	}

	/// <summary>
	/// AABB同士の当たり判定
	/// </summary>
	/// <param name="other"></param>
	/// <returns></returns>
	bool IsCollidingWithAABB(const AABB& other) const {
		return std::abs(center.x - other.center.x) <= (halfSize.x + other.halfSize.x) &&
			std::abs(center.y - other.center.y) <= (halfSize.y + other.halfSize.y) &&
			std::abs(center.z - other.center.z) <= (halfSize.z + other.halfSize.z);
	}

	/// <summary>
	/// 線分との当たり判定
	/// </summary>
	/// <param name="s"></param>
	/// <param name="e"></param>
	/// <returns></returns>
	bool IsIntersectSegment(const Vector3& s, const Vector3& e) const {
		Vector3 d = e - s;
		float tmin = 0.0f;
		float tmax = 1.0f;

		auto update = [&](float minB, float maxB, float start, float dir) {
			if (fabsf(dir) < 1e-6f) {
				// 動いてない軸は、スタート位置がボックス内にいなければ即アウト
				return (start >= minB && start <= maxB);
			}
			float t1 = (minB - start) / dir;
			float t2 = (maxB - start) / dir;
			if (t1 > t2) std::swap(t1, t2);
			if (tmin > t2 || tmax < t1) return false;
			tmin = std::max(tmin, t1);
			tmax = std::min(tmax, t2);
			return true;
			};

		// ✅ AABB min/max（halfSize を使う）
		float minX = center.x - halfSize.x;
		float maxX = center.x + halfSize.x;
		float minY = center.y - halfSize.y;
		float maxY = center.y + halfSize.y;
		float minZ = center.z - halfSize.z;
		float maxZ = center.z + halfSize.z;

		if (!update(minX, maxX, s.x, d.x)) return false;
		if (!update(minY, maxY, s.y, d.y)) return false;
		if (!update(minZ, maxZ, s.z, d.z)) return false;

		return true;
	}
public:
	//======================================================================
	// AABB（軸平行境界ボックス）
	//======================================================================
	Vector3 center{}; // ボックスの中心座標（ワールド）
	Vector3 halfSize{}; // ボックスの半分のサイズ（各軸方向）
};