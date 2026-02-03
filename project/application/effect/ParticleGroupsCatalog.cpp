#include "ParticleGroupsCatalog.h"

namespace TKM {
	void ParticleGroupsCatalog::RegisterGameScene(ParticleManager* pm) {
		if (!pm) { return; }

		// 共通
		pm->CreateParticleGroup("uv", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);

		/// ===== パーティクルグループの作成 =====
		// 開幕用：うっすら光が吸い込まれるリング
		pm->CreateParticleGroup("irisOpen", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
		// 花火用：放射状に飛ぶ粒（通常クアッド）
		pm->CreateParticleGroup("irisFire", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);

		// 花火用：打ち上げ＆閃光＆爆発
		pm->CreateParticleGroup("fw_launch", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
		pm->CreateParticleGroup("fw_flash", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
		pm->CreateParticleGroup("fw_burst", "./resources/firework_star.png", ParticleManager::ParticleType::NORMAL);

		// 空気の流れ(風)エフェクト
		pm->CreateParticleGroup("airStreak", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);

		// 敵スポーン
		pm->CreateParticleGroup("enemySpawn", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);

		/// === ここから被弾エフェクト用 ===
		// 中央の強いフラッシュ
		pm->CreateParticleGroup("enemyHit_flash", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 外側に広がるリング
		pm->CreateParticleGroup("enemyHit_ring", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
		// 放射状のレイ（細い光の筋）
		pm->CreateParticleGroup("enemyHit_rays", "./resources/gradationLine.png", ParticleManager::ParticleType::NORMAL);
		// 小さいスパーク
		pm->CreateParticleGroup("enemyHit_spark", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);

		/// === ここから LT弾ヒット用・さらにド派手版 ===
		// 爆心コア（まぶしい光の玉）
		pm->CreateParticleGroup("lt_nova_core", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 球状ショックウェーブ（外側のエネルギー殻）
		pm->CreateParticleGroup("lt_nova_wave", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
		// デブリ＆煙（暗い破片、煙っぽい粒）
		pm->CreateParticleGroup("lt_nova_debris", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
		// 亀裂エフェクト（空間が裂けるような光の筋）
		pm->CreateParticleGroup("lt_nova_crack", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 爆発バースト（明るい爆発の粒）
		pm->CreateParticleGroup("lt_nova_burst", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);

		/// === 敵吹っ飛び死亡専用エフェクト ===
		// 核となる小さな光の塊（中央でフッと光って消える）
		pm->CreateParticleGroup("enemyDeath_core", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 周りに飛び散る光の破片
		pm->CreateParticleGroup("enemyDeath_shard", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 残り香みたいにふわっと残る煙
		pm->CreateParticleGroup("enemyDeath_smoke", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);

		/// === ボス撃破専用エフェクト ===
		// 揺れている最中にボンボン出る中サイズ爆発
		pm->CreateParticleGroup("bossDeath_bomb", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 最後にドカンと出るリング衝撃波
		pm->CreateParticleGroup("bossDeath_ring", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
		// 倒れたあとしばらく残る大きめの煙
		pm->CreateParticleGroup("bossDeath_smoke", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);

		// 爆心コア（画面中央でドーンと光る玉）
		pm->CreateParticleGroup("bossClear_core", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 超デカいショックウェーブ（リング）
		pm->CreateParticleGroup("bossClear_ring", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
		// 四方八方に飛ぶ光の破片
		pm->CreateParticleGroup("bossClear_spark", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 重めの破片・残り香みたいな煙
		pm->CreateParticleGroup("bossClear_debris", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);

		/// === 敵飛び掛かり用エフェクト群 ===
		// 敵の飛び掛かり軌道レール
		pm->CreateParticleGroup("enemyPounceTrail", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
		// 軌道上のスパーク
		pm->CreateParticleGroup("enemyPounceSpark", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);

		/// === 蘇生核チャージ演出 ===
		// 外側を覆うエネルギー殻
		pm->CreateParticleGroup("core_charge_shell", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
		// 内向きに吸い込まれる粒子
		pm->CreateParticleGroup("core_charge_inward", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// リボン状のエネルギー帯
		pm->CreateParticleGroup("core_charge_ribbon", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
		// 中心の強いフラッシュ
		pm->CreateParticleGroup("core_charge_flash", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);

		/// === LT弾のチャージエフェクト ===
		pm->CreateParticleGroup("trail_lt_path", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);

		/// --- Boss Windup FX（予備動作）---
		// 外側を覆うリング状エネルギー
		pm->CreateParticleGroup("boss_windup_shell", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
		// 火花がパチパチ飛ぶエフェクト
		pm->CreateParticleGroup("boss_windup_crackle", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 内向きに吸い込まれる粒子
		pm->CreateParticleGroup("boss_windup_inward", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);

		/// === Boss Evil Bullet（邪悪弾 ===
		// コア：強い光（中心の発光）
		pm->CreateParticleGroup("bossEvil_core", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// モアモア：黒紫の煙
		pm->CreateParticleGroup("bossEvil_smoke", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
		// スパーク：バチバチの欠片
		pm->CreateParticleGroup("bossEvil_spark", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// オーラ輪：うっすらリング（邪悪な気配）
		pm->CreateParticleGroup("bossEvil_ring", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
		// 軌道トレイル：尾を引く粒
		pm->CreateParticleGroup("bossEvil_trail", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
		// ボディ：メインの球体部分
		pm->CreateParticleGroup("bossEvil_glow", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// ボディ：メインの球体部分
		pm->CreateParticleGroup("bossEvil_body", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
		// コロナ：外側の光輪
		pm->CreateParticleGroup("bossEvil_corona", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 軌道トレイル：尾を引く粒
		pm->CreateParticleGroup("bossEvil_trail", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	}
}