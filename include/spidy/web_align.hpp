#pragma once
#include "math.hpp"
#include <algorithm>
#include <array>
#include <cmath>

// Web align (the VR settings' WEB ALIGN): while a web holds the player in the
// air, the body leans so its head points up the web, as a pendulum hangs from
// its pivot; with BODY + VIEW the view leans with it. Let go, or land, and it
// comes back upright. Engine independent: the webs and the body in, a tilt of
// the player in world axes (the identity: upright) out.
namespace spidy::web_align {
enum Mode : int { off = 0, body = 1, bodyAndView = 2 };
class WebAlign {
  public:
    // The most it leans, radians, for the body and (gentler) for the view; it
    // reaches 63% of the way to the web's line in followSeconds, and back
    // upright in returnSeconds. An anchor less than minRise above the body's
    // centre (a web shot level or down) leans nothing.
    float maxBody = 1.15f, maxView = .7f, followSeconds = .18f, returnSeconds = .3f, minRise = .5f;
    // One frame. `holding`: in the air with these webs attached (their
    // anchors, world space), from a body whose centre is `centre`.
    Quat update(float seconds, bool holding, std::array<bool, 2> attached, std::array<Vec3, 2> anchors, Vec3 centre,
                bool view) {
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.f, .1f) : 0.f;
        Quat target{};
        Vec3 sum{};
        int count = 0;
        for (unsigned i = 0; i < 2; ++i)
            if (attached[i] && finite(anchors[i])) {
                sum = sum + anchors[i];
                ++count;
            }
        if (holding && count && finite(centre)) {
            const Vec3 up = sum / static_cast<float>(count) - centre;
            if (up.y > minRise && length(up) > 1e-3f) {
                const Vec3 d = normalized(up);
                const float angle = std::min(std::acos(std::clamp(d.y, -1.f, 1.f)), view ? maxView : maxBody);
                const Vec3 axis = cross(Vec3{0, 1, 0}, d);
                if (length(axis) > 1e-4f)
                    target = Quat::around(axis, angle);
            }
        }
        const bool leaning = target.x != 0 || target.y != 0 || target.z != 0;
        const float k = 1 - std::exp(-dt / (leaning ? followSeconds : returnSeconds));
        tilt_ = toward(tilt_, target, k);
        return tilt_;
    }
    Quat tilt() const {
        return tilt_;
    }
    // How far it leans now, radians.
    float angle() const {
        return 2 * std::atan2(std::sqrt(tilt_.x * tilt_.x + tilt_.y * tilt_.y + tilt_.z * tilt_.z), std::abs(tilt_.w));
    }
    void reset() {
        tilt_ = {};
    }

  private:
    // The share k of the way from a to b (the short way), as a unit quaternion.
    static Quat toward(Quat a, Quat b, float k) {
        if (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w < 0)
            b = {-b.x, -b.y, -b.z, -b.w};
        Quat q{a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k, a.z + (b.z - a.z) * k, a.w + (b.w - a.w) * k};
        const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        if (!(n > 1e-6f) || !std::isfinite(n))
            return {};
        q = {q.x / n, q.y / n, q.z / n, q.w / n};
        // Close enough to upright is upright: the identity, which the body
        // and the view take as no tilt at all.
        if (std::abs(q.w) > .99999f)
            return {};
        return q;
    }
    Quat tilt_{};
};
} // namespace spidy::web_align
