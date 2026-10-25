#pragma once

namespace expanded {

// A charge Xiao's slingshot takes from being held drawn: frames held for it to be ready, the game's charge-complete
// flash when it is, and whether the shot let go was charged.
class ShotCharge {
public:
    explicit ShotCharge(int frames) : frames_(frames) {}

    // One step (once a frame the monsters step unpaused): active is whether the weapon charges at all.
    void Step(bool active);
    // The shot just let go: whether it was held long enough, the hold then spent.
    bool TakeCharged();

    // How long the slingshot has been held drawn, and whether that is a charge.
    int Held() const { return hold_ > held_ ? hold_ : held_; }

    bool Ready() const { return Held() >= frames_; }

    // Whether it is held drawn now.
    bool Holding() const { return hold_ > 0; }

    void Reset() { hold_ = held_ = 0; }

private:
    int frames_;
    int hold_ = 0; // held drawn now
    int held_ = 0; // held before it last let go
};

} // namespace expanded
