#include "BarrierConfig.h"
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

namespace {
	Vector3 ParseVector3(const json& j, const Vector3& fallback) {
		Vector3 result = fallback;

		if (!j.is_object()) {
			return result;
		}

		if (j.contains("x")) { result.x = j["x"].get<float>(); }
		if (j.contains("y")) { result.y = j["y"].get<float>(); }
		if (j.contains("z")) { result.z = j["z"].get<float>(); }

		return result;
	}

	Vector4 ParseVector4(const json& j, const Vector4& fallback) {
		Vector4 result = fallback;

		if (!j.is_object()) {
			return result;
		}

		if (j.contains("x")) { result.x = j["x"].get<float>(); }
		if (j.contains("y")) { result.y = j["y"].get<float>(); }
		if (j.contains("z")) { result.z = j["z"].get<float>(); }
		if (j.contains("w")) { result.w = j["w"].get<float>(); }

		return result;
	}
}

bool BarrierConfig::Load(const char* path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		return false;
	}

	json root;
	file >> root;

	if (root.contains("barrier")) {
		const auto& b = root["barrier"];

		if (b.contains("followCore")) { barrier_.followCore_ = b["followCore"].get<bool>(); }
		if (b.contains("offset")) { barrier_.offset_ = ParseVector3(b["offset"], barrier_.offset_); }
		if (b.contains("radius")) { barrier_.radius_ = b["radius"].get<float>(); }
		if (b.contains("shapeScale")) { barrier_.shapeScale_ = ParseVector3(b["shapeScale"], barrier_.shapeScale_); }
		if (b.contains("color")) { barrier_.color_ = ParseVector4(b["color"], barrier_.color_); }

		if (b.contains("breakDuration")) {
			barrier_.breakDuration_ = b["breakDuration"].get<float>();
		}

		if (b.contains("shader")) {
			const auto& s = b["shader"];

			if (s.contains("fresnelPower")) { barrier_.shader_.fresnelPower_ = s["fresnelPower"].get<float>(); }
			if (s.contains("baseStrength")) { barrier_.shader_.baseStrength_ = s["baseStrength"].get<float>(); }
			if (s.contains("rimStrength")) { barrier_.shader_.rimStrength_ = s["rimStrength"].get<float>(); }
			if (s.contains("alphaBase")) { barrier_.shader_.alphaBase_ = s["alphaBase"].get<float>(); }
			if (s.contains("alphaRim")) { barrier_.shader_.alphaRim_ = s["alphaRim"].get<float>(); }

			if (s.contains("tint")) {
				barrier_.shader_.tint_ = ParseVector3(s["tint"], barrier_.shader_.tint_);
			}

			if (s.contains("hexScale")) { barrier_.shader_.hexScale_ = s["hexScale"].get<float>(); }
			if (s.contains("hexLineWidth")) { barrier_.shader_.hexLineWidth_ = s["hexLineWidth"].get<float>(); }
			if (s.contains("hexGlowStrength")) { barrier_.shader_.hexGlowStrength_ = s["hexGlowStrength"].get<float>(); }
			if (s.contains("hexAlpha")) { barrier_.shader_.hexAlpha_ = s["hexAlpha"].get<float>(); }

			if (s.contains("breakEdgeWidth")) { barrier_.shader_.breakEdgeWidth_ = s["breakEdgeWidth"].get<float>(); }
			if (s.contains("breakGlowStrength")) { barrier_.shader_.breakGlowStrength_ = s["breakGlowStrength"].get<float>(); }
			if (s.contains("breakNoiseScale")) { barrier_.shader_.breakNoiseScale_ = s["breakNoiseScale"].get<float>(); }

			if (s.contains("breakOrigin")) {
				barrier_.shader_.breakOrigin_ = ParseVector3(s["breakOrigin"], barrier_.shader_.breakOrigin_);
			}
		}
	}

	if (root.contains("core")) {
		const auto& c = root["core"];

		if (c.contains("model")) { core_.model_ = c["model"].get<std::string>(); }
		if (c.contains("count")) { core_.count_ = c["count"].get<int>(); }
		if (c.contains("hp")) { core_.hp_ = c["hp"].get<int>(); }
		if (c.contains("scale")) { core_.scale_ = ParseVector3(c["scale"], core_.scale_); }
		if (c.contains("colliderScale")) { core_.colliderScale_ = ParseVector3(c["colliderScale"], core_.colliderScale_); }

		if (c.contains("placement")) {
			const auto& p = c["placement"];

			if (p.contains("barrierOuterRadius")) { core_.placement_.barrierOuterRadius_ = p["barrierOuterRadius"].get<float>(); }
			if (p.contains("outerMargin")) { core_.placement_.outerMargin_ = p["outerMargin"].get<float>(); }
			if (p.contains("startAngleDeg")) { core_.placement_.startAngleDeg_ = p["startAngleDeg"].get<float>(); }
			if (p.contains("zOffset")) { core_.placement_.zOffset_ = p["zOffset"].get<float>(); }
		}

		if (c.contains("death")) {
			const auto& d = c["death"];

			if (d.contains("duration")) { core_.death_.duration_ = d["duration"].get<float>(); }
			if (d.contains("hitDirSpeed")) { core_.death_.hitDirSpeed_ = d["hitDirSpeed"].get<float>(); }
			if (d.contains("upVelocity")) { core_.death_.upVelocity_ = ParseVector3(d["upVelocity"], core_.death_.upVelocity_); }
			if (d.contains("rotateSpeed")) { core_.death_.rotateSpeed_ = ParseVector3(d["rotateSpeed"], core_.death_.rotateSpeed_); }
		}
	}

	return true;
}