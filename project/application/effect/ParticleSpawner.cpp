#define NOMINMAX
#include "ParticleSpawner.h"
#include "TextureManager.h"
#include "MyMath.h"
#include <numbers>
#include <algorithm>

namespace TKM {

	ParticleManager::Particle ParticleSpawner::MakeNewParticle(std::mt19937& rng, const std::string& groupName, const Vector3& center) {
		ParticleManager::Particle p{}; // 新規パーティクル

		// 共通：発生位置を中心±オフセット
		std::uniform_real_distribution<float> offXY(-0.3f, 0.3f);
		std::uniform_real_distribution<float> offZ(-0.3f, 0.3f);
		Vector3 offset{ offXY(rng), offXY(rng) * 0.6f, offZ(rng) };
		p.transform_.translate_ = center + offset;

		if (groupName == "irisOpen") { //── 開幕用：中心から“放出”する粒 ──
			// ── 開幕用：中心へ“吸い込む”柔らかい粒 ──
			// 方向＝中心へ向かう（= -offset の方向）
			Vector3 dir = MyMath::Normalize(-offset);
			std::uniform_real_distribution<float> spd(0.06f, 0.14f);
			float s = spd(rng);
			p.velocity_ = dir * s;

			// 小さめ＆短命、青白〜白
			std::uniform_real_distribution<float> scl(0.6f, 1.2f);
			float sc = scl(rng);
			p.transform_.scale_ = { sc, sc, sc };

			float life = std::uniform_real_distribution<float>(0.35f, 0.65f)(rng);
			p.lifeTime_ = life; p.currentTime_ = 0.0f;

			float c = std::uniform_real_distribution<float>(0.85f, 1.0f)(rng);
			p.color_ = { 0.85f * c, 0.90f * c, 1.00f, 1.0f };
		} else if (groupName == "irisFire") { //── 開幕用：中心から“放出”する粒 ──
			// --- 花火演出（画面全体に放射） ---
			// 広い範囲にオフセット
			std::uniform_real_distribution<float> offXY(-20.0f, 20.0f);
			std::uniform_real_distribution<float> offZ(-20.0f, 20.0f);
			Vector3 offset = { offXY(rng), offXY(rng), offZ(rng) };

			// ランダム方向ベクトル（正規化）
			Vector3 dir = MyMath::Normalize(offset);

			// 強めの速度
			std::uniform_real_distribution<float> spd(0.5f, 2.5f);
			p.velocity_ = dir * spd(rng);

			// 大小ランダム
			std::uniform_real_distribution<float> scl(0.8f, 1.6f);
			float sc = scl(rng);
			p.transform_.scale_ = { sc, sc, sc };

			// 寿命長め（広く散っても見えるように）
			std::uniform_real_distribution<float> life(0.8f, 1.5f);
			p.lifeTime_ = life(rng);
			p.currentTime_ = 0.0f;

			// 明るくランダムカラー（花火っぽく）
			float hue = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.9f + 0.1f * sin(hue * 6.283f);
			float g = 0.8f + 0.2f * cos(hue * 6.283f);
			float b = 1.0f - 0.3f * sin(hue * 3.142f);
			p.color_ = { r, g, b, 1.0f };
		} else if (groupName == "jetSmoke") {
			// ─────────────────────────────
			// jetSmoke v3：クララン推進痕（煙じゃない）
			//  - 濁り(INK) : 暗いインク雲（低α）
			//  - リボン(RIBBON) : ヌメリの筋（細長い帯）
			//  ※ 白禁止 / 自機を隠さない（中心空け＆後方発生）
			// ─────────────────────────────

			auto frand = [&rng](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			float kind = frand(0.0f, 1.0f);

			float ang = frand(0.0f, 2.0f * std::numbers::pi_v<float>);
			float c = std::cos(ang);
			float s = std::sin(ang);

			// 自機の輪郭上に出さない：ドーナツ＋さらに後ろ
			float r = frand(0.25f, 0.60f);

			Vector3 localPos = {
				c * r,
				s * r * 0.40f + frand(-0.02f, 0.08f),
				frand(-1.20f, -0.85f)   // かなり後ろ
			};
			p.transform_.translate_ = center + localPos;

			// 接線方向（渦）＋少し上へ
			Vector3 swirl = { -s, c * 0.40f, 0.0f };

			// “煙”みたいに速く飛ばさない（濁りは遅い）
			Vector3 baseV = {};
			baseV += swirl * frand(0.4f, 1.8f);
			baseV.x += frand(-0.14f, 0.14f);
			baseV.y += frand(0.10f, 0.30f);
			baseV.z += frand(-14.0f, -8.0f);

			if (kind < 0.55f) {
				// ============================
				// ① INK：暗い濁り雲（“煙”を捨てる）
				// ============================
				float sc = frand(0.35f, 0.85f);
				p.transform_.scale_ = { sc, sc, sc };

				Vector3 v = baseV;
				// 濁りはさらに遅い
				v.z += frand(2.0f, 5.0f);
				p.velocity_ = v;

				p.lifeTime_ = frand(0.80f, 1.60f);
				p.currentTime_ = 0.0f;

				// 暗い青紫インク（背景が白でも沈まない）
				float t = frand(0.0f, 1.0f);
				float rCol = 0.18f + 0.06f * t;
				float gCol = 0.22f + 0.08f * t;
				float bCol = 0.30f + 0.18f * t;

				// 低α（自機を殺さない）
				float a = frand(0.08f, 0.16f);
				p.color_ = { rCol, gCol, bCol, a };

			} else {
				// ============================
				// ② RIBBON：ヌメリの筋（ありきたり回避の主役）
				// ============================
				// “帯”にする：薄く・長く
				float thin = frand(0.06f, 0.14f);
				float len = frand(0.70f, 1.60f);
				p.transform_.scale_ = { thin, thin, len }; // ※エンジン側の見え方に合わせて xyz 入れ替えOK

				Vector3 v = baseV;
				// 帯は少しだけ速く後ろへ
				v.z += frand(-10.0f, -6.0f);
				p.velocity_ = v;

				p.lifeTime_ = frand(0.18f, 0.42f);
				p.currentTime_ = 0.0f;

				// 生体っぽい紫〜ピンク（でも白にしない）
				float t = frand(0.0f, 1.0f);
				float rCol = 0.45f + 0.25f * t;
				float gCol = 0.30f + 0.10f * t;
				float bCol = 0.60f + 0.30f * t;

				float a = frand(0.10f, 0.22f);
				p.color_ = { rCol, gCol, bCol, a };
			}
		} else if (groupName == "trail_rb") {
			// RB：青いスパーク（クールで安定）
			std::uniform_real_distribution<float> velX(-0.03f, 0.03f);
			std::uniform_real_distribution<float> velY(-0.03f, 0.03f);
			std::uniform_real_distribution<float> velZ(-2.0f, -0.6f);
			p.velocity_ = { velX(rng), velY(rng), velZ(rng) };

			float sc = std::uniform_real_distribution<float>(0.10f, 0.22f)(rng);
			p.transform_.scale_ = { sc, sc, sc };
			p.lifeTime_ = std::uniform_real_distribution<float>(0.20f, 0.35f)(rng);
			p.currentTime_ = 0.0f;

			// 青～水色
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 col = { 0.2f + 0.1f * t, 0.5f + 0.3f * t, 1.0f };
			p.color_ = { 0.1f, 0.3f, 1.0f, 1.0f };  // 鮮やかな青（R10%, G30%, B100%）
		} else if (groupName == "trail_lb") {
			// LB：黄〜金色の尾（エネルギー感）
			std::uniform_real_distribution<float> velX(-0.02f, 0.02f);
			std::uniform_real_distribution<float> velY(-0.02f, 0.02f);
			std::uniform_real_distribution<float> velZ(-2.2f, -0.8f);
			p.velocity_ = { velX(rng), velY(rng), velZ(rng) };

			float sc = std::uniform_real_distribution<float>(0.12f, 0.26f)(rng);
			p.transform_.scale_ = { sc, sc, sc };
			p.lifeTime_ = std::uniform_real_distribution<float>(0.25f, 0.45f)(rng);
			p.currentTime_ = 0.0f;

			// 明るい黄～金色
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 col = { 1.0f, 0.8f + 0.2f * t, 0.1f + 0.2f * t };
			p.color_ = { 1.0f, 0.9f, 0.1f, 1.0f };  // ほぼ純黄色（R100%, G90%, B10%）
		} else if (groupName == "trail_rt") {
			// RT：赤い尾（情熱・攻撃的）
			std::uniform_real_distribution<float> velX(-0.015f, 0.015f);
			std::uniform_real_distribution<float> velY(-0.015f, 0.015f);
			std::uniform_real_distribution<float> velZ(-2.8f, -1.2f);
			p.velocity_ = { velX(rng), velY(rng), velZ(rng) };

			float sc = std::uniform_real_distribution<float>(0.20f, 0.40f)(rng);
			p.transform_.scale_ = { sc, sc, sc };
			p.lifeTime_ = std::uniform_real_distribution<float>(0.35f, 0.60f)(rng);
			p.currentTime_ = 0.0f;

			// 純赤～オレンジ寄り
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 col = { 1.0f, 0.2f + 0.3f * t, 0.1f };
			p.color_ = { 1.0f, 0.05f, 0.05f, 1.0f };  // 強い赤（R100%, G5%, B5%）
		} else if (groupName == "damageSpark") { //── 故障スパーク ──
			// 放射状に高速で飛ぶ、短命、明るくチカチカ
			std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
			Vector3 v = { dir(rng), dir(rng) * 0.6f, dir(rng) };
			Vector3 n = (MyMath::Length(v) > 0.001f) ? MyMath::Normalize(v) : Vector3{ 0,0,1 };
			float spd = std::uniform_real_distribution<float>(1.2f, 2.4f)(rng);
			p.velocity_ = n * spd;

			float sc = std::uniform_real_distribution<float>(0.08f, 0.18f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.18f, 0.35f)(rng);
			p.currentTime_ = 0.0f;

			// 強い黄～白（火花）
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 1.0f;
			float g = 0.85f + 0.15f * t;
			float b = 0.1f + 0.2f * (1.0f - t);
			p.color_ = { r, g, b, 1.0f };
		} else if (groupName == "crashFlame") {
			// 基本は上向き。横に少し拡散して“躍る”感じ
			std::uniform_real_distribution<float> velX(-0.06f, 0.06f);
			std::uniform_real_distribution<float> velY(1.20f, 2.40f); // ↑ ぐっと強く
			std::uniform_real_distribution<float> velZ(-0.06f, 0.06f);
			p.velocity_ = { velX(rng), velY(rng), velZ(rng) };

			// 粒は大きめ（炎舌が見えるサイズ）
			float sc = std::uniform_real_distribution<float>(0.28f, 0.55f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			// ほんの少し長命（バースト直後の見栄えを持たせる）
			p.lifeTime_ = std::uniform_real_distribution<float>(0.35f, 0.60f)(rng);
			p.currentTime_ = 0.0f;

			// “灼熱コア”～“黄炎”に振る（加算でギラッと出る）
			// tが小さいほど赤寄りコア、tが大きいほど黄寄り
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 1.0f;
			float g = 0.55f + 0.40f * (1.0f - t);  // 0.95..0.55
			float b = 0.05f + 0.20f * t;           // 0.05..0.25
			p.color_ = { r, g, b, 1.0f };
		} else if (groupName == "fallStreak") {

			// 線を少し長く
			p.transform_.scale_ = { 0.10f, 2.6f, 1.0f };
			p.transform_.rotate_ = { 0.0f, 0.0f, 0.0f };
			p.transform_.translate_ = center;

			// ほぼ垂直にゆっくり落下（見やすさ重視）
			float vx = ((rand() % 40) - 20) / 800.0f;     // ±0.025
			float vz = ((rand() % 40) - 20) / 1200.0f;    // ±0.016
			float vy = -(1.2f + (rand() % 40) / 100.0f);  // -1.2 ～ -1.6
			p.velocity_ = { vx, vy, vz };

			// 深紅
			p.color_ = { 1.35f, 0.10f, 0.06f, 1.0f };

			// 画面下まで十分に保つ寿命
			p.lifeTime_ = 10.0f + (rand() % 80) / 100.0f; // 10.0 ～ 10.8秒
			p.currentTime_ = 0.0f;

			return p;
		} else if (groupName == "fallStreakUp") {

			// 下→上に向かうストリーク
			p.transform_.scale_ = { 0.10f, 2.6f, 1.0f };
			p.transform_.rotate_ = { 0.0f, 0.0f, 0.0f };
			p.transform_.translate_ = center;

			// ゆっくり上昇（反対方向）
			float vx = ((rand() % 40) - 20) / 800.0f;
			float vz = ((rand() % 40) - 20) / 1200.0f;
			float vy = (1.2f + (rand() % 40) / 100.0f);  // +1.2 ～ +1.6
			p.velocity_ = { vx, vy, vz };

			// 色は上昇らしく少し淡く
			p.color_ = { 1.2f, 0.25f, 0.15f, 1.0f };

			// 寿命長め
			p.lifeTime_ = 10.0f + (rand() % 80) / 100.0f;
			p.currentTime_ = 0.0f;

			return p;
		} else if (groupName == "fw_launch") {
			// 上にまっすぐ伸びる光の線
			p.transform_.scale_ = { 1.5f, 3.5f, 1.5f };
			p.velocity_ = { 0, 18.0f + (float)(rand() % 5), 0 };
			p.color_ = { 1.0f, 0.8f, 0.3f, 1.0f };
			p.lifeTime_ = 0.40f;
		} else if (groupName == "fw_flash") {
			// 爆発直後のまぶしい閃光
			p.transform_.scale_ = { 5.0f, 5.0f, 5.0f };
			p.velocity_ = { 0, 0, 0 };
			p.color_ = { 1, 1, 1, 1 };
			p.lifeTime_ = 0.2f;
		} else if (groupName == "fw_burst") {
			// 花火本体（放射状）
			float a1 = (float)rand() / RAND_MAX * 6.28f;
			float a2 = (float)rand() / RAND_MAX * 3.14f;

			Vector3 dir;
			dir.x = std::cos(a1) * std::sin(a2);
			dir.y = std::cos(a2) * 0.8f; // 上に散りすぎ防止
			dir.z = std::sin(a1) * std::sin(a2);

			float spd = 10.0f + ((float)rand() / RAND_MAX * 12.0f);
			p.velocity_ = dir * spd;

			float sc = 1.5f + ((float)rand() / RAND_MAX * 1.2f);
			p.transform_.scale_ = { sc, sc, sc };

			// カラフル！（鮮やか〜中間）
			float r = 0.4f + ((float)rand() / RAND_MAX * 0.6f);
			float g = 0.4f + ((float)rand() / RAND_MAX * 0.6f);
			float b = 0.4f + ((float)rand() / RAND_MAX * 0.6f);

			p.color_ = { r, g, b, 1.0f };
			p.lifeTime_ = 3.0f;
		} else if (groupName == "airStreak") {
			// ─────────────────────
			// 空で飛んでるときの「風の筋」
			// ─────────────────────

			// 細長いライン（ビルボードでカメラ向きになる）
			p.transform_.scale_ = { 0.2f, 0.2f, 0.2f }; // 幅, 高さ, 奥行き
			p.transform_.rotate_ = { 0.0f, 0.0f, 0.0f };
			p.transform_.translate_.z = 50.0f;

			// ちょっとだけブレを入れながら手前(-Z)に流す
			float vx = ((rand() % 40) - 20) / 200.0f; // -0.1 ～ +0.1
			float vy = ((rand() % 40) - 20) / 200.0f; // -0.1 ～ +0.1

			float baseSpeed = 30.0f + (rand() % 40) / 10.0f; // 12.0 ～ 16.0 くらい
			float vz = -baseSpeed; // カメラ手前方向（-Z）へシュッと流れる

			p.velocity_ = { vx, vy, vz };

			// ほぼ白～薄い青でうっすら
			float c = 0.85f + (rand() % 15) / 100.0f; // 0.85 ～ 1.0
			// 濃い砂埃の色
			p.color_ = { 0.9f * c, 0.9f * c, 1.0f * c, 0.6f }; // 少し透明感あり

			p.lifeTime_ = 1.8f;   // だいたい3秒くらい生きる
			p.currentTime_ = 0.0f; // 初期化

		} else if (groupName == "ribbonTest") {

			// リボンは横長の板を想定
			//   X方向に長く、Y方向は少しだけ
			p.transform_.scale_ = { 8.0f, 1.0f, 1.0f };

			// 少しだけ上にフワっと浮く
			p.velocity_ = { 0.0f, 3.0f, 0.0f };

			// 色（薄い紫っぽく）
			p.color_ = { 0.8f, 0.6f, 1.0f, 1.0f };

			// 1秒くらい残る
			p.lifeTime_ = 1.0f;
			p.currentTime_ = 0.0f;

		} else if (groupName == "enemySpawn") {
			//=========================================================
			// 敵出現用：中心で「ボフッ」と光って、周りに粒が広がる
			//   ・25%くらいは中心のフラッシュ
			//   ・残りは円状に飛び散る粒
			//=========================================================
			std::uniform_real_distribution<float> patternDist(0.0f, 1.0f);
			float pattern = patternDist(rng);

			if (pattern < 0.25f) {
				// ── 中央フラッシュ ──
				// 敵のど真ん中で大きく光るだけ（ほぼ動かない）
				p.transform_.translate_ = center;
				p.velocity_ = { 0.0f, 0.0f, 0.0f };

				// 大きめサイズで「出現した！」感
				float sc = std::uniform_real_distribution<float>(1.8f, 2.6f)(rng);
				p.transform_.scale_ = { sc, sc, sc };

				// 短命だけど強く光る
				p.lifeTime_ = std::uniform_real_distribution<float>(0.25f, 0.40f)(rng);
				p.currentTime_ = 0.0f;

				// 白に近いシアン系（コアがピカッと光るイメージ）
				float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
				float rCol = 0.3f * (1.0f - t);
				float gCol = 0.9f;
				float bCol = 1.2f - 0.2f * t;
				p.color_ = { rCol, gCol, bCol, 1.0f };
			} else {
				// ── 周囲に円状に広がる粒 ──
				// ちょっと広めの半径＆Zにもバラつきを持たせる
				std::uniform_real_distribution<float> radiusDist(0.5f, 3.0f);
				std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * std::numbers::pi_v<float>);
				std::uniform_real_distribution<float> zOffsetDist(-0.8f, 0.8f);

				float r = radiusDist(rng);
				float th = angleDist(rng);

				Vector3 offsetLocal{
					std::cos(th) * r,
					std::sin(th) * 0.7f,   // Yを少し強めて“湧き上がる”感じ
					zOffsetDist(rng)       // 手前/奥にも少し散らす
				};

				p.transform_.translate_ = center + offsetLocal;

				// オフセット方向に外へ飛ばす
				Vector3 dir = (MyMath::Length(offsetLocal) > 0.001f)
					? MyMath::Normalize(offsetLocal)
					: Vector3{ 0.0f, 1.0f, 0.0f };

				std::uniform_real_distribution<float> spd(1.2f, 3.2f);
				p.velocity_ = dir * spd(rng);

				// 粒自体も少し大きめ
				float sc = std::uniform_real_distribution<float>(0.6f, 1.3f)(rng);
				p.transform_.scale_ = { sc, sc, sc };

				// ちょい長めに残す
				p.lifeTime_ = std::uniform_real_distribution<float>(0.60f, 1.00f)(rng);
				p.currentTime_ = 0.0f;

				// 青〜シアン系で、中心フラッシュより少し落ち着いた色
				float t2 = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
				float rCol = 0.05f + 0.10f * (1.0f - t2);
				float gCol = 0.80f + 0.15f * t2;
				float bCol = 1.00f;
				p.color_ = { rCol, gCol, bCol, 1.0f };
			}

		} else if (groupName == "enemyHit_flash") {
			// 中央のまぶしいフラッシュ（一瞬だけ）＋色は毎回ちょっと変える
			p.transform_.translate_ = center; // 完全センター固定

			// 少し大きめにして「ドンッ」と光る感じ
			float sc = std::uniform_real_distribution<float>(1.3f, 1.8f)(rng);
			p.transform_.scale_ = { sc, sc, sc };
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.07f, 0.10f)(rng);
			p.currentTime_ = 0.0f;

			// --- 白ベース＋アクセントカラーをランダム ---
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 coreColor;
			if (t < 0.33f) {
				// ゴールド寄り
				coreColor = { 1.0f, 0.95f, 0.70f };
			} else if (t < 0.66f) {
				// シアン寄り
				coreColor = { 0.75f, 0.95f, 1.0f };
			} else {
				// マゼンタ寄り
				coreColor = { 1.0f, 0.75f, 1.0f };
			}
			p.color_ = { coreColor.x, coreColor.y, coreColor.z, 1.0f };

		} else if (groupName == "enemyHit_ring") {
			// 外側に広がるショックウェーブリング（カラフル）
			p.transform_.translate_ = center; // ぴったり中心

			// ちょっと大きめ＆強め
			float sc = std::uniform_real_distribution<float>(1.6f, 2.3f)(rng);
			p.transform_.scale_ = { sc, sc, sc };
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.22f, 0.30f)(rng);
			p.currentTime_ = 0.0f;

			// ビビッドな色を何パターンかからランダム
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 ringColor;
			if (t < 0.25f) {
				// ゴールド
				ringColor = { 1.0f, 0.9f, 0.4f };
			} else if (t < 0.5f) {
				// シアン
				ringColor = { 0.4f, 0.95f, 1.0f };
			} else if (t < 0.75f) {
				// ピンク
				ringColor = { 1.0f, 0.55f, 0.8f };
			} else {
				// ライム
				ringColor = { 0.6f, 1.0f, 0.6f };
			}
			p.color_ = { ringColor.x, ringColor.y, ringColor.z, 1.0f };

		} else if (groupName == "enemyHit_rays") {
			// 放射状の細長いレイ（光の筋）
			p.transform_.translate_ = center;

			// 画面上の回転角
			float angle = std::uniform_real_distribution<float>(0.0f, 2.0f * std::numbers::pi_v<float>)(rng);
			p.transform_.rotate_ = { 0.0f, 0.0f, angle };

			float len = std::uniform_real_distribution<float>(0.4f, 0.8f)(rng);
			float thin = std::uniform_real_distribution<float>(0.1f, 0.18f)(rng);
			p.transform_.scale_ = { thin, len, 1.0f }; // ← XとYを逆転させる

			// 少しだけ外側に膨らむように動かす
			Vector3 dir = { std::cos(angle), 0.0f, std::sin(angle) };
			dir = (MyMath::Length(dir) > 0.001f) ? MyMath::Normalize(dir) : Vector3{ 1,0,0 };
			float spd = std::uniform_real_distribution<float>(3.0f, 6.0f)(rng);
			p.velocity_ = dir * spd;

			p.lifeTime_ = std::uniform_real_distribution<float>(0.18f, 0.26f)(rng);
			p.currentTime_ = 0.0f;

			// レイも派手色で
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 rayColor;
			if (t < 0.25f) {
				rayColor = { 1.0f, 0.85f, 0.45f };   // ゴールド
			} else if (t < 0.5f) {
				rayColor = { 0.5f, 0.95f, 1.0f };    // シアン
			} else if (t < 0.75f) {
				rayColor = { 1.0f, 0.6f, 0.9f };     // ピンク
			} else {
				rayColor = { 0.7f, 1.0f, 0.6f };     // ライム
			}
			p.color_ = { rayColor.x, rayColor.y, rayColor.z, 1.0f };

		} else if (groupName == "enemyHit_spark") {
			// 周りに飛び散る小さな火花（スピード＆色増し）
			p.transform_.translate_ = center;

			// ランダム方向（XZメイン、少しだけY）
			float a = std::uniform_real_distribution<float>(0.0f, 2.0f * std::numbers::pi_v<float>)(rng);
			float up = std::uniform_real_distribution<float>(-0.25f, 0.55f)(rng);
			Vector3 dir = MyMath::Normalize(Vector3{ std::cos(a), up, std::sin(a) });

			// スピード強め
			float spd = std::uniform_real_distribution<float>(8.0f, 16.0f)(rng);
			p.velocity_ = dir * spd;

			float sc = std::uniform_real_distribution<float>(0.22f, 0.40f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.32f, 0.52f)(rng);
			p.currentTime_ = 0.0f;

			// 暖色・寒色・マゼンタをミックス
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 spColor;
			if (t < 0.33f) {
				// オレンジ
				spColor = { 1.0f, 0.65f, 0.25f };
			} else if (t < 0.66f) {
				// シアン
				spColor = { 0.4f, 0.9f, 1.0f };
			} else {
				// マゼンタ
				spColor = { 1.0f, 0.45f, 0.9f };
			}
			p.color_ = { spColor.x, spColor.y, spColor.z, 1.0f };
		} else if (groupName == "lt_nova_core") {
			// 爆心コア：画面を埋めるくらいのまぶしいエネルギー球
			p.transform_.translate_ = center;

			// サイズ大幅アップ（敵が完全に飲み込まれるレベル）
			float sc = std::uniform_real_distribution<float>(4.0f, 6.0f)(rng);
			p.transform_.scale_ = { sc, sc, sc };
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// 残光長め（ドーンと光が残る）
			p.lifeTime_ = std::uniform_real_distribution<float>(0.45f, 0.65f)(rng);
			p.currentTime_ = 0.0f;

			// 中心は白＋黄金（太陽みたいな爆心）
			p.color_ = { 1.0f, 0.96f, 0.70f, 1.0f };


		} else if (groupName == "lt_nova_wave") {
			// 球状ショックウェーブ（外側のエネルギー殻。コアよりさらに大きい）
			p.transform_.translate_ = center;

			// 半径かなり拡大（画面を貫く衝撃波）
			float sc = std::uniform_real_distribution<float>(5.0f, 7.5f)(rng);
			p.transform_.scale_ = { sc, sc, sc };
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// コアより少し長く残して「爆風の壁」感
			p.lifeTime_ = std::uniform_real_distribution<float>(0.50f, 0.80f)(rng);
			p.currentTime_ = 0.0f;

			// 内側が黄〜外側オレンジに見えるような暖色
			p.color_ = { 1.0f, 0.78f, 0.32f, 1.0f };

		} else if (groupName == "lt_nova_burst") {
			// シンプルな光の爆発粒子
			p.transform_.translate_ = center;

			// 飛び散り方向ランダム
			Vector3 dir = {
				std::uniform_real_distribution<float>(-1,1)(rng),
				std::uniform_real_distribution<float>(-1,1)(rng),
				std::uniform_real_distribution<float>(-1,1)(rng)
			};
			if (MyMath::Length(dir) < 0.001f) dir = { 0,1,0 };
			dir = MyMath::Normalize(dir);

			// スピード（ランダム）
			float spd = std::uniform_real_distribution<float>(0.3f, 1.2f)(rng);
			p.velocity_ = dir * spd;

			// 大きさ（小さめの点）
			float sc = std::uniform_real_distribution<float>(0.2f, 0.6f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			// 色（白ベース）
			p.color_ = { 1,1,1,1 };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.20f, 0.40f)(rng);
		} else if (groupName == "lt_nova_debris") {
			// 破片：暗い塊が高速で四方八方に飛ぶ
			p.transform_.translate_ = center;

			// バラバラの方向に飛ばす
			float ang1 = std::uniform_real_distribution<float>(0, 2 * std::numbers::pi_v<float>)(rng);
			float ang2 = std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng);

			Vector3 dir = {
				std::cos(ang1),
				ang2,
				std::sin(ang1)
			};
			dir = MyMath::Normalize(dir);

			float speed = std::uniform_real_distribution<float>(4.0f, 9.0f)(rng);
			p.velocity_ = dir * speed;

			// 小さめの塊＋ランダム
			float sc = std::uniform_real_distribution<float>(0.2f, 0.45f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			// 暗い破片 → 茶色〜黒
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float c = MyMath::Lerp(0.05f, 0.20f, t);
			p.color_ = { c, c * 0.9f, c * 0.8f, 1.0f };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.4f, 0.8f)(rng);
			p.currentTime_ = 0.0f;
		} else if (groupName == "lt_nova_crack") {
			// 亀裂：空間を裂くような細長いスパーク
			p.transform_.translate_ = center;

			// ランダム方向へ細く長いひび
			float ang = std::uniform_real_distribution<float>(0, 2 * std::numbers::pi_v<float>)(rng);
			Vector3 dir = { std::cos(ang), 0.0f, std::sin(ang) };

			float len = std::uniform_real_distribution<float>(1.5f, 3.0f)(rng);
			float thin = std::uniform_real_distribution<float>(0.05f, 0.12f)(rng);
			p.transform_.scale_ = { len, thin, 1.0f };

			// ほぼ動かないが少しだけ散る
			p.velocity_ = dir * std::uniform_real_distribution<float>(0.5f, 1.5f)(rng);

			// 黒〜赤黒いひび
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 col = {
				MyMath::Lerp(0.05f, 0.3f, t),
				MyMath::Lerp(0.0f, 0.05f, t),
				MyMath::Lerp(0.0f, 0.05f, t)
			};
			p.color_ = { col.x, col.y, col.z, 1.0f };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.35f, 0.5f)(rng);
			p.currentTime_ = 0.0f;
		} else if (groupName == "enemyDeath_core") {
			// 敵が消える瞬間、中心にフッと出る小さな光

			// 位置は完全にセンター
			p.transform_.translate_ = center;

			// 少しだけ大きめだけど、そこまでド派手じゃない
			float sc = std::uniform_real_distribution<float>(0.9f, 1.4f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			// 動かない（その場で光って消える）
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// 寿命はかなり短いパッと光る感じ
			p.lifeTime_ = std::uniform_real_distribution<float>(0.12f, 0.20f)(rng);
			p.currentTime_ = 0.0f;

			// 少し黄味がかった白い光
			p.color_ = { 1.0f, 0.96f, 0.86f, 1.0f };

		} else if (groupName == "enemyDeath_shard") {
			// バラバラに飛び散る光の破片

			// 中心からごく小さなオフセット
			std::uniform_real_distribution<float> offSmall(-0.15f, 0.15f);
			Vector3 localOffset{
				offSmall(rng),
				offSmall(rng),
				offSmall(rng)
			};
			p.transform_.translate_ = center + localOffset;

			// ランダム方向（やや上＋後ろに飛ぶ、ふわっと散るイメージ）
			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			Vector3 dir{
				frand(-0.6f, 0.6f),
				frand(0.0f, 0.9f),    // 上方向に少しバイアス
				frand(-1.0f, 0.2f)    // 画面奥〜少し手前
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			float spd = frand(0.6f, 1.6f);
			p.velocity_ = dir * spd;

			// 小さい光の破片
			float sc = frand(0.25f, 0.55f);
			p.transform_.scale_ = { sc, sc, sc };

			// ちょっとだけ残る
			p.lifeTime_ = frand(0.45f, 0.85f);
			p.currentTime_ = 0.0f;

			// 色は少しだけカラフル（青〜シアン〜マゼンタの中間）
			float t = frand(0.0f, 1.0f);
			Vector3 col = {
				0.6f + 0.3f * t,      // R : 0.6〜0.9
				0.7f + 0.2f * (1 - t),// G : 0.7〜0.9
				1.0f                  // B : 1.0（青白い感じ）
			};
			p.color_ = { col.x, col.y, col.z, 1.0f };

		} else if (groupName == "enemyDeath_smoke") {
			// ふわっと残る煙（あまり主張しない）

			// 位置はほぼセンター
			p.transform_.translate_ = center;

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// 少しだけゆっくり上昇
			p.velocity_ = {
				frand(-0.15f, 0.15f),
				frand(0.15f, 0.35f),
				frand(-0.15f, 0.15f)
			};

			// 丸くて少し大きめ
			float sc = frand(0.9f, 1.8f);
			p.transform_.scale_ = { sc, sc, sc };

			// わりと長めに残って、消えたあとも余韻がある
			p.lifeTime_ = frand(0.9f, 1.5f);
			p.currentTime_ = 0.0f;

			// 薄いグレー〜少し青み
			Vector3 col3 = {
				frand(0.70f, 0.85f),
				frand(0.72f, 0.88f),
				frand(0.80f, 0.95f)
			};
			p.color_ = { col3.x, col3.y, col3.z, 1.0f };
		} else if (groupName == "enemyPounceTrail") {
			// ─────────────────────────────
			// 敵の飛び掛かり軌道：コアレール（闇マゼンタ一本軸）
			// ─────────────────────────────
			p.transform_.translate_ = center;

			float height = std::uniform_real_distribution<float>(1.6f, 2.4f)(rng); // 縦の長さ
			float width = std::uniform_real_distribution<float>(0.08f, 0.14f)(rng); // 横の太さ
			// スプライトの縦横を入れ替える
			p.transform_.scale_ = { width, height, 1.0f };

			p.velocity_ = { 0.0f, 0.0f, 0.0f };
			p.lifeTime_ = std::uniform_real_distribution<float>(0.55f, 0.9f)(rng);
			p.currentTime_ = 0.0f;

			// ── ベースカラー：闇マゼンタ ──
			// あえて 1色＋ほんの少しだけバリエーションに絞る
			Vector3 base = { 0.70f, 0.05f, 0.85f }; // 赤強めの紫

			// 彩度・明るさを微妙に揺らす（全部ランダムにしない）
			float valueJitter = std::uniform_real_distribution<float>(0.75f, 0.95f)(rng);
			float hueJitter = std::uniform_real_distribution<float>(-0.05f, 0.05f)(rng);

			Vector3 col = {
				std::clamp(base.x + hueJitter, 0.0f, 1.0f),
				base.y,
				std::clamp(base.z - hueJitter, 0.0f, 1.0f)
			};

			// 全体を少し暗くして「白・黄」に寄らないように
			col.x *= valueJitter;
			col.y *= valueJitter;
			col.z *= valueJitter;

			float alpha = std::uniform_real_distribution<float>(0.85f, 1.0f)(rng);

			p.color_ = { col.x, col.y, col.z, alpha };
		} else if (groupName == "enemyPounceSpark") {
			// ─────────────────────────────
			// 軌道上から飛び散るスパーク（血・毒・火花）
			// ─────────────────────────────
			p.transform_.translate_ = center;

			Vector3 dir = {
				std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng),
				std::uniform_real_distribution<float>(-0.3f, 0.9f)(rng),
				std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng)
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			float spd = std::uniform_real_distribution<float>(2.0f, 4.0f)(rng);
			p.velocity_ = dir * spd;

			float baseScale = std::uniform_real_distribution<float>(0.20f, 0.35f)(rng);

			// 0.0–0.5  血スパーク（赤系）
			// 0.5–0.8  毒スパーク（緑系）※Gは高いけどRをかなり抑える
			// 0.8–1.0  火花スパーク（赤橙）※黄色に寄りすぎない
			float   r = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 spColor;
			float   alpha = 1.0f;

			if (r < 0.5f) {
				// 血
				float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
				if (t < 0.5f) {
					spColor = { 0.95f, 0.15f, 0.25f }; // 鮮血
				} else {
					spColor = { 0.75f, 0.05f, 0.15f }; // どす黒い血
				}
				alpha = std::uniform_real_distribution<float>(0.85f, 1.0f)(rng);
				baseScale *= std::uniform_real_distribution<float>(1.0f, 1.3f)(rng);

			} else if (r < 0.8f) {
				// 毒
				float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
				if (t < 0.5f) {
					spColor = { 0.10f, 0.80f, 0.25f }; // 毒緑
				} else {
					spColor = { 0.05f, 0.60f, 0.20f }; // 少し暗い毒緑
				}
				alpha = std::uniform_real_distribution<float>(0.8f, 0.95f)(rng);
				baseScale *= std::uniform_real_distribution<float>(0.9f, 1.1f)(rng);

			} else {
				// 火花（赤橙）※黄色まで行かない
				float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
				if (t < 0.5f) {
					spColor = { 0.95f, 0.35f, 0.15f }; // 赤寄りオレンジ
				} else {
					spColor = { 0.85f, 0.25f, 0.15f }; // ちょい暗め
				}
				alpha = std::uniform_real_distribution<float>(0.85f, 1.0f)(rng);
				baseScale *= std::uniform_real_distribution<float>(0.8f, 1.0f)(rng);
			}

			// ここでも「白・黄に寄せない」ために少し暗くする
			float darken = 0.85f;
			spColor.x *= darken;
			spColor.y *= darken;
			spColor.z *= darken;

			p.transform_.scale_ = { baseScale, baseScale, baseScale };
			p.color_ = { spColor.x, spColor.y, spColor.z, alpha };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.20f, 0.40f)(rng);
			p.currentTime_ = 0.0f;
		} else if (groupName == "core_charge_shell") {
			p.transform_.translate_ = center;
			float startScale = 1.6f;
			p.transform_.scale_ = { startScale, startScale, startScale };
			p.velocity_ = { 0.0f, 0.0f, 0.0f };
			// 邪悪な黒紫
			p.color_ = { 0.35f, 0.0f, 0.5f, 0.35f };
			p.lifeTime_ = 0.45f;
			p.currentTime_ = 0.0f;
		} else if (groupName == "core_charge_inward") {

			std::uniform_real_distribution<float> dirDist(-1.0f, 1.0f);
			Vector3 dirRand = { dirDist(rng), dirDist(rng), dirDist(rng) };

			if (MyMath::Length(dirRand) < 0.001f) dirRand = { 0,1,0 };
			dirRand = MyMath::Normalize(dirRand);

			float radius = std::uniform_real_distribution<float>(2.0f, 5.0f)(rng);
			Vector3 offset = dirRand * radius;

			p.transform_.translate_ = center + offset;

			Vector3 dirToCenter = MyMath::Normalize(-offset);

			float speed = std::uniform_real_distribution<float>(2.0f, 5.0f)(rng);
			p.velocity_ = dirToCenter * speed;

			p.transform_.scale_ = { 0.25f, 0.25f, 0.25f };

			// 怪しい血のような禍々しく濃い赤
			p.color_ = { 0.8f, 0.05f, 0.1f, 0.9f };

			p.lifeTime_ = 0.7f;
			p.currentTime_ = 0.0f;
		} else if (groupName == "core_charge_ribbon") {

			float height = std::uniform_real_distribution<float>(-1.2f, 1.2f)(rng);
			p.transform_.translate_.y += height;

			// もっと長く・太くして視認性アップ
			p.transform_.scale_ = { 3.2f, 0.14f, 1.0f };

			// 明るめの邪悪紫に変更（青成分足すと光って見える）
			p.color_ = { 0.85f, 0.2f, 1.0f, 0.95f };

			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// 寿命もちょい伸ばすと“渦巻き”感が出る
			p.lifeTime_ = 0.55f;
		} else if (groupName == "core_charge_flash") {

			float s = std::uniform_real_distribution<float>(0.25f, 0.55f)(rng);

			// 完全に正面に出るように Z も s にする
			p.transform_.scale_ = { s, s, s };

			// 発光強めの深紅（alpha も上げる）
			p.color_ = { 1.0f, 0.05f, 0.15f, 1.0f };

			// 寿命をほんの少しだけ伸ばすと見える
			p.lifeTime_ = 0.18f;

			p.velocity_ = { 0.0f, 0.0f, 0.0f };
		} else if (groupName == "bossDeath_bomb") {
			// モンストっぽい「丸い爆発」がボンボン出るやつ

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// ボスの周囲にランダム配置（ちょっと縦長）
			Vector3 dir{
				frand(-1.0f, 1.0f),
				frand(-0.6f, 1.0f),
				frand(-1.0f, 1.0f)
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			float radius = frand(1.5f, 5.0f);               // 中心からの距離
			p.transform_.translate_ = center + dir * radius;

			// 外向きにそこそこ速く飛ぶ（画面全体に広がる感じ）
			float speed = frand(2.5f, 6.0f);
			p.velocity_ = dir * speed;
			// 爆発本体をかなり大きく
			float sc = frand(3.0f, 6.0f);
			p.transform_.scale_ = { sc, sc, sc };
			// 少しだけ長く見せる
			p.lifeTime_ = frand(0.35f, 0.65f);
			p.currentTime_ = 0.0f;

			// オレンジ〜黄色の炎色
			Vector3 col{
				1.0f,
				frand(0.45f, 0.9f),
				frand(0.0f, 0.25f)
			};
			p.color_ = { col.x, col.y, col.z, 1.0f };

		} else if (groupName == "bossDeath_ring") {
			// 画面を埋めるくらいの衝撃波リング

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// 中心固定
			p.transform_.translate_ = center;

			// 大きなリング
			float sc = frand(6.0f, 10.0f);
			p.transform_.scale_ = { sc, sc, sc };

			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// 少し長めに残る
			p.lifeTime_ = frand(0.5f, 0.9f);
			p.currentTime_ = 0.0f;

			// 黄白っぽい衝撃波カラー
			p.color_ = { 1.0f, 0.88f, 0.55f, 1.0f };

		} else if (groupName == "bossDeath_smoke") {
			// 爆発のあとの大きな煙

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// ボスの周囲に大きく散らす
			Vector3 dir{
				frand(-1.0f, 1.0f),
				frand(0.0f, 1.0f),
				frand(-1.0f, 1.0f)
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			// ボスの周囲にさらに広く散らす
			float radius = frand(6.0f, 14.0f);
			p.transform_.translate_ = center + dir * radius;
			// 上方向にもう少し強めに流れる
			float speed = frand(0.8f, 1.6f);
			// 大きめの煙
			float sc = frand(3.0f, 5.5f);
			p.transform_.scale_ = { sc, sc, sc };
			// かなり長く残して「余韻」を出す
			p.lifeTime_ = frand(2.0f, 3.0f);
			p.currentTime_ = 0.0f;

			// 少し青みがかった白煙〜灰
			Vector3 col3{
				frand(0.70f, 0.90f),
				frand(0.72f, 0.92f),
				frand(0.78f, 0.96f)
			};
			p.color_ = { col3.x, col3.y, col3.z, 1.0f };
		} else if (groupName == "bossClear_core") {
			// ボス撃破後の「クリア決定打」になる爆心コア

			p.transform_.translate_ = center;

			// かなりデカい光球（ボスを飲み込むレベル）
			float sc = std::uniform_real_distribution<float>(5.0f, 8.0f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			// 動かない・その場でドーンと光る
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// 少し長めに残して余韻を出す
			p.lifeTime_ = std::uniform_real_distribution<float>(0.6f, 0.9f)(rng);
			p.currentTime_ = 0.0f;

			// 白〜黄金色のまぶしい爆心
			p.color_ = { 1.0f, 0.96f, 0.80f, 1.0f };

		} else if (groupName == "bossClear_ring") {
			// ボス撃破後の超デカいショックウェーブ

			p.transform_.translate_ = center;

			// 半径かなり大きめ
			float sc = std::uniform_real_distribution<float>(7.0f, 11.0f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			// ほぼ動かない（サイズで見せる）
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// 少し残る
			p.lifeTime_ = std::uniform_real_distribution<float>(0.45f, 0.7f)(rng);
			p.currentTime_ = 0.0f;

			// 外周が少しオレンジがかった白
			p.color_ = { 1.0f, 0.9f, 0.65f, 1.0f };

		} else if (groupName == "bossClear_spark") {
			// 爆発で四方八方に飛ぶ光の破片

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// 中心からランダム方向へ
			Vector3 dir{
				frand(-1.0f, 1.0f),
				frand(-1.0f, 1.0f),
				frand(-1.0f, 1.0f)
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			// 少しだけ中心から離して出す
			float radius = frand(0.5f, 4.0f);
			p.transform_.translate_ = center + dir * radius;

			// かなり速く飛ばす
			float speed = frand(8.0f, 18.0f);
			p.velocity_ = dir * speed;

			// 細長い破片っぽく
			float len = frand(0.6f, 1.4f);
			float thin = frand(0.12f, 0.25f);
			p.transform_.scale_ = { len, thin, 1.0f };

			// 短命〜中くらい
			p.lifeTime_ = frand(0.25f, 0.5f);
			p.currentTime_ = 0.0f;

			// 黄〜白〜オレンジ寄りの光
			Vector3 col{
				1.0f,
				frand(0.7f, 0.95f),
				frand(0.3f, 0.6f)
			};
			p.color_ = { col.x, col.y, col.z, 1.0f };

		} else if (groupName == "bossClear_debris") {
			// 重めに飛んでいく破片（ゲームクリア感の余韻）

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			Vector3 dir{
				frand(-1.0f, 1.0f),
				frand(0.2f, 1.0f),      // ちょい上向き寄り
				frand(-1.0f, 1.0f)
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			float radius = frand(1.0f, 5.0f);
			p.transform_.translate_ = center + dir * radius;

			// スパークより遅め・重たい感じ
			float speed = frand(3.0f, 8.0f);
			p.velocity_ = dir * speed;

			// ゴツい破片
			float sc = frand(0.4f, 0.9f);
			p.transform_.scale_ = { sc, sc, sc };

			// 長めに残る
			p.lifeTime_ = frand(0.7f, 1.4f);
			p.currentTime_ = 0.0f;

			// 暗めのオレンジ〜焦げ茶っぽい色
			Vector3 col{
				frand(0.4f, 0.8f),
				frand(0.2f, 0.4f),
				frand(0.05f, 0.15f)
			};
			p.color_ = { col.x, col.y, col.z, 1.0f };
		} else if (groupName == "boss_windup_inward") {
			// 外→内へ吸い込まれる粒（溜め感の主成分）
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			std::uniform_real_distribution<float> off(-1.8f, 1.8f);
			Vector3 o = { off(rng), off(rng) * 0.4f, off(rng) };
			p.transform_.translate_ = center + o;

			Vector3 dir = MyMath::Normalize(-o); // 中心へ
			float spd = frand(0.55f, 1.05f);
			p.velocity_ = dir * spd;

			float sc = frand(6.0f, 14.0f);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = frand(0.22f, 0.40f);
			p.currentTime_ = 0.0f;

			p.color_ = { frand(0.90f, 1.00f), frand(0.20f, 0.45f), frand(0.90f, 1.00f), 1.0f };

		} else if (groupName == "boss_windup_crackle") {
			// バチバチ（短命スパーク）
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			std::uniform_real_distribution<float> off(-0.55f, 0.55f);
			Vector3 o = { off(rng), off(rng) * 0.3f, off(rng) };
			p.transform_.translate_ = center + o;

			// ランダムに散る
			Vector3 dir = MyMath::Normalize(o);
			float spd = frand(0.40f, 1.10f);
			p.velocity_ = dir * spd;

			float sc = frand(4.0f, 10.0f);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = frand(0.10f, 0.20f);
			p.currentTime_ = 0.0f;

			// 白〜紫寄り
			float t = frand(0.0f, 1.0f);
			p.color_ = { 1.0f, 0.55f - 0.25f * t, 1.0f, 1.0f };

		} else if (groupName == "boss_windup_shell") {
			// 外周リング（パルス）
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			p.transform_.translate_ = center;

			float sc = frand(14.0f, 26.0f);
			p.transform_.scale_ = { sc, sc, sc };

			// ゆっくり拡張（リングなのでXYよりXZのイメージだけど簡略化でOK）
			p.velocity_ = { 0.0f, frand(0.02f, 0.06f), 0.0f };

			p.lifeTime_ = frand(0.18f, 0.28f);
			p.currentTime_ = 0.0f;

			p.color_ = { frand(0.95f, 1.0f), frand(0.15f, 0.35f), frand(0.95f, 1.0f), 1.0f };
		} else if (groupName == "boss_slash_windup_line") {
			// 刃が形成される線エネルギー（前方に伸びる感じ）
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			// 生成位置：中心近くで細長くバラける
			std::uniform_real_distribution<float> ox(-0.55f, 0.55f);
			std::uniform_real_distribution<float> oy(-0.20f, 0.20f);
			std::uniform_real_distribution<float> oz(-0.35f, 0.35f);
			Vector3 o = { ox(rng), oy(rng), oz(rng) };
			p.transform_.translate_ = center + o;

			// ちょい前方へ流す（※向きはBossController側で center を“前に出す”とよりそれっぽい）
			p.velocity_ = { frand(-0.05f, 0.05f), frand(-0.03f, 0.06f), frand(0.65f, 1.35f) };

			// 細長い筋（gradationLine を想定）
			float scX = frand(0.25f, 0.55f);
			float scY = frand(0.25f, 0.55f);
			float scZ = frand(3.5f, 7.5f);
			p.transform_.scale_ = { scX, scY, scZ };

			p.lifeTime_ = frand(0.18f, 0.32f);
			p.currentTime_ = 0.0f;

			// 邪悪：深紅〜黒紫（ミサイルの綺麗紫と差別化）
			p.color_ = { frand(0.85f, 1.0f), frand(0.00f, 0.08f), frand(0.15f, 0.50f), 1.0f };
		} else if (groupName == "boss_slash_windup_spark") {
			// バチバチ：邪悪スパーク（短命）
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			std::uniform_real_distribution<float> off(-0.45f, 0.45f);
			Vector3 o = { off(rng), off(rng) * 0.25f, off(rng) };
			p.transform_.translate_ = center + o;

			// 外へ散る
			Vector3 dir = MyMath::Normalize(o);
			float spd = frand(0.55f, 1.55f);
			p.velocity_ = dir * spd;

			float sc = frand(0.40f, 1.05f);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = frand(0.08f, 0.15f);
			p.currentTime_ = 0.0f;

			// 8割：紅紫 / 2割：毒っぽい緑（邪悪感UP、不要なら消してOK）
			float r = frand(0.0f, 1.0f);
			if (r < 0.80f) {
				p.color_ = { 1.0f, frand(0.03f, 0.12f), frand(0.25f, 0.75f), 1.0f };
			} else {
				p.color_ = { frand(0.15f, 0.35f), 1.0f, frand(0.10f, 0.25f), 1.0f };
			}
		} else if (groupName == "boss_slash_windup_arc") {
			// 弧の輪郭（リングで一瞬だけ出す）
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			p.transform_.translate_ = center;

			// 弧を大きめに（RINGなのでスケール大きめでも破綻しにくい）
			float sc = frand(10.0f, 16.0f);
			p.transform_.scale_ = { sc, sc, sc };

			// ほぼ静止（輪郭なので）
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			p.lifeTime_ = frand(0.12f, 0.20f);
			p.currentTime_ = 0.0f;

			// 血の結界っぽい色
			p.color_ = { frand(0.90f, 1.0f), frand(0.00f, 0.06f), frand(0.10f, 0.28f), 1.0f };
		} else if (groupName == "bossSlash_cut") {
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			// 位置：中心（ブレはほぼ無しでOK。形はBossBulletが作る）
			std::uniform_real_distribution<float> off(-0.25f, 0.25f);
			p.transform_.translate_ = center + Vector3{ off(rng), off(rng) * 0.25f, off(rng) };

			// 静止（弧は“配置”で作る）
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// 小粒→発光ブレード（桁を上げる）
			float thick_ = frand(6.0f, 10.0f);     // 太さ
			float len_ = frand(55.0f, 90.0f);    // 長さ（ここが斬撃感の核）
			p.transform_.scale_ = { thick_, thick_, len_ };

			// 寿命も長めにして“面”を作る
			p.lifeTime_ = frand(0.22f, 0.38f);
			p.currentTime_ = 0.0f;

			// 画像寄せ：中心＝黄〜橙、外＝ピンク紫（2系統で擬似グラデ）
			float r = frand(0.0f, 1.0f);
			if (r < 0.70f) {
				// 芯：黄〜橙
				p.color_ = { 1.0f, frand(0.55f, 0.90f), frand(0.02f, 0.20f), 0.95f };
			} else {
				// 縁：ピンク紫
				p.color_ = { 1.0f, frand(0.08f, 0.22f), frand(0.60f, 0.95f), 0.80f };
			}
		} else if (groupName == "bossSlash_spark") {
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			std::uniform_real_distribution<float> off(-0.25f, 0.25f);
			Vector3 o = { off(rng), off(rng) * 0.25f, off(rng) };
			p.transform_.translate_ = center + o;

			Vector3 dir = MyMath::SafeNormalize(o, { 0.0f, 1.0f, 0.0f });
			float spd = frand(4.0f, 10.0f);
			p.velocity_ = dir * spd;

			float sc = frand(0.20f, 0.55f);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = frand(0.08f, 0.16f);
			p.currentTime_ = 0.0f;

			// 赤〜橙の火花
			p.color_ = { 1.0f, frand(0.25f, 0.65f), frand(0.05f, 0.18f), 1.0f };
		} else if (groupName == "bossSlash_main") {
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			// 位置はBossBullet側で形を作るので、ここではブレ最小
			std::uniform_real_distribution<float> off(-0.10f, 0.10f);
			p.transform_.translate_ = center + Vector3{ off(rng), off(rng) * 0.25f, off(rng) };

			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// 芯：細く長い（GIFの白線）
			float thick = frand(1.2f, 2.0f);
			float len = frand(18.0f, 28.0f);
			p.transform_.scale_ = { thick, thick, len };

			p.lifeTime_ = frand(0.16f, 0.24f);
			p.currentTime_ = 0.0f;

			// 白〜薄黄
			p.color_ = { 1.0f, frand(0.92f, 1.0f), frand(0.75f, 0.92f), 1.0f };
		} else if (groupName == "bossSlash_glow") {
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			std::uniform_real_distribution<float> off(-0.18f, 0.18f);
			p.transform_.translate_ = center + Vector3{ off(rng), off(rng) * 0.25f, off(rng) };

			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// 発光：太め＆長め（黄色帯）
			float thick = frand(2.8f, 4.8f);
			float len = frand(22.0f, 36.0f);
			p.transform_.scale_ = { thick, thick, len };

			p.lifeTime_ = frand(0.14f, 0.20f);
			p.currentTime_ = 0.0f;

			// 黄〜橙（alpha薄め）
			p.color_ = { 1.0f, frand(0.65f, 0.90f), frand(0.05f, 0.20f), 0.70f };
		} else if (groupName == "bossSlash_tail") {
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			std::uniform_real_distribution<float> off(-0.25f, 0.25f);
			p.transform_.translate_ = center + Vector3{ off(rng), off(rng) * 0.20f, off(rng) };

			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// 残り：さらに太く、少し長い（暗赤のスミア）
			float thick = frand(4.0f, 7.0f);
			float len = frand(26.0f, 46.0f);
			p.transform_.scale_ = { thick, thick, len };

			p.lifeTime_ = frand(0.20f, 0.32f);
			p.currentTime_ = 0.0f;

			// 暗赤（alpha薄）
			p.color_ = { frand(0.25f, 0.45f), frand(0.02f, 0.06f), frand(0.02f, 0.05f), 0.35f };
		} else if (groupName == "titleExplode_core") {
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = frand(2.4f, 4.2f);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = frand(0.06f, 0.12f);
			p.currentTime_ = 0.0f;

			// 白〜黄白（中心が白飛びする感じ）
			p.color_ = { 1.0f, frand(0.92f, 1.0f), frand(0.65f, 0.90f), 1.0f };

		} else if (groupName == "titleExplode_rays") {
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			// 放射方向
			Vector3 dir = MyMath::Normalize(offset);

			// Rayは“伸びる光線”なので移動はほぼ無し（中心から刺さる）
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// dir に向ける（Z forward を dir に合わせる想定）
			float yaw = std::atan2(dir.x, dir.z);
			float pitch = -std::atan2(dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));
			p.transform_.rotate_ = { pitch, yaw, 0.0f };

			float thick = frand(0.25f, 0.60f);   // 細い方が“線”になる
			float len = frand(16.0f, 44.0f);   // ここが爽快感（桁が足りないと弱い）
			p.transform_.scale_ = { thick, thick, len };

			p.lifeTime_ = frand(0.10f, 0.22f);
			p.currentTime_ = 0.0f;

			// 黄〜白（画像寄せ）
			float k = frand(0.85f, 1.0f);
			p.color_ = { 1.0f, 0.92f * k, 0.35f * k, 1.0f };

		} else if (groupName == "titleExplode_debris") {
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			Vector3 dir = MyMath::Normalize(offset);

			// 破片/火の粉はちゃんと飛ばす（ここも“勢い”）
			float spd = frand(8.0f, 22.0f);
			p.velocity_ = dir * spd;

			float sc = frand(0.18f, 0.55f);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = frand(0.22f, 0.55f);
			p.currentTime_ = 0.0f;

			// オレンジ〜赤（爆発っぽさ）
			p.color_ = { 1.0f, frand(0.35f, 0.65f), frand(0.05f, 0.25f), 1.0f };

		} else if (groupName == "titleExplode_ring") {
			auto frand = [&rng](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = frand(0.6f, 1.0f);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = frand(0.18f, 0.30f);
			p.currentTime_ = 0.0f;

			p.color_ = { 1.0f, 0.85f, 0.25f, 1.0f };
		} else if (groupName == "titleBeam_player" || groupName == "titleBeam_boss") {
			// “線”を作る：短命・細長い・ほぼ動かない（出す位置を線上にばら撒く想定）
			p.transform_.scale_ = { 0.24f, 0.24f, 2.4f };   // 少し太く&長くして視認性UP
			p.velocity_ = { 0.0f, 0.0f, 0.0f };
			p.lifeTime_ = 0.08f;

			// 色を分ける（白禁止）
			if (groupName == "titleBeam_player") {
				// プレイヤー：はっきりシアン〜青（冷色）
				p.color_ = { 0.20f, 0.95f, 1.00f, 1.0f };
			} else {
				// ボス：はっきりマゼンタ〜赤紫（暖色寄り）
				p.color_ = { 1.00f, 0.20f, 0.75f, 1.0f };
			}

		} else if (groupName == "titleBeamClash_core") {
			// 衝突点：両者と違う色（黄緑/電気っぽい）
			p.transform_.scale_ = { 0.70f, 0.70f, 0.70f }; // ちょい大きくして“衝突点”を強調
			p.velocity_ = { 0.0f, 0.0f, 0.0f };
			p.lifeTime_ = 0.14f;
			p.color_ = { 0.85f, 1.00f, 0.10f, 1.0f };

		} else if (groupName == "titleBeamClash_rays") {
			// バチバチ：短い線がランダム方向に飛ぶ
			float spd = 18.0f + (MyMath::Rand01() * 24.0f);
			Vector3 dir = MyMath::SafeNormalize({
				MyMath::Rand01() * 2.0f - 1.0f,
				MyMath::Rand01() * 2.0f - 1.0f,
				MyMath::Rand01() * 2.0f - 1.0f
				});
			p.velocity_ = dir * spd;

			p.transform_.scale_ = { 0.14f, 0.14f, 1.8f };
			p.lifeTime_ = 0.10f;

			// 衝突の火花も黄緑寄りで統一（ビームと別系統）
			p.color_ = { 0.95f, 1.00f, 0.20f, 1.0f };

			// CYLINDERを“飛ぶ方向”に向ける（簡易：Yaw/Pitch）
			const float yaw = std::atan2f(dir.x, dir.z);
			const float horiz = std::sqrt(dir.x * dir.x + dir.z * dir.z);
			const float pitch = -std::atan2f(dir.y, (horiz < 0.0001f ? 0.0001f : horiz));
			p.transform_.rotate_ = { pitch, yaw, 0.0f };
		} else if (groupName == "titleBeamClash_ring") {
			// 衝撃波：リングが広がる（Update側でgrowさせる）
			p.transform_.scale_ = { 0.35f, 0.35f, 0.35f };
			p.velocity_ = { 0.0f, 0.0f, 0.0f };
			p.lifeTime_ = 0.18f;
			p.color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		} else if (groupName == "trail_lt_ribbon") {
			// =========================================
			// LT メルヘン弾道：Ribbon（本線）
			// ・細長い光のストリーク
			// ・弾の移動区間に沿って高密度に置く前提
			// =========================================
			p.transform_.translate_ = center;

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// ほぼ静止でOK（位置は弾側で線に沿って配置する）
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			// カメラ正対の板に「ねじれ」だけ付ける（メルヘン感）
			p.transform_.rotate_ = { 0.0f, 0.0f, frand(0.0f, 6.2831853f) };

			// 太く＆長く（目玉技）
			float thick = frand(0.35f, 0.75f);
			float len = frand(6.0f, 12.0f);
			p.transform_.scale_ = { thick, thick, len };

			// 残像長め（道筋が残る）
			p.lifeTime_ = frand(0.28f, 0.55f);
			p.currentTime_ = 0.0f;

			// 飽和パステル（白を使わない）
			float kind = frand(0.0f, 1.0f);
			if (kind < 0.25f) {
				// ネオンシアン
				p.color_ = { frand(0.05f, 0.25f), frand(1.05f, 1.35f), frand(1.10f, 1.45f), frand(0.75f, 0.95f) };
			} else if (kind < 0.50f) {
				// マゼンタ
				p.color_ = { frand(1.10f, 1.45f), frand(0.10f, 0.35f), frand(1.00f, 1.35f), frand(0.75f, 0.95f) };
			} else if (kind < 0.75f) {
				// バイオレット
				p.color_ = { frand(0.65f, 1.05f), frand(0.15f, 0.35f), frand(1.15f, 1.55f), frand(0.70f, 0.92f) };
			} else {
				// ライム/ミント（アクセント）
				p.color_ = { frand(0.55f, 0.85f), frand(1.10f, 1.50f), frand(0.10f, 0.35f), frand(0.65f, 0.90f) };
			}
		} else if (groupName == "trail_lb_glitter") {
			auto frand = [&rng](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			p.transform_.translate_ = center;

			p.velocity_ = {
				frand(-0.012f, 0.012f),
				frand(-0.012f, 0.012f),
				frand(-0.012f, 0.012f)
			};

			float sc = frand(0.16f, 0.34f);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = frand(0.08f, 0.16f);
			p.currentTime_ = 0.0f;

			// ミント系の粒
			float t = frand(0.0f, 1.0f);
			float r = 0.25f + 0.10f * t;
			float g = 0.95f;
			float b = 0.45f + 0.20f * t;
			p.color_ = { r, g, b, 1.0f };
		} else if (groupName == "trail_lb_bolt_main") {
			auto frand = [&rng](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			p.transform_.translate_ = center;

			// ここ大事：長さは Y に持たせる
			float width = frand(0.060f, 0.125f);
			float length = frand(0.85f, 1.85f); // ここが“ビーム感”の核（長いほどビームっぽい、短いとただの板）
			p.transform_.scale_ = { width, length, 1.0f };

			// 板の中で斜めに見せる
			p.transform_.rotate_ = {
				0.0f,
				0.0f,
				frand(-1.15f, 1.15f)
			};

			// ほぼその場、ほんの少しだけブレる
			p.velocity_ = {
				frand(-0.004f, 0.004f),
				frand(-0.004f, 0.004f),
				frand(-0.004f, 0.004f)
			};

			p.lifeTime_ = frand(0.05f, 0.10f);
			p.currentTime_ = 0.0f;

			// メインの緑エネルギー
			float t = frand(0.0f, 1.0f);
			float r = 0.15f + 0.10f * t;
			float g = 0.95f;
			float b = 0.25f + 0.15f * t;
			p.color_ = { r, g, b, 1.0f };
		} else if (groupName == "trail_lb_bolt_core") {
			auto frand = [&rng](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			p.transform_.translate_ = center;

			// 本体より細く長い芯
			float width = frand(0.018f, 0.040f);
			float length = frand(0.60f, 1.25f);
			p.transform_.scale_ = { width, length, 1.0f };

			p.transform_.rotate_ = {
				0.0f,
				0.0f,
				frand(-1.00f, 1.00f)
			};

			p.velocity_ = {
				frand(-0.003f, 0.003f),
				frand(-0.003f, 0.003f),
				frand(-0.003f, 0.003f)
			};

			p.lifeTime_ = frand(0.035f, 0.075f);
			p.currentTime_ = 0.0f;

			// 白ライムコア（トレイルより明るい）
			float t = frand(0.0f, 1.0f);
			float r = 0.85f + 0.10f * t;
			float g = 1.0f;
			float b = 0.55f + 0.10f * t;
			p.color_ = { r, g, b, 1.0f };
		} else if (groupName == "bossWarp_core") {

			p.transform_.translate_ = center;

			float sc = std::uniform_real_distribution<float>(3.5f, 6.5f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.35f, 0.65f)(rng);
			p.currentTime_ = 0.0f;

			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			p.color_ = {
				0.70f + 0.10f * t,
				0.88f + 0.07f * t,
				1.10f + 0.15f * t,
				1.0f
			};
		} else if (groupName == "bossWarp_swirl") {

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			float ang = frand(0.0f, 2.0f * std::numbers::pi_v<float>);
			float radius = frand(6.0f, 15.0f);
			float y = frand(-5.0f, 5.0f);

			Vector3 local = {
				std::cos(ang) * radius,
				y,
				std::sin(ang) * radius
			};

			p.transform_.translate_ = center + local;

			Vector3 toCenter = MyMath::Normalize(-local);
			Vector3 tangent = MyMath::Normalize(Vector3{ -local.z, 0.0f, local.x });

			float inward = frand(3.0f, 5.8f);
			float spin = frand(1.0f, 2.6f);

			p.velocity_ = toCenter * inward + tangent * spin;

			float sc = frand(0.45f, 1.10f);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = frand(0.55f, 1.00f);
			p.currentTime_ = 0.0f;

			float c = frand(0.0f, 1.0f);
			p.color_ = {
				0.45f + 0.12f * c,
				0.65f + 0.15f * c,
				0.95f + 0.20f * c,
				1.0f
			};
		} else if (groupName == "bossWarp_dust") {

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			float ang = frand(0.0f, 2.0f * std::numbers::pi_v<float>);
			float radius = frand(8.0f, 18.0f);
			float y = frand(-4.0f, 4.0f);

			Vector3 local = {
				std::cos(ang) * radius,
				y,
				std::sin(ang) * radius
			};

			p.transform_.translate_ = center + local;

			Vector3 toCenter = MyMath::Normalize(-local);
			float spd = frand(2.0f, 4.0f);
			p.velocity_ = toCenter * spd;

			float thin = frand(0.12f, 0.24f);
			float len = frand(0.8f, 1.6f);
			p.transform_.scale_ = { thin, len, 1.0f };

			p.lifeTime_ = frand(0.35f, 0.60f);
			p.currentTime_ = 0.0f;

			float c = frand(0.0f, 1.0f);
			p.color_ = {
				0.72f + 0.08f * c,
				0.84f + 0.08f * c,
				1.00f,
				1.0f
			};
		} else if (groupName == "bossNoticeMark") {

			p.transform_.translate_ = center;

			// ！マークなので少し縦長に
			float sx = std::uniform_real_distribution<float>(1.4f, 1.8f)(rng);
			float sy = std::uniform_real_distribution<float>(2.2f, 3.0f)(rng);
			p.transform_.scale_ = { sx, sy, 1.0f };

			// 少し上に抜ける
			float vx = std::uniform_real_distribution<float>(-0.15f, 0.15f)(rng);
			float vy = std::uniform_real_distribution<float>(1.8f, 2.6f)(rng);
			float vz = std::uniform_real_distribution<float>(-0.15f, 0.15f)(rng);
			p.velocity_ = { vx, vy, vz };

			p.lifeTime_ = std::uniform_real_distribution<float>(1.2f, 1.6f)(rng);
			p.currentTime_ = 0.0f;

			// アニメっぽく黄色寄り。白でもいいけどまずは見やすさ優先
			p.color_ = { 0.0f, 1.0f, 1.0f, 1.0f };
		} else if (groupName == "bossEscape_warpCore") {

			p.transform_.translate_ = center;

			float sc = std::uniform_real_distribution<float>(3.5f, 5.8f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.28f, 0.45f)(rng);
			p.currentTime_ = 0.0f;

			float c = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			p.color_ = {
				0.70f + 0.12f * c,
				0.90f + 0.06f * c,
				1.15f + 0.18f * c,
				1.0f
			};
		} else if (groupName == "bossEscape_warpSwirl") {

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			float ang = frand(0.0f, 2.0f * std::numbers::pi_v<float>);
			float radius = frand(4.5f, 10.0f);
			float y = frand(-3.5f, 3.5f);

			Vector3 local = {
				std::cos(ang) * radius,
				y,
				std::sin(ang) * radius
			};

			p.transform_.translate_ = center + local;

			Vector3 toCenter = MyMath::Normalize(-local);
			Vector3 tangent = MyMath::Normalize(Vector3{ -local.z, 0.0f, local.x });

			float inward = frand(4.5f, 8.0f);
			float spin = frand(0.8f, 2.2f);

			p.velocity_ = toCenter * inward + tangent * spin;

			float sc = frand(0.45f, 1.00f);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = frand(0.35f, 0.70f);
			p.currentTime_ = 0.0f;

			float t = frand(0.0f, 1.0f);
			p.color_ = {
				0.38f + 0.10f * t,
				0.62f + 0.12f * t,
				0.95f + 0.18f * t,
				1.0f
			};
		} else if (groupName == "bossEscape_warpShred") {

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			float ang = frand(0.0f, 2.0f * std::numbers::pi_v<float>);
			float radius = frand(2.0f, 6.0f);
			float y = frand(-3.0f, 3.0f);

			Vector3 local = {
				std::cos(ang) * radius,
				y,
				std::sin(ang) * radius
			};

			p.transform_.translate_ = center + local;

			Vector3 toCenter = MyMath::Normalize(-local);
			float spd = frand(8.0f, 13.0f);
			p.velocity_ = toCenter * spd;

			float thin = frand(0.10f, 0.20f);
			float len = frand(1.6f, 3.4f);
			p.transform_.scale_ = { thin, len, 1.0f };

			p.lifeTime_ = frand(0.20f, 0.35f);
			p.currentTime_ = 0.0f;

			float t = frand(0.0f, 1.0f);
			p.color_ = {
				0.65f + 0.10f * t,
				0.82f + 0.08f * t,
				1.00f,
				1.0f
			};
		} else if (groupName == "bossEscape_warpRing") {

			p.transform_.translate_ = center;

			float sc = std::uniform_real_distribution<float>(6.0f, 8.0f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.30f, 0.45f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 0.72f, 0.88f, 1.08f, 1.0f };
		} else if (groupName == "bossEntrance_gather") {
			std::uniform_real_distribution<float> rDist(1.8f, 5.0f);
			std::uniform_real_distribution<float> ang(0.0f, 6.2831853f);
			std::uniform_real_distribution<float> h(-1.5f, 1.8f);

			float r = rDist(rng);
			float a = ang(rng);

			Vector3 offset{
				std::cos(a) * r,
				h(rng),
				std::sin(a) * r
			};

			p.transform_.translate_ = center + offset;

			Vector3 dir = MyMath::Normalize(center - p.transform_.translate_);
			float speed = std::uniform_real_distribution<float>(18.0f, 30.0f)(rng);
			p.velocity_ = dir * speed * 0.016f;

			float sc = std::uniform_real_distribution<float>(0.18f, 0.45f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.25f, 0.45f)(rng);
			p.currentTime_ = 0.0f;

			float red = std::uniform_real_distribution<float>(0.85f, 1.00f)(rng);
			float green = std::uniform_real_distribution<float>(0.05f, 0.18f)(rng);
			float blue = std::uniform_real_distribution<float>(0.05f, 0.14f)(rng);
			p.color_ = { red, green, blue, 1.0f };
		} else if (groupName == "bossEntrance_ringShock") {
			p.transform_.translate_ = center;
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = std::uniform_real_distribution<float>(1.8f, 2.6f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.22f, 0.34f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 1.0f, 0.18f, 0.12f, 1.0f };
		} else if (groupName == "bossEntrance_smoke") {
			auto frand = [&rng](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// 出現位置のまわりに広く、特にZ方向へ厚みを持たせる
			p.transform_.translate_ = center + Vector3{
				frand(-6.5f, 6.5f),
				frand(-3.0f, 3.0f),
				frand(-10.0f, 10.0f)
			};

			// ゆっくり広がる
			p.velocity_ = {
				frand(-0.24f, 0.24f),
				frand(0.03f, 0.18f),
				frand(-0.20f, 0.20f)
			};

			// 大きめ
			float sc = frand(5.5f, 9.5f);
			p.transform_.scale_ = { sc, sc, sc };

			// 少し長めに残す
			p.lifeTime_ = frand(0.90f, 1.40f);
			p.currentTime_ = 0.0f;

			float t = frand(0.0f, 1.0f);
			p.color_ = {
				0.15f + 0.10f * t,
				0.03f + 0.03f * t,
				0.03f + 0.03f * t,
				0.92f
			};
		} else if (groupName == "bossEntrance_ringThin") {
			p.transform_.translate_ = center;
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = std::uniform_real_distribution<float>(1.2f, 1.8f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.18f, 0.28f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 0.95f, 0.08f, 0.08f, 1.0f };
		} else if (groupName == "bossEntrance_streak") {
			std::uniform_real_distribution<float> ang(0.0f, 6.2831853f);
			std::uniform_real_distribution<float> rad(0.4f, 1.5f);

			float a = ang(rng);
			float r = rad(rng);

			p.transform_.translate_ = center + Vector3{
				std::cos(a) * r,
				std::uniform_real_distribution<float>(-0.6f, 0.6f)(rng),
				std::sin(a) * r
			};

			Vector3 dir = MyMath::Normalize(p.transform_.translate_ - center);
			float speed = std::uniform_real_distribution<float>(26.0f, 44.0f)(rng);
			p.velocity_ = dir * speed * 0.016f;

			float thin = std::uniform_real_distribution<float>(0.08f, 0.16f)(rng);
			float len = std::uniform_real_distribution<float>(0.8f, 1.9f)(rng);
			p.transform_.scale_ = { thin, thin, len };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.12f, 0.22f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 1.0f, 0.22f, 0.10f, 1.0f };
		} else if (groupName == "bossEntrance_spark") {
			std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
			Vector3 v{ dir(rng), dir(rng) * 0.45f, dir(rng) };
			if (MyMath::Length(v) < 0.001f) {
				v = { 0.0f, 0.0f, 1.0f };
			}
			v = MyMath::Normalize(v);

			float speed = std::uniform_real_distribution<float>(20.0f, 36.0f)(rng);
			p.velocity_ = v * speed * 0.016f;

			p.transform_.translate_ = center;

			float sc = std::uniform_real_distribution<float>(0.10f, 0.22f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.10f, 0.18f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 1.0f, 0.35f, 0.12f, 1.0f };
		} else if (groupName == "w1sp_stream") {
			// 帯の中に混ざる補助粒
			std::uniform_real_distribution<float> vel(-0.006f, 0.006f);
			p.velocity_ = { vel(rng), vel(rng), vel(rng) };

			float sc = std::uniform_real_distribution<float>(0.10f, 0.18f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.10f, 0.18f)(rng);
			p.currentTime_ = 0.0f;

			// 邪悪な赤
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.88f + 0.12f * t;
			float g = 0.02f + 0.02f * t;
			float b = 0.42f + 0.18f * t;
			p.color_ = { r, g, b, 0.82f };

		} else if (groupName == "w1sp_sender_glow") {
			// 発射元の小さな発光
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = std::uniform_real_distribution<float>(0.45f, 0.85f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.12f, 0.22f)(rng);
			p.currentTime_ = 0.0f;

			// 色は赤系で、明るさにばらつきを持たせる
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.45f + 0.18f * t;
			float g = 0.01f + 0.03f * t;
			float b = 0.10f + 0.18f * t;
			p.color_ = { r, g, b, 0.82f };

		} else if (groupName == "w1sp_core_body") {
			// コア本体の発光
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = std::uniform_real_distribution<float>(2.0f, 3.2f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.12f, 0.20f)(rng);
			p.currentTime_ = 0.0f;

			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.08f + 0.10f * t;
			float g = 0.00f + 0.03f * t;
			float b = 0.28f + 0.18f * t;
			p.color_ = { r, g, b, 0.96f };

		} else if (groupName == "w1sp_core_ring") {
			// コア周囲のリング
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = std::uniform_real_distribution<float>(1.6f, 2.8f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.10f, 0.18f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 0.85f, 0.06f, 0.45f, 0.78f };

		} else if (groupName == "w1sp_core_smoke") {
			// コア周りのエネルギーもや
			std::uniform_real_distribution<float> velX(-0.02f, 0.02f);
			std::uniform_real_distribution<float> velY(-0.01f, 0.03f);
			std::uniform_real_distribution<float> velZ(-0.02f, 0.02f);
			p.velocity_ = { velX(rng), velY(rng), velZ(rng) };

			float sc = std::uniform_real_distribution<float>(0.8f, 1.5f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.35f, 0.65f)(rng);
			p.currentTime_ = 0.0f;

			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			// 色は赤系のもやっとした感じで、明るさにばらつきを持たせる
			float r = 0.10f + 0.08f * t;
			float g = 0.01f + 0.02f * t;
			float b = 0.12f + 0.10f * t;
			p.color_ = { r, g, b, 0.42f };

		} else if (groupName == "w1sp_core_flash") {
			// 発射直前 / 発射瞬間の強い閃光
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = std::uniform_real_distribution<float>(1.8f, 3.2f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.08f, 0.14f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 1.00f, 0.10f, 0.55f, 1.0f };

		} else if (groupName == "w1sp_core_spark") {
			// 発射時に飛び散る火花
			std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
			Vector3 v = { dir(rng), dir(rng), dir(rng) };
			if (MyMath::Length(v) > 0.001f) {
				v = MyMath::Normalize(v);
			} else {
				v = { 0.0f, 1.0f, 0.0f };
			}
			float spd = std::uniform_real_distribution<float>(0.4f, 1.2f)(rng);
			p.velocity_ = v * spd;

			float sc = std::uniform_real_distribution<float>(0.12f, 0.28f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.18f, 0.32f)(rng);
			p.currentTime_ = 0.0f;

			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 1.00f;
			float g = 0.22f + 0.25f * t;
			float b = 0.03f + 0.06f * t;
			p.color_ = { r, g, b, 1.0f };
		} else if (groupName == "w1sp_fly_body") {
			// 飛翔中コア本体の発光
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = std::uniform_real_distribution<float>(2.4f, 3.8f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.08f, 0.14f)(rng);
			p.currentTime_ = 0.0f;

			// 色は暗い赤紫系で、明るさにばらつきを持たせる
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			// 黒ベース + 赤い割れ目
			float r = 0.35f + 0.45f * t;
			float g = 0.01f + 0.03f * t;
			float b = 0.01f + 0.02f * t;

			p.color_ = { r, g, b, 0.98f };

		} else if (groupName == "w1sp_fly_spark") {
			// 周囲に散る火花
			std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
			Vector3 v = { dir(rng), dir(rng), dir(rng) };
			if (MyMath::Length(v) > 0.001f) {
				v = MyMath::Normalize(v);
			} else {
				v = { 0.0f, 1.0f, 0.0f };
			}
			float spd = std::uniform_real_distribution<float>(0.12f, 0.45f)(rng);
			p.velocity_ = v * spd;

			float sc = std::uniform_real_distribution<float>(0.10f, 0.24f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.12f, 0.24f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 0.75f, 0.95f, 1.00f, 0.95f };
		} else if (groupName == "w1sp_stream_streak") {
			// 敵→コアへ走る補助の細線
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float len = std::uniform_real_distribution<float>(2.4f, 4.2f)(rng);
			float thin = std::uniform_real_distribution<float>(0.05f, 0.10f)(rng);
			p.transform_.scale_ = { thin, thin, len };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.10f, 0.18f)(rng);
			p.currentTime_ = 0.0f;

			// 真っ赤
			p.color_ = { 1.0f, 0.0f, 0.0f, 1.0f };
		} else if (groupName == "w1sp_core_inner") {
			// コア内部で暴れる高密度粒
			std::uniform_real_distribution<float> vel(-0.08f, 0.08f);
			p.velocity_ = { vel(rng), vel(rng), vel(rng) };

			float sc = std::uniform_real_distribution<float>(0.18f, 0.42f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.10f, 0.18f)(rng);
			p.currentTime_ = 0.0f;

			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.12f + 0.10f * t;
			float g = 0.05f + 0.05f * t;
			float b = 0.55f + 0.20f * t;
			p.color_ = { r, g, b, 1.0f };

		} else if (groupName == "w1sp_core_shell") {
			// コアの外殻リング
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = std::uniform_real_distribution<float>(2.2f, 3.8f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.14f, 0.24f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 0.10f, 0.01f, 0.01f, 0.88f };

		} else if (groupName == "w1sp_core_arc") {
			// コアの周囲を暴れる放電
			std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
			Vector3 v = { dir(rng), dir(rng), dir(rng) };
			if (MyMath::Length(v) > 0.001f) {
				v = MyMath::Normalize(v);
			} else {
				v = { 1.0f, 0.0f, 0.0f };
			}
			float spd = std::uniform_real_distribution<float>(0.10f, 0.30f)(rng);
			p.velocity_ = v * spd;

			float len = std::uniform_real_distribution<float>(0.8f, 1.8f)(rng);
			float thin = std::uniform_real_distribution<float>(0.03f, 0.08f)(rng);
			p.transform_.scale_ = { thin, thin, len };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.06f, 0.12f)(rng);
			p.currentTime_ = 0.0f;

			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.05f + 0.05f * t;
			float g = 0.75f + 0.20f * t;
			float b = 0.45f + 0.20f * t;
			p.color_ = { r, g, b, 0.95f };

		} else if (groupName == "w1sp_core_burst") {
			// 発射時の破裂片
			std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
			Vector3 v = { dir(rng), dir(rng), dir(rng) };
			if (MyMath::Length(v) > 0.001f) {
				v = MyMath::Normalize(v);
			} else {
				v = { 0.0f, 1.0f, 0.0f };
			}
			float spd = std::uniform_real_distribution<float>(0.45f, 1.40f)(rng);
			p.velocity_ = v * spd;

			float sc = std::uniform_real_distribution<float>(0.14f, 0.28f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.14f, 0.24f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 0.90f, 0.08f, 0.35f, 1.0f };

		} else if (groupName == "w1sp_fly_body") {
			// 飛翔中の核（黒い塊の中に赤い割れ目）
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = std::uniform_real_distribution<float>(2.8f, 4.2f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.05f, 0.09f)(rng);
			p.currentTime_ = 0.0f;

			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.08f + 0.10f * t;
			float g = 0.01f + 0.03f * t;
			float b = 0.32f + 0.20f * t;
			p.color_ = { r, g, b, 0.98f };

		} else if (groupName == "w1sp_fly_shell") {
			// 飛翔中の外殻（黒赤い輪郭）
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float sc = std::uniform_real_distribution<float>(3.2f, 4.8f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.06f, 0.10f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 0.10f, 0.01f, 0.01f, 0.90f };

		} else if (groupName == "w1sp_fly_arc") {
			// 飛翔中の赤い放電
			std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
			Vector3 v = { dir(rng), dir(rng), dir(rng) };
			if (MyMath::Length(v) > 0.001f) {
				v = MyMath::Normalize(v);
			} else {
				v = { 1.0f, 0.0f, 0.0f };
			}
			float spd = std::uniform_real_distribution<float>(0.10f, 0.28f)(rng);
			p.velocity_ = v * spd;

			float len = std::uniform_real_distribution<float>(1.5f, 2.8f)(rng);
			float thin = std::uniform_real_distribution<float>(0.03f, 0.08f)(rng);
			p.transform_.scale_ = { thin, thin, len };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.05f, 0.09f)(rng);
			p.currentTime_ = 0.0f;

			p.color_ = { 0.06f, 0.95f, 0.62f, 0.98f };

		} else if (groupName == "w1sp_fly_tail") {
			// 飛翔中の暗赤い残滓
			std::uniform_real_distribution<float> vx(-0.03f, 0.03f);
			std::uniform_real_distribution<float> vy(-0.03f, 0.03f);
			std::uniform_real_distribution<float> vz(0.18f, 0.40f);
			p.velocity_ = { vx(rng), vy(rng), vz(rng) };

			float sc = std::uniform_real_distribution<float>(0.9f, 1.8f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.10f, 0.16f)(rng);
			p.currentTime_ = 0.0f;

			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.04f + 0.05f * t;
			float g = 0.00f + 0.02f * t;
			float b = 0.18f + 0.14f * t;
			p.color_ = { r, g, b, 0.72f };

		} else if (groupName == "w1sp_fly_spark") {
			// 飛翔中の赤橙火花
			std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
			Vector3 v = { dir(rng), dir(rng), dir(rng) };
			if (MyMath::Length(v) > 0.001f) {
				v = MyMath::Normalize(v);
			} else {
				v = { 0.0f, 1.0f, 0.0f };
			}
			float spd = std::uniform_real_distribution<float>(0.70f, 1.80f)(rng);
			p.velocity_ = v * spd;

			float sc = std::uniform_real_distribution<float>(0.10f, 0.22f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.07f, 0.13f)(rng);
			p.currentTime_ = 0.0f;

			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 1.00f;
			float g = 0.18f + 0.22f * t;
			float b = 0.02f + 0.05f * t;
			p.color_ = { r, g, b, 1.0f };

			p.color_ = { r, g, b, 1.0f };
		} else if (groupName == "w1sp_fly_corona") {
			// 飛翔中の外周炎膜
			std::uniform_real_distribution<float> vel(-0.05f, 0.05f);
			p.velocity_ = { vel(rng), vel(rng), vel(rng) };

			float sc = std::uniform_real_distribution<float>(2.6f, 4.4f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.08f, 0.14f)(rng);
			p.currentTime_ = 0.0f;

			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.85f + 0.15f * t;
			float g = 0.10f + 0.10f * t;
			float b = 0.03f + 0.03f * t;
			p.color_ = { r, g, b, 0.70f };
		} else if (groupName == "w1sp_stream_core") {
			// 敵→コアへ送るエネルギー線の芯
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float thin = std::uniform_real_distribution<float>(0.5f, 0.6f)(rng);
			float len = std::uniform_real_distribution<float>(2.8f, 4.8f)(rng);
			p.transform_.scale_ = { thin, thin, len };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.12f, 0.22f)(rng);
			p.currentTime_ = 0.0f;

			// 赤白ベースに少しピンクを混ぜる
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.10f + 0.08f * t;
			float g = 0.02f + 0.05f * t;
			float b = 0.42f + 0.22f * t;
			p.color_ = { r, g, b, 0.88f };

		} else if (groupName == "w1sp_stream_glow") {
			// 敵→コアへ送るエネルギー線の外側グロー
			p.velocity_ = { 0.0f, 0.0f, 0.0f };

			float thin = std::uniform_real_distribution<float>(0.16f, 0.28f)(rng);
			float len = std::uniform_real_distribution<float>(3.4f, 5.8f)(rng);
			p.transform_.scale_ = { thin, thin, len };

			p.lifeTime_ = std::uniform_real_distribution<float>(0.16f, 0.28f)(rng);
			p.currentTime_ = 0.0f;

			// 赤グロー
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.85f + 0.15f * t;
			float g = 0.08f + 0.10f * t;
			float b = 0.02f + 0.04f * t;
			p.color_ = { r, g, b, 0.55f };
		} else { // 上記意外
			// ── 既存：ヒット/汎用（上にふわっと・暖色系） ──
			std::uniform_real_distribution<float> velX(-0.15f, 0.15f);
			std::uniform_real_distribution<float> velY(0.10f, 0.30f);
			p.velocity_ = { velX(rng), velY(rng), velX(rng) };

			float sc = std::uniform_real_distribution<float>(1.0f, 2.0f)(rng);
			p.transform_.scale_ = { sc, sc, sc };

			float life = std::uniform_real_distribution<float>(0.6f, 1.2f)(rng);
			p.lifeTime_ = life; p.currentTime_ = 0.0f;

			float base = std::uniform_real_distribution<float>(0.8f, 1.0f)(rng);
			p.color_ = { base, base * 0.5f, base * 0.2f, 1.0f };
		}
		return p;
	}

}