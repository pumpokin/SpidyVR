#pragma once
#include "math.hpp"
#include <array>
#include <cmath>
#include <cstdint>

// Cutscenes in first person (the VR settings' CUTSCENES: FIRST PERSON; the
// player, October 20: "id like the cutscenes to stay as vr, not forcing me to
// watch him in third person"). While the game plays a scene, the eyes go to
// Spider-Man's eyes as the scene animates him (native_body reads his head
// from the game's own pose of the hero) and the player looks around with his
// own head: the hero's head turns never turn the view. As the scene starts the
// player faces the way Spider-Man does; when Spider-Man turns more than
// turnLimit from that, or jumps more than `jump` in a frame (a cut), the view
// turns once to match, as a snap turn does. A head that is stale, or far from
// the scene's own camera (he is not in this scene), is no first person: the
// scene goes on its screen as before. Engine independent: his head and the
// headset in, the eyes' world poses out.
namespace spidy::scene_view {
enum Mode : int { screen = 0, firstPerson = 1 };
// Spider-Man's head as the game posed it last (world).
struct Head {
    bool valid{};
    uint64_t tickMs{};  // when (GetTickCount64)
    Vec3 eyes{};        // between his eyes
    Vec3 forward{};     // where his face points
    Vec3 feet{};        // his actor's origin
};
// A native view pose (rows right, down, forward, position) for an OpenXR-axes
// pose (x right, y up, -z ahead), as game_tracking's eyes are.
inline Mat4 nativePose(Pose p) {
    const Vec3 x = p.orientation.rotate({1, 0, 0}), y = p.orientation.rotate({0, -1, 0}),
               z = p.orientation.rotate({0, 0, -1}), o = p.position;
    return {x.x, x.y, x.z, 0, y.x, y.y, y.z, 0, z.x, z.y, z.z, 0, o.x, o.y, o.z, 1};
}
struct Out {
    Pose head{};                  // world
    std::array<Pose, 2> eyes{};   // world
    std::array<Mat4, 2> native{}; // the eyes as native view poses
    bool realigned{};             // the view turned to his facing this frame
};
class SceneView {
  public:
    float freshMs = 250, cameraReach = 40, jump = 1, turnLimit = 1.0472f, leanMost = .4f;
    // nowMs: GetTickCount64. sceneCamera: the game's camera for the scene
    // (world; non-finite: unknown). trackedHead, trackedEyes: the headset in
    // the tracking space. False: no first person this frame.
    bool update(uint64_t nowMs, const Head& h, Vec3 sceneCamera, Pose trackedHead,
                const std::array<Pose, 2>& trackedEyes, Out& out) {
        out = {};
        const bool fresh = h.valid && nowMs >= h.tickMs && nowMs - h.tickMs <= freshMs;
        const bool near = !finite(sceneCamera) || length(h.eyes - sceneCamera) <= cameraReach;
        const float heroYaw = yawOf(h.forward), playerYaw = yawOf(trackedHead.orientation.rotate({0, 0, -1}));
        // His head stopped coming during a scene in first person (the game
        // stopped posing him): the rest of that scene stays on its screen,
        // rather than flickering between the two.
        if (started_ && !fresh)
            lapsed_ = true;
        if (lapsed_ || !fresh || !near || !finite(h.eyes) || !std::isfinite(heroYaw) || !std::isfinite(playerYaw) ||
            !finite(trackedHead.position)) {
            started_ = false;
            return false;
        }
        if (!started_ || length(h.eyes - lastEyes_) > jump) {
            // The scene began, or cut: face the way he does.
            align_ = heroYaw - playerYaw;
            alignedHero_ = heroYaw;
            alignHead_ = trackedHead.position;
            started_ = true;
            out.realigned = true;
        } else if (const float turned = wrap(heroYaw - alignedHero_); std::abs(turned) > turnLimit) {
            // He turned well away: the view turns with him, once.
            align_ += turned;
            alignedHero_ = heroYaw;
            out.realigned = true;
        }
        lastEyes_ = h.eyes;
        const Quat q = Quat::yaw(align_);
        // The player's own lean moves the eyes a little, never far.
        Vec3 lean = q.rotate(trackedHead.position - alignHead_);
        if (length(lean) > leanMost)
            lean = normalized(lean) * leanMost;
        out.head = {h.eyes + lean, q * trackedHead.orientation};
        for (unsigned i = 0; i < 2; ++i) {
            out.eyes[i] = {out.head.position + q.rotate(trackedEyes[i].position - trackedHead.position),
                           q * trackedEyes[i].orientation};
            out.native[i] = nativePose(out.eyes[i]);
        }
        return true;
    }
    // The scene is over (the game plays again): the next one starts afresh.
    void reset() {
        started_ = lapsed_ = false;
    }
    bool started() const {
        return started_;
    }

  private:
    // Quat::yaw(r) turns -z to (-sin r, 0, -cos r).
    static float yawOf(Vec3 forward) {
        const Vec3 f{forward.x, 0, forward.z};
        if (!finite(f) || length(f) < .1f)
            return NAN;
        return std::atan2(-f.x, -f.z);
    }
    static float wrap(float a) {
        return a - 6.2831853f * std::round(a / 6.2831853f);
    }
    float align_{}, alignedHero_{};
    Vec3 alignHead_{}, lastEyes_{};
    bool started_{}, lapsed_{};
};
} // namespace spidy::scene_view
