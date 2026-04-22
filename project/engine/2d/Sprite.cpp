#define NOMINMAX
#include "Sprite.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {
	Sprite::Sprite() {
		++activeCount_; //アクティブスプライト数をインクリメント
	}

	Sprite::~Sprite() {
		--activeCount_; //アクティブスプライト数をデクリメント
	}

	void Sprite::SetPosition(const Vector2& position) {
		this->position_ = position; // 位置を設定
	}

	void Sprite::SetTransform(const Transform& transform) {
		this->transform_ = transform; // 変換情報を設定
	}

	void Sprite::SetRotation(float rotation) {
		this->rotation_ = rotation; // 回転角を設定
	}

	void Sprite::SetColor(const Vector4& color) {
		materialData_->color = color; // 色を設定
	}

	void Sprite::SetSize(const Vector2& size) {
		this->size_ = size; // サイズを設定
	}

	void Sprite::SetAnchorPoint(const Vector2& anchorPoint) {
		this->anchorPoint_ = anchorPoint; // アンカーポイントを設定
	}

	void Sprite::SetIsFlipX(bool isFlipX) {
		this->isFlipX_ = isFlipX; // 左右フリップを設定
	}

	void Sprite::SetIsFlipY(bool isFlipY) {
		this->isFlipY_ = isFlipY; // 上下フリップを設定
	}

	void Sprite::SetTextureLeftTop(const Vector2& textureLeftTop) {
		this->textureLeftTop_ = textureLeftTop; // テクスチャ左上座標を設定
	}

	void Sprite::SetTextureSize(const Vector2& textureSize) {
		this->textureSize_ = textureSize; // テクスチャ切り出しサイズを設定
	}

	void Sprite::SetParentScene(BaseScene* parentScene) {
		parentScene_ = parentScene; //親シーンを設定
	}

	void Sprite::SetAutoAdjustTextureSize(bool enable) {
		autoAdjustTextureSize_ = enable; // テクスチャサイズ自動調整の有効/無効を設定
	}

	void Sprite::Initialize(SpriteCommon* spriteCommon, TKM::DirectXCommon* dxCommon, const std::string textureFilePath) {
		//引数で受け取ったメンバ変数に記録する
		this->spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		this->textureFilePath_ = textureFilePath;

		//VertexResourceを作る
		vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * 4);
		//IndexResourceを作る
		indexResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * 6);
		//VertexBufferViewを作成する
		//Resourceの先頭のアドレスから使う
		vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
		//使用するResourceのサイズは頂点4つ分のサイズ
		vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
		//1頂点あたりのサイズ
		vertexBufferView_.StrideInBytes = sizeof(VertexData);
		//リソースの先頭のアドレスから使う
		indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
		//使用するリソースのサイズはインデックス6つ分のサイズ
		indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
		//インデックスはuint32_tとする
		indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
		//書き込むためのアドレスを取得
		vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
		//書き込むためのアドレスを取得
		indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

		//マテリアルリソースを作る
		materialResource_ = dxCommon->CreateBufferResource(sizeof(Material));
		//書き込むためのアドレスを取得
		materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
				// 色は白
		materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

		// 発光初期値
		materialData_->glowColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		materialData_->glowParam = Vector4(0.0f, 1.0f, 0.05f, 2.0f); // intensity, width, threshold, softness
		materialData_->glowEnabled = 0;

		// UV変換

		//座標変換行列リソースを作る
		transformationMatrixResource_ = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
		//書き込むためのアドレスを取得
		transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
		//単位行列を書き込んでおく
		transformationMatrixData_->wvp = MyMath::MakeIdentity4x4();
		// 単位行列を書き込んでおく
		transformationMatrixData_->World = MyMath::MakeIdentity4x4();

		transformSprite_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }; // スプライトの変換情報
		cameraTransform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,-10.0f} }; // カメラの変換情報

		//単位行列を書き込んでおく
		textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
	}

	void Sprite::Update() {

		float left = 0.0f - anchorPoint_.x; // アンカーポイントを考慮した左端座標
		float right = 1.0f - anchorPoint_.x; // アンカーポイントを考慮した右端座標
		float top = 0.0f - anchorPoint_.y; // アンカーポイントを考慮した上端座標
		float bottom = 1.0f - anchorPoint_.y; // アンカーポイントを考慮した下端座標

		//左右反転
		if (isFlipX_) { // 左右反転
			left = -left; // 左端と右端を入れ替え
			right = -right; // 右端と左端を入れ替え
		}
		//上下反転
		if (isFlipY_) { // 上下反転
			top = -top; // 上端と下端を入れ替え
			bottom = -bottom; // 下端と上端を入れ替え
		}

		const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetadata(textureFilePath_); // テクスチャのメタデータを取得
		float tex_left = textureLeftTop_.x / metadata.width; // テクスチャの左端UV座標
		float tex_right = (textureLeftTop_.x + textureSize_.x) / metadata.width; // テクスチャの右端UV座標
		float tex_top = textureLeftTop_.y / metadata.height; // テクスチャの上端UV座標
		float tex_bottom = (textureLeftTop_.y + textureSize_.y) / metadata.height; // テクスチャの下端UV座標

		//左下
		vertexData_[0].position = { left,bottom,0.0f,1.0f };
		vertexData_[0].texcoord = { tex_left,tex_bottom };
		vertexData_[0].normal = { 0.0f,0.0f,-1.0f };
		//左上
		vertexData_[1].position = { left,top,0.0f,1.0f };
		vertexData_[1].texcoord = { tex_left,tex_top };
		vertexData_[1].normal = { 0.0f,0.0f,-1.0f };
		//右下
		vertexData_[2].position = { right,bottom,0.0f,1.0f };
		vertexData_[2].texcoord = { tex_right,tex_bottom };
		vertexData_[2].normal = { 0.0f,0.0f,-1.0f };
		//右上
		vertexData_[3].position = { right,top,0.0f,1.0f };
		vertexData_[3].texcoord = { tex_right,tex_top };
		vertexData_[3].normal = { 0.0f,0.0f,-1.0f };

		//インデックスデータ
		indexData_[0] = 0;
		indexData_[1] = 1;
		indexData_[2] = 2;
		indexData_[3] = 1;
		indexData_[4] = 3;
		indexData_[5] = 2;

		//座標変換行列の計算
		Matrix4x4 worldMatrixSprite = MyMath::MakeAffineMatrix(transformSprite_.scale_, transformSprite_.rotate_, transformSprite_.translate_);
		// ビュー行列は単位行列
		Matrix4x4 viewMatrixSprite = MyMath::MakeIdentity4x4();
		// 射影行列は直交投影行列
		Matrix4x4 projectionMatrixSprite = MyMath::MakeOrthographicMatrix(0.0f, 0.0f, static_cast<float>(WindowsAPI::GetClientWidth()), static_cast<float>(WindowsAPI::GetClientHeight()), 0.0f, 100.0f);

		transformationMatrixData_->wvp = MyMath::Multiply(worldMatrixSprite, MyMath::Multiply(viewMatrixSprite, projectionMatrixSprite)); // WVP行列の計算
		transformationMatrixData_->World = worldMatrixSprite; // ワールド行列の設定

		//反映処理
		transformSprite_.translate_ = { position_.x,position_.y,0.0f };
		transformSprite_.rotate_ = { 0.0f,0.0f,rotation_ };
		transformSprite_.scale_ = { size_.x,size_.y,1.0f };

		if (autoAdjustTextureSize_) {
			AdjustTextureSize();
		}
	}

	void Sprite::Draw() {

		//VertexBufferViewを設定
		//Spriteの描画。変更が必要なものだけ変更する
		dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);//VBVを設定
		//TransformationMatrixCBufferの場所を設定
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());
		
		//IndexBufferViewを設定
		//IBVを設定
		dxCommon_->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

		//マテリアルCBufferの場所を設定
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
		//座標変換行列CBufferの場所を設定
		Transform uvTransformSprite{
		{1.0f,1.0f,1.0f}, // scale
		{0.0f,0.0f,0.0f}, // rotate
		{0.0f,0.0f,0.0f}, // translate
		};

		Matrix4x4 uvTransformMatrix = MyMath::MakeScaleMatrix(uvTransformSprite.scale_); // スケーリング行列を作成
		uvTransformMatrix = MyMath::Multiply(uvTransformMatrix, MyMath::MakeRotateZMatrix(uvTransformSprite.rotate_.z)); // Z回転行列を掛ける
		uvTransformMatrix = MyMath::Multiply(uvTransformMatrix, MyMath::MakeTranslateMatrix(uvTransformSprite.translate_)); // 平行移動行列を掛ける
		materialData_->uvTransform = uvTransformMatrix; // UV変換行列を更新

		//SRVのDescriptorTableの先頭を設定
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_));
		//描画。6個のインデックスを使用し1つのインスタンスを描画。その他は当面0で良い
		dxCommon_->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	void Sprite::ImGuiDebug() {
#ifdef USE_IMGUI
		// ImGui ウィジェット: スプライトの座標操作
		ImGui::Begin("Sprite");
		// スプライトの座標を操作するスライダー
		ImGui::SliderFloat2("Sprite Position", &position_.x, 0.0f, 500.0f, "%.1f");
		ImGui::End();
#endif
	}

	void Sprite::AdjustTextureSize() {
		//テクスチャデータを取得
		const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetadata(textureFilePath_);
		textureSize_.x = static_cast<float>(metadata.width); //テクスチャの幅を取得
		textureSize_.y = static_cast<float>(metadata.height); //テクスチャの高さを取得
		//画像サイズをテクスチャサイズに合わせる
		size_ = textureSize_;
	}

	void Sprite::SetGlowEnabled(bool enable) {
		materialData_->glowEnabled = enable ? 1 : 0;
	}

	void Sprite::SetGlowColor(const Vector4& color) {
		materialData_->glowColor = color;
	}

	void Sprite::SetGlowIntensity(float intensity) {
		materialData_->glowParam.x = std::max(0.0f, intensity);
	}

	void Sprite::SetGlowWidth(float width) {
		materialData_->glowParam.y = std::max(0.0f, width);
	}

	void Sprite::SetGlowThreshold(float threshold) {
		materialData_->glowParam.z = std::max(0.0f, threshold);
	}

	void Sprite::SetGlowSoftness(float softness) {
		materialData_->glowParam.w = std::max(0.001f, softness);
	}

	void Sprite::SetGlowParams(bool enable, const Vector4& color, float intensity, float width, float threshold, float softness) { // 引数::グローの有効/無効、色、強さ、幅、閾値、柔らかさ
		// グローのパラメータを一括で設定する便利な関数
		SetGlowEnabled(enable); // グローの有効/無効を設定
		SetGlowColor(color); // グローの色を設定
		SetGlowIntensity(intensity); // グローの強さを設定
		SetGlowWidth(width); // グローの幅を設定s
		SetGlowThreshold(threshold); // グローの閾値を設定
		SetGlowSoftness(softness); // グローの柔らかさを設定
	}
} //namespace TKM