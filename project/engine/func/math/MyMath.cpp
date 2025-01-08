#include "MyMath.h"
#include "algorithm"

// π
float MyMath::GetPI() { return (float)M_PI; }

// 加算(Vector3)
Vector3 MyMath::Add(const Vector3& v1, const Vector3& v2) {
	return Vector3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

// 加算(Matrix4x4)
Matrix4x4 MyMath::Add(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] = m1.m[i][j] + m2.m[i][j];
		}
	}
	return result;
}

// 減算(Vector3)
Vector3 MyMath::Subtract(const Vector3& v1, const Vector3& v2) {
	return Vector3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

// 減算(Matrix4x4)
Matrix4x4 MyMath::Subtract(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] = m1.m[i][j] - m2.m[i][j];
		}
	}
	return result;
}

// 積(Vector3)
Vector3 MyMath::Multiply(const Vector3& v1, const Vector3& v2) {
	return Vector3(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
}

// 積(Matrix4x4)
Matrix4x4 MyMath::Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = {};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}
	return result;
}

// 積(Vector3,Matrix4x4)
Vector3 MyMath::Multiply(const Vector3& vec, const Matrix4x4& mat) {
	Vector3 result;

	result.x = mat.m[0][0] * vec.x + mat.m[0][1] * vec.y + mat.m[0][2] * vec.z + mat.m[0][3] * 1.0f;
	result.y = mat.m[1][0] * vec.x + mat.m[1][1] * vec.y + mat.m[1][2] * vec.z + mat.m[1][3] * 1.0f;
	result.z = mat.m[2][0] * vec.x + mat.m[2][1] * vec.y + mat.m[2][2] * vec.z + mat.m[2][3] * 1.0f;

	return result;
}

// 　スカラー倍(Vector3)
Vector3 MyMath::Multiply(float scalar, const Vector3& v) {
	return Vector3(scalar * v.x, scalar * v.y, scalar * v.z);
}

// 　長さ(ノルム)(Vector3)
float MyMath::Length(const Vector3& v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }

// 距離(Vector3同士)
float MyMath::Distance(const Vector3& v1, const Vector3& v2) {
	float dx = v2.x - v1.x;
	float dy = v2.y - v1.y;
	float dz = v2.z - v1.z;

	return sqrtf(dx * dx + dy * dy + dz * dz);
}

// 正規化(Vector3)
Vector3 MyMath::Normalize(const Vector3& v) {

	float length = Length(v);
	if (length != 0.0f) {
		float invLength = 1.0f / length;
		return Multiply(invLength, v);
	} else {
		return v;
	}
}

// 線形補間(float)
float MyMath::Lerp(float p1, float p2, float t) { return p1 + (p2 - p1) * t; }

// 線形補間(Vector3)
Vector3 MyMath::Vector3Lerp(const Vector3& p1, const Vector3& p2, float t) {
	return { Lerp(p1.x, p2.x, t), Lerp(p1.y, p2.y, t), Lerp(p1.z, p2.z, t) };
}

// 逆行列
Matrix4x4 MyMath::Inverse4x4(Matrix4x4& m) {
	Matrix4x4 matrix = {};

	float det =
		m.m[0][0] * (m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1] +
			m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][3] * m.m[3][2] -
			m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[1][3] * m.m[2][2] * m.m[3][1]) -
		m.m[0][1] * (m.m[1][0] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][0] +
			m.m[1][3] * m.m[2][0] * m.m[3][2] - m.m[1][0] * m.m[2][3] * m.m[3][2] -
			m.m[1][2] * m.m[2][0] * m.m[3][3] - m.m[1][3] * m.m[2][2] * m.m[3][0]) +
		m.m[0][2] * (m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0] +
			m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][0] * m.m[2][3] * m.m[3][1] -
			m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[1][3] * m.m[2][1] * m.m[3][0]) -
		m.m[0][3] * (m.m[1][0] * m.m[2][1] * m.m[3][2] + m.m[1][1] * m.m[2][2] * m.m[3][0] +
			m.m[1][2] * m.m[2][0] * m.m[3][1] - m.m[1][0] * m.m[2][2] * m.m[3][1] -
			m.m[1][1] * m.m[2][0] * m.m[3][2] - m.m[1][2] * m.m[2][1] * m.m[3][0]);

	float invDet = 1.0f / det;

	matrix.m[0][0] = (m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1] +
		m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][3] * m.m[3][2] -
		m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[1][3] * m.m[2][2] * m.m[3][1]) *
		invDet;
	matrix.m[0][1] = (m.m[0][1] * m.m[2][3] * m.m[3][2] + m.m[0][2] * m.m[2][1] * m.m[3][3] +
		m.m[0][3] * m.m[2][2] * m.m[3][1] - m.m[0][1] * m.m[2][2] * m.m[3][3] -
		m.m[0][2] * m.m[2][3] * m.m[3][1] - m.m[0][3] * m.m[2][1] * m.m[3][2]) *
		invDet;
	matrix.m[0][2] = (m.m[0][1] * m.m[1][2] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[3][1] +
		m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][3] * m.m[3][2] -
		m.m[0][2] * m.m[1][1] * m.m[3][3] - m.m[0][3] * m.m[1][2] * m.m[3][1]) *
		invDet;
	matrix.m[0][3] = (m.m[0][1] * m.m[1][3] * m.m[2][2] + m.m[0][2] * m.m[1][1] * m.m[2][3] +
		m.m[0][3] * m.m[1][2] * m.m[2][1] - m.m[0][1] * m.m[1][2] * m.m[2][3] -
		m.m[0][2] * m.m[1][3] * m.m[2][1] - m.m[0][3] * m.m[1][1] * m.m[2][2]) *
		invDet;

	matrix.m[1][0] = (m.m[1][0] * m.m[2][3] * m.m[3][2] + m.m[1][2] * m.m[2][0] * m.m[3][3] +
		m.m[1][3] * m.m[2][2] * m.m[3][0] - m.m[1][0] * m.m[2][2] * m.m[3][3] -
		m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[1][3] * m.m[2][0] * m.m[3][2]) *
		invDet;
	matrix.m[1][1] = (m.m[0][0] * m.m[2][2] * m.m[3][3] + m.m[0][2] * m.m[2][3] * m.m[3][0] +
		m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][0] * m.m[2][3] * m.m[3][2] -
		m.m[0][2] * m.m[2][0] * m.m[3][3] - m.m[0][3] * m.m[2][2] * m.m[3][0]) *
		invDet;
	matrix.m[1][2] = (m.m[0][0] * m.m[1][3] * m.m[3][2] + m.m[0][2] * m.m[1][0] * m.m[3][3] +
		m.m[0][3] * m.m[1][2] * m.m[3][0] - m.m[0][0] * m.m[1][2] * m.m[3][3] -
		m.m[0][2] * m.m[1][3] * m.m[3][0] - m.m[0][3] * m.m[1][0] * m.m[3][2]) *
		invDet;
	matrix.m[1][3] = (m.m[0][0] * m.m[1][2] * m.m[2][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] +
		m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][3] * m.m[2][2] -
		m.m[0][2] * m.m[1][0] * m.m[2][3] - m.m[0][3] * m.m[1][2] * m.m[2][0]) *
		invDet;

	matrix.m[2][0] = (m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0] +
		m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][0] * m.m[2][3] * m.m[3][1] -
		m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[1][3] * m.m[2][1] * m.m[3][0]) *
		invDet;
	matrix.m[2][1] = (m.m[0][0] * m.m[2][3] * m.m[3][1] + m.m[0][1] * m.m[2][0] * m.m[3][3] +
		m.m[0][3] * m.m[2][1] * m.m[3][0] - m.m[0][0] * m.m[2][1] * m.m[3][3] -
		m.m[0][1] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[2][0] * m.m[3][1]) *
		invDet;
	matrix.m[2][2] = (m.m[0][0] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][3] * m.m[3][0] +
		m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][0] * m.m[1][3] * m.m[3][1] -
		m.m[0][1] * m.m[1][0] * m.m[3][3] - m.m[0][3] * m.m[1][1] * m.m[3][0]) *
		invDet;
	matrix.m[2][3] = (m.m[0][0] * m.m[1][3] * m.m[2][1] + m.m[0][1] * m.m[1][0] * m.m[2][3] +
		m.m[0][3] * m.m[1][1] * m.m[2][0] - m.m[0][0] * m.m[1][1] * m.m[2][3] -
		m.m[0][1] * m.m[1][3] * m.m[2][0] - m.m[0][3] * m.m[1][0] * m.m[2][1]) *
		invDet;

	matrix.m[3][0] = (m.m[1][0] * m.m[2][2] * m.m[3][1] + m.m[1][1] * m.m[2][0] * m.m[3][2] +
		m.m[1][2] * m.m[2][1] * m.m[3][0] - m.m[1][0] * m.m[2][1] * m.m[3][2] -
		m.m[1][1] * m.m[2][2] * m.m[3][0] - m.m[1][2] * m.m[2][0] * m.m[3][1]) *
		invDet;
	matrix.m[3][1] = (m.m[0][0] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][2] * m.m[3][0] +
		m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][0] * m.m[2][2] * m.m[3][1] -
		m.m[0][1] * m.m[2][0] * m.m[3][2] - m.m[0][2] * m.m[2][1] * m.m[3][0]) *
		invDet;
	matrix.m[3][2] = (m.m[0][0] * m.m[1][2] * m.m[3][1] + m.m[0][1] * m.m[1][0] * m.m[3][2] +
		m.m[0][2] * m.m[1][1] * m.m[3][0] - m.m[0][0] * m.m[1][1] * m.m[3][2] -
		m.m[0][1] * m.m[1][2] * m.m[3][0] - m.m[0][2] * m.m[1][0] * m.m[3][1]) *
		invDet;
	matrix.m[3][3] = (m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0] +
		m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][0] * m.m[1][2] * m.m[2][1] -
		m.m[0][1] * m.m[1][0] * m.m[2][2] - m.m[0][2] * m.m[1][1] * m.m[2][0]) *
		invDet;

	if (det == 0) {

		return matrix;
	}

	return matrix;
}

// 平行移動行列
Matrix4x4 MyMath::MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 translateMatrix;

	translateMatrix.m[0][0] = 1.0f;
	translateMatrix.m[0][1] = 0.0f;
	translateMatrix.m[0][2] = 0.0f;
	translateMatrix.m[0][3] = 0.0f;

	translateMatrix.m[1][0] = 0.0f;
	translateMatrix.m[1][1] = 1.0f;
	translateMatrix.m[1][2] = 0.0f;
	translateMatrix.m[1][3] = 0.0f;

	translateMatrix.m[2][0] = 0.0f;
	translateMatrix.m[2][1] = 0.0f;
	translateMatrix.m[2][2] = 1.0f;
	translateMatrix.m[2][3] = 0.0f;

	translateMatrix.m[3][0] = translate.x;
	translateMatrix.m[3][1] = translate.y;
	translateMatrix.m[3][2] = translate.z;
	translateMatrix.m[3][3] = 1.0f;

	return translateMatrix;
}

// 拡大縮小行列
Matrix4x4 MyMath::MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 scaleMatrix;

	scaleMatrix.m[0][0] = scale.x;
	scaleMatrix.m[0][1] = 0.0f;
	scaleMatrix.m[0][2] = 0.0f;
	scaleMatrix.m[0][3] = 0.0f;

	scaleMatrix.m[1][0] = 0.0f;
	scaleMatrix.m[1][1] = scale.y;
	scaleMatrix.m[1][2] = 0.0f;
	scaleMatrix.m[1][3] = 0.0f;

	scaleMatrix.m[2][0] = 0.0f;
	scaleMatrix.m[2][1] = 0.0f;
	scaleMatrix.m[2][2] = scale.z;
	scaleMatrix.m[2][3] = 0.0f;

	scaleMatrix.m[3][0] = 0.0f;
	scaleMatrix.m[3][1] = 0.0f;
	scaleMatrix.m[3][2] = 0.0f;
	scaleMatrix.m[3][3] = 1.0f;

	return scaleMatrix;
}

// 座標変換
Vector3 MyMath::Transform(const Vector3& vector, const Matrix4x4& matrix) {
	Vector3 result;

	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] +
		1.0f * matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] +
		1.0f * matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] +
		1.0f * matrix.m[3][2];
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] +
		1.0f * matrix.m[3][3];
	assert(w != 0.0f);
	result.x /= w;
	result.y /= w;
	result.z /= w;

	return result;
}

Matrix4x4 MyMath::MakeRotateXMatrix(float radian) {
	Matrix4x4 rotationMatrix = {};

	rotationMatrix.m[0][0] = 1.0f;
	rotationMatrix.m[0][1] = 0.0f;
	rotationMatrix.m[0][2] = 0.0f;
	rotationMatrix.m[0][3] = 0.0f;

	rotationMatrix.m[1][0] = 0.0f;
	rotationMatrix.m[1][1] = std::cos(radian);
	rotationMatrix.m[1][2] = std::sin(radian);
	rotationMatrix.m[1][3] = 0.0f;

	rotationMatrix.m[2][0] = 0.0f;
	rotationMatrix.m[2][1] = -std::sin(radian);
	rotationMatrix.m[2][2] = std::cos(radian);
	rotationMatrix.m[2][3] = 0.0f;

	rotationMatrix.m[3][0] = 0.0f;
	rotationMatrix.m[3][1] = 0.0f;
	rotationMatrix.m[3][2] = 0.0f;
	rotationMatrix.m[3][3] = 1.0f;

	return rotationMatrix;
}

Matrix4x4 MyMath::MakeRotateYMatrix(float radian) {
	Matrix4x4 rotationMatrix = {};

	rotationMatrix.m[0][0] = std::cos(radian);
	rotationMatrix.m[0][1] = 0.0f;
	rotationMatrix.m[0][2] = -std::sin(radian);
	rotationMatrix.m[0][3] = 0.0f;

	rotationMatrix.m[1][0] = 0.0f;
	rotationMatrix.m[1][1] = 1.0f;
	rotationMatrix.m[1][2] = 0.0f;
	rotationMatrix.m[1][3] = 0.0f;

	rotationMatrix.m[2][0] = std::sin(radian);
	rotationMatrix.m[2][1] = 0.0f;
	rotationMatrix.m[2][2] = std::cos(radian);
	rotationMatrix.m[2][3] = 0.0f;

	rotationMatrix.m[3][0] = 0.0f;
	rotationMatrix.m[3][1] = 0.0f;
	rotationMatrix.m[3][2] = 0.0f;
	rotationMatrix.m[3][3] = 1.0f;

	return rotationMatrix;
}

Matrix4x4 MyMath::MakeRotateZMatrix(float radian) {
	Matrix4x4 rotationMatrix = {};

	rotationMatrix.m[0][0] = std::cos(radian);
	rotationMatrix.m[0][1] = std::sin(radian);
	rotationMatrix.m[0][2] = 0.0f;
	rotationMatrix.m[0][3] = 0.0f;

	rotationMatrix.m[1][0] = -std::sin(radian);
	rotationMatrix.m[1][1] = std::cos(radian);
	rotationMatrix.m[1][2] = 0.0f;
	rotationMatrix.m[1][3] = 0.0f;

	rotationMatrix.m[2][0] = 0.0f;
	rotationMatrix.m[2][1] = 0.0f;
	rotationMatrix.m[2][2] = 1.0f;
	rotationMatrix.m[2][3] = 0.0f;

	rotationMatrix.m[3][0] = 0.0f;
	rotationMatrix.m[3][1] = 0.0f;
	rotationMatrix.m[3][2] = 0.0f;
	rotationMatrix.m[3][3] = 1.0f;

	return rotationMatrix;
}

Matrix4x4 MyMath::MakeRotateMatrix(Vector3 rotate) {

	Matrix4x4 rotateXYZMatrix = Multiply(
		MakeRotateXMatrix(rotate.x),
		Multiply(MakeRotateYMatrix(rotate.y), MakeRotateZMatrix(rotate.x)));

	return rotateXYZMatrix;
}

// 3次元アフィン変換行列
Matrix4x4 MyMath::MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate) {
	Matrix4x4 affineMatrix;

	// 各変換行列を
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

	Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
	Matrix4x4 rotateXYZMatrix = Multiply(rotateXMatrix, Multiply(rotateYMatrix, rotateZMatrix));

	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

	// 各変換行列を合成してアフィン変換行列を作成
	affineMatrix = Multiply(scaleMatrix, Multiply(rotateXYZMatrix, translateMatrix));

	return affineMatrix;
}

// ベクトル変換
Vector3 MyMath::TransformNormal(const Vector3& v, const Matrix4x4& m) {

	Vector3 result{
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] };

	return result;
}

// ビューポート変換行列
Matrix4x4 MyMath::MakeViewportMatrix(
	float left, float top, float width, float height, float minDepth, float maxDepth) {

	Matrix4x4 viewportMatrix;

	viewportMatrix.m[0][0] = width / 2.0f;
	viewportMatrix.m[0][1] = 0.0f;
	viewportMatrix.m[0][2] = 0.0f;
	viewportMatrix.m[0][3] = 0.0f;

	viewportMatrix.m[1][0] = 0.0f;
	viewportMatrix.m[1][1] = -height / 2.0f;
	viewportMatrix.m[1][2] = 0.0f;
	viewportMatrix.m[1][3] = 0.0f;

	viewportMatrix.m[2][0] = 0.0f;
	viewportMatrix.m[2][1] = 0.0f;
	viewportMatrix.m[2][2] = maxDepth - minDepth;
	viewportMatrix.m[2][3] = 0.0f;

	viewportMatrix.m[3][0] = left + width / 2.0f;
	viewportMatrix.m[3][1] = top + height / 2.0f;
	viewportMatrix.m[3][2] = minDepth;
	viewportMatrix.m[3][3] = 1.0f;

	return viewportMatrix;
}

Matrix4x4  MyMath::MakeIdentity4x4() {

	Matrix4x4 identity;

	// 単位行列を生成する
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			identity.m[i][j] = (i == j) ? 1.0f : 0.0f;
		}
	}

	return identity;

}

Matrix4x4  MyMath::MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
	Matrix4x4 mat;

	mat.m[0][0] = 2.0f / (right - left);
	mat.m[0][1] = 0.0f;
	mat.m[0][2] = 0.0f;
	mat.m[0][3] = 0.0f;

	mat.m[1][0] = 0.0f;
	mat.m[1][1] = 2.0f / (top - bottom);
	mat.m[1][2] = 0.0f;
	mat.m[1][3] = 0.0f;

	mat.m[2][0] = 0.0f;
	mat.m[2][1] = 0.0f;
	mat.m[2][2] = 1.0f / (farClip - nearClip);
	mat.m[2][3] = 0.0f;

	mat.m[3][0] = -(right + left) / (right - left);
	mat.m[3][1] = -(top + bottom) / (top - bottom);
	mat.m[3][2] = (nearClip) / (nearClip - farClip);
	mat.m[3][3] = 1.0f;

	return mat;
}

Matrix4x4  MyMath::MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
	float tanHalfFovY = tanf(fovY * 0.5f);
	float scaleX = 1.0f / (aspectRatio * tanHalfFovY);
	float scaleY = 1.0f / tanHalfFovY;
	//float nearFarRange = farClip - nearClip;

	Matrix4x4 result;

	result.m[0][0] = scaleX;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = scaleY;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);
	result.m[3][3] = 0.0f;

	return result;
}

Matrix4x4 MyMath::DirectionToDirection(const Vector3& from, const Vector3& to)
{
	Vector3 fromNormalized = Normalize(from);
	Vector3 toNormalized = Normalize(to);

	// ベクトル間の軸を計算
	Vector3 axis = Cross(fromNormalized, toNormalized);
	float axisLength = Length(axis);

	// 方向が逆の場合
	if (axisLength == 0.0f) {
		if (Dot(fromNormalized, toNormalized) < 0.0f) {
			// 180度回転の場合、適当な垂直軸を選ぶ
			axis = Normalize(Perpendicular(fromNormalized));
		} else {
			// 同じ方向の場合、単位行列を返す
			return MakeIdentity4x4();
		}
	}

	axis = Normalize(axis);

	// ベクトル間の角度を計算
	float angle = acosf(std::clamp(Dot(fromNormalized, toNormalized), -1.0f, 1.0f));

	// 回転行列を生成
	return MakeRotateAxisAngle(axis, angle);
}


Matrix4x4 MyMath::MakeRotationMatrix(const Vector3& axis, float angle)
{
	Matrix4x4 rotationMatrix;

	// 回転軸を正規化
	Vector3 normalizedAxis = Normalize(axis);
	float x = normalizedAxis.x;
	float y = normalizedAxis.y;
	float z = normalizedAxis.z;

	// 三角関数
	float cosAngle = cosf(angle);
	float sinAngle = sinf(angle);
	float oneMinusCos = 1.0f - cosAngle;

	// 回転行列の各成分を計算 (ロドリゲスの回転公式)
	rotationMatrix.m[0][0] = cosAngle + x * x * oneMinusCos;
	rotationMatrix.m[0][1] = x * y * oneMinusCos - z * sinAngle;
	rotationMatrix.m[0][2] = x * z * oneMinusCos + y * sinAngle;
	rotationMatrix.m[0][3] = 0.0f;

	rotationMatrix.m[1][0] = y * x * oneMinusCos + z * sinAngle;
	rotationMatrix.m[1][1] = cosAngle + y * y * oneMinusCos;
	rotationMatrix.m[1][2] = y * z * oneMinusCos - x * sinAngle;
	rotationMatrix.m[1][3] = 0.0f;

	rotationMatrix.m[2][0] = z * x * oneMinusCos - y * sinAngle;
	rotationMatrix.m[2][1] = z * y * oneMinusCos + x * sinAngle;
	rotationMatrix.m[2][2] = cosAngle + z * z * oneMinusCos;
	rotationMatrix.m[2][3] = 0.0f;

	rotationMatrix.m[3][0] = 0.0f;
	rotationMatrix.m[3][1] = 0.0f;
	rotationMatrix.m[3][2] = 0.0f;
	rotationMatrix.m[3][3] = 1.0f;

	return rotationMatrix;
}

Vector3 MyMath::Cross(const Vector3& v1, const Vector3& v2)
{
	return Vector3(
		v1.y * v2.z - v1.z * v2.y, // x成分
		v1.z * v2.x - v1.x * v2.z, // y成分
		v1.x * v2.y - v1.y * v2.x  // z成分
	);
}

float MyMath::Dot(const Vector3& v1, const Vector3& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

Vector3 MyMath::Perpendicular(const Vector3& v)
{
	// 垂直なベクトルを生成するための基準ベクトル
	Vector3 reference = (std::fabs(v.x) > std::fabs(v.z)) ? Vector3(0, 0, 1) : Vector3(1, 0, 0);

	// 外積を計算して垂直なベクトルを取得
	return Cross(v, reference);
}

Matrix4x4 MyMath::MakeRotateAxisAngle(const Vector3& axis, float angle)
{
	// 必要な値を事前計算
	float cosTheta = cos(angle);
	float sinTheta = sin(angle);
	float oneMinusCos = 1.0f - cosTheta;

	// 回転行列の各要素を計算
	Matrix4x4 rotationMatrix = {};

	rotationMatrix.m[0][0] = cosTheta + axis.x * axis.x * oneMinusCos;
	rotationMatrix.m[0][1] = axis.y * axis.x * oneMinusCos + axis.z * sinTheta;
	rotationMatrix.m[0][2] = axis.z * axis.x * oneMinusCos - axis.y * sinTheta;
	rotationMatrix.m[0][3] = 0.0f;

	rotationMatrix.m[1][0] = axis.x * axis.y * oneMinusCos - axis.z * sinTheta;
	rotationMatrix.m[1][1] = cosTheta + axis.y * axis.y * oneMinusCos;
	rotationMatrix.m[1][2] = axis.z * axis.y * oneMinusCos + axis.x * sinTheta;
	rotationMatrix.m[1][3] = 0.0f;

	rotationMatrix.m[2][0] = axis.x * axis.z * oneMinusCos + axis.y * sinTheta;
	rotationMatrix.m[2][1] = axis.y * axis.z * oneMinusCos - axis.x * sinTheta;
	rotationMatrix.m[2][2] = cosTheta + axis.z * axis.z * oneMinusCos;
	rotationMatrix.m[2][3] = 0.0f;

	rotationMatrix.m[3][0] = 0.0f;
	rotationMatrix.m[3][1] = 0.0f;
	rotationMatrix.m[3][2] = 0.0f;
	rotationMatrix.m[3][3] = 1.0f;


	return rotationMatrix;
}

Quaternion MyMath::Multiply(const Quaternion& lhs, const Quaternion& rhs)
{
	Quaternion result;

	// 四元数の掛け算の公式
	result.w = lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z;
	result.x = lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y;
	result.y = lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x;
	result.z = lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w;

	return result;
}

Quaternion MyMath::IdentityQuaternion()
{
	Quaternion identity;

	// 単位四元数の定義
	identity.w = 1.0f;
	identity.x = 0.0f;
	identity.y = 0.0f;
	identity.z = 0.0f;

	return identity;
}
Quaternion MyMath::Conjugate(const Quaternion& quaternion)
{
	Quaternion conjugate;

	// 四元数の共役の計算
	conjugate.w = quaternion.w;
	conjugate.x = -quaternion.x;
	conjugate.y = -quaternion.y;
	conjugate.z = -quaternion.z;

	return conjugate;
}

float MyMath::Norm(const Quaternion& quaternion)
{
	// 四元数のノルムの計算
	return std::sqrt(
		quaternion.w * quaternion.w +
		quaternion.x * quaternion.x +
		quaternion.y * quaternion.y +
		quaternion.z * quaternion.z
	);
}

Quaternion MyMath::Normalize(const Quaternion& quaternion)
{
	Quaternion normalized;

	// ノルムを計算
	float norm = MyMath::Norm(quaternion);

	// ノルムがゼロでない場合に正規化
	if (norm > 0.0f)
	{
		normalized.w = quaternion.w / norm;
		normalized.x = quaternion.x / norm;
		normalized.y = quaternion.y / norm;
		normalized.z = quaternion.z / norm;
	} else
	{
		// ノルムがゼロの場合は単位四元数を返す
		normalized = MyMath::IdentityQuaternion();
	}

	return normalized;
}

Quaternion MyMath::Invers(const Quaternion& quaternion)
{
	Quaternion inverse;

	// 四元数のノルムの二乗を計算
	float normSquared = quaternion.w * quaternion.w +
		quaternion.x * quaternion.x +
		quaternion.y * quaternion.y +
		quaternion.z * quaternion.z;

	// ノルムがゼロでない場合に逆数を計算
	if (normSquared > 0.0f)
	{
		Quaternion conjugate = MyMath::Conjugate(quaternion);
		inverse.w = conjugate.w / normSquared;
		inverse.x = conjugate.x / normSquared;
		inverse.y = conjugate.y / normSquared;
		inverse.z = conjugate.z / normSquared;
	} else
	{
		// ノルムがゼロの場合は単位四元数を返す
		inverse = MyMath::IdentityQuaternion();
	}

	return inverse;
}

Quaternion MyMath::MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle)
{
	// 軸ベクトルを正規化する
	Vector3 normalizedAxis = Normalize(axis);

	// 角度をラジアンから半分にする
	float halfAngle = angle * 0.5f;

	// サインとコサインを計算
	float sinHalfAngle = sin(halfAngle);
	float cosHalfAngle = cos(halfAngle);

	// クォータニオンを生成
	Quaternion quaternion;
	quaternion.x = normalizedAxis.x * sinHalfAngle;
	quaternion.y = normalizedAxis.y * sinHalfAngle;
	quaternion.z = normalizedAxis.z * sinHalfAngle;
	quaternion.w = cosHalfAngle;

	return quaternion;
}

Vector3 MyMath::RotateVector(const Vector3& vector, const Quaternion& quaternion)
{
	Quaternion normalizedQuaternion = Normalize(quaternion); // クォータニオンを正規化

	Quaternion vectorQuat{ vector.x, vector.y, vector.z, 0.0f }; // ベクトルを四元数として扱う
	Quaternion inverseQuat = Conjugate(normalizedQuaternion);  // クォータニオンの共役

	Quaternion rotatedQuat = Multiply(Multiply(normalizedQuaternion, vectorQuat), inverseQuat);

	return Vector3(rotatedQuat.x, rotatedQuat.y, rotatedQuat.z);
}

// 回転行列を生成
Matrix4x4 MyMath::MakeRotateMatrix(const Quaternion& q)
{
	Matrix4x4 rotateMatrix = {};

	rotateMatrix.m[0][0] = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
	rotateMatrix.m[0][1] = 2.0f * (q.x * q.y + q.z * q.w);
	rotateMatrix.m[0][2] = 2.0f * (q.x * q.z - q.y * q.w);

	rotateMatrix.m[1][0] = 2.0f * (q.x * q.y - q.z * q.w);
	rotateMatrix.m[1][1] = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
	rotateMatrix.m[1][2] = 2.0f * (q.y * q.z + q.x * q.w);

	rotateMatrix.m[2][0] = 2.0f * (q.x * q.z + q.y * q.w);
	rotateMatrix.m[2][1] = 2.0f * (q.y * q.z - q.x * q.w);
	rotateMatrix.m[2][2] = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);

	rotateMatrix.m[3][3] = 1.0f;

	return rotateMatrix;
}

Quaternion MyMath::Slerp(const Quaternion& q1, const Quaternion& q2, float t)
{
	// クォータニオンを正規化
	Quaternion q1Norm = Normalize(q1);
	Quaternion q2Norm = Normalize(q2);

	// 内積を計算
	float dot = Dot(q1Norm, q2Norm);

	// 補間の方向を調整
	if (dot < 0.0f) {
		q2Norm.x = -q2Norm.x;
		q2Norm.y = -q2Norm.y;
		q2Norm.z = -q2Norm.z;
		q2Norm.w = -q2Norm.w;
		dot = -dot;
	}

	// しきい値 (内積が 1 に近い場合は線形補間)
	const float THRESHOLD = 0.9995f;
	if (dot > THRESHOLD) {
		// 線形補間 (Lerp)
		Quaternion result = {
			Lerp(q1Norm.x, q2Norm.x, t),
			Lerp(q1Norm.y, q2Norm.y, t),
			Lerp(q1Norm.z, q2Norm.z, t),
			Lerp(q1Norm.w, q2Norm.w, t)
		};
		return Normalize(result);
	}

	// 球面線形補間 (Slerp)
	float theta = acosf(dot);      // 角度
	float sinTheta = sinf(theta);  // sin(θ)

	float weight1 = sinf((1.0f - t) * theta) / sinTheta;
	float weight2 = sinf(t * theta) / sinTheta;

	Quaternion result = {
		weight1 * q1Norm.x + weight2 * q2Norm.x,
		weight1 * q1Norm.y + weight2 * q2Norm.y,
		weight1 * q1Norm.z + weight2 * q2Norm.z,
		weight1 * q1Norm.w + weight2 * q2Norm.w
	};

	return Normalize(result);
}

float MyMath::Dot(const Quaternion& q1, const Quaternion& q2)
{
	return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

