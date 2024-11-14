#pragma once
struct Matrix4x4 final {
	float m[4][4];

	// 加算演算子
	Matrix4x4 operator+(const Matrix4x4& other) const {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = this->m[i][j] + other.m[i][j];
			}
		}
		return result;
	}

	// 減算演算子
	Matrix4x4 operator-(const Matrix4x4& other) const {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = this->m[i][j] - other.m[i][j];
			}
		}
		return result;
	}

	// 乗算演算子
	Matrix4x4 operator*(const Matrix4x4& other) const {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = 0;
				for (int k = 0; k < 4; ++k) {
					result.m[i][j] += this->m[i][k] * other.m[k][j];
				}
			}
		}
		return result;
	}

	// スカラー乗算演算子
	Matrix4x4 operator*(float scalar) const {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = this->m[i][j] * scalar;
			}
		}
		return result;
	}

	// *= 演算子 (行列乗算)
	Matrix4x4& operator*=(const Matrix4x4& other) {
		*this = *this * other;
		return *this;
	}

	// *= 演算子 (スカラー乗算)
	Matrix4x4& operator*=(float scalar) {
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				this->m[i][j] *= scalar;
			}
		}
		return *this;
	}

	//// コピー代入演算子
	//Matrix4x4& operator=(const Matrix4x4& other) {
	//	// 自己代入チェック
	//	if (this != &other) {
	//		// 他の行列データをこの行列にコピー
	//		for (int i = 0; i < 4; ++i) {
	//			for (int j = 0; j < 4; ++j) {
	//				m[i][j] = other.m[i][j];
	//			}
	//		}
	//	}
	//	return *this;
	//}
};