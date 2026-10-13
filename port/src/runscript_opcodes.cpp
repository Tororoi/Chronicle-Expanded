#include "runscript_opcodes.hpp"

#include <libvu0.h>

#include "confuse.hpp"
#include "dun/gameloop.hpp"
#include "mathutil.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"

namespace {

// ps2/src/runscript_opcodes.cpp's own readers and writers of script arguments.
float StackFloat(RS_STACKDATA *argument) {
    if (argument->type == RS_INT) {
        return (float) argument->i;
    }

    return argument->f;
}

void SetStackFloat(RS_STACKDATA *argument, float value) {
    if (argument->type == RS_PTR) {
        argument->p->f = value;
    }
}

// Where the current monster's script sees the player: the player, or what a confused monster (or one it
// provoked) goes after (confuse.hpp).
void PlayerPosition(float *position) {
    if (!ConfuseTarget(*NowMonstorUnit, NowMonstorUnit->current_monster, position)) {
        sceVu0CopyVector(position, CharaMain.pos);
    }
}

} // namespace

// Retail's _GET_DISTANCE, with the player's position as the monster's script sees it.
PORT_OVERRIDE int _GET_DISTANCE(RS_STACKDATA *stack, int argc) {
    int   monster_no = NowMonstorUnit->current_monster;
    float monster_position[4];
    float target_position[4];

    NowMonstorUnit->chara[monster_no][0].GetPosition(monster_position);

    if (argc == 1) {
        PlayerPosition(target_position);
    } else {
        target_position[0] = StackFloat(stack++);
        target_position[1] = StackFloat(stack++);
        target_position[2] = StackFloat(stack++);
    }

    SetStackFloat(stack, DistVector(monster_position, target_position));
    return 1;
}

// Retail's _GET_POSITION, with the player's position (target -2) as the monster's script sees it.
PORT_OVERRIDE int _GET_POSITION(RS_STACKDATA *stack, int argc) {
    int   monster_no = NowMonstorUnit->current_monster;
    int   target = (int) StackFloat(stack++);
    float position[4];

    if (target == -1) {
        NowMonstorUnit->chara[monster_no][0].GetPosition(position);
    }

    if (target == -2) {
        PlayerPosition(position);
    }

    SetStackFloat(stack++, position[0]);
    SetStackFloat(stack++, position[1]);
    SetStackFloat(stack, position[2]);
    return 1;
}
