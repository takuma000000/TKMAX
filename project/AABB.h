#pragma once
#include "Vector3.h"
#include <cmath>

class AABB {
public:
	AABB() = default;

	AABB(const Vector3& center, const Vector3& size)
		: center(center), halfSize(size * 0.5f) {
	}

	// 中心座標と半サイズからAABBを構成
	void Set(const Vector3& center, const Vector3& size) {
		this->center = center;
		this->halfSize = size * 0.5f;
	}

	// 点との当たり判定
	bool IsCollidingWithPoint(const Vector3& point) const {
		return std::abs(point.x - center.x) <= halfSize.x &&
			std::abs(point.y - center.y) <= halfSize.y &&
			std::abs(point.z - center.z) <= halfSize.z;
	}

	// AABB同士の当たり判定
	bool IsCollidingWithAABB(const AABB& other) const {
		return std::abs(center.x - other.center.x) <= (halfSize.x + other.halfSize.x) &&
			std::abs(center.y - other.center.y) <= (halfSize.y + other.halfSize.y) &&
			std::abs(center.z - other.center.z) <= (halfSize.z + other.halfSize.z);
	}

public:
	Vector3 center{};
	Vector3 halfSize{};
};
