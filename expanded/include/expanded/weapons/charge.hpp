#pragma once

namespace expanded {

// The cyan a charge builds up toward.
inline constexpr float kChargeColour[3] = {0.0f, 122.0f, 208.0f};

// The game's charge-complete flash on the party as a charge is ready (Toan's sword charge's, Goro's): a pulse of
// kChargeColour.
void ChargeCompleteFlash();

// A hold of the guard: counted from the first frame in the guard pose with R1 down, every frame R1 stays down; latched
// once its owner acts on it, until R1 is let go.
class GuardHold {
public:
    // One step (once a frame), button whether R1 is down.
    void Step(bool button);

    // Frames since the pose was first taken, or -1 while R1 is up.
    int Held() const { return held_; }

    // Whether it has come to frames in the guard pose, not yet acted on.
    bool Reached(int frames) const;

    void Latch() { latched_ = true; }

    bool Latched() const { return latched_; }

    void Reset() {
        held_ = -1;
        latched_ = false;
    }

private:
    int  held_ = -1;
    bool latched_ = false;
};

// A charge the guard builds (the Solar Flash's and the shots that share it): started as the guard goes up with nothing
// blinded, it counts the frames the guard is up, forgives kGuardGrace frames down (a blocked hit drops it for a few),
// and drops on a longer lapse or a blinding.
inline constexpr int kGuardGrace = 10;

class GuardCharge {
public:
    enum class Step {
        kHolding,
        kDropped,
        kFull
    };

    // Starts a charge when the guard is up and nothing is blinded; false otherwise.
    bool Start();
    // One frame of the charge toward frames: kDropped as it lapses, kFull once it has them, else kHolding.
    Step Charge(int frames);

    // The frames it has, and how far that is toward frames (0 to 1).
    int Charged() const { return charged_; }

    float Progress(int frames) const;

private:
    int charged_ = 0;
    int unguarded_ = 0; // frames the guard has been down
};

// A charge Xiao's slingshot takes from being drawn and held drawn: frames held for it to be ready, the game's
// charge-complete flash when it is (and at early too, when given), and whether the shot let go was charged. A charged
// shot costs kChargedShotWear times the weapon HP an ordinary one does, as the mod's do.
inline constexpr float kChargedShotWear = 2.0f;

class ShotCharge {
public:
    explicit ShotCharge(int frames, int early = 0) : frames_(frames), early_(early) {}

    // One step (once a frame the monsters step unpaused): active is whether the weapon charges at all.
    void Step(bool active);
    // The shot just let go: whether it was held long enough, the hold then spent (a charged one's wear marked).
    bool TakeCharged();

    // How long the slingshot has been held drawn, and whether that is a charge.
    int Held() const { return hold_ > held_ ? hold_ : held_; }

    bool Ready() const { return Held() >= frames_; }

    // Whether it is drawn now, and how long it has been this time.
    bool Holding() const { return hold_ > 0; }

    int Drawn() const { return hold_; }

    // The shot let go counts as charged whatever the hold (a primed charge's).
    void MarkReady() { held_ = frames_; }

    void Reset() { hold_ = held_ = 0; }

private:
    int frames_;
    int early_;
    int hold_ = 0; // held drawn now
    int held_ = 0; // held before it last let go
};

// The shot let go now was charged: its wear is kChargedShotWear times.
void ChargedShotLetGo();

// Hook (BattleActionPlay_Jinn, with the Steel Slingshot's): the WHP the shot just let go costs, wear an ordinary one's.
float ChargedShotWear(float wear);

} // namespace expanded
