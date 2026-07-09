#include "ParticleGroupsCatalog.h"

namespace TKM {
	void ParticleGroupsCatalog::RegisterScene(ParticleManager* pm) {
		// パーティクルマネージャーが無ければ登録できない
		if (!pm) { return; }

		//=========================================================
		// 共通パーティクル
		//=========================================================

		// 汎用的に使う通常粒子
		pm->CreateParticleGroup("uv", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// 開幕・空間演出系
		//=========================================================

		// アイリス開幕時に薄く吸い込まれるリング
		pm->CreateParticleGroup("irisOpen", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// アイリス開幕時の花火粒
		pm->CreateParticleGroup("irisFire", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// 花火の打ち上げ軌跡
		pm->CreateParticleGroup("fw_launch", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// 花火の爆発フラッシュ
		pm->CreateParticleGroup("fw_flash", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// 花火本体の星粒
		pm->CreateParticleGroup("fw_burst", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);

		// 空気の流れを見せる風の筋
		pm->CreateParticleGroup("airStreak", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// 敵出現時のスポーン演出
		pm->CreateParticleGroup("enemySpawn", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// 被弾エフェクト
		//=========================================================

		// 被弾中心の強いフラッシュ
		pm->CreateParticleGroup("enemyHit_flash", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 被弾位置から外へ広がるリング
		pm->CreateParticleGroup("enemyHit_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// 放射状に伸びる光の筋
		pm->CreateParticleGroup("enemyHit_rays", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// 被弾時の細かいスパーク
		pm->CreateParticleGroup("enemyHit_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// LT弾ヒット専用エフェクト
		//=========================================================

		// LT弾ヒット時の爆心コア
		pm->CreateParticleGroup("lt_nova_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// LT弾ヒット時のショックウェーブ
		pm->CreateParticleGroup("lt_nova_wave", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// LT弾ヒット時の破片・煙
		pm->CreateParticleGroup("lt_nova_debris", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// LT弾ヒット時の空間亀裂
		pm->CreateParticleGroup("lt_nova_crack", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// LT弾ヒット時の爆発粒
		pm->CreateParticleGroup("lt_nova_burst", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// 敵死亡エフェクト
		//=========================================================

		// 敵死亡時の中心光
		pm->CreateParticleGroup("enemyDeath_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 敵死亡時に飛び散る破片
		pm->CreateParticleGroup("enemyDeath_shard", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 敵死亡後に残る煙
		pm->CreateParticleGroup("enemyDeath_smoke", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// ボス撃破エフェクト
		//=========================================================

		// ボス死亡中に出る中サイズ爆発
		pm->CreateParticleGroup("bossDeath_bomb", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// ボス死亡時のリング衝撃波
		pm->CreateParticleGroup("bossDeath_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// ボス死亡後に残る煙
		pm->CreateParticleGroup("bossDeath_smoke", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// ボス撃破完了時の爆心コア
		pm->CreateParticleGroup("bossClear_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// ボス撃破完了時の巨大ショックウェーブ
		pm->CreateParticleGroup("bossClear_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// ボス撃破完了時に飛び散る光の破片
		pm->CreateParticleGroup("bossClear_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// ボス撃破完了時の重めの破片・煙
		pm->CreateParticleGroup("bossClear_debris", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// 敵飛び掛かりエフェクト
		//=========================================================

		// 敵の飛び掛かり軌道を見せる粒
		pm->CreateParticleGroup("enemyPounceTrail", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// 飛び掛かり軌道上のスパーク
		pm->CreateParticleGroup("enemyPounceSpark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// 蘇生核チャージ演出
		//=========================================================

		// チャージ中の外殻リング
		pm->CreateParticleGroup("core_charge_shell", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// 中心へ吸い込まれる粒子
		pm->CreateParticleGroup("core_charge_inward", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// チャージ中のエネルギー帯
		pm->CreateParticleGroup("core_charge_ribbon", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// チャージ中心の強いフラッシュ
		pm->CreateParticleGroup("core_charge_flash", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// ボスミサイル発射システム演出
		//=========================================================

		// ミサイル発射口の点火ノード
		pm->CreateParticleGroup("bossMissile_node", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// ミサイル発射方向に伸びるレーン
		pm->CreateParticleGroup("bossMissile_lane", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// 兵器UI風の骨組みリング
		pm->CreateParticleGroup("bossMissile_grid", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// 発射口から吹き出すスパーク
		pm->CreateParticleGroup("bossMissile_jet", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// ミサイル発射直前の点火フラッシュ
		pm->CreateParticleGroup("bossMissile_flash", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// ボス斬撃予兆エフェクト
		//=========================================================

		// 斬撃予兆の中心核
		pm->CreateParticleGroup("boss_slash_omen_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 周囲から吸い込まれる瘴気粒
		pm->CreateParticleGroup("boss_slash_omen_inward", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// 中心核を囲う不穏なリング
		pm->CreateParticleGroup("boss_slash_omen_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// 空間が裂けるような亀裂
		pm->CreateParticleGroup("boss_slash_omen_crack", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// 発射直前の脈動
		pm->CreateParticleGroup("boss_slash_omen_pulse", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// ボス斬撃軌道エフェクト
		//=========================================================

		// 斬撃軌道上の火花
		pm->CreateParticleGroup("bossSlash_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 斬撃本体
		pm->CreateParticleGroup("bossSlash_main", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// 斬撃の発光部分
		pm->CreateParticleGroup("bossSlash_glow", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);

		// 斬撃の尾を引く部分
		pm->CreateParticleGroup("bossSlash_tail", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// タイトル爆発エフェクト
		//=========================================================

		// タイトル爆発の中心光
		pm->CreateParticleGroup("titleExplode_core", "./resources/texture/circle.png", TKM::ParticleManager::ParticleType::NORMAL);

		// タイトル爆発の放射レイ
		pm->CreateParticleGroup("titleExplode_rays", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::CYLINDER);

		// タイトル爆発の破片粒
		pm->CreateParticleGroup("titleExplode_debris", "./resources/texture/circle.png", TKM::ParticleManager::ParticleType::NORMAL);

		// タイトル爆発の衝撃波リング
		pm->CreateParticleGroup("titleExplode_ring", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::RING);

		//=========================================================
		// タイトルビームエフェクト
		//=========================================================

		// プレイヤー側ビーム本体
		pm->CreateParticleGroup("titleBeam_player", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::CYLINDER);

		// ボス側ビーム本体
		pm->CreateParticleGroup("titleBeam_boss", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::CYLINDER);

		// ビーム衝突中心の光
		pm->CreateParticleGroup("titleBeamClash_core", "./resources/texture/circle2.png", TKM::ParticleManager::ParticleType::NORMAL);

		// ビーム衝突時の放射スパーク
		pm->CreateParticleGroup("titleBeamClash_rays", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::CYLINDER);

		// ビーム衝突時の衝撃波リング
		pm->CreateParticleGroup("titleBeamClash_ring", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::RING);

		//=========================================================
		// スタート演出ボス用エフェクト
		//=========================================================

		// ボスワープ出現の中心光
		pm->CreateParticleGroup("bossWarp_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// ボスワープ出現の渦巻き粒
		pm->CreateParticleGroup("bossWarp_swirl", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// ボスワープ出現の破片
		pm->CreateParticleGroup("bossWarp_dust", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// ボスが驚いた時の注意マーク
		pm->CreateParticleGroup("bossNoticeMark", "./resources/texture/exclamation.png", ParticleManager::ParticleType::NORMAL);

		// ボス逃走ワープの中心光
		pm->CreateParticleGroup("bossEscape_warpCore", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// ボス逃走ワープの渦巻き粒
		pm->CreateParticleGroup("bossEscape_warpSwirl", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// ボス逃走ワープの裂ける破片
		pm->CreateParticleGroup("bossEscape_warpShred", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// ボス逃走ワープのリング
		pm->CreateParticleGroup("bossEscape_warpRing", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		//=========================================================
		// ボス登場用エフェクト
		//=========================================================

		// ボス登場時のリング衝撃波
		pm->CreateParticleGroup("bossEntrance_ringShock", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// ボス登場時の細い高速リング
		pm->CreateParticleGroup("bossEntrance_ringThin", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// ボス登場時の煙
		pm->CreateParticleGroup("bossEntrance_smoke", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// ボス登場時の細い光の筋
		pm->CreateParticleGroup("bossEntrance_streak", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// ボス登場時の火花
		pm->CreateParticleGroup("bossEntrance_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// ボス登場時に集まる光粒
		pm->CreateParticleGroup("bossEntrance_gather", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// クリア祝福エフェクト
		//=========================================================

		// クリア祝福の中心光
		pm->CreateParticleGroup("clearCelebrate_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// クリア祝福の大量スパーク
		pm->CreateParticleGroup("clearCelebrate_spark", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);

		// クリア祝福の強いレイ
		pm->CreateParticleGroup("clearCelebrate_ray", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// Wave1Special エフェクト
		//=========================================================

		// 敵からコアへ送るライン上の粒
		pm->CreateParticleGroup("w1sp_stream", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 送信元の発光
		pm->CreateParticleGroup("w1sp_sender_glow", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア本体の粒
		pm->CreateParticleGroup("w1sp_core_body", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア周囲のリング
		pm->CreateParticleGroup("w1sp_core_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// 特殊コア周囲の煙
		pm->CreateParticleGroup("w1sp_core_smoke", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア発射時のフラッシュ
		pm->CreateParticleGroup("w1sp_core_flash", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア発射時のスパーク
		pm->CreateParticleGroup("w1sp_core_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア飛翔中の本体粒
		pm->CreateParticleGroup("w1sp_fly_body", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア飛翔中の尾
		pm->CreateParticleGroup("w1sp_fly_tail", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア飛翔中のスパーク
		pm->CreateParticleGroup("w1sp_fly_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 集束線のストリーク
		pm->CreateParticleGroup("w1sp_stream_streak", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア内部の乱流粒
		pm->CreateParticleGroup("w1sp_core_inner", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア外殻のショックリング
		pm->CreateParticleGroup("w1sp_core_shell", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// 特殊コア周辺の放電
		pm->CreateParticleGroup("w1sp_core_arc", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア発射時の破裂片
		pm->CreateParticleGroup("w1sp_core_burst", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア飛翔中の外殻リング
		pm->CreateParticleGroup("w1sp_fly_shell", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// 特殊コア飛翔中の放電
		pm->CreateParticleGroup("w1sp_fly_arc", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// 特殊コア飛翔中のコロナ
		pm->CreateParticleGroup("w1sp_fly_corona", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 集束線の芯
		pm->CreateParticleGroup("w1sp_stream_core", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);

		// 集束線のグロー
		pm->CreateParticleGroup("w1sp_stream_glow", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// GAME CLEAR 表示時エフェクト
		//=========================================================

		// GAME CLEAR表示時の祝福コア
		pm->CreateParticleGroup("clearBannerBurst_core", "./resources/texture/flower.png", ParticleManager::ParticleType::NORMAL);

		// GAME CLEAR表示時の紙吹雪
		pm->CreateParticleGroup("clearBannerBurst_confetti", "./resources/texture/flower.png", ParticleManager::ParticleType::NORMAL);

		// GAME CLEAR表示時のレイ
		pm->CreateParticleGroup("clearBannerBurst_ray", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// クリアコミカル逃走：三体のワープ出現
		//=========================================================

		// ワープ出現の中心光
		pm->CreateParticleGroup("clearComedyWarp_core", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 空間が開くリング
		pm->CreateParticleGroup("clearComedyWarp_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// 前後に突き抜けるワープストリーク
		pm->CreateParticleGroup("clearComedyWarp_streak", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// ワープ破裂時の細かい火花
		pm->CreateParticleGroup("clearComedyWarp_spark", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// ワープ後の余韻きらめき
		pm->CreateParticleGroup("clearComedyWarp_glitter", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// クリアコミカル逃走：雑魚Bのスリップ
		//=========================================================

		// 滑った瞬間のスピード線
		pm->CreateParticleGroup("clearComedySlip_streak", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// 滑った瞬間の火花
		pm->CreateParticleGroup("clearComedySlip_spark", "./resources/texture/flower.png", ParticleManager::ParticleType::NORMAL);

		// 滑った瞬間のリング
		pm->CreateParticleGroup("clearComedySlip_ring", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::RING);

		// 滑った瞬間の小片
		pm->CreateParticleGroup("clearComedySlip_chip", "./resources/texture/flower.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// クリアコミカル逃走：雑魚B転倒
		//=========================================================

		// 転倒時の土煙
		pm->CreateParticleGroup("clearComedyFall_dust", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);

		// 転倒時の漫画風星
		pm->CreateParticleGroup("clearComedyFall_star", "./resources/texture/flower.png", ParticleManager::ParticleType::NORMAL);

		// 転倒時の衝撃線
		pm->CreateParticleGroup("clearComedyFall_line", "./resources/texture/flower.png", ParticleManager::ParticleType::NORMAL);

		// 転倒後の余韻パフ
		pm->CreateParticleGroup("clearComedyFall_puff", "./resources/texture/flower.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// クリアシーン：ライブ風ファイアー柱
		//=========================================================

		// 炎柱のコア
		pm->CreateParticleGroup("clearStageFire_column", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// 炎柱上部のグロー
		pm->CreateParticleGroup("clearStageFire_top", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		//=========================================================
		// プレイヤー専用パーティクル
		//=========================================================

		// ジェット煙パーティクル
		pm->CreateParticleGroup("jetSmoke", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

		// LT弾の軌跡パーティクル
		pm->CreateParticleGroup("trail_lt", "./resources/texture/circle2.png", ParticleManager::ParticleType::NORMAL);

		// LB弾のキラキラ演出パーティクル
		pm->CreateParticleGroup("trail_lb_glitter", "./resources/texture/firework_star.png", ParticleManager::ParticleType::NORMAL);

		// LB弾の稲光メイン演出パーティクル
		pm->CreateParticleGroup("trail_lb_bolt_main", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);

		// LB弾の稲光コア演出パーティクル
		pm->CreateParticleGroup("trail_lb_bolt_core", "./resources/texture/gradationLine.png", ParticleManager::ParticleType::NORMAL);
	}
}