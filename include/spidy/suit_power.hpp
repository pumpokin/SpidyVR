#pragma once
#include "math.hpp"
#include <algorithm>
#include <cmath>

// The suit power by a gesture (the player, October 20, with a picture: arms
// crossed over the chest, then flung out wide): the flat game's L3 + R3,
// which in VR the sticks' clicks cannot give (each has a use; both together
// latch the pad layer). Cross the arms - each hand by the other shoulder, at
// the chest, close to the body - and hold them there armSeconds (a tick in
// both hands says it is ready); then throw them out wide within
// throwSeconds: the suit power goes off. Engine independent: the head and
// the hands' grip poses in the tracking space in; when to fire out.
namespace spidy::suit_power {
class Gesture {
  public:
    // Crossed: each hand at least `across` metres past the middle on the
    // other side, between chestLow and chestHigh below the eyes, no farther
    // than `reach` ahead of them. Out: the hands at least `wide` apart
    // across, each on its own side.
    float across = .04f, chestLow = .75f, chestHigh = .12f, reach = .45f, wide = 1.f, armSeconds = .25f,
          throwSeconds = 1.2f;
    struct Out {
        bool armed{}; // crossed long enough, this frame: a tick
        bool fired{}; // flung out: the suit power
    };
    Out update(float seconds, Pose head, Vec3 left, Vec3 right, bool tracked) {
        Out out;
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.f, .25f) : 0.f;
        if (!tracked || !finite(head.position) || !finite(left) || !finite(right)) {
            crossed_ = 0;
            armed_ = false;
            return out;
        }
        // The head's level axes: across to the right, and ahead.
        Vec3 ahead = head.orientation.rotate({0, 0, -1});
        ahead.y = 0;
        if (length(ahead) < .2f)
            return out;
        ahead = normalized(ahead);
        const Vec3 side{-ahead.z, 0, ahead.x};
        const auto local = [&](Vec3 p, float& x, float& y, float& z) {
            const Vec3 d = p - head.position;
            x = dot(d, side);
            y = d.y;
            z = dot(d, ahead);
        };
        float lx{}, ly{}, lz{}, rx{}, ry{}, rz{};
        local(left, lx, ly, lz);
        local(right, rx, ry, rz);
        const auto atChest = [&](float y, float z) { return y < -chestHigh && y > -chestLow && z < reach && z > -.2f; };
        const bool crossed = lx > across && rx < -across && atChest(ly, lz) && atChest(ry, rz);
        if (crossed) {
            crossed_ += dt;
            if (!armed_ && crossed_ >= armSeconds) {
                armed_ = true;
                out.armed = true;
            }
            since_ = 0;
        } else {
            crossed_ = 0;
            if (armed_) {
                since_ += dt;
                if (lx < 0 && rx > 0 && rx - lx >= wide) {
                    out.fired = true;
                    armed_ = false;
                } else if (since_ > throwSeconds) {
                    armed_ = false;
                }
            }
        }
        return out;
    }
    bool armed() const {
        return armed_;
    }

  private:
    float crossed_{}, since_{};
    bool armed_{};
};
} // namespace spidy::suit_power
