#pragma once
#include "math.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// Spider-sense (the VR settings' SPIDER-SENSE): an enemy about to strike - a
// thug winding up a blow near the player, a gunman aiming or firing from
// farther off (game_targets::threat) - flashes jagged lines at the edge of the
// view on his side, white for a blow and red for a shot, and buzzes both
// controllers, the hand on his side harder. Engine independent: the threats
// and the head in, what to show and when to buzz out.
namespace spidy::spider_sense {
enum Mode : int { off = 0, visual = 1, vibration = 2, both = 3 };
struct Threat {
    uint64_t id{};  // the enemy (his actor record), to tell a new threat from one already warned of
    Vec3 position{}; // world
    bool ranged{};
};
// What an eye image shows of it (NativeEyeFrame::sense), world space.
struct View {
    float strength{}; // 0..1: none to a full flash
    float age{};      // seconds since this warning began, for the lines' motion
    bool ranged{};
    // Where the lines spring from, and the axes they lie in: the way to the
    // threat across the view (unit) and the view's up there.
    Vec3 centre{}, out{}, up{};
};
// One frame's warning: whether to buzz now, and how hard each hand (0 left).
struct Buzz {
    bool now{};
    float hands[2]{};
};
class SpiderSense {
  public:
    // A blow is warned of within meleeReach of the player, a shot within
    // rangedReach. A warning flashes in at once and fades over fadeSeconds
    // after the last threat; one enemy warns again only after rearmSeconds
    // without threatening. The lines show at viewAngle from straight ahead
    // toward the threat, distance metres from the eyes.
    float meleeReach = 7, rangedReach = 35, fadeSeconds = .45f, rearmSeconds = .6f, viewAngle = .55f,
          distance = .9f;
    Buzz update(float seconds, const std::vector<Threat>& threats, Pose head) {
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.f, .25f) : 0.f;
        Buzz buzz;
        const Threat* nearest{};
        float best = 1e9f;
        for (const auto& t : threats) {
            if (!finite(t.position) || !finite(head.position))
                continue;
            const float d = length(t.position - head.position);
            if (d > (t.ranged ? rangedReach : meleeReach))
                continue;
            // Melee first: the blow is the one that lands soonest.
            const float rank = d + (t.ranged ? 100.f : 0.f);
            if (rank < best) {
                best = rank;
                nearest = &t;
            }
        }
        // Each enemy that starts threatening (or does again, after a pause).
        for (auto& w : warned_)
            w.quiet += dt;
        for (const auto& t : threats) {
            if (!finite(t.position) || length(t.position - head.position) > (t.ranged ? rangedReach : meleeReach))
                continue;
            auto known = std::find_if(warned_.begin(), warned_.end(), [&](const Warned& w) { return w.id == t.id; });
            if (known == warned_.end()) {
                warned_.push_back({t.id, 0});
                buzz.now = true;
            } else {
                if (known->quiet >= rearmSeconds)
                    buzz.now = true;
                known->quiet = 0;
            }
        }
        warned_.erase(std::remove_if(warned_.begin(), warned_.end(), [&](const Warned& w) { return w.quiet > 10; }),
                      warned_.end());
        if (nearest) {
            if (strength_ <= 0)
                age_ = 0;
            strength_ = 1;
            ranged_ = nearest->ranged;
            way_ = nearest->position - head.position;
        } else {
            strength_ = std::max(0.f, strength_ - dt / fadeSeconds);
        }
        age_ += dt;
        if (buzz.now) {
            // Harder in the hand on his side; both feel it.
            const Vec3 right = head.orientation.rotate({1, 0, 0});
            const float side = length(way_) > 1e-3f ? dot(normalized(way_), right) : 0.f;
            buzz.hands[0] = .55f + .35f * std::max(0.f, -side);
            buzz.hands[1] = .55f + .35f * std::max(0.f, side);
        }
        return buzz;
    }
    // The flash for an eye image, from the head that image is drawn for.
    View view(Pose head) const {
        View v;
        if (strength_ <= 0 || !finite(head.position))
            return v;
        const Vec3 ahead = head.orientation.rotate({0, 0, -1}), up = head.orientation.rotate({0, 1, 0}),
                   right = head.orientation.rotate({1, 0, 0});
        // The way to him across the view: right and up of the head (ahead or
        // behind, he is to a side of it); straight ahead, above.
        Vec3 across = right * dot(way_, right) + up * dot(way_, up);
        if (length(across) < 1e-3f * std::max(length(way_), 1.f))
            across = up;
        across = normalized(across);
        const Vec3 at = normalized(ahead * std::cos(viewAngle) + across * std::sin(viewAngle));
        v.strength = strength_;
        v.age = age_;
        v.ranged = ranged_;
        v.centre = head.position + at * distance;
        v.out = normalized(across - at * dot(across, at));
        v.up = normalized(cross(v.out, at)) * -1.f;
        return v;
    }
    float strength() const {
        return strength_;
    }

  private:
    struct Warned {
        uint64_t id{};
        float quiet{};
    };
    std::vector<Warned> warned_;
    float strength_{}, age_{};
    bool ranged_{};
    Vec3 way_{};
};
} // namespace spidy::spider_sense
