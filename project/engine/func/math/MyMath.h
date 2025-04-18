#pragma once
#include "Matrix4x4.h"
#include "Vector3.h"
#include <cassert>
#define _USE_MATH_DEFINES
#include <cmath>
#include <Quaternion.h>

class MyMath {
public:
	// π
	static float GetPI();
	// 加算(Vector3)
	static Vector3 Add(const Vector3& v1, const Vector3& v2);

	// 加算(Matrix4x4)
	static Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

	// 減算(Vector3)
	static Vector3 Subtract(const Vector3& v1, const Vector3& v2);

	// 減算(Matrix4x4)
	static Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

	// 　スカラー倍(Vector3)
	static Vector3 Multiply(float scalar, const Vector3& v);

	// 積(Vector3)
	static Vector3 Multiply(const Vector3& v1, const Vector3& v2);

	// 積(Matrix4x4)
	static Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

	// 積(Vector3,Matrix4x4)
	static Vector3 Multiply(const Vector3& vec, const Matrix4x4& mat);

	// 　長さ(ノルム)(Vector3)
	static float Length(const Vector3& v);

	// 距離(Vector3同士)
	static float Distance(const Vector3& v1, const Vector3& v2);

	// 正規化(Vector3)
	static Vector3 Normalize(const Vector3& v);

	// 逆行列
	static Matrix4x4 Inverse4x4(Matrix4x4& matrix);

	// 平行移動行列
	static Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

	// 拡大縮小行列
	static Matrix4x4 MakeScaleMatrix(const Vector3& scale);

	// 座標変換
	static Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

	// X回転行列
	static Matrix4x4 MakeRotateXMatrix(float radian);
	// Y回転行列
	static Matrix4x4 MakeRotateYMatrix(float radian);
	// Z回転行列
	static Matrix4x4 MakeRotateZMatrix(float radian);

	// 回転行列
	static Matrix4x4 MakeRotateMatrix(Vector3 rotate);

	// 3次元アフィン変換行列
	static Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate);

	// 線形補間関数(float)
	static float Lerp(float p1, float p2, float t);

	// 線形補間関数(Vector3)
	static Vector3 Vector3Lerp(const Vector3& p1, const Vector3& p2, float t);

	// ベクトル変換
	static Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

	// ビューポート変換行列
	static Matrix4x4 MakeViewportMatrix(
		float left, float top, float width, float height, float minDepth, float maxDepth);

	//単位行列
	static Matrix4x4 MakeIdentity4x4();

	static Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

	static Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

	///----------Quaternion-----------

	//Quaternion積
	static Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs);
	//単位Quaternionを返す
	static Quaternion IdentityQuaternion();
	//共役Quaternionを返す
	static Quaternion Conjugate(const Quaternion& quaternion);
	//Quaternionのnormを返す
	static float Norm(const Quaternion& quaternion);
	//正規化したQuaternionを返す
	static Quaternion NormalizeQua(const Quaternion& quaternion);
	//逆Quaternionを返す
	static Quaternion Invers(const Quaternion& quaternion);

	//任意回転軸を表すQuaternionの作成
	static Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle);
	//ベクトルをQuaternionで回転させた結果のベクトルを求める
	static Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion);
	//Quaternionから回転行列を求める
	static Matrix4x4 MakeRotateMatrix(const Quaternion& q);
	//Quaternionの球面線形補間
	static Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t);
	//Quaternionの内積
	static float Dot(const Quaternion& q1, const Quaternion& q2);


	///-------------------------------
};