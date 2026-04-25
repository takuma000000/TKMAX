#define NOMINMAX
#include "ParticleManager.h"
#include "TextureManager.h"
#include "ParticleSpawner.h"
#include "MyMath.h"
#include <numbers>
#include <algorithm>

namespace TKM {

	ParticleManager* ParticleManager::GetInstance() {
		// ParticleManagerはシングルトンとして扱う
		static ParticleManager instance;
		return &instance;
	}

	void ParticleManager::Initialize(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager, TKM::Camera* camera) {
		// 外部から受け取った描画関連の参照を保持する
		dxCommon_ = dxCommon;
		srvManager_ = srvManager;
		camera_ = camera;

		//=============================================================
		// 加速度フィールド初期化
		//=============================================================
		// 初期状態では加速度をかけない
		acc.acc_ = { 0.0f,0.0f,0.0f };

		// 加速度フィールドの有効範囲を初期化する
		acc.area_.min_ = { -1.0f,-1.00f,-1.0f };
		acc.area_.max_ = { 1.0f,1.0f,1.0f };

		//=============================================================
		// 永続マテリアルCB作成
		//=============================================================
		// 毎フレーム作り直さず、使い回すマテリアル用定数バッファを作成する
		materialCB_ = dxCommon_->CreateBufferResource(sizeof(Material));

		// 一度だけMapして、以降はmaterialCPU_へ直接書き込む
		materialCB_->Map(0, nullptr, reinterpret_cast<void**>(&materialCPU_));

		// Mapに失敗していないか確認する
		assert(materialCPU_);

		//=============================================================
		// ランダムエンジン初期化
		//=============================================================
		// ローカルの乱数エンジンを初期化する
		std::random_device seedGenerator;
		std::mt19937 randomEngine(seedGenerator());

		//=============================================================
		// 描画リソース初期化
		//=============================================================
		CreatePipeline();   // パーティクル描画用パイプラインを作成
		InitializeVD();     // 各形状の頂点データを作成
		CreateVR();         // 頂点リソースを作成
		CreateVB();         // 頂点バッファビューを作成
		WriteResource();    // 頂点データをGPUリソースへ書き込む
	}

	void ParticleManager::Update(float dt) {
		//=============================================================
		// ビルボード行列更新
		//=============================================================
		// 全パーティクルをカメラ方向へ向けるための行列を作成する
		MakeBillboardMatrix();

		//=============================================================
		// 全パーティクルグループ更新
		//=============================================================
		for (std::unordered_map<std::string, ParticleGroup>::iterator particleGroupIterator = particleGroups_.begin(); particleGroupIterator != particleGroups_.end();) {
			// 更新対象のパーティクルグループを取得する
			ParticleGroup* particleGroup = &(particleGroupIterator->second);

			// 今フレーム描画するインスタンス数をリセットする
			particleGroupIterator->second.kNumInstance_ = 0;

			//=========================================================
			// グループ内の各パーティクル更新
			//=========================================================
			for (std::list<Particle>::iterator particleIterator = particleGroup->particles_.begin(); particleIterator != particleGroup->particles_.end();) {

				//=====================================================
				// 寿命チェック
				//=====================================================
				// 生存時間を過ぎたパーティクルは削除して、描画対象から外す
				if ((*particleIterator).lifeTime_ <= (*particleIterator).currentTime_) {
					particleIterator = particleGroup->particles_.erase(particleIterator);
					continue;
				}

				//=====================================================
				// ワールド行列計算
				//=====================================================
				// パーティクルごとのスケール行列を作成する
				Matrix4x4 scaleMatrix = MyMath::MakeScaleMatrix((*particleIterator).transform_.scale_);

				// パーティクルごとの平行移動行列を作成する
				Matrix4x4 translateMatrix = MyMath::MakeTranslateMatrix((*particleIterator).transform_.translate_);

				// パーティクルごとの回転行列を作成する
				Matrix4x4 rotateMatrix = MyMath::MakeRotateMatrix((*particleIterator).transform_.rotate_);

				Matrix4x4 worldMatrix{};

				// リボンもカメラ向きにして、線状の板として見えるようにする
				if (particleGroupIterator->second.type_ == ParticleType::RIBBON) {
					worldMatrix = scaleMatrix * rotateMatrix * billboardMatrix_ * translateMatrix;
				} else {
					worldMatrix = scaleMatrix * rotateMatrix * billboardMatrix_ * translateMatrix;
				}

				// カメラ行列・ビュー行列・射影行列を取得する
				Matrix4x4 cameraMatrix = MyMath::MakeAffineMatrix(camera_->GetScale(), camera_->GetRotate(), camera_->GetTranslate());
				Matrix4x4 viewMatrix = camera_->GetViewMatrix();
				Matrix4x4 projectionMatrix = camera_->GetProjectionMatrix();

				// ワールド行列からWVP行列を作成する
				Matrix4x4 worldViewProjectionMatrix =
					MyMath::Multiply(worldMatrix, MyMath::Multiply(viewMatrix, projectionMatrix));

				//=====================================================
				// インスタンス数制限
				//=====================================================
				// 最大インスタンス数以下の場合だけ、更新して描画対象にする
				if (particleGroupIterator->second.kNumInstance_ < kNumMaxInstance_) {

					//=================================================
					// 加速度フィールド適用
					//=================================================
					// 指定範囲内にいるパーティクルだけ加速度を加える
					if (IsCollision(acc.area_, (*particleIterator).transform_.translate_)) {
						(*particleIterator).velocity_ += acc.acc_ * kDeltaTime_;
					}

					//=================================================
					// 位置・時間更新
					//=================================================
					// 速度をもとに位置を更新する
					(*particleIterator).transform_.translate_ += (*particleIterator).velocity_ * kDeltaTime_;

					// 生存時間の経過を進める
					(*particleIterator).currentTime_ += kDeltaTime_;

					//=================================================
					// インスタンスデータ書き込み
					//=================================================
					particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].wvp_ = worldViewProjectionMatrix;
					particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].World_ = worldMatrix;
					particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_ = (*particleIterator).color_;

					// 基本のアルファは寿命に応じて徐々に薄くする
					float alpha = 1.0f - ((*particleIterator).currentTime_ / (*particleIterator).lifeTime_);

					// 現在更新中のグループ名を取得する
					const std::string& g = particleGroupIterator->first;

					// LTリボンは線として繋がって見せたいので、透明化させない
					if (g == "trail_lt_ribbon") {
						alpha = 1.0f;
					}

					// 基本アルファをGPU用カラーに反映する
					particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = alpha;

					// 寿命に対する進行率を0.0f～1.0fで作る
					float t = (*particleIterator).currentTime_ / (*particleIterator).lifeTime_;
					t = std::clamp(t, 0.0f, 1.0f);

					//=================================================
					// スケール変化：タイトル爆発系
					//=================================================
					if (g == "titleExplode_ring") {
						float grow = 1.0f + 12.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "titleExplode_core") {
						float grow = 1.0f + 18.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}

					//=================================================
					// スケール変化：タイトルビーム衝突系
					//=================================================
					if (g == "titleBeamClash_ring") {
						float grow = 1.0f + 22.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "titleBeamClash_core") {
						float grow = 1.0f + 10.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}

					//=================================================
					// スケール変化：ボスワープ・注意マーク系
					//=================================================
					if (g == "bossWarp_core") {
						float grow = 1.0f + 10.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "bossWarp_dust") {
						float growY = 1.0f + 5.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.y *= growY;
					}
					if (g == "bossNoticeMark") {
						float grow = 1.0f + 2.5f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
					}

					//=================================================
					// スケール変化：ボス逃走ワープ系
					//=================================================
					if (g == "bossEscape_warpCore") {
						float shrink = 1.0f - 1.4f * kDeltaTime_;
						if (shrink < 0.0f) { shrink = 0.0f; }
						(*particleIterator).transform_.scale_.x *= shrink;
						(*particleIterator).transform_.scale_.y *= shrink;
						(*particleIterator).transform_.scale_.z *= shrink;
					}
					if (g == "bossEscape_warpRing") {
						float grow = 1.0f + 16.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "bossEscape_warpShred") {
						float stretch = 1.0f + 8.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.y *= stretch;
					}

					//=================================================
					// スケール変化：ボス登場演出系
					//=================================================
					if (g == "bossEntrance_ringShock") {
						float grow = 1.0f + 30.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "bossEntrance_ringThin") {
						float grow = 1.0f + 18.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "bossEntrance_smoke") {
						float grow = 1.0f + 4.8f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}

					//=================================================
					// スケール変化：W1特殊演出系
					//=================================================
					if (g == "w1sp_core_shell") {
						float grow = 1.0f + 10.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "w1sp_core_ring") {
						float grow = 1.0f + 6.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "w1sp_core_flash") {
						float grow = 1.0f + 18.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "w1sp_fly_shell") {
						float grow = 1.0f + 10.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "w1sp_fly_corona") {
						float grow = 1.0f + 14.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}

					//=================================================
					// スケール変化：ボススラッシュ予兆系
					//=================================================
					if (g == "boss_slash_omen_core") {
						float grow = 1.0f + 7.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "boss_slash_omen_ring") {
						float grow = 1.0f + 11.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "boss_slash_omen_pulse") {
						float grow = 1.0f + 16.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}

					//=================================================
					// スケール変化：クリアコミカルワープ系
					//=================================================
					if (g == "clearComedyWarp_core") {
						float grow = 1.0f + 30.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "clearComedyWarp_ring") {
						float grow = 1.0f + 22.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "clearComedyWarp_streak") {
						float stretch = 1.0f + 16.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.z *= stretch;
					}
					if (g == "clearComedyWarp_spark") {
						float grow = 1.0f + 6.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "clearComedyWarp_glitter") {
						float grow = 1.0f + 2.5f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}

					//=================================================
					// スケール変化：クリアコミカル転倒系
					//=================================================
					if (g == "clearComedyFall_dust") {
						float grow = 1.0f + 2.4f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "clearComedyFall_star") {
						float grow = 1.0f + 2.8f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "clearComedyFall_line") {
						float stretch = 1.0f + 10.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.z *= stretch;
					}
					if (g == "clearComedyFall_puff") {
						float grow = 1.0f + 2.8f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}

					//=================================================
					// スケール変化：クリアコミカル滑り系
					//=================================================
					if (g == "clearComedySlip_streak") {
						float stretch = 1.0f + 12.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.z *= stretch;
					}
					if (g == "clearComedySlip_spark") {
						float grow = 1.0f + 5.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "clearComedySlip_ring") {
						float grow = 1.0f + 16.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "clearComedySlip_chip") {
						float grow = 1.0f + 4.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}

					//=================================================
					// スケール変化：GAME CLEARバースト系
					//=================================================
					if (g == "clearBannerBurst_core") {
						float grow = 1.0f + 18.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "clearBannerBurst_confetti") {
						float grow = 1.0f + 2.5f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "clearBannerBurst_ray") {
						float stretch = 1.0f + 10.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.z *= stretch;
					}

					//=================================================
					// スケール変化：クリアステージ炎系
					//=================================================
					if (g == "clearStageFire_column") {
						float growY = 1.0f + 7.5f * kDeltaTime_;
						(*particleIterator).transform_.scale_.y *= growY;
					}
					if (g == "clearStageFire_top") {
						float grow = 1.0f + 4.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}

					//=================================================
					// スケール変化：ボスミサイル予兆系
					//=================================================
					if (g == "bossMissile_lane") {
						float stretch = 1.0f + 18.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.z *= stretch;
					}
					if (g == "bossMissile_grid") {
						float grow = 1.0f + 8.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "bossMissile_flash") {
						float grow = 1.0f + 24.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}

					//=================================================
					// アルファカーブ：タイトル爆発系
					//=================================================
					float a = 1.0f - t;

					if (g == "titleExplode_core") {
						a = a * a * a * a;
					} else if (g == "titleExplode_rays") {
						a = a * a;
					} else if (g == "titleExplode_ring") {
						a = std::pow(a, 1.2f);
					} else if (g == "titleExplode_debris") {
						a = std::pow(a, 1.6f);
					}

					//=================================================
					// アルファカーブ：タイトルビーム系
					//=================================================
					if (g == "titleBeam_player" || g == "titleBeam_boss") {
						a = a * a;
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}
					if (g == "titleBeamClash_core") {
						a = a * a * a;
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}
					if (g == "titleBeamClash_rays") {
						a = a * a;
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}
					if (g == "titleBeamClash_ring") {
						a = std::pow(a, 1.1f);
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}

					//=================================================
					// アルファカーブ：ボスワープ系
					//=================================================
					if (g == "bossWarp_core") {
						a = a * a * a;
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}
					if (g == "bossWarp_swirl") {
						a = std::pow(a, 1.4f);
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}
					if (g == "bossWarp_dust") {
						a = a * a;
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a * 0.75f;
					}
					if (g == "bossNoticeMark") {
						a = a * a;
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}

					//=================================================
					// アルファカーブ：ボス逃走ワープ系
					//=================================================
					if (g == "bossEscape_warpCore") {
						a = a * a * a;
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}
					if (g == "bossEscape_warpSwirl") {
						a = std::pow(a, 1.4f);
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}
					if (g == "bossEscape_warpShred") {
						a = a * a;
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a * 0.85f;
					}
					if (g == "bossEscape_warpRing") {
						a = std::pow(a, 1.1f);
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a * 0.75f;
					} else if (g == "clearComedyWarp_core") {
						a = a * a * a * a;
					} else if (g == "clearComedyWarp_ring") {
						a = std::pow(a, 1.4f);
					} else if (g == "clearComedyWarp_streak") {
						a = a * a;
					} else if (g == "clearComedyWarp_spark") {
						a = a * a * a;
					} else if (g == "clearComedyWarp_glitter") {
						a = std::pow(a, 0.8f);
					} else if (g == "clearComedyFall_dust") {
						a = a * a;
					} else if (g == "clearComedyFall_star") {
						a = a * a;
					} else if (g == "clearComedyFall_line") {
						a = a * a;
					} else if (g == "clearComedyFall_puff") {
						a = std::pow(a, 1.6f);
					} else if (g == "clearComedySlip_streak") {
						a = a * a;
					} else if (g == "clearComedySlip_spark") {
						a = a * a * a;
					} else if (g == "clearComedySlip_ring") {
						a = a * a;
					} else if (g == "clearComedySlip_chip") {
						a = std::pow(a, 1.4f);
					} else if (g == "clearBannerBurst_core") {
						a = a * a * a;
					} else if (g == "clearBannerBurst_confetti") {
						a = std::pow(a, 1.2f);
					} else if (g == "clearBannerBurst_ray") {
						a = a * a;
					} else if (g == "clearStageFire_column") {
						a = a * a;
					} else if (g == "clearStageFire_top") {
						a = a * a * a;
					}

					// 生きていて描画対象になったパーティクル数を増やす
					++particleGroupIterator->second.kNumInstance_;
				}

				// 次のパーティクルへ進める
				++particleIterator;
			}

			// 次のパーティクルグループへ進める
			++particleGroupIterator;
		}
	}

	void ParticleManager::Draw() {
		// コマンドリストを取得する
		auto* cmd = dxCommon_->GetCommandList();

		//=============================================================
		// 共通描画設定
		//=============================================================
		cmd->SetGraphicsRootSignature(rootSignature_.Get());
		cmd->SetPipelineState(graphicsPipelineState_.Get());
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//=============================================================
		// 形状ごとの頂点数取得
		//=============================================================
		const UINT vtxCountNormal = static_cast<UINT>(modelData_.vertices_.size());
		const UINT vtxCountRing = static_cast<UINT>(ringModelData_.vertices_.size());
		const UINT vtxCountCylinder = static_cast<UINT>(cylinderModelData_.vertices_.size());
		const UINT vtxCountRibbon = static_cast<UINT>(ribbonModelData_.vertices_.size());

		//=============================================================
		// 全パーティクルグループ描画
		//=============================================================
		for (auto it = particleGroups_.begin(); it != particleGroups_.end(); ++it) {
			ParticleGroup& group = it->second;

			// インスタンスがないグループは描画しない
			if (group.kNumInstance_ == 0) {
				continue;
			}

			// 形状ごとの頂点数が0なら描画しない
			if (group.type_ == ParticleType::NORMAL && vtxCountNormal == 0) { continue; }
			if (group.type_ == ParticleType::RING && vtxCountRing == 0) { continue; }
			if (group.type_ == ParticleType::CYLINDER && vtxCountCylinder == 0) { continue; }
			if (group.type_ == ParticleType::RIBBON && vtxCountRibbon == 0) { continue; }

			//=========================================================
			// マテリアルCB更新
			//=========================================================
			// materialCB_はInitialize時に永続Map済みなので、ここでは値だけ書き換える
			materialCPU_->color_ = Vector4(1, 1, 1, 1);
			materialCPU_->enableLighting_ = false;
			materialCPU_->uvTransform_ = MyMath::MakeIdentity4x4();

			//=========================================================
			// RootParameterバインド
			//=========================================================
			// マテリアルCBをPixelShaderへ渡す
			cmd->SetGraphicsRootConstantBufferView(0, materialCB_->GetGPUVirtualAddress());

			// インスタンシング用SRVをVertexShaderへ渡す
			cmd->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group.srvIndex_));

			// パーティクルのテクスチャSRVをPixelShaderへ渡す
			cmd->SetGraphicsRootDescriptorTable(2, srvManager_->GetGPUDescriptorHandle(group.materialData_.textureIndex_));

			//=========================================================
			// 形状ごとの頂点バッファ切り替え・描画
			//=========================================================
			if (group.type_ == ParticleType::NORMAL) {
				cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);
				cmd->DrawInstanced(vtxCountNormal, group.kNumInstance_, 0, 0);
			} else if (group.type_ == ParticleType::RING) {
				cmd->IASetVertexBuffers(0, 1, &ringVertexBufferView_);
				cmd->DrawInstanced(vtxCountRing, group.kNumInstance_, 0, 0);
			} else if (group.type_ == ParticleType::CYLINDER) {
				cmd->IASetVertexBuffers(0, 1, &cylinderVertexBufferView_);
				cmd->DrawInstanced(vtxCountCylinder, group.kNumInstance_, 0, 0);
			} else if (group.type_ == ParticleType::RIBBON) {
				cmd->IASetVertexBuffers(0, 1, &ribbonVertexBufferView_);
				cmd->DrawInstanced(vtxCountRibbon, group.kNumInstance_, 0, 0);
			}
		}
	}

	void ParticleManager::CreatePipeline() {
		HRESULT hr;

		//=============================================================
		// RootSignature作成
		//=============================================================
		CreateRootSignature();

		//=============================================================
		// InputLayout設定
		//=============================================================
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};

		// 頂点位置
		inputElementDescs[0].SemanticName = "POSITION";
		inputElementDescs[0].SemanticIndex = 0;
		inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		// UV座標
		inputElementDescs[1].SemanticName = "TEXCOORD";
		inputElementDescs[1].SemanticIndex = 0;
		inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		// 法線
		inputElementDescs[2].SemanticName = "NORMAL";
		inputElementDescs[2].SemanticIndex = 0;
		inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs;
		inputLayoutDesc.NumElements = _countof(inputElementDescs);

		//=============================================================
		// ブレンドステート設定
		//=============================================================
		D3D12_BLEND_DESC blendDesc{};
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

		// 加算合成を有効にして、光やエフェクト感を出す
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

		//=============================================================
		// ラスタライザーステート設定
		//=============================================================
		D3D12_RASTERIZER_DESC resterizerDesc{};

		// パーティクルは板ポリゴンなので、両面表示できるようカリングなしにする
		resterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		resterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

		//=============================================================
		// シェーダー読み込み
		//=============================================================
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(L"resources/shaders/Particle.VS.hlsl", L"vs_6_0");
		assert(vertexShaderBlob != nullptr);

		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(L"resources/shaders/Particle.PS.hlsl", L"ps_6_0");
		assert(pixelShaderBlob != nullptr);

		//=============================================================
		// DepthStencilState設定
		//=============================================================
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

		// 深度テストは有効にする
		depthStencilDesc.DepthEnable = true;

		// パーティクルは深度を書き込まない
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

		// 近いものを描画する
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		//=============================================================
		// グラフィックスパイプライン設定
		//=============================================================
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
		graphicPipelineStateDesc.pRootSignature = rootSignature_.Get();
		graphicPipelineStateDesc.InputLayout = inputLayoutDesc;
		graphicPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() };
		graphicPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() };
		graphicPipelineStateDesc.BlendState = blendDesc;
		graphicPipelineStateDesc.RasterizerState = resterizerDesc;
		graphicPipelineStateDesc.NumRenderTargets = 1;
		graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		graphicPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		graphicPipelineStateDesc.SampleDesc.Count = 1;
		graphicPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		graphicPipelineStateDesc.DepthStencilState = depthStencilDesc;
		graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		// パイプラインステートを生成する
		hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState_));
		assert(SUCCEEDED(hr));
	}

	void ParticleManager::CreateRootSignature() {
		HRESULT hr;

		//=============================================================
		// RootSignature設定
		//=============================================================
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		//=============================================================
		// DescriptorRange：PixelShader用テクスチャSRV
		//=============================================================
		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 0;
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		//=============================================================
		// DescriptorRange：VertexShader用インスタンシングSRV
		//=============================================================
		D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
		descriptorRangeForInstancing[0].BaseShaderRegister = 0;
		descriptorRangeForInstancing[0].NumDescriptors = 1;
		descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		//=============================================================
		// RootParameter作成
		//=============================================================
		D3D12_ROOT_PARAMETER rootParameters[4] = {};

		// b0：PixelShader用マテリアルCBV
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		// t0：VertexShader用インスタンシングSRV
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);

		// t0：PixelShader用テクスチャSRV
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

		// b1：PixelShader用追加CBV
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].Descriptor.ShaderRegister = 1;

		descriptionRootSignature.pParameters = rootParameters;
		descriptionRootSignature.NumParameters = _countof(rootParameters);

		//=============================================================
		// StaticSampler設定
		//=============================================================
		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};

		// テクスチャを線形補間でサンプリングする
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

		// U/W方向は繰り返し、V方向はクランプにする
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplers[0].ShaderRegister = 0;
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		descriptionRootSignature.pStaticSamplers = staticSamplers;
		descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

		//=============================================================
		// RootSignatureシリアライズ
		//=============================================================
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlog = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlog = nullptr;

		hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlog, &errorBlog);

		// シリアライズ失敗時はエラーを出力する
		if (FAILED(hr)) {
			Logger::Log(reinterpret_cast<char*>(errorBlog->GetBufferPointer()));
			assert(false);
		}

		//=============================================================
		// RootSignature生成
		//=============================================================
		rootSignature_ = nullptr;
		hr = dxCommon_->GetDevice()->CreateRootSignature(
			0,
			signatureBlog->GetBufferPointer(),
			signatureBlog->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature_)
		);
		assert(SUCCEEDED(hr));
	}

	void ParticleManager::InitializeVD() {
		//=============================================================
		// 通常パーティクル用四角形頂点作成
		//=============================================================
		modelData_.vertices_.clear();
		modelData_.vertices_.reserve(6);

		// 四角形を2枚の三角形で作る
		modelData_.vertices_.push_back({ .position_ = {1.0f,1.0f,0.0f,1.0f},.texcoord_ = {0.0f,0.0f},.normal_ = {0.0f,0.0f,1.0f} });
		modelData_.vertices_.push_back({ .position_ = {-1.0f,1.0f,0.0f,1.0f},.texcoord_ = {1.0f,0.0f},.normal_ = {0.0f,0.0f,1.0f} });
		modelData_.vertices_.push_back({ .position_ = {1.0f,-1.0f,0.0f,1.0f},.texcoord_ = {0.0f,1.0f},.normal_ = {0.0f,0.0f,1.0f} });
		modelData_.vertices_.push_back({ .position_ = {1.0f,-1.0f,0.0f,1.0f},.texcoord_ = {0.0f,1.0f},.normal_ = {0.0f,0.0f,1.0f} });
		modelData_.vertices_.push_back({ .position_ = {-1.0f,1.0f,0.0f,1.0f},.texcoord_ = {1.0f,0.0f},.normal_ = {0.0f,0.0f,1.0f} });
		modelData_.vertices_.push_back({ .position_ = {-1.0f,-1.0f,0.0f,1.0f},.texcoord_ = {1.0f,1.0f},.normal_ = {0.0f,0.0f,1.0f} });

		// 通常パーティクル用テクスチャ
		modelData_.material_.textureFilePath_ = "./resources/texture/circle.png";

		//=============================================================
		// リング頂点作成
		//=============================================================
		CreateRingVertices();
		ringModelData_.material_.textureFilePath_ = "./resources/texture/gradationLine.png";

		//=============================================================
		// シリンダー頂点作成
		//=============================================================
		CreateCylinderVertices();
		cylinderModelData_.material_.textureFilePath_ = "./resources/texture/gradationLine.png";

		//=============================================================
		// リボン頂点作成
		//=============================================================
		CreateRibbonVertices();
		ribbonModelData_.material_.textureFilePath_ = "./resources/texture/circle.png";
	}

	void ParticleManager::CreateVR() {
		// 通常パーティクル用頂点リソースを作成する
		vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData_.vertices_.size());

		// リング用頂点リソースを作成する
		ringVertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * ringModelData_.vertices_.size());

		// シリンダー用頂点リソースを作成する
		cylinderVertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * cylinderModelData_.vertices_.size());

		// リボン用頂点リソースを作成する
		ribbonVertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * ribbonModelData_.vertices_.size());
	}

	void ParticleManager::CreateVB() {
		// 通常パーティクル用頂点バッファビューを作成する
		vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
		vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices_.size());
		vertexBufferView_.StrideInBytes = sizeof(VertexData);

		// リング用頂点バッファビューを作成する
		ringVertexBufferView_.BufferLocation = ringVertexResource_->GetGPUVirtualAddress();
		ringVertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * ringModelData_.vertices_.size());
		ringVertexBufferView_.StrideInBytes = sizeof(VertexData);

		// シリンダー用頂点バッファビューを作成する
		cylinderVertexBufferView_.BufferLocation = cylinderVertexResource_->GetGPUVirtualAddress();
		cylinderVertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * cylinderModelData_.vertices_.size());
		cylinderVertexBufferView_.StrideInBytes = sizeof(VertexData);

		// リボン用頂点バッファビューを作成する
		ribbonVertexBufferView_.BufferLocation = ribbonVertexResource_->GetGPUVirtualAddress();
		ribbonVertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * ribbonModelData_.vertices_.size());
		ribbonVertexBufferView_.StrideInBytes = sizeof(VertexData);
	}

	void ParticleManager::WriteResource() {
		//=============================================================
		// 通常パーティクル頂点データ書き込み
		//=============================================================
		VertexData* vertexData = nullptr;

		// 書き込み先アドレスを取得する
		vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

		// 頂点データをコピーする
		std::memcpy(vertexData, modelData_.vertices_.data(), sizeof(VertexData) * modelData_.vertices_.size());

		//=============================================================
		// リング頂点データ書き込み
		//=============================================================
		VertexData* ringVertexData = nullptr;

		// 書き込み先アドレスを取得する
		ringVertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&ringVertexData));

		// 頂点データをコピーする
		std::memcpy(ringVertexData, ringModelData_.vertices_.data(), sizeof(VertexData) * ringModelData_.vertices_.size());

		//=============================================================
		// シリンダー頂点データ書き込み
		//=============================================================
		VertexData* cylinderVertexData = nullptr;

		// 書き込み先アドレスを取得する
		cylinderVertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&cylinderVertexData));

		// 頂点データをコピーする
		std::memcpy(cylinderVertexData, cylinderModelData_.vertices_.data(), sizeof(VertexData) * cylinderModelData_.vertices_.size());

		//=============================================================
		// リボン頂点データ書き込み
		//=============================================================
		VertexData* ribbonVertexData = nullptr;

		// 書き込み先アドレスを取得する
		ribbonVertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&ribbonVertexData));

		// 頂点データをコピーする
		std::memcpy(ribbonVertexData, ribbonModelData_.vertices_.data(), sizeof(VertexData) * ribbonModelData_.vertices_.size());
	}

	void ParticleManager::ClearAllGroups() {
		// 全グループのパーティクルを削除する
		for (auto& it : particleGroups_) {
			// パーティクル本体をクリアする
			it.second.particles_.clear();

			// 描画インスタンス数もリセットする
			it.second.kNumInstance_ = 0;
		}
	}

	void ParticleManager::CreateParticleGroup(const std::string& name, const std::string& textureFilePath, ParticleType type) {
		// すでに同名グループがある場合は何もしない
		if (particleGroups_.find(name) != particleGroups_.end()) {
			return;
		}

		//=============================================================
		// グループ情報作成
		//=============================================================
		ParticleGroup newGroup;
		newGroup.materialData_.textureFilePath_ = textureFilePath;
		newGroup.type_ = type;

		//=============================================================
		// テクスチャ読み込み・SRVインデックス取得
		//=============================================================
		TKM::TextureManager::GetInstance()->LoadTexture(textureFilePath);

		uint32_t srvIndex = TKM::TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
		newGroup.materialData_.textureIndex_ = srvIndex;

		//=============================================================
		// インスタンシング用バッファ作成
		//=============================================================
		newGroup.kNumInstance_ = kNumMaxInstance_;

		size_t bufferSize = sizeof(ParticleForGPU) * newGroup.kNumInstance_;
		newGroup.instancingResource_ = dxCommon_->CreateBufferResource(bufferSize);

		// GPUへ渡すインスタンスデータを書き込めるようにMapしておく
		newGroup.instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&newGroup.instancingData_));

		//=============================================================
		// インスタンシング用SRV作成
		//=============================================================
		uint32_t instanceSrvIndex = srvManager_->Allocate();

		srvManager_->CreateSRVforStructureBuffer(
			instanceSrvIndex,
			newGroup.instancingResource_.Get(),
			newGroup.kNumInstance_,
			sizeof(ParticleForGPU)
		);

		newGroup.srvIndex_ = instanceSrvIndex;

		// 作成したグループを登録する
		particleGroups_[name] = newGroup;
	}

	void ParticleManager::MakeBillboardMatrix() {
		// カメラの向きに合わせるため、前後を反転するY回転行列を作る
		Matrix4x4 backToFrontMatrix = MyMath::MakeRotateYMatrix(std::numbers::pi_v<float>);

		// カメラのワールド行列と組み合わせてビルボード行列を作る
		billboardMatrix_ = MyMath::Multiply(backToFrontMatrix, camera_->GetWorldMatrix());

		// ビルボードには向きだけ使いたいので、平行移動成分は消す
		billboardMatrix_.m[3][0] = 0.0f;
		billboardMatrix_.m[3][1] = 0.0f;
		billboardMatrix_.m[3][2] = 0.0f;
	}

	void ParticleManager::Emit(const std::string name, const Vector3& pos, uint32_t count) {
		// 指定されたグループが存在することを確認する
		assert(particleGroups_.find(name) != particleGroups_.end());

		// 指定グループを取得する
		ParticleGroup& group = particleGroups_[name];

		// 生成しすぎを防ぐための上限を作る
		const size_t kHardCap = std::max<size_t>(group.kNumInstance_, 200);

		for (uint32_t i = 0; i < count; ++i) {
			// 上限を超えている場合は古いパーティクルから削除する
			while (group.particles_.size() >= kHardCap) {
				group.particles_.pop_front();
			}

			// グループ名と中心位置から新しいパーティクルを作成する
			Particle newParticle = MakeNewParticle(randomEngine_, name, pos);

			// グループへ追加する
			group.particles_.push_back(newParticle);
		}
	}

	void ParticleManager::EmitWithTransform(const std::string& name, const Transform& tr, const Vector4& color, uint32_t count) {
		// 指定グループを探す
		auto it = particleGroups_.find(name);

		// グループがなければ発生させない
		if (it == particleGroups_.end()) {
			return;
		}

		ParticleGroup& group = it->second;

		// 生成しすぎを防ぐための上限を作る
		const size_t kHardCap = std::max<size_t>(group.kNumInstance_, 200);

		for (uint32_t i = 0; i < count; ++i) {
			// 上限を超えている場合は古いパーティクルから削除する
			while (group.particles_.size() >= kHardCap) {
				group.particles_.pop_front();
			}

			// 指定Transformをそのまま使うパーティクルを作成する
			Particle p{};
			p.transform_ = tr;
			p.velocity_ = { 0.0f, 0.0f, 0.0f };
			p.color_ = color;
			p.lifeTime_ = 0.1f;
			p.currentTime_ = 0.0f;

			// 生まれた瞬間の時間を0秒にする
			p.currentTime_ = 0.0f;

			// グループへ追加する
			group.particles_.push_back(p);
		}
	}

	void ParticleManager::ClearGroup(const std::string& name) {
		// 指定グループを探す
		auto it = particleGroups_.find(name);

		// グループがなければ何もしない
		if (it == particleGroups_.end()) {
			return;
		}

		// 指定グループ内のパーティクルを削除する
		it->second.particles_.clear();
	}

	ParticleManager::Particle ParticleManager::MakeNewParticle(std::mt19937& rng, const std::string& groupName, const Vector3& center) {
		// 具体的な生成内容はParticleSpawner側に任せる
		return ParticleSpawner::MakeNewParticle(rng, groupName, center);
	}

	void ParticleManager::CreateRingVertices() {
		// リングを分割数ぶんの四角形で作る
		for (uint32_t index = 0; index < kRingDivide_; ++index) {
			// 現在の角度と次の角度を求める
			float theta = index * radianPerDivide_;
			float nextTheta = (index + 1) * radianPerDivide_;

			// 現在角度と次角度のsin/cosを計算する
			float sin = std::sin(theta);
			float cos = std::cos(theta);
			float sinNext = std::sin(nextTheta);
			float cosNext = std::cos(nextTheta);

			// UVのU座標を分割位置から計算する
			float u = float(index) / float(kRingDivide_);
			float uNext = float(index + 1) / float(kRingDivide_);

			// 外側と内側の現在・次の頂点位置を作る
			Vector4 outerCurr = { -sin * kOuterRadius_, cos * kOuterRadius_, 0.0f, 1.0f };
			Vector4 outerNext = { -sinNext * kOuterRadius_, cosNext * kOuterRadius_, 0.0f, 1.0f };
			Vector4 innerCurr = { -sin * kInnerRadius_, cos * kInnerRadius_, 0.0f, 1.0f };
			Vector4 innerNext = { -sinNext * kInnerRadius_, cosNext * kInnerRadius_, 0.0f, 1.0f };

			// 1枚目の三角形
			ringModelData_.vertices_.push_back({ outerCurr, {u, 0.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData_.vertices_.push_back({ outerNext, {uNext, 0.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData_.vertices_.push_back({ innerCurr, {u, 1.0f}, {0.0f, 0.0f, 1.0f} });

			// 2枚目の三角形
			ringModelData_.vertices_.push_back({ innerCurr, {u, 1.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData_.vertices_.push_back({ outerNext, {uNext, 0.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData_.vertices_.push_back({ innerNext, {uNext, 1.0f}, {0.0f, 0.0f, 1.0f} });
		}
	}

	void ParticleManager::CreateCylinderVertices() {
		// 縦方向の分割数
		const uint32_t kHeightDivide = 8;

		// 円柱の高さ
		const float height = 2.0f;

		// 中心基準にするため、高さの半分を求める
		const float halfHeight = height / 2.0f;

		// 縦方向に分割して側面を作る
		for (uint32_t h = 0; h < kHeightDivide; ++h) {
			// 現在段と次段のY座標を計算する
			float y0 = -halfHeight + height * (float(h) / kHeightDivide);
			float y1 = -halfHeight + height * (float(h + 1) / kHeightDivide);

			// V座標を計算する
			float v0 = float(h) / kHeightDivide;
			float v1 = float(h + 1) / kHeightDivide;

			// 横方向にリング分割して側面を作る
			for (uint32_t i = 0; i < kRingDivide_; ++i) {
				// 現在と次の角度を求める
				float theta0 = i * radianPerDivide_;
				float theta1 = (i + 1) * radianPerDivide_;

				// sin/cosを計算する
				float sin0 = std::sin(theta0);
				float cos0 = std::cos(theta0);
				float sin1 = std::sin(theta1);
				float cos1 = std::cos(theta1);

				// 円周上の現在・次の位置を計算する
				float x0 = cos0 * kOuterRadius_;
				float z0 = -sin0 * kOuterRadius_;
				float x1 = cos1 * kOuterRadius_;
				float z1 = -sin1 * kOuterRadius_;

				// U座標を計算する
				float u0 = float(i) / kRingDivide_;
				float u1 = float(i + 1) / kRingDivide_;

				// 側面用の法線を計算する
				Vector3 normal0 = { cos0, 0.0f, -sin0 };
				Vector3 normal1 = { cos1, 0.0f, -sin1 };

				// 1枚目の三角形
				cylinderModelData_.vertices_.push_back({ {x0, y0, z0, 1.0f}, {u0, v0}, normal0 });
				cylinderModelData_.vertices_.push_back({ {x1, y0, z1, 1.0f}, {u1, v0}, normal1 });
				cylinderModelData_.vertices_.push_back({ {x0, y1, z0, 1.0f}, {u0, v1}, normal0 });

				// 2枚目の三角形
				cylinderModelData_.vertices_.push_back({ {x0, y1, z0, 1.0f}, {u0, v1}, normal0 });
				cylinderModelData_.vertices_.push_back({ {x1, y0, z1, 1.0f}, {u1, v0}, normal1 });
				cylinderModelData_.vertices_.push_back({ {x1, y1, z1, 1.0f}, {u1, v1}, normal1 });
			}
		}
	}

	void ParticleManager::CreateRibbonVertices() {
		// Z方向に長く、Y方向に細い板としてリボンを作る
		const float halfL = 1.0f;
		const float halfH = 0.15f;

		// 既存頂点をクリアして、三角形2枚分を確保する
		ribbonModelData_.vertices_.clear();
		ribbonModelData_.vertices_.reserve(6);

		// YZ平面上の板なので、法線は+X方向にする
		const Vector3 n = { 1.0f, 0.0f, 0.0f };

		// 1枚目の三角形
		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f,  halfH,  halfL, 1.0f}, .texcoord_ = {0.0f, 0.0f}, .normal_ = n });
		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f,  halfH, -halfL, 1.0f}, .texcoord_ = {1.0f, 0.0f}, .normal_ = n });
		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f, -halfH,  halfL, 1.0f}, .texcoord_ = {0.0f, 1.0f}, .normal_ = n });

		// 2枚目の三角形
		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f, -halfH,  halfL, 1.0f}, .texcoord_ = {0.0f, 1.0f}, .normal_ = n });
		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f,  halfH, -halfL, 1.0f}, .texcoord_ = {1.0f, 0.0f}, .normal_ = n });
		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f, -halfH, -halfL, 1.0f}, .texcoord_ = {1.0f, 1.0f}, .normal_ = n });
	}

	size_t ParticleManager::GetActiveParticleCount() const {
		// 全グループの生存パーティクル数を合計する
		size_t total_ = 0;

		for (const auto& [name_, group_] : particleGroups_) {
			total_ += group_.particles_.size();
		}

		return total_;
	}

	ParticleManager::LoadLevel ParticleManager::GetLoadLevel() const {
		// 現在の総パーティクル数を取得する
		const size_t active_ = GetActiveParticleCount();

		// 閾値に応じて負荷レベルを判定する
		if (active_ >= loadThresholdCritical_) {
			return LoadLevel::Critical;
		}
		if (active_ >= loadThresholdHigh_) {
			return LoadLevel::High;
		}
		if (active_ >= loadThresholdMedium_) {
			return LoadLevel::Medium;
		}

		return LoadLevel::Low;
	}

	uint32_t ParticleManager::GetEmitCountScaled(uint32_t baseCount, bool isPriorityEffect) const {
		// そもそも発生数が0なら何も出さない
		if (baseCount == 0) {
			return 0;
		}

		// 現在の負荷レベルを取得する
		const LoadLevel level_ = GetLoadLevel();

		//=============================================================
		// 主役演出の発生数調整
		//=============================================================
		// 主役演出は残しつつ、負荷が上がったら段階的に減らす
		if (isPriorityEffect) {
			switch (level_) {
			case LoadLevel::Low:
				return baseCount;

			case LoadLevel::Medium:
				return std::max<uint32_t>(1, baseCount * 2 / 3);

			case LoadLevel::High:
				return std::max<uint32_t>(1, baseCount / 2);

			case LoadLevel::Critical:
				return std::max<uint32_t>(1, baseCount / 3);
			}
		}

		//=============================================================
		// 背景・補助演出の発生数調整
		//=============================================================
		// 補助演出は負荷が上がった時に早めに減らす
		switch (level_) {
		case LoadLevel::Low:
			return baseCount;

		case LoadLevel::Medium:
			return std::max<uint32_t>(1, baseCount / 2);

		case LoadLevel::High:
			return std::max<uint32_t>(1, baseCount / 3);

		case LoadLevel::Critical:
			return std::max<uint32_t>(1, baseCount / 5);
		}

		return baseCount;
	}

} // namespace TKM