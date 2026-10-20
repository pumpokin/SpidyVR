// The VR tutorial (tutorial.hpp): its cards, how a poke presses their
// buttons, and how they are drawn.
#include "spidy/tutorial.hpp"
#include "spidy/overlay_text.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace spidy::tutorial {
namespace {
Vec3 level(Vec3 v) {
    return {v.x, 0, v.z};
}
// The way the head looks, level, as a yaw (Quat::yaw turns -z to (-sin r, 0, -cos r)).
float headYaw(Pose head, float fallback) {
    const Vec3 f = level(head.orientation.rotate({0, 0, -1}));
    return length(f) > .1f ? std::atan2(-f.x, -f.z) : fallback;
}
} // namespace

std::string buttonName(char button, ControllerKind kind) {
    if (kind == ControllerKind::index) {
        // Spidy's X and Y are the Index's left A and B; it has no menu button:
        // the left trackpad pressed down pauses.
        switch (button) {
        case 'A':
            return "RIGHT A BUTTON";
        case 'B':
            return "RIGHT B BUTTON";
        case 'X':
            return "LEFT A BUTTON";
        case 'Y':
            return "LEFT B BUTTON";
        case 'M':
            return "BOTH TRACKPADS, PRESSED TOGETHER";
        default:
            break;
        }
    }
    if (button == 'M')
        return "MENU";
    return std::string(1, button);
}

std::vector<Step> steps(const Wording& w) {
    const bool index = w.controller == ControllerKind::index;
    const auto b = [&](char c) { return buttonName(c, w.controller); };
    // The web button and the reel button, named as the hands work them. The
    // Index grip reads how hard the handle is squeezed: it wants a firm one.
    const std::string shootWeb = w.triggerWebs ? "PULL A TRIGGER"
                                 : index       ? "SQUEEZE A HANDLE FIRMLY (GRIP)"
                                               : "SQUEEZE A GRIP";
    const std::string reelIt = w.triggerWebs ? (index ? "SQUEEZE ITS HANDLE (GRIP)" : "SQUEEZE ITS GRIP")
                                             : "PULL ITS TRIGGER";
    const std::string reelName = w.triggerWebs ? (index ? "HANDLE (GRIP)" : "GRIP") : "TRIGGER";
    const std::string stick = index ? "THUMBSTICK" : "STICK";
    const std::string left = "LEFT " + stick, right = "RIGHT " + stick;
    const std::string healToo = index ? "OR PRESS THE RIGHT TRACKPAD DOWN" : "OR HOLD THE RIGHT STICK DOWN";
    return {
        {"WEB SWINGING",
         {"POINT A HAND AT A BUILDING, " + shootWeb, "IT SHOOTS A WEB: KEEP SQUEEZING AND SWING",
          "LET GO TO RELEASE. SWAP HANDS TO KEEP GOING"},
         Goal::swing},
        {"REEL IN",
         {"WHILE A HAND HOLDS A WEB, " + reelIt, "TO REEL YOURSELF IN AND PICK UP SPEED"},
         Goal::reel},
        {"WEB ZIP",
         {"WITH A WEB HELD, YANK THAT HAND SHARPLY BACK TO ZIP",
          "TOO HARD? RAISE ARM SENSITIVITY IN SETTINGS, SPIDY VR",
          index ? "OR HOLD THE LEFT TRACKPAD DOWN, LET GO: ZIP TO A POINT"
                : "OR HOLD THE " + right + " UP: ZIP TO A POINT"},
         Goal::zip},
        {"JUMP AND LEAP",
         {"THE " + b('A') + " JUMPS", "HOLD IT ON THE GROUND, THEN LET GO",
          "FOR A CHARGED LEAP: THE LONGER, THE HIGHER"},
         Goal::jump},
        {"WALLS",
         {"WALK, JUMP OR SWING INTO A WALL TO STICK TO IT", "THE " + left + " WALKS ON IT",
          "THE " + b('A') + " JUMPS OFF IT"},
         Goal::wall},
        {"WEB SHOOTER",
         {"ON A HAND WITH NO WEB, PULL THE " + reelName, "TO SHOOT WEB BALLS FROM YOUR WRIST",
          "THREE HITS WEB AN ENEMY UP"},
         Goal::shot},
        {"PUNCH",
         {"THROW A FAST FIST INTO AN ENEMY", "AT ONE UP TO 10 M AWAY, YOU FLY IN AND HIT",
          "THE RING SHOWS WHO YOUR NEXT PUNCH HITS"},
         Goal::punch},
        {"CALLED BLOWS AND HOOKS",
         {"AN ARROW INTO THE RING CALLS THE NEXT BLOW", "FROM THE SIDE: SWING A HOOK FROM THAT SIDE",
          "A YELLOW ARROW CALLS THE FINISHER"},
         Goal::called},
        {"UPPERCUT",
         {"ARROW FROM BELOW: SWING YOUR FIST UP THROUGH HIM", "HE FLIES UP. FOLLOW HIM",
          "AND KEEP HITTING HIM IN THE AIR"},
         Goal::uppercut},
        {"DOWNSLAM",
         {"ARROW FROM ABOVE: BRING YOUR FIST DOWN ON HIM", "HIT AN ENEMY IN THE AIR THAT WAY",
          "AND HE SLAMS INTO THE GROUND"},
         Goal::slam},
        {"SPIDER-SENSE AND DODGE",
         {"LINES OVER AN ENEMY: HE IS ABOUT TO STRIKE", "BLUE: THROW A PUNCH AT HIM TO COUNTER",
          "RED: DODGE (" + b('B') + "), YELLOW: JUMP (" + b('A') + ")"},
         Goal::dodge},
        {"SLOW MOTION",
         {"CLICK THE " + left + " IN TO SLOW TIME", "THE RING ON YOUR LEFT WRIST IS ITS FOCUS",
          "CLICK IT AGAIN TO END IT"},
         Goal::slow},
        {"HEALTH AND HEALING",
         {"YOUR HEALTH BAR IS ON YOUR LEFT WRIST", "POKE THE + BESIDE IT WITH YOUR RIGHT FINGER",
          healToo},
         Goal::heal},
        {"GADGETS",
         {"HOLD THE " + b('X') + ": A GADGET WHEEL OPENS", "MOVE THAT HAND TO ONE AND LET GO",
          "A FREE HAND FIRES IT WITH ITS " + reelName},
         Goal::gadget},
        {"SUIT POWER",
         {"CROSS YOUR ARMS ON YOUR CHEST, HANDS AT YOUR SHOULDERS", "HOLD THEM THERE UNTIL BOTH HANDS TICK",
          "THEN THROW YOUR ARMS OUT WIDE"},
         Goal::suit},
        {"STEALTH WEB TAKEDOWNS",
         {"SNEAK UP ON, OR PERCH ABOVE, AN ENEMY WHO HAS NOT SEEN YOU",
          "WHEN THE GAME SHOWS TAKEDOWN, LOOK AT HIM, PRESS THE " + b('Y'),
          index ? "TO ZIP TO A PERCH: LOOK AT IT, HOLD THE LEFT TRACKPAD, LET GO"
                : "TO ZIP TO A PERCH: LOOK AT IT, HOLD THE " + right + " UP"},
         Goal::none},
        {"PROMPTS AND MENUS",
         {"LOOK AT WHAT THE GAME ASKS FOR, PRESS THE " + b('Y'), "TAP THE " + b('X') + ": MAP, SUITS, SKILLS",
          index ? std::string("TO PAUSE: PRESS BOTH TRACKPADS TOGETHER") : "TO PAUSE: PRESS " + b('M')},
         Goal::none},
        {"YOU ARE READY",
         {"THAT IS EVERYTHING. GO SWING, SPIDEY!", "TO SEE THIS AGAIN, RUN REPLAY SPIDY TUTORIAL",
          "IN THE SPIDY FOLDER BEFORE YOU START VR"},
         Goal::none},
    };
}

std::vector<Button> buttons(Stage stage, int step, int count, bool done) {
    if (stage == Stage::offer)
        return {{"START", Choice::start, -.2f, -.065f},
                {"NOT NOW", Choice::later, -.055f, .08f},
                {"NEVER ASK", Choice::never, .09f, .2f}};
    if (stage != Stage::step)
        return {};
    if (step >= count - 1)
        return {{"BACK", Choice::back, -.2f, -.09f}, {"FINISH", Choice::finish, .05f, .2f}};
    return {{"BACK", Choice::back, -.2f, -.09f},
            {done ? "NEXT" : "SKIP", Choice::next, -.055f, .08f},
            {"END", Choice::end, .11f, .2f}};
}

void Tutorial::place(Pose head, float side) {
    const float yaw = headYaw(head, 0) + side;
    const Vec3 toward{-std::sin(yaw), 0, -std::cos(yaw)};
    card_ = {head.position + toward * cardDistance + Vec3{0, -.18f, 0}, Quat::yaw(yaw)};
    away_ = 0;
}
void Tutorial::offer(Pose head) {
    stage_ = Stage::offer;
    age_ = 0;
    pressed_ = -1;
    flash_ = 0;
    place(head, 0);
}
void Tutorial::go(int step, Pose head) {
    step_ = std::clamp(step, 0, std::max(count() - 1, 0));
    stage_ = Stage::step;
    done_ = attached_ = false;
    doneAge_ = reel_ = age_ = 0;
    // Off to the left of the view, out of the way of play ahead.
    place(head, .55f);
}

Choice Tutorial::update(float seconds, bool active, Pose head, const Fingers& fingers, const Events& seen) {
    hover_ = {};
    if (stage_ == Stage::off || !active)
        return Choice::none;
    const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.f, .25f) : 0.f;
    age_ += dt;
    flash_ = std::max(0.f, flash_ - dt);
    if (flash_ <= 0)
        pressed_ = -1;
    // The card comes back beside the head when the player has turned or
    // walked well away from it.
    {
        const Vec3 to = card_.position - head.position;
        const Vec3 f = level(head.orientation.rotate({0, 0, -1})), t = level(to);
        const float angle = length(f) > .1f && length(t) > .05f
                                ? std::acos(std::clamp(dot(normalized(f), normalized(t)), -1.f, 1.f))
                                : 0.f;
        away_ = angle > 1.4f || length(to) > 1.3f ? away_ + dt : 0.f;
        if (away_ > 1.f)
            place(head, stage_ == Stage::step ? .55f : 0.f);
    }
    // The move: done once, then on to the next card after a moment.
    if (stage_ == Stage::step && !done_) {
        bool made = false;
        switch (step_ < count() ? steps_[static_cast<size_t>(step_)].goal : Goal::none) {
        case Goal::swing:
            // A web held, then let go.
            attached_ = attached_ || seen.webAttached;
            made = attached_ && !seen.webAttached;
            break;
        case Goal::reel:
            reel_ = seen.reeling ? reel_ + dt : reel_;
            made = reel_ >= reelSeconds;
            break;
        case Goal::zip:
            made = seen.zip;
            break;
        case Goal::jump:
            made = seen.jump;
            break;
        case Goal::wall:
            made = seen.wall;
            break;
        case Goal::shot:
            made = seen.shot;
            break;
        case Goal::punch:
            made = seen.punch;
            break;
        case Goal::called:
            made = seen.called;
            break;
        case Goal::uppercut:
            made = seen.uppercut;
            break;
        case Goal::slam:
            made = seen.slam;
            break;
        case Goal::dodge:
            made = seen.dodge;
            break;
        case Goal::slow:
            made = seen.slow;
            break;
        case Goal::heal:
            made = seen.heal;
            break;
        case Goal::gadget:
            made = seen.gadget;
            break;
        case Goal::suit:
            made = seen.suit;
            break;
        case Goal::none:
            break;
        }
        if (made) {
            done_ = true;
            doneAge_ = 0;
        }
    } else if (stage_ == Stage::step) {
        doneAge_ += dt;
        if (doneAge_ >= doneSeconds && step_ < count() - 1) {
            go(step_ + 1, head);
            return Choice::next;
        }
    }
    // A pointing fingertip at a button, just in front of the card or a little
    // through it, presses it once; it leaves before it presses again.
    const bool nothingToDo = step_ < count() && steps_[static_cast<size_t>(step_)].goal == Goal::none;
    const auto list = buttons(stage_, step_, count(), done_ || nothingToDo);
    const Vec3 right = card_.orientation.rotate({1, 0, 0}), up = card_.orientation.rotate({0, 1, 0}),
               out = card_.orientation.rotate({0, 0, 1});
    Choice chosen = Choice::none;
    int chosenIndex = -1;
    for (unsigned h = 0; h < 2; ++h) {
        const Vec3 d = fingers.tips[h] - card_.position;
        const float x = dot(d, right), y = dot(d, up), z = dot(d, out);
        if (!fingers.pointing[h] || !finite(fingers.tips[h])) {
            armed_[h] = true;
            continue;
        }
        bool inside = false;
        for (size_t i = 0; i < list.size() && i < hover_.size(); ++i) {
            const auto& b = list[i];
            const bool over = x >= b.x0 - pokeMargin && x <= b.x1 + pokeMargin && y >= buttonLow - pokeMargin &&
                              y <= buttonHigh + pokeMargin;
            if (!over)
                continue;
            // Nearer the card, brighter.
            hover_[i] = std::max(hover_[i], z > -pokeBehind ? std::clamp(1 - (z - pokeDepth) / .1f, 0.f, 1.f) : 0.f);
            if (z <= pokeDepth && z >= -pokeBehind) {
                inside = true;
                if (armed_[h] && chosen == Choice::none) {
                    chosen = b.choice;
                    chosenIndex = static_cast<int>(i);
                }
            }
        }
        armed_[h] = !inside;
    }
    if (chosen == Choice::none)
        return chosen;
    pressed_ = chosenIndex;
    flash_ = flashSeconds;
    switch (chosen) {
    case Choice::start:
        go(0, head);
        break;
    case Choice::next:
        if (step_ < count() - 1)
            go(step_ + 1, head);
        break;
    case Choice::back:
        go(step_ - 1, head);
        break;
    case Choice::later:
    case Choice::never:
    case Choice::end:
    case Choice::finish:
        stage_ = Stage::off;
        break;
    default:
        break;
    }
    pressed_ = stage_ == Stage::off ? -1 : chosenIndex;
    return chosen;
}

View Tutorial::view() const {
    View v;
    v.stage = stage_;
    v.step = step_;
    v.count = count();
    v.done = done_;
    v.doneAge = doneAge_;
    v.age = age_;
    v.progress = stage_ == Stage::step && step_ < count() && steps_[static_cast<size_t>(step_)].goal == Goal::reel
                     ? std::clamp(reel_ / reelSeconds, 0.f, 1.f)
                     : 0.f;
    v.card = card_;
    v.hover = hover_;
    v.pressed = pressed_;
    v.wording = wording_;
    return v;
}

namespace {
constexpr Vec3 background{.010f, .012f, .020f}, accent{.62f, .02f, .03f}, white{1, 1, 1}, soft{.78f, .8f, .84f},
    amber{1, .52f, .05f}, green{.1f, .78f, .22f}, grey{.32f, .33f, .38f}, plate{.06f, .065f, .08f};
struct Card {
    Vec3 centre, right, up, out;
    Vec3 at(float x, float y, float z = 0) const {
        return centre + right * x + up * y + out * z;
    }
    void rect(std::vector<Vertex>& v, float x0, float y0, float x1, float y1, float z, Vec3 color,
              float alpha = 1) const {
        const Vertex a{at(x0, y0, z), color, alpha}, b{at(x1, y0, z), color, alpha}, c{at(x1, y1, z), color, alpha},
            d{at(x0, y1, z), color, alpha};
        v.insert(v.end(), {a, b, c, a, c, d});
    }
    void text(std::vector<Vertex>& v, const std::string& s, float x, float y, float height, Vec3 color,
              float align, float width) const {
        TextStyle style;
        style.height = fittedHeight(s, height, width);
        style.color = color;
        appendText(v, s, at(x - textWidth(s, style.height) * align, y, .003f), right, up, style);
    }
};
} // namespace

void appendView(std::vector<Vertex>& out, const View& view, Vec3 viewer) {
    if (view.stage == Stage::off || !finite(view.card.position) || !finite(viewer))
        return;
    const auto& q = view.card.orientation;
    const Card card{view.card.position, q.rotate({1, 0, 0}), q.rotate({0, 1, 0}), q.rotate({0, 0, 1})};
    // Seen from behind (the player walked round it): nothing.
    if (dot(viewer - card.centre, card.out) <= 0)
        return;
    const float w = cardWidth / 2, h = cardHeight / 2, edge = .004f, inner = w - .02f;
    // Opens over a fifth of a second.
    const float grow = std::clamp(view.age / .2f, .05f, 1.f);
    const Card shown{card.centre, card.right * grow, card.up * grow, card.out};
    shown.rect(out, -w, -h, w, h, 0, background);
    shown.rect(out, -w, h - edge, w, h, .001f, accent);
    shown.rect(out, -w, -h, w, -h + edge, .001f, accent);
    shown.rect(out, -w, -h, -w + edge, h, .001f, accent);
    shown.rect(out, w - edge, -h, w, h, .001f, accent);
    if (grow < 1)
        return;
    const float lineHeight = .0125f;
    if (view.stage == Stage::offer) {
        card.text(out, "NEW TO SPIDY VR?", 0, .075f, .024f, white, .5f, 2 * inner);
        card.text(out, "A QUICK TUTORIAL SHOWS EVERY MOVE:", 0, .035f, lineHeight, soft, .5f, 2 * inner);
        card.text(out, "SWINGING, ZIPS, PUNCHES, UPPERCUTS, DOWNSLAMS", 0, .012f, lineHeight, soft, .5f, 2 * inner);
        card.text(out, "POKE A BUTTON WITH YOUR FINGER", 0, -.03f, .011f, grey, .5f, 2 * inner);
    } else {
        const auto all = steps(view.wording);
        if (view.step < 0 || view.step >= static_cast<int>(all.size()))
            return;
        const auto& step = all[static_cast<size_t>(view.step)];
        char count[48];
        std::snprintf(count, sizeof count, "%s%d / %d",
                      view.wording.controller == ControllerKind::index ? "VALVE INDEX   " : "", view.step + 1,
                      view.count);
        card.text(out, count, inner, .098f, .011f, grey, 1, .15f);
        card.text(out, step.title, -inner, .09f, .022f, white, 0, 2 * inner - .19f);
        // One size for every line of the card: the largest at which all fit.
        float height = lineHeight;
        for (const auto& line : step.lines)
            height = std::min(height, fittedHeight(line, lineHeight, 2 * inner));
        float y = .05f;
        for (const auto& line : step.lines) {
            card.text(out, line, -inner, y, height, soft, 0, 2 * inner);
            y -= .024f;
        }
        if (step.goal != Goal::none) {
            if (view.done) {
                // DONE! pops in large, then settles.
                const float pop = 1 + .35f * std::max(0.f, 1 - view.doneAge / .25f);
                card.text(out, "DONE!", 0, -.035f, .02f * pop, green, .5f, .2f);
            } else {
                const float pulse = .75f + .25f * std::sin(view.age * 4);
                card.text(out, "TRY IT", 0, -.035f, .016f, amber * pulse, .5f, .2f);
                if (view.progress > 0) {
                    const float x0 = -.08f, x1 = .08f;
                    card.rect(out, x0, -.05f, x1, -.044f, .002f, grey);
                    card.rect(out, x0, -.05f, x0 + (x1 - x0) * view.progress, -.044f, .003f, green);
                }
            }
        }
    }
    bool readOnly = false;
    if (view.stage == Stage::step) {
        const auto all = steps(view.wording);
        readOnly = view.step >= 0 && view.step < static_cast<int>(all.size()) &&
                   all[static_cast<size_t>(view.step)].goal == Goal::none;
    }
    const auto list = buttons(view.stage, view.step, view.count, view.done || readOnly);
    for (size_t i = 0; i < list.size() && i < view.hover.size(); ++i) {
        const auto& b = list[i];
        const float hover = std::clamp(view.hover[i], 0.f, 1.f);
        const bool pressed = view.pressed == static_cast<int>(i);
        const bool main = b.choice == Choice::start || b.choice == Choice::finish ||
                          (b.choice == Choice::next && view.done);
        const Vec3 fill = pressed ? green : main ? accent * (1 + .5f * hover) : plate + (grey - plate) * hover;
        card.rect(out, b.x0 - .002f, buttonLow - .002f, b.x1 + .002f, buttonHigh + .002f, .002f,
                  hover > 0 ? white * (.4f + .6f * hover) : grey);
        card.rect(out, b.x0, buttonLow, b.x1, buttonHigh, .003f, fill);
        card.text(out, b.label, (b.x0 + b.x1) / 2, (buttonLow + buttonHigh) / 2 - .006f, .012f, white, .5f,
                  b.x1 - b.x0 - .012f);
    }
}
} // namespace spidy::tutorial
