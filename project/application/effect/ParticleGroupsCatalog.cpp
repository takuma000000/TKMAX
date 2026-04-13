#include "ParticleGroupsCatalog.h"

namespace TKM {
	void ParticleGroupsCatalog::RegisterScene(ParticleManager* pm) {
		if (!pm) { return; }

		/// === 共通 ===
		pm->CreateParticleGroup("uv", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		///==============================
		///=== パーティクルグループの作成 ===
		///==============================

		/// === エフェクト用 ===
		// 開幕用：うっすら光が吸い込まれるリング
		pm->CreateParticleGroup("irisOpen", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// 花火用：放射状に飛ぶ粒（通常クアッド）
		pm->CreateParticleGroup("irisFire", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		// 花火用：打ち上げ＆閃光＆爆発
		pm->CreateParticleGroup("fw_launch", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		pm->CreateParticleGroup("fw_flash", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		pm->CreateParticleGroup("fw_burst", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);
		// 空気の流れ(風)エフェクト
		pm->CreateParticleGroup("airStreak", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		// 敵スポーン
		pm->CreateParticleGroup("enemySpawn", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		/// === 被弾エフェクト用 ===
		// 中央の強いフラッシュ
		pm->CreateParticleGroup("enemyHit_flash", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 外側に広がるリング
		pm->CreateParticleGroup("enemyHit_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// 放射状のレイ（細い光の筋）
		pm->CreateParticleGroup("enemyHit_rays", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);
		// 小さいスパーク
		pm->CreateParticleGroup("enemyHit_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		/// === LT弾ヒット用・さらにド派手版 ===
		// 爆心コア（まぶしい光の玉）
		pm->CreateParticleGroup("lt_nova_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 球状ショックウェーブ（外側のエネルギー殻）
		pm->CreateParticleGroup("lt_nova_wave", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// デブリ＆煙（暗い破片、煙っぽい粒）
		pm->CreateParticleGroup("lt_nova_debris", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		// 亀裂エフェクト（空間が裂けるような光の筋）
		pm->CreateParticleGroup("lt_nova_crack", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 爆発バースト（明るい爆発の粒）
		pm->CreateParticleGroup("lt_nova_burst", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		/// === 敵吹っ飛び死亡専用エフェクト ===
		// 核となる小さな光の塊（中央でフッと光って消える）
		pm->CreateParticleGroup("enemyDeath_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 周りに飛び散る光の破片
		pm->CreateParticleGroup("enemyDeath_shard", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 残り香みたいにふわっと残る煙
		pm->CreateParticleGroup("enemyDeath_smoke", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		/// === ボス撃破専用エフェクト ===
		// 揺れている最中にボンボン出る中サイズ爆発
		pm->CreateParticleGroup("bossDeath_bomb", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 最後にドカンと出るリング衝撃波
		pm->CreateParticleGroup("bossDeath_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// 倒れたあとしばらく残る大きめの煙
		pm->CreateParticleGroup("bossDeath_smoke", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		// 爆心コア（画面中央でドーンと光る玉）
		pm->CreateParticleGroup("bossClear_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 超デカいショックウェーブ（リング）
		pm->CreateParticleGroup("bossClear_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// 四方八方に飛ぶ光の破片
		pm->CreateParticleGroup("bossClear_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 重めの破片・残り香みたいな煙
		pm->CreateParticleGroup("bossClear_debris", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		/// === 敵飛び掛かり用エフェクト群 ===
		// 敵の飛び掛かり軌道レール
		pm->CreateParticleGroup("enemyPounceTrail", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		// 軌道上のスパーク
		pm->CreateParticleGroup("enemyPounceSpark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		/// === 蘇生核チャージ演出 ===
		// 外側を覆うエネルギー殻
		pm->CreateParticleGroup("core_charge_shell", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// 内向きに吸い込まれる粒子
		pm->CreateParticleGroup("core_charge_inward", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// リボン状のエネルギー帯
		pm->CreateParticleGroup("core_charge_ribbon", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// 中心の強いフラッシュ
		pm->CreateParticleGroup("core_charge_flash", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		/// === Boss Windup FX（予備動作）===
		// 外側を覆うリング状エネルギー
		pm->CreateParticleGroup("boss_windup_shell", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// 火花がパチパチ飛ぶエフェクト
		pm->CreateParticleGroup("boss_windup_crackle", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 内向きに吸い込まれる粒子
		pm->CreateParticleGroup("boss_windup_inward", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		/// === Boss Slash Omen FX（斬撃前の邪悪な予兆） ===
		// 中心の邪核
		pm->CreateParticleGroup("boss_slash_omen_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 周囲から吸い込まれる瘴気粒
		pm->CreateParticleGroup("boss_slash_omen_inward", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		// 核を拘束する禍々しいリング
		pm->CreateParticleGroup("boss_slash_omen_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// 空間が裂けるような細い亀裂
		pm->CreateParticleGroup("boss_slash_omen_crack", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);
		// 発射直前の不穏な脈動
		pm->CreateParticleGroup("boss_slash_omen_pulse", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		/// === Boss Slash Trail（斬撃の軌道）===
		// 火花（間引き）
		pm->CreateParticleGroup("bossSlash_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		/// === Boss Slash Trail (3レイヤー) ===
		// 斬撃のメイン
		pm->CreateParticleGroup("bossSlash_main", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);
		// 斬撃のグロー
		pm->CreateParticleGroup("bossSlash_glow", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);
		// 斬撃の尾っぽ
		pm->CreateParticleGroup("bossSlash_tail", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);

		/// === タイトル用の爆発エフェクト ===
		// 爆心コア（中心の光の塊）
		pm->CreateParticleGroup("titleExplode_core", "./resources/texture/circle.png", TKM::ParticleManager::ParticleType::NORMAL);
		// 放射状のレイ（光の筋）
		pm->CreateParticleGroup("titleExplode_rays", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::CYLINDER);
		// 破片（小さな光の粒）
		pm->CreateParticleGroup("titleExplode_debris", "./resources/texture/circle.png", TKM::ParticleManager::ParticleType::NORMAL);
		// 衝撃波リング（波紋のように広がるリング）
		pm->CreateParticleGroup("titleExplode_ring", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::RING);

		/// === タイトル用のビームエフェクト ===
		// ビーム本体（線状に粒を並べて表現）
		pm->CreateParticleGroup("titleBeam_player", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::CYLINDER);
		// ビーム本体（線状に粒を並べて表現）
		pm->CreateParticleGroup("titleBeam_boss", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::CYLINDER);
		// 衝突コア（白い光の塊）
		pm->CreateParticleGroup("titleBeamClash_core", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL);
		// 放射スパーク（線っぽく）
		pm->CreateParticleGroup("titleBeamClash_rays", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::CYLINDER);
		// 衝撃波リング（リングはgradationLineの方が“波紋/衝撃波”っぽい）
		pm->CreateParticleGroup("titleBeamClash_ring", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::RING);

		/// === スタート演出のボス用エフェクト ===
		// ワープエフェクト：コア
		pm->CreateParticleGroup("bossWarp_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// ワープエフェクト：渦巻き
		pm->CreateParticleGroup("bossWarp_swirl", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		// ワープエフェクト：破片
		pm->CreateParticleGroup("bossWarp_dust", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);
		// ボスの注意マーク（！）
		pm->CreateParticleGroup("bossNoticeMark", "./resources/texture/exclamation.png", ParticleManager::ParticleType::NORMAL);
		// ワープエフェクト：コア
		pm->CreateParticleGroup("bossEscape_warpCore", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// ワープエフェクト：渦巻き
		pm->CreateParticleGroup("bossEscape_warpSwirl", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		// ワープエフェクト：破片
		pm->CreateParticleGroup("bossEscape_warpShred", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);
		// ワープエフェクト：リング
		pm->CreateParticleGroup("bossEscape_warpRing", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		/// === ボス登場用エフェクト ===
		// 渦巻き：コアを包むように渦巻くエネルギー
		pm->CreateParticleGroup("bossEntrance_ringShock", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// 細いリング：コアの周りを高速で回る細いリング
		pm->CreateParticleGroup("bossEntrance_ringThin", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// スパーク：コアから飛び散る火花
		pm->CreateParticleGroup("bossEntrance_smoke", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		// ストリーク：コアから引きずるように伸びる細い光の筋
		pm->CreateParticleGroup("bossEntrance_streak", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);
		// スパーク：コアから飛び散る火花
		pm->CreateParticleGroup("bossEntrance_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// スパーク：コアから飛び散る火花
		pm->CreateParticleGroup("bossEntrance_gather", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);	

		/// === Wave1Special用エフェクト ===
		// 送るライン上の粒
		pm->CreateParticleGroup("w1sp_stream", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 発射元の火花
		pm->CreateParticleGroup("w1sp_sender_glow", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// コア本体の見た目
		pm->CreateParticleGroup("w1sp_core_body", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// コアの周りのリング
		pm->CreateParticleGroup("w1sp_core_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// コアの周りの煙
		pm->CreateParticleGroup("w1sp_core_smoke", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		// 発射時のド派手フラッシュ
		pm->CreateParticleGroup("w1sp_core_flash", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 発射時のド派手フラッシュ
		pm->CreateParticleGroup("w1sp_core_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 敵からコアへ送るライン上の粒
		pm->CreateParticleGroup("w1sp_fly_body", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 敵からコアへ送るライン上の粒
		pm->CreateParticleGroup("w1sp_fly_tail", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);
		// 敵からコアへ送るライン上の粒
		pm->CreateParticleGroup("w1sp_fly_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 集束線の芯
		pm->CreateParticleGroup("w1sp_stream_streak", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);
		// コア内部の乱流粒
		pm->CreateParticleGroup("w1sp_core_inner", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// コアの外殻ショック
		pm->CreateParticleGroup("w1sp_core_shell", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// コア周辺の放電
		pm->CreateParticleGroup("w1sp_core_arc", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);
		// 発射時の破裂片
		pm->CreateParticleGroup("w1sp_core_burst", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 飛翔中の外殻リング
		pm->CreateParticleGroup("w1sp_fly_shell", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);
		// 飛翔中の放電
		pm->CreateParticleGroup("w1sp_fly_arc", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);
		// 飛翔中のコロナ
		pm->CreateParticleGroup("w1sp_fly_corona", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);
		// 集束線の芯
		pm->CreateParticleGroup("w1sp_stream_core", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);
		// 集束線のグロー
		pm->CreateParticleGroup("w1sp_stream_glow", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);
	}
}