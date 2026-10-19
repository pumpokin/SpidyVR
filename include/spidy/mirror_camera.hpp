#pragma once
#include "math.hpp"
#include <algorithm>
#include <cmath>

// The recording view: what the game's window (and so a recording of it) shows
// of the left eye in VR (the VR settings' RECORDING VIEW and RECORDING FOV).
// RAW is the eye as it is, cut to the window's shape. SMOOTH and STEADY show
// it through a camera that follows the head smoothly - STEADY slower, and with
// the horizon held level - by drawing the part of the eye image that camera
// sees: the eye is rendered wider than a recording needs, so a camera a few
// degrees behind the head still sees inside it. Engine independent: the eye
// image's orientation and lens in, where the window samples the image out.
namespace spidy::mirror_camera {
enum Mode : int { raw = 0, smooth = 1, steady = 2 };
// An eye image's lens as tangents: left and down below 0.
struct Tangents {
    float left{}, right{}, up{}, down{};
};
// Where the window samples the eye image (texture coordinates, x right and y
// down): at the window's centre, and how far each window half-axis reaches.
// A window pixel at (wx, wy), 0 to 1 across it, samples
// centre + (2 wx - 1) axisX + (2 wy - 1) axisY.
struct Part {
    float centre[2]{.5f, .5f}, axisX[2]{.5f, 0}, axisY[2]{0, .5f};
};
// Where a direction in the eye's own axes (OpenXR: -z ahead, y up) lands on
// its image; false behind the eye.
inline bool project(Vec3 d, Tangents t, float& u, float& v) {
    if (!(d.z < -1e-4f) || !(t.right > t.left) || !(t.up > t.down))
        return false;
    const float x = d.x / -d.z, y = d.y / -d.z;
    u = (x - t.left) / (t.right - t.left);
    v = (t.up - y) / (t.up - t.down);
    return std::isfinite(u) && std::isfinite(v);
}
// The widest a window of this shape (`aspect`, width over height) may see of
// an image with this lens, as the tangent of half its width: every edge
// inside the image about its own straight ahead.
inline float widest(Tangents t, float aspect) {
    const float x = std::min(-t.left, t.right), y = std::min(t.up, -t.down);
    return std::max(.05f, std::min(x, y * aspect));
}
// The part of the image a camera turned `relative` from the eye (the camera's
// orientation in the eye's axes) sees, `halfWidth` wide (tangent; held to
// what the image covers) for a window `aspect` wide per high.
inline bool part(Quat relative, Tangents t, float halfWidth, float aspect, Part& out) {
    if (!(aspect > .1f && aspect < 10) || !(halfWidth > 0) || !std::isfinite(halfWidth))
        return false;
    const float hx = std::min(halfWidth, widest(t, aspect)), hy = hx / aspect;
    float cu{}, cv{}, xu{}, xv{}, yu{}, yv{};
    if (!project(relative.rotate({0, 0, -1}), t, cu, cv) || !project(relative.rotate({hx, 0, -1}), t, xu, xv) ||
        !project(relative.rotate({0, -hy, -1}), t, yu, yv))
        return false;
    out.centre[0] = cu;
    out.centre[1] = cv;
    out.axisX[0] = xu - cu;
    out.axisX[1] = xv - cv;
    out.axisY[0] = yu - cu;
    out.axisY[1] = yv - cv;
    return true;
}
// RAW's part: the image's middle cut to the window's shape, as before.
inline Part rawPart(float imageAspect, float windowAspect) {
    Part p;
    const float across = windowAspect < imageAspect ? windowAspect / imageAspect : 1.f;
    const float down = windowAspect < imageAspect ? 1.f : imageAspect / windowAspect;
    p.axisX[0] = across / 2;
    p.axisY[1] = down / 2;
    return p;
}
// The camera of SMOOTH and STEADY, following a head in the tracking space.
class Follow {
  public:
    // It closes 63% of the gap to the head in `seconds` (SMOOTH, STEADY),
    // and never lags more than `lag` radians behind it, so it stays inside
    // the eye image.
    float smoothSeconds = .12f, steadySeconds = .3f, smoothLag = .26f, steadyLag = .35f;
    Quat update(float seconds, Quat head, int mode) {
        if (mode == raw || !std::isfinite(seconds)) {
            started_ = false;
            return head;
        }
        Quat want = head;
        if (mode == steady) {
            // The horizon level: the head's look, with no roll.
            const Vec3 f = head.rotate({0, 0, -1});
            const float yaw = std::atan2(-f.x, -f.z), pitch = std::asin(std::clamp(f.y, -1.f, 1.f));
            want = Quat::yaw(yaw) * Quat::around({1, 0, 0}, pitch);
        }
        if (!started_) {
            camera_ = want;
            started_ = true;
            return camera_;
        }
        const float dt = std::clamp(seconds, 0.f, .25f);
        camera_ = toward(camera_, want, 1 - std::exp(-dt / (mode == steady ? steadySeconds : smoothSeconds)));
        // Held within the lag of where the head looks.
        const float most = mode == steady ? steadyLag : smoothLag;
        const float behind = angle(camera_, want);
        if (behind > most)
            camera_ = toward(camera_, want, 1 - most / behind);
        return camera_;
    }
    void reset() {
        started_ = false;
    }
    static float angle(Quat a, Quat b) {
        const float d = std::abs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
        return 2 * std::acos(std::clamp(d, 0.f, 1.f));
    }

  private:
    static Quat toward(Quat a, Quat b, float k) {
        if (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w < 0)
            b = {-b.x, -b.y, -b.z, -b.w};
        Quat q{a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k, a.z + (b.z - a.z) * k, a.w + (b.w - a.w) * k};
        const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        return n > 1e-6f ? Quat{q.x / n, q.y / n, q.z / n, q.w / n} : b;
    }
    Quat camera_{};
    bool started_{};
};
} // namespace spidy::mirror_camera
