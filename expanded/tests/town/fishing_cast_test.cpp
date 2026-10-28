#include <gtest/gtest.h>

#include <array>
#include <cmath>

#include "camerafollow.hpp"
#include "ebattle.hpp"
#include "edit.hpp"
#include "expanded/data/fishing_spots.hpp"
#include "expanded/town/fishing.hpp"
#include "fishing.hpp"
#include "main.hpp"

namespace {

constexpr int   kNorune = 0, kQueens = 2, kYellowDrops = 23;
constexpr float kRetail = 1.6666666f;

// The town's first spot loaded, as its sign's _LOAD_FISHING_DATA loads it (none for a town without one).
void LoadTownSpot(int map_no) {
    const expanded::FishingSpot *spot = expanded::FishingSpotOfMap(map_no);
    expanded::FishingSpotLoad(spot != nullptr ? spot->area : 0, 0.0f);
}

// The globals a test sets, put back after: the town and its spot, the fishing state, the line's length, the water and
// ground, the fishing collision.
class Fishing {
public:
    explicit Fishing(int map_no)
        : map_no_(MapNo), state_(chara_fishing), distp_(distp), water_(WaterLevel), ground_(GroundLevel), cpoly_(cpoly), cpoly_num_(cpoly_num) {
        MapNo = map_no;
        LoadTownSpot(map_no);
        chara_fishing = ED_FISHING_STAND;
        cpoly = nullptr;
        cpoly_num = 0;
    }

    ~Fishing() {
        MapNo = map_no_;
        LoadTownSpot(map_no_);
        chara_fishing = state_;
        distp = distp_;
        FishingSetWaterLevel(water_, ground_);
        cpoly = cpoly_;
        cpoly_num = cpoly_num_;
    }

    Fishing(const Fishing &) = delete;
    Fishing &operator=(const Fishing &) = delete;

private:
    int     map_no_, state_;
    float   distp_, water_, ground_;
    CCPoly *cpoly_;
    int     cpoly_num_;
};

void Put(float *at, float x, float y, float z) {
    at[0] = x;
    at[1] = y;
    at[2] = z;
    at[3] = 1.0f;
}

// Every point of the line, float and hook at one place, still.
void LineAt(float x, float y, float z) {
    for (int i = 0; i < 24; i++) {
        Put(point[i], x, y, z);
        Put(old_p[i], x, y, z);
    }
    for (int i = 0; i < 4; i++) {
        Put(ukip[i], x, y, z);
        Put(ukiop[i], x, y, z);
        Put(ukiv[i], 0.0f, 0.0f, 0.0f);
    }
    for (int i = 0; i < 3; i++) {
        Put(hookp[i], x, y, z);
        Put(hookop[i], x, y, z);
        Put(hookv[i], 0.0f, 0.0f, 0.0f);
    }
}

// The bait's depth under the water once a line hanging from a rod over the water at the spot's length settles.
float SettledBaitDepth(float water, float line) {
    FishingSetWaterLevel(water, water - 40.0f);
    distp = kRetail * line;
    float rod[4] = {0.0f, water + 25.0f, 0.0f, 1.0f};
    FishLineInit(rod);
    for (int frame = 0; frame < 2000; frame++) {
        FishLineStep(rod, rod);
    }
    return water - (hookp[0][1] - 3.0f);
}

} // namespace

TEST(FishingCast, TheLineUnderTheFloatHangsTheBaitAtTheFish) {
    for (int map_no : {kQueens, kYellowDrops}) {
        Fishing                      town(map_no);
        const expanded::FishingSpot *spot = expanded::FishingSpotOfMap(map_no);
        ASSERT_NE(spot, nullptr);
        EXPECT_NEAR(SettledBaitDepth(spot->water, spot->line), spot->fish_depth, 1.0f) << map_no;
        EXPECT_EQ(expanded::LineSegment(18), distp) << map_no; // to the float
        EXPECT_NEAR(expanded::LineSegment(19), (spot->fish_depth - 3.47f) / 5.0f, 1e-5f) << map_no;
    }
    Fishing norune(kNorune);
    EXPECT_NEAR(SettledBaitDepth(0.0f, 1.0f), 12.0f, 1.0f); // retail's line and fish
    for (int segment = 1; segment < 24; segment++) {
        EXPECT_EQ(expanded::LineSegment(segment), distp) << segment;
    }
}

TEST(FishingCast, ALineOverOneStartsShortAndPaysOutOnTheFling) {
    Fishing     town(kYellowDrops);
    const float start = kRetail * expanded::kCastStart, full = kRetail * expanded::FishingSpotOfMap(kYellowDrops)->line;
    EXPECT_FLOAT_EQ(expanded::SpotLineSegment(7), start);
    float angler[4] = {0.0f, 30.0f, 0.0f, 1.0f};
    LineAt(0.0f, 40.0f, 4.0f); // the rod tip 4 ahead, facing +z
    expanded::FishLineStepped(angler, 0.0f);
    EXPECT_FLOAT_EQ(distp, start);

    // A swing forward before the wind-up starts nothing.
    chara_fishing = ED_FISHING_CASTING;
    Put(point[0], 0.0f, 40.0f, 7.0f);
    expanded::FishLineStepped(angler, 0.0f);
    EXPECT_FLOAT_EQ(distp, start);

    // Back, then forward: the line grows a frame at a time, carrying the float's flight across the ground.
    Put(point[0], 0.0f, 40.0f, -2.0f);
    expanded::FishLineStepped(angler, 0.0f);
    EXPECT_FLOAT_EQ(distp, start);
    Put(point[0], 0.0f, 40.0f, 7.0f);
    Put(ukip[0], 0.0f, 40.0f, 30.0f);
    Put(ukiv[0], 1.0f, -0.5f, 2.0f);
    expanded::FishLineStepped(angler, 0.0f);
    EXPECT_FLOAT_EQ(distp, start + 0.25f);
    EXPECT_FLOAT_EQ(ukiv[0][0], 1.16f);
    EXPECT_FLOAT_EQ(ukiv[0][1], -0.5f);
    EXPECT_FLOAT_EQ(ukiv[0][2], 2.32f);
    int frames = 1;
    Put(point[0], 0.0f, 40.0f, 0.0f); // the rod back at rest: paying out goes on
    while (distp < full && frames < 100) {
        expanded::FishLineStepped(angler, 0.0f);
        frames++;
    }
    EXPECT_FLOAT_EQ(distp, full);
    EXPECT_EQ(frames, static_cast<int>(std::ceil((full - start) / 0.25f)));
    const float carried = ukiv[0][0];
    expanded::FishLineStepped(angler, 0.0f);
    EXPECT_FLOAT_EQ(distp, full);
    EXPECT_EQ(ukiv[0][0], carried); // paid out: no more carry

    // Reeled in, or the float back at the rod: short again.
    chara_fishing = ED_FISHING_WAIT_BITE;
    Put(ukip[0], 0.0f, 38.0f, 3.0f);
    expanded::FishLineStepped(angler, 0.0f);
    EXPECT_FLOAT_EQ(distp, start);
    distp = full;
    chara_fishing = ED_FISHING_STAND;
    expanded::FishLineStepped(angler, 0.0f);
    EXPECT_FLOAT_EQ(distp, start);
}

TEST(FishingCast, ARetailLineDoesNotPayOut) {
    for (int map_no : {kNorune, kQueens + 100}) {
        Fishing town(map_no);
        distp = kRetail;
        float angler[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        LineAt(0.0f, 10.0f, 4.0f);
        chara_fishing = ED_FISHING_CASTING;
        for (float ahead : {4.0f, -6.0f, 9.0f, 9.0f}) {
            Put(point[0], 0.0f, 10.0f, ahead);
            expanded::FishLineStepped(angler, 0.0f);
        }
        EXPECT_EQ(distp, kRetail) << map_no;
    }
}

TEST(FishingCast, ACastFromTheCanalFloorStopsAtItsWalls) {
    Fishing town(kQueens);
    distp = kRetail;
    float angler[4] = {300.0f, 0.0f, 0.0f, 1.0f};
    LineAt(300.0f, 10.0f, 30.0f); // the float hanging low inside the canal
    expanded::FishLineStepped(angler, 0.0f);
    chara_fishing = ED_FISHING_CASTING;
    for (float *at : {point[20], old_p[20], ukip[1], ukiop[1], hookp[2], hookop[2]}) {
        at[2] = 55.0f;
    }
    point[21][2] = old_p[21][2] = -53.0f;
    expanded::FishLineStepped(angler, 0.0f);
    for (const float *at : {point[20], old_p[20], ukip[1], ukiop[1], hookp[2], hookop[2]}) {
        EXPECT_EQ(at[2], 47.0f);
    }
    EXPECT_EQ(point[21][2], -47.0f);
    EXPECT_EQ(old_p[21][2], -47.0f);
    point[17][2] = 55.0f; // the line up to the float flies on
    expanded::FishLineStepped(angler, 0.0f);
    EXPECT_EQ(point[17][2], 55.0f);
}

TEST(FishingCast, ACastFromTheBankPassesOverTheWalls) {
    Fishing town(kQueens);
    distp = kRetail;
    float angler[4] = {250.0f, 70.0f, -70.0f, 1.0f};
    LineAt(250.0f, 80.0f, -68.0f); // the float at the rod, high on the bank
    expanded::FishLineStepped(angler, 0.0f);
    chara_fishing = ED_FISHING_CASTING;
    point[20][2] = 55.0f;
    expanded::FishLineStepped(angler, 0.0f);
    EXPECT_EQ(point[20][2], 55.0f);
    LineAt(250.0f, 30.0f, 70.0f); // a low float outside the canal, on the far bank's side
    chara_fishing = ED_FISHING_STAND;
    expanded::FishLineStepped(angler, 0.0f);
    chara_fishing = ED_FISHING_CASTING;
    expanded::FishLineStepped(angler, 0.0f);
    EXPECT_EQ(point[20][2], 70.0f);
}

TEST(FishingCast, EveryCastStopsAtTheBridgesLegs) {
    Fishing town(kQueens);
    distp = kRetail;
    float angler[4] = {-50.0f, 70.0f, -70.0f, 1.0f};
    LineAt(-50.0f, 80.0f, -68.0f);
    expanded::FishLineStepped(angler, 0.0f);
    chara_fishing = ED_FISHING_CASTING;
    Put(point[19], -50.0f, 20.0f, 37.0f); // nearest the leg's canal side
    Put(ukip[2], -24.0f, 20.0f, 45.0f);   // nearest its east side
    Put(hookp[0], -50.0f, 48.0f, 40.0f);  // over the legs
    expanded::FishLineStepped(angler, 0.0f);
    EXPECT_EQ(point[19][2], 35.0f);
    EXPECT_EQ(old_p[19][2], 35.0f);
    EXPECT_FLOAT_EQ(ukip[2][0], -22.93f);
    EXPECT_FLOAT_EQ(ukiop[2][0], -22.93f);
    EXPECT_EQ(hookp[0][2], 40.0f);
}

TEST(FishingCast, AWaitingFloatOutOfTheCanalIsReeledIn) {
    Fishing town(kQueens);
    chara_fishing = ED_FISHING_WAIT_BITE;
    LineAt(250.0f, 80.0f, -68.0f);
    float uki[4];
    Put(uki, 250.0f, 31.0f, 50.0f);
    EXPECT_TRUE(expanded::FloatOutOfCanal(uki)); // past the wall
    Put(uki, 250.0f, 31.0f, -49.0f);
    EXPECT_FALSE(expanded::FloatOutOfCanal(uki));
    Put(uki, 800.0f, 31.0f, 40.0f);
    EXPECT_TRUE(expanded::FloatOutOfCanal(uki)); // in a leg
    Put(uki, 800.0f, 31.0f, 36.0f);
    EXPECT_FALSE(expanded::FloatOutOfCanal(uki)); // at its face
    Put(point[0], -50.0f, 62.0f, 0.0f);           // the rod over the west bridge's deck
    Put(uki, -40.0f, 31.0f, 0.0f);
    EXPECT_TRUE(expanded::FloatOutOfCanal(uki)); // under it
    Put(uki, -80.0f, 31.0f, 0.0f);
    EXPECT_FALSE(expanded::FloatOutOfCanal(uki)); // off its side
    Put(uki, 250.0f, 31.0f, 50.0f);
    chara_fishing = ED_FISHING_REEL_IN;
    EXPECT_FALSE(expanded::FloatOutOfCanal(uki));
    Fishing yellow_drops(kYellowDrops);
    chara_fishing = ED_FISHING_WAIT_BITE;
    EXPECT_FALSE(expanded::FloatOutOfCanal(uki));
}

TEST(FishingCast, TheFloatDoesNotLiftOntoAFloorOverIt) {
    Fishing town(kQueens);
    Put(point[18], 0.0f, 25.0f, 0.0f);
    EXPECT_TRUE(expanded::FloorOverFloat(60.0f)); // a deck
    EXPECT_FALSE(expanded::FloorOverFloat(26.0f));
    EXPECT_FALSE(expanded::FloorOverFloat(10.0f)); // the canal's floor
    Fishing norune(kNorune);
    EXPECT_FALSE(expanded::FloorOverFloat(60.0f));
}

TEST(FishingCast, OnlyASettledFloatIsReeledInForItsHeight) {
    Put(ukiv[0], 0.3f, 0.0f, 0.0f);
    EXPECT_FALSE(expanded::FloatSettled());
    Put(ukiv[0], 0.1f, -0.1f, 0.1f);
    EXPECT_TRUE(expanded::FloatSettled());
    Put(ukiv[0], 0.0f, 0.0f, 0.0f);
}

TEST(FishingCamera, HeightBySpot) {
    {
        Fishing norune(kNorune);
        EXPECT_EQ(expanded::FishingCameraHeight(40.0f), 40.0f);
    }
    {
        Fishing yellow_drops(kYellowDrops);
        EXPECT_EQ(expanded::FishingCameraHeight(40.0f), 30.0f);
    }
    // Queens: the eye under the walls' tops, 4 down, over the water the float rides.
    Fishing town(kQueens);
    for (auto [water, height] : std::array<std::array<float, 2>, 4>{
             {{31.0f, 35.0f}, {6.0f, 40.0f}, {52.0f, 14.0f}, {60.0f, expanded::kLowestFishingCamera}}
    }) {
        FishingSetWaterLevel(water, 0.0f);
        EXPECT_EQ(expanded::FishingCameraHeight(40.0f), height) << water;
    }
}

TEST(FishingCamera, InTheCanalTheEyeStaysInsideItsWalls) {
    Fishing   town(kQueens);
    const int fishing = EdMoveCharaInfo.fishing;
    EdMoveCharaInfo.fishing = 1;
    chara_fishing = ED_FISHING_WAIT_BITE;
    EXPECT_TRUE(expanded::FishingCameraInCanal());
    chara_fishing = ED_FISHING_STAND; // watching the angler, over the bank
    EXPECT_FALSE(expanded::FishingCameraInCanal());

    // Behind the float, over the north bank: turned along the wall, on the side it leant to.
    CCameraFollow camera(70.0f, 35.0f, 3.0f, 8.0f);
    camera.SetFollow(250.0f, 31.0f, -20.0f);
    expanded::KeepFishingCameraInCanal(camera);
    EXPECT_EQ(camera.GetDistance(), 70.0f);
    EXPECT_NEAR(-20.0f + 70.0f * std::cos(camera.angle), -45.0f, 1e-3f);
    EXPECT_GT(std::sin(camera.angle), 0.0f);
    EXPECT_EQ(camera.next_angle, camera.angle);
    camera.SetAngleSoon(-0.2f); // over the south bank, leaning the other way
    expanded::KeepFishingCameraInCanal(camera);
    EXPECT_NEAR(-20.0f + 70.0f * std::cos(camera.angle), 45.0f, 1e-3f);
    EXPECT_LT(std::sin(camera.angle), 0.0f);
    camera.SetAngleSoon(1.5f); // along the canal: left as it is
    expanded::KeepFishingCameraInCanal(camera);
    EXPECT_EQ(camera.angle, 1.5f);

    Fishing yellow_drops(kYellowDrops);
    chara_fishing = ED_FISHING_WAIT_BITE;
    EXPECT_FALSE(expanded::FishingCameraInCanal());
    camera.SetAngleSoon(3.0f);
    expanded::KeepFishingCameraInCanal(camera);
    EXPECT_EQ(camera.angle, 3.0f);
    EdMoveCharaInfo.fishing = fishing;
}

TEST(FishingCamera, ACatchsCloseUpGivesTheCameraBack) {
    CCameraFollow camera(70.0f, 40.0f, 0.5f, 8.0f);
    camera.SetAngle(1.25f);
    expanded::FishingCatchCamera(camera, ED_FISHING_BATTLE);
    for (int state : {ED_FISHING_LANDED, ED_FISHING_RESULT, ED_FISHING_FINISH}) {
        camera.SetAngle(-2.0f);
        camera.SetDistance(30.0f);
        expanded::FishingCatchCamera(camera, state);
        EXPECT_EQ(camera.GetDistance(), 30.0f) << state;
    }
    expanded::FishingCatchCamera(camera, ED_FISHING_INIT);
    EXPECT_EQ(camera.GetDistance(), 70.0f);
    EXPECT_EQ(camera.next_angle, 1.25f);
    camera.SetAngle(0.25f); // turned while fishing on
    expanded::FishingCatchCamera(camera, ED_FISHING_STAND);
    EXPECT_EQ(camera.next_angle, 0.25f);
}
