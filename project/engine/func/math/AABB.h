#pragma once
#undef max
#undef min

#include "MyMath.h"
#include <cmath>
#include <algorithm>

//=============================================================
// AABBクラス
// 軸平行境界ボックス（AABB）の当たり判定を扱うクラス。
//=============================================================
class AABB {
public:
	AABB() = default;
	AABB(const Vector3& center, const Vector3& size)
		: center_(center), halfSize_(size * 0.5f) {
	}

	/// <summary>
	/// AABBの設定
	/// </summary>
	/// <param name="center"></param>
	/// <param name="size"></param>
	void Set(const Vector3& center, const Vector3& size) {
		this->center_ = center;
		this->halfSize_ = size * 0.5f;
	}

	/// <summary>
	/// 点との当たり判定
	/// </summary>
	/// <param name="point"></param>
	/// <returns></returns>
	bool IsCollidingWithPoint(const Vector3& point) const {
		return std::abs(point.x - center_.x) <= halfSize_.x &&
			std::abs(point.y - center_.y) <= halfSize_.y &&
			std::abs(point.z - center_.z) <= halfSize_.z;
	}

	/// <summary>
	/// AABB同士の当たり判定
	/// </summary>
	/// <param name="other"></param>
	/// <returns></returns>
	bool IsCollidingWithAABB(const AABB& other) const {
		return std::abs(center_.x - other.center_.x) <= (halfSize_.x + other.halfSize_.x) &&
			std::abs(center_.y - other.center_.y) <= (halfSize_.y + other.halfSize_.y) &&
			std::abs(center_.z - other.center_.z) <= (halfSize_.z + other.halfSize_.z);
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

		// AABB min/max（halfSize を使う）
		float minX = center_.x - halfSize_.x;
		float maxX = center_.x + halfSize_.x;
		float minY = center_.y - halfSize_.y;
		float maxY = center_.y + halfSize_.y;
		float minZ = center_.z - halfSize_.z;
		float maxZ = center_.z + halfSize_.z;

		if (!update(minX, maxX, s.x, d.x)) return false;
		if (!update(minY, maxY, s.y, d.y)) return false;
		if (!update(minZ, maxZ, s.z, d.z)) return false;

		return true;
	}

	const Vector3& GetCenter() const { return center_; }
	const Vector3& GetHalfSize() const { return halfSize_; }

private:
	//======================================================================
	// AABB（軸平行境界ボックス）
	//======================================================================
	Vector3 center_{};    // ボックスの中心座標（ワールド）
	Vector3 halfSize_{};  // ボックスの半分のサイズ（各軸方向）
};