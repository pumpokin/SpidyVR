#pragma once
#include "math.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

// The wrist HUD: over the back of the left wrist, beside slow motion's focus
// ring, the player's health as a bar and a heal button at its end. Look at
// your wrist to read them; poke the button with your right index finger to
// heal (the game's D-pad down, as the right stick pushed down and, on the
// Index, the right trackpad pressed down send it).
//
// Everything here is placed the way the focus ring is (web_visual's
// appendSlowMotionMeter): from the glove's pose (fingers along -z, the back
// of the hand +y), level and upright across the line of sight. The XR worker
// lays it out in tracking space to find the finger on the button; the eye
// images lay it out again in the world, with the up the tracking space has
// there, so both agree.
namespace spidy::wrist_hud {
struct Layout {
    bool valid{};
    // The focus ring's centre, and the panel's axes: right and up across the
    // line of sight, `view` from the panel to the viewer.
    Vec3 ring{}, right{}, up{}, view{};
    // The health bar's ends and half its height; the heal button's centre
    // and radius. Metres.
    Vec3 barStart{}, barEnd{};
    float barHalf{};
    Vec3 button{};
    float buttonRadius{};
};
// The glove's offset to the focus ring (appendSlowMotionMeter's).
inline constexpr Vec3 ringOffset{0, .045f, .065f};
// Sizes, metres: the bar above the ring, the button at its right end.
inline constexpr float barRise = .034f, barLeft = .042f, barRight = .032f, barHalfHeight = .0048f,
                       buttonGap = .024f, buttonSize = .0115f;
inline Layout layout(Pose hand, Vec3 viewer, Vec3 worldUp = {0, 1, 0}) {
    Layout l;
    const Vec3 ring = hand.position + hand.orientation.rotate(ringOffset);
    const float distance = length(viewer - ring);
    if (!finite(ring) || !finite(viewer) || !finite(worldUp) || !(distance > .08f && distance < 3) ||
        length(worldUp) < .5f)
        return l;
    const Vec3 view = (viewer - ring) / distance;
    Vec3 right = cross(normalized(worldUp), view);
    if (length(right) < .1f) {
        // Looking straight down at it: any level right will do.
        right = cross(Vec3{0, 0, 1}, view);
        if (length(right) < .1f)
            right = Vec3{1, 0, 0};
    }
    right = normalized(right);
    const Vec3 up = cross(view, right);
    const Vec3 anchor = ring + up * barRise;
    l.valid = true;
    l.ring = ring;
    l.right = right;
    l.up = up;
    l.view = view;
    l.barStart = anchor - right * barLeft;
    l.barEnd = anchor + right * barRight;
    l.barHalf = barHalfHeight;
    l.button = l.barEnd + right * buttonGap;
    l.buttonRadius = buttonSize;
    return l;
}
// The right index fingertip from that hand's glove pose: ahead of the grip
// along the fingers, as a pointing finger reaches.
inline Vec3 fingertip(Pose hand) {
    return hand.position + hand.orientation.rotate({0, -.012f, -.085f});
}

// The player's health as the bar shows it. The game keeps the hit points;
// the most it gives is the most seen on that health component, so a session
// starting hurt shows a full bar until he heals or is healed past it.
class Health {
  public:
    // Seconds a hit's trail waits before draining, and how fast it drains
    // (bar fractions a second); how long the hurt and healed flashes last.
    static constexpr float trailDelay = .45f, trailDrain = .7f, flashSeconds = .5f;
    // `known`: `points` is this frame's hit points on `component` (a new
    // component, a load or a respawn, starts over).
    void update(float seconds, bool known, float points, uint64_t component) {
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.f, .25f) : 0.f;
        hurt_ = std::max(0.f, hurt_ - dt / flashSeconds);
        healed_ = std::max(0.f, healed_ - dt / flashSeconds);
        if (!known || !std::isfinite(points) || points < 0 || !component) {
            known_ = false;
            return;
        }
        if (component != component_) {
            component_ = component;
            most_ = 0;
            fraction_ = trail_ = 1;
            hold_ = 0;
        }
        most_ = std::max(most_, points);
        const float next = most_ > 0 ? std::clamp(points / most_, 0.f, 1.f) : 0.f;
        if (next < fraction_ - 1e-4f) {
            hurt_ = 1;
            hold_ = trailDelay;
            trail_ = std::max(trail_, fraction_);
        } else if (next > fraction_ + .01f) {
            healed_ = 1;
        }
        fraction_ = next;
        if (hold_ > 0)
            hold_ = std::max(0.f, hold_ - dt);
        else
            trail_ = std::max(fraction_, trail_ - dt * trailDrain);
        trail_ = std::max(trail_, fraction_);
        known_ = true;
    }
    bool known() const {
        return known_;
    }
    float fraction() const {
        return fraction_;
    }
    float trail() const {
        return trail_;
    }
    float hurt() const {
        return hurt_;
    }
    float healed() const {
        return healed_;
    }

  private:
    uint64_t component_{};
    float most_{}, fraction_ = 1, trail_ = 1, hold_{}, hurt_{}, healed_{};
    bool known_{};
};

// The heal button: a fingertip within `press` of its centre presses it, and
// it stays pressed until the finger leaves `release` and at least `hold`
// seconds have passed, the moment the game wants its D-pad down held.
class HealButton {
  public:
    static constexpr float press = .024f, release = .036f, nearby = .09f, hold = .35f;
    // `enabled`: in play, both hands tracked, the right hand pointing (no
    // web, trigger and grip let go). True while it sends the heal.
    bool update(float seconds, bool enabled, Vec3 tip, const Layout& l) {
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.f, .25f) : 0.f;
        pressedNow_ = false;
        const float d = enabled && l.valid && finite(tip) ? length(tip - l.button) : 1e9f;
        hover_ = d < nearby ? std::clamp(1 - (d - press) / (nearby - press), 0.f, 1.f) : 0.f;
        if (held_) {
            heldFor_ += dt;
            if (!enabled || (d > release && heldFor_ >= hold))
                held_ = false;
        } else if (enabled && d < press && armed_) {
            held_ = true;
            pressedNow_ = true;
            armed_ = false;
            heldFor_ = 0;
            ++presses_;
        }
        // One press a poke: the finger leaves before it can press again.
        if (!held_ && d > release)
            armed_ = true;
        return held_;
    }
    bool held() const {
        return held_;
    }
    bool pressedNow() const {
        return pressedNow_;
    }
    // 0..1: how near the finger is.
    float hover() const {
        return hover_;
    }
    uint32_t presses() const {
        return presses_;
    }

  private:
    bool held_{}, pressedNow_{}, armed_ = true;
    float heldFor_{}, hover_{};
    uint32_t presses_{};
};

// What an eye image shows of it (NativeEyeFrame::wrist).
struct View {
    bool shown{};
    bool known{};       // the health is read from the game; else the bar shows unknown
    float health = 1;   // 0..1
    float trail = 1;    // 0..1, at least health: what a hit took, draining
    float hurt{};       // 0..1: a red flash fading after a hit
    float healed{};     // 0..1: a green flash fading after a heal
    float hover{};      // 0..1: the finger near the button
    bool pressed{};     // the button is sending the heal
    Vec3 up{0, 1, 0};   // the tracking space's up in the world (the layout's)
};
} // namespace spidy::wrist_hud
