#pragma once
#include "math.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

// Dodge assist (the VR settings' DODGE ASSIST), on top of Spider-Sense
// (spider_sense.hpp, game_sense.hpp): the game's own warnings of a blow about
// to land, each with what answers it - a counter (blue), a dodge (red), a
// jump (yellow).
// - AIM: B pressed with the left stick at rest while a warning stands dodges
//   straight away from that attacker, not the game's default way.
// - AUTO: that too, and a red warning dodges on its own and a yellow one
//   jumps, as the blow is about to land, once a warning, if the player has
//   not answered it himself. A blue one is left to the player's fist.
// Engine independent: the warning that lands soonest and the player in, the
// presses and the way to dodge out.
namespace spidy::dodge_assist {
enum Mode : int { off = 0, aim = 1, automatic = 2 };
// SenseAnswer's values (spider_sense.hpp).
enum Answer : int { none = 0, counter = 1, dodge = 2, jump = 3 };
struct Threat {
    bool live{};
    uint64_t attacker{};
    Vec3 at{};          // where he is, world
    int answer{};       // Answer
    bool timed{};       // toImpact holds the game seconds to his blow
    float toImpact{};   // game seconds until it lands
    float warned{};     // game seconds since the warning
};
struct Out {
    bool dodge{}, jump{}; // press B, A this frame
    bool aimed{};         // the dodge goes `away` (else the player's stick)
    Vec3 away{};          // level, unit, world
};
class DodgeAssist {
  public:
    // AUTO answers a timed blow this many game seconds before it lands, an
    // untimed one this long after its warning; it holds the press pressSeconds.
    float autoBefore = .22f, untimedAfter = .35f, pressSeconds = .12f;
    // `pressedDodge`/`pressedJump`: the player's own B/A now; `stickMoved`:
    // the left stick is pushed (his dodge goes his way).
    Out update(float seconds, int mode, const Threat& t, Vec3 player, bool pressedDodge, bool pressedJump,
               bool stickMoved) {
        Out out;
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.f, .25f) : 0.f;
        holding_ = std::max(0.f, holding_ - dt);
        if (mode == off || !t.live || !finite(t.at) || !finite(player)) {
            if (!t.live)
                answered_ = 0;
            if (holding_ > 0) {
                out.dodge = heldDodge_;
                out.jump = !heldDodge_;
            }
            return out;
        }
        Vec3 away{player.x - t.at.x, 0, player.z - t.at.z};
        const bool placed = length(away) > .05f;
        if (placed)
            away = normalized(away);
        // A warning answered by the player himself is not answered again.
        if ((pressedDodge || pressedJump) && t.attacker)
            answered_ = t.attacker;
        if (pressedDodge && !stickMoved && placed) {
            out.aimed = true;
            out.away = away;
        }
        if (mode == automatic && t.attacker != answered_ && (t.answer == dodge || t.answer == jump)) {
            const bool due = t.timed ? t.toImpact <= autoBefore : t.warned >= untimedAfter;
            if (due) {
                answered_ = t.attacker;
                holding_ = pressSeconds;
                heldDodge_ = t.answer == dodge;
                ++autoAnswers_;
            }
        }
        if (holding_ > 0) {
            if (heldDodge_) {
                out.dodge = true;
                if (placed && !stickMoved) {
                    out.aimed = true;
                    out.away = away;
                }
            } else {
                out.jump = true;
            }
        }
        return out;
    }
    // Warnings AUTO answered so far.
    uint32_t autoAnswers() const {
        return autoAnswers_;
    }

  private:
    uint64_t answered_{};
    float holding_{};
    bool heldDodge_{};
    uint32_t autoAnswers_{};
};
// The left stick that dodges `away` (level, world) for a game camera looking
// along `cameraForward`: its forward and right, as the stick's walk is sent.
inline void stickToward(Vec3 away, Vec3 cameraForward, float& forward, float& right) {
    Vec3 ahead{cameraForward.x, 0, cameraForward.z};
    forward = right = 0;
    if (length(ahead) < .1f || !finite(away))
        return;
    ahead = normalized(ahead);
    forward = dot(away, ahead);
    right = dot(away, cross(ahead, {0, 1, 0}));
}
} // namespace spidy::dodge_assist
