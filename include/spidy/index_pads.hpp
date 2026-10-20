#pragma once
#include <algorithm>
#include <cmath>

// The Index's trackpads, pressed down (their force), as Spidy's buttons: the
// right one heals (the game's D-pad down), the left one is the zip to a point
// (as the right stick pushed up: held aims, let go zips), and both together
// pause (the menu button the Index lacks). Each pad presses past `press` of
// its force and lets go below `release` (INDEX TRACKPAD PRESS sets them):
// a light press, not SteamVR's own click. A pad acts on its own only once the
// other has not joined it within chordSeconds, so pressing both to pause
// neither heals nor zips; once a pad acts, the other joining is not a pause.
namespace spidy::index_pads {
// INDEX TRACKPAD PRESS: light, medium, firm.
inline constexpr float pressForces[] = {.15f, .3f, .55f};
inline float pressForce(int choice) {
    return pressForces[choice < 0 ? 0 : choice > 2 ? 2 : choice];
}
struct Out {
    bool heal{}, zip{}, pause{};
};
class Pads {
  public:
    static constexpr float chordSeconds = .12f;
    float press = .15f;
    float release() const {
        return press * .5f;
    }
    // forces: 0 the left pad, 1 the right, 0 to 1.
    Out update(float seconds, float left, float right) {
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.f, .25f) : 0.f;
        const float forces[2] = {left, right};
        for (int i = 0; i < 2; ++i) {
            const float f = std::isfinite(forces[i]) ? forces[i] : 0.f;
            down_[i] = down_[i] ? f > release() : f > press;
            held_[i] = down_[i] ? held_[i] + dt : 0.f;
        }
        if (!down_[0] && !down_[1]) {
            mode_ = Mode::none;
        } else if (mode_ == Mode::none) {
            if (down_[0] && down_[1])
                mode_ = Mode::pause;
            else if (down_[0] && held_[0] >= chordSeconds)
                mode_ = Mode::zip;
            else if (down_[1] && held_[1] >= chordSeconds)
                mode_ = Mode::heal;
        }
        Out out;
        out.pause = mode_ == Mode::pause;
        out.zip = mode_ == Mode::zip && down_[0];
        out.heal = mode_ == Mode::heal && down_[1];
        // A pad let go ends its own action; the pause lasts while either is held.
        if ((mode_ == Mode::zip && !down_[0]) || (mode_ == Mode::heal && !down_[1]))
            mode_ = down_[0] || down_[1] ? Mode::spent : Mode::none;
        return out;
    }

  private:
    enum class Mode { none, heal, zip, pause, spent };
    Mode mode_ = Mode::none;
    bool down_[2]{};
    float held_[2]{};
};
} // namespace spidy::index_pads
