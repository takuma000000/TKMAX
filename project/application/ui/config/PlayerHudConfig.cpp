#include "PlayerHudConfig.h"

#include <fstream>
#include <algorithm>
#include <cctype>
#include "json.hpp"

using json = nlohmann::json; // JSONライブラリの名前空間と型エイリアス

namespace TKM {
	namespace {
		/// <summary>
		/// JSONオブジェクトからVector2を読み取る。指定されたキーが存在しない場合はfallbackの値を使用する。
		/// </summary>
		/// <param name="root">JSONオブジェクト</param>
		/// <param name="fallback">キーが存在しない場合のフォールバック値</param>
		/// <returns>読み取ったVector2</returns>
		Vector2 ReadVector2_(const json& root, const Vector2& fallback) {
			Vector2 result = fallback;

			if (root.contains("x")) {
				result.x = root["x"].get<float>();
			}
			if (root.contains("y")) {
				result.y = root["y"].get<float>();
			}

			return result;
		}
		/// <summary>
		/// JSONオブジェクトからVector4を読み取る。指定されたキーが存在しない場合はfallbackの値を使用する。
		/// </summary>
		/// <param name="root">JSONオブジェクト</param>
		/// <param name="fallback">キーが存在しない場合のフォールバック値</param>
		/// <returns>読み取ったVector4</returns>
		Vector4 ReadVector4_(const json& root, const Vector4& fallback) {
			Vector4 result = fallback;

			if (root.contains("x")) {
				result.x = root["x"].get<float>();
			}
			if (root.contains("y")) {
				result.y = root["y"].get<float>();
			}
			if (root.contains("z")) {
				result.z = root["z"].get<float>();
			}
			if (root.contains("w")) {
				result.w = root["w"].get<float>();
			}

			return result;
		}
	}
	/// <summary>
	/// ファイルパスが指定された拡張子で終わっているかをチェックする。大文字小文字は区別しない。
	/// </summary>
	/// <param name="path">ファイルパス</param>
	/// <param name="ext">拡張子（例: ".json"）</param>
	/// <returns>持っていればtrue、持っていなければfalse</returns>
	bool PlayerHudConfig::HasExtension(const std::string& path, const char* ext) {
		if (!ext) {
			return false;
		}

		std::string lowerPath = path;
		std::string lowerExt = ext;

		std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (lowerPath.size() < lowerExt.size()) {
			return false;
		}

		return lowerPath.compare(lowerPath.size() - lowerExt.size(), lowerExt.size(), lowerExt) == 0;
	}

	bool PlayerHudConfig::Load(const char* path) {
		if (!path) {
			return false;
		}
		// 拡張子チェックは大文字小文字を区別しない
		const std::string path_ = path;
		// 現在はJSONのみ対応。将来的に他の形式を追加する場合はここで分岐させる。
		if (HasExtension(path_, ".json")) {
			return LoadJson(path);
		}

		return false;
	}

	bool PlayerHudConfig::LoadJson(const char* path) {
		if (!path) {
			return false;
		}
		// JSONファイルを開く
		std::ifstream ifs(path);
		// ファイルが開けなかった場合は失敗
		if (!ifs.is_open()) {
			return false;
		}
		// JSONをパースする
		json root;
		ifs >> root;

		// 各設定を読み取る。キーが存在しない場合はデフォルト値が使用される。
		if (root.contains("texture")) {
			auto& texture = root["texture"];

			if (texture.contains("rbGaugeIconTex")) {
				texture_.rbGaugeIconTex_ = texture["rbGaugeIconTex"].get<std::string>();
			}
			if (texture.contains("hpFrameTex")) {
				texture_.hpFrameTex_ = texture["hpFrameTex"].get<std::string>();
			}
			if (texture.contains("hpFillTex")) {
				texture_.hpFillTex_ = texture["hpFillTex"].get<std::string>();
			}
			if (texture.contains("hpIconTex")) {
				texture_.hpIconTex_ = texture["hpIconTex"].get<std::string>();
			}
		}
		// RBゲージアイコン設定の読み取り
		if (root.contains("rbGaugeIcon")) {
			auto& rbGaugeIcon = root["rbGaugeIcon"];

			if (rbGaugeIcon.contains("scale")) {
				rbGaugeIcon_.scale_ = rbGaugeIcon["scale"].get<float>();
			}
			if (rbGaugeIcon.contains("offset")) {
				rbGaugeIcon_.offset_ = ReadVector2_(rbGaugeIcon["offset"], rbGaugeIcon_.offset_);
			}
			if (rbGaugeIcon.contains("padX")) {
				rbGaugeIcon_.padX_ = rbGaugeIcon["padX"].get<float>();
			}
			if (rbGaugeIcon.contains("color")) {
				rbGaugeIcon_.color_ = ReadVector4_(rbGaugeIcon["color"], rbGaugeIcon_.color_);
			}
			if (rbGaugeIcon.contains("shakeAmpPx")) {
				rbGaugeIcon_.shakeAmpPx_ = rbGaugeIcon["shakeAmpPx"].get<float>();
			}
		}
		// LBゲージ設定の読み取り
		if (root.contains("lbGauge")) {
			auto& lbGauge = root["lbGauge"];

			if (lbGauge.contains("spacingY")) {
				lbGauge_.spacingY_ = lbGauge["spacingY"].get<float>();
			}
			if (lbGauge.contains("offset")) {
				lbGauge_.offset_ = ReadVector2_(lbGauge["offset"], lbGauge_.offset_);
			}
		}
		// HPゲージ設定の読み取り
		if (root.contains("hpGauge")) {
			auto& hpGauge = root["hpGauge"];

			if (hpGauge.contains("shakePower")) {
				hpGauge_.shakePower_ = hpGauge["shakePower"].get<float>();
			}
			if (hpGauge.contains("frameColor")) {
				hpGauge_.frameColor_ = ReadVector4_(hpGauge["frameColor"], hpGauge_.frameColor_);
			}
			if (hpGauge.contains("fillColor")) {
				hpGauge_.fillColor_ = ReadVector4_(hpGauge["fillColor"], hpGauge_.fillColor_);
			}
			if (hpGauge.contains("iconColor")) {
				hpGauge_.iconColor_ = ReadVector4_(hpGauge["iconColor"], hpGauge_.iconColor_);
			}
			if (hpGauge.contains("size")) {
				hpGauge_.size_ = ReadVector2_(hpGauge["size"], hpGauge_.size_);
			}
			if (hpGauge.contains("offset")) {
				hpGauge_.offset_ = ReadVector2_(hpGauge["offset"], hpGauge_.offset_);
			}
			if (hpGauge.contains("framePad")) {
				hpGauge_.framePad_ = hpGauge["framePad"].get<float>();
			}
			if (hpGauge.contains("iconOffset")) {
				hpGauge_.iconOffset_ = ReadVector2_(hpGauge["iconOffset"], hpGauge_.iconOffset_);
			}
			if (hpGauge.contains("iconScale")) {
				hpGauge_.iconScale_ = hpGauge["iconScale"].get<float>();
			}
			if (hpGauge.contains("segmentCount")) {
				hpGauge_.segmentCount_ = hpGauge["segmentCount"].get<int>();
			}
			if (hpGauge.contains("segmentGap")) {
				hpGauge_.segmentGap_ = hpGauge["segmentGap"].get<float>();
			}
			if (hpGauge.contains("segmentMinW")) {
				hpGauge_.segmentMinW_ = hpGauge["segmentMinW"].get<float>();
			}
			if (hpGauge.contains("segmentMaxW")) {
				hpGauge_.segmentMaxW_ = hpGauge["segmentMaxW"].get<float>();
			}
			if (hpGauge.contains("segmentSkewX")) {
				hpGauge_.segmentSkewX_ = hpGauge["segmentSkewX"].get<float>();
			}
			if (hpGauge.contains("backSegmentColor")) {
				hpGauge_.backSegmentColor_ = ReadVector4_(hpGauge["backSegmentColor"], hpGauge_.backSegmentColor_);
			}
			if (hpGauge.contains("outerFrameColor")) {
				hpGauge_.outerFrameColor_ = ReadVector4_(hpGauge["outerFrameColor"], hpGauge_.outerFrameColor_);
			}
			if (hpGauge.contains("outerFramePad")) {
				hpGauge_.outerFramePad_ = ReadVector2_(hpGauge["outerFramePad"], hpGauge_.outerFramePad_);
			}
		}
		// レイアウト設定の読み取り
		if (root.contains("layout")) {
			auto& layout = root["layout"];

			if (layout.contains("ammoUiRaiseY")) {
				layout_.ammoUiRaiseY_ = layout["ammoUiRaiseY"].get<float>();
			}
			if (layout.contains("hudLeftMargin")) {
				layout_.hudLeftMargin_ = layout["hudLeftMargin"].get<float>();
			}
			if (layout.contains("hudReserveLeftW")) {
				layout_.hudReserveLeftW_ = layout["hudReserveLeftW"].get<float>();
			}
			if (layout.contains("hudReserveGap")) {
				layout_.hudReserveGap_ = layout["hudReserveGap"].get<float>();
			}
			if (layout.contains("hudBottomMargin")) {
				layout_.hudBottomMargin_ = layout["hudBottomMargin"].get<float>();
			}
		}
		// HPエフェクト設定の読み取り
		if (root.contains("hpEffect")) {
			auto& hpEffect = root["hpEffect"];

			if (hpEffect.contains("hitFlashSec")) {
				hpEffect_.hitFlashSec_ = hpEffect["hitFlashSec"].get<float>();
			}
			if (hpEffect.contains("shakeSec")) {
				hpEffect_.shakeSec_ = hpEffect["shakeSec"].get<float>();
			}
			if (hpEffect.contains("shakeAmpPx")) {
				hpEffect_.shakeAmpPx_ = hpEffect["shakeAmpPx"].get<float>();
			}
			if (hpEffect.contains("drainEaseSec")) {
				hpEffect_.drainEaseSec_ = hpEffect["drainEaseSec"].get<float>();
			}
		}

		return true;
	}
}