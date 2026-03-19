#pragma once

/// <summary>
/// 3次元ベクトル
/// </summary>
//=============================================================
// Vector3構造体
// 3次元ベクトルを表す構造体。
//=============================================================
struct Vector3 final {
	float x;
	float y;
	float z;

	float lengthSquared() const { return x * x + y * y + z * z; }

	// ベクトルの内積を計算する
	float Dot(const Vector3& other) const { return x * other.x + y * other.y + z * other.z; }

	// 加算演算子
	Vector3 operator+(const Vector3& other) const {
		return Vector3(this->x + other.x, this->y + other.y, this->z + other.z);
	}

	// 減算演算子
	Vector3 operator-(const Vector3& other) const {
		return Vector3(this->x - other.x, this->y - other.y, this->z - other.z);
	}

	// 内積演算子
	float operator*(const Vector3& other) const {
		return (this->x * other.x + this->y * other.y + this->z * other.z);
	}

	// スカラー減算演算子
	Vector3 operator-(float scalar) const {
		return Vector3(this->x - scalar, this->y - scalar, this->z - scalar);
	}

	// スカラー乗算演算子
	Vector3 operator*(float scalar) const {
		return Vector3(this->x * scalar, this->y * scalar, this->z * scalar);
	}

	// スカラー除算演算子
	Vector3 operator/(float scalar) const {
		return Vector3(this->x / scalar, this->y / scalar, this->z / scalar);
	}

	// 単項マイナス演算子（符号反転）
	Vector3 operator-() const {
		return Vector3(-x, -y, -z);
	}

	// 加算代入演算子
	Vector3& operator+=(const Vector3& other) {
		this->x += other.x;
		this->y += other.y;
		this->z += other.z;
		return *this;
	}

	// スカラー乗算代入演算子
	Vector3& operator*=(float scalar) {
		this->x *= scalar;
		this->y *= scalar;
		this->z *= scalar;
		return *this;
	}

	// スカラー除算代入演算子
	Vector3& operator/=(float scalar) {
		this->x /= scalar;
		this->y /= scalar;
		this->z /= scalar;
		return *this;
	}

	Vector3& operator-=(const Vector3& other) {
		this->x -= other.x;
		this->y -= other.y;
		this->z -= other.z;
		return *this;
	}
};