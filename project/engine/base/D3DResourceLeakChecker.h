#pragma once

//=============================================================
// D3DResourceLeakCheckerクラス
// DirectXリソースのリークを検出するためのクラス。
//=============================================================
namespace TKM {
	class D3DResourceLeakChecker {
	public:
		/// <summary>
		/// D3DResourceLeakCheckerのデストラクタ。DirectXリソースのリークを検出するための処理を行います。
		/// </summary>
		~D3DResourceLeakChecker();
	};
}