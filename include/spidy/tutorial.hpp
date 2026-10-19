#pragma once
#include "controller_layout.hpp"
#include "math.hpp"
#include "vertex.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// The VR tutorial: a card floating beside the player, one move at a time
// (web swinging, reeling in, zipping, jumping, walls, the web shooter,
// punches, called blows, uppercuts, downslams, dodging, slow motion,
// healing, gadgets, the game's prompts). It is offered once at the first
// gameplay: START, NOT NOW (asked again next session) or NEVER ASK. Each card
// ticks itself off when the player does the move and moves on; NEXT (SKIP
// before it is done), BACK and END are poked with either index finger, so no
// game button is taken from play. Engine independent: tracking-space poses
// and what happened this frame in, the card out.
namespace spidy::tutorial {
// What the player did this frame (the XR worker gathers it).
struct Events {
    bool webAttached{}; // a web holds in either hand now
    bool reeling{};     // a hand with a web pulls its reel button
    bool zip{}, jump{}, wall{}, shot{}, punch{}, called{}, uppercut{}, slam{}, dodge{}, slow{}, heal{}, gadget{};
};
enum class Goal : uint8_t {
    none, // read it, then NEXT
    swing,
    reel,
    zip,
    jump,
    wall,
    shot,
    punch,
    called,
    uppercut,
    slam,
    dodge,
    slow,
    heal,
    gadget,
};
// How the buttons are named for the player's controllers and settings.
struct Wording {
    ControllerKind controller = ControllerKind::touch;
    bool triggerWebs{}; // WEB BUTTON: TRIGGER (the grip reels)
};
struct Step {
    std::string title;
    std::vector<std::string> lines;
    Goal goal{};
};
// The cards, in order, worded for these controllers and settings.
std::vector<Step> steps(const Wording&);
// A button's name on these controllers: "A", "RIGHT A" (Index), ...
std::string buttonName(char button, ControllerKind);

enum class Stage : uint8_t { off, offer, step };
// What a poke chose. The XR worker keeps the answer (never, end, finish:
// the player has seen it; later: ask next session).
enum class Choice : uint8_t { none, start, later, never, next, back, end, finish };
// The index fingertips, tracking space, and whether each hand points (no web
// in it, trigger and grip let go): only a pointing finger presses.
struct Fingers {
    std::array<bool, 2> pointing{};
    std::array<Vec3, 2> tips{};
};
// The card, metres; its buttons along the bottom in its own axes (x right, y
// up from its centre).
inline constexpr float cardWidth = .44f, cardHeight = .26f, cardDistance = .58f;
struct Button {
    const char* label{};
    Choice choice{};
    float x0{}, x1{};
};
inline constexpr float buttonLow = -cardHeight / 2 + .018f, buttonHigh = buttonLow + .042f;
// The buttons a card shows: the offer's three, a step's BACK, NEXT (SKIP
// before it is done) and END, the last step's BACK and FINISH.
std::vector<Button> buttons(Stage, int step, int count, bool done);

// What an eye image shows of it (NativeEyeFrame::tutorial), world space.
struct View {
    Stage stage{};
    int step{}, count{};
    bool done{};       // this step's move was made
    float doneAge{};   // seconds since
    float age{};       // seconds the card has shown
    float progress{};  // 0..1 toward the goal (reeling)
    Pose card{};       // +z toward the player
    std::array<float, 4> hover{};
    int pressed = -1;  // the button the last poke pressed, flashing
    Wording wording{};
};
// The card: a dark plate with a red rim, the move's name and how to do it,
// TRY IT or DONE!, the step count and the buttons. `viewer` between the eyes.
void appendView(std::vector<Vertex>& out, const View&, Vec3 viewer);

class Tutorial {
  public:
    static constexpr float doneSeconds = 1.6f, reelSeconds = .5f, pokeDepth = .035f, pokeBehind = .05f,
                           pokeMargin = .008f, flashSeconds = .25f;
    explicit Tutorial(Wording w = {}) {
        wording(w);
    }
    void wording(const Wording& w) {
        wording_ = w;
        steps_ = steps(w);
    }
    // Asks whether to take it, on a card straight ahead of `head`.
    void offer(Pose head);
    void stop() {
        stage_ = Stage::off;
    }
    // One frame while VR shows play (`active`; else it waits). Returns what a
    // poke chose this frame.
    Choice update(float seconds, bool active, Pose head, const Fingers&, const Events&);
    Stage stage() const {
        return stage_;
    }
    int step() const {
        return step_;
    }
    int count() const {
        return static_cast<int>(steps_.size());
    }
    bool done() const {
        return done_;
    }
    // The card in tracking space, and the rest of what shows (the XR worker
    // moves the card into the world).
    Pose card() const {
        return card_;
    }
    View view() const;

  private:
    void place(Pose head, float side);
    void go(int step, Pose head);
    Wording wording_{};
    std::vector<Step> steps_;
    Stage stage_{};
    int step_{};
    bool done_{}, attached_{};
    float doneAge_{}, age_{}, reel_{}, away_{}, flash_{};
    int pressed_ = -1;
    Pose card_{};
    std::array<float, 4> hover_{};
    std::array<bool, 2> armed_{true, true};
};
} // namespace spidy::tutorial
