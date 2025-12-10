#include <gtest/gtest.h>
#include "battery_sim.hpp"
using namespace batt;

static std::vector<Knot> demoCurve() {
    return {
        {2000, 4.20},
        {1800, 4.00},
        {1200, 3.85},
        { 800, 3.75},
        { 400, 3.60},
        { 200, 3.45},
        {   0, 3.20}
    };
}

TEST(BatterySim, Interpolation_MidSegment) {
    auto c = demoCurve();
    // 1000 mAh ligt tussen 1200->3.85 en 800->3.75 -> verwacht 3.80 V
    double v = targetVoltageFromRemaining(1000.0, c);
    EXPECT_NEAR(v, 3.80, 1e-6);
}

TEST(BatterySim, Interpolation_Clamps) {
    auto c = demoCurve();
    EXPECT_DOUBLE_EQ(targetVoltageFromRemaining(3000.0, c), 4.20); // boven max
    EXPECT_DOUBLE_EQ(targetVoltageFromRemaining(-10.0,  c), 3.20); // onder min
}

TEST(BatterySim, Integrate_mAh) {
    // 2 A gedurende 30 s => (2 * 30/3600 * 1000) = 16.666... mAh
    double dmAh = integrate_mAh(2.0, 30.0);
    EXPECT_NEAR(dmAh, 16.6667, 1e-3);
}

TEST(BatterySim, RpotFormula_Guard) {
    // Bij vout=1.01 -> d≈0.01, guard maakt d=0.05 -> R=840k
    double r = rpot_from_vout(1.01, 0.05);
    EXPECT_NEAR(r, 42000.0 / 0.05, 1e-9);
}

TEST(BatterySim, WiperMapping_Clamps) {
    // Rmax = 100k; wiper=~128 bij 50k
    EXPECT_EQ(wiper_from_rpot(50000.0, 100000.0), 128);
    // onder 0 -> 0
    EXPECT_EQ(wiper_from_rpot(-10.0, 100000.0), 0);
    // boven Rmax -> 255
    EXPECT_EQ(wiper_from_rpot(200000.0, 100000.0), 255);
}


TEST(BatterySim, CapacityTracker_IntegratesWithoutClamp) {
    batt::CapacityTracker t(5000.0);        // groot genoeg → geen clamp
    t.update(1.0, 3600.0);                   // 1 A * 1 h = 1000 mAh
    EXPECT_NEAR(t.used_mAh, 1000.0, 1e-6);
    EXPECT_NEAR(t.left_mAh(), 4000.0, 1e-6);
}

TEST(BatterySim, CapacityTracker_ClampsToTotal) {
    batt::CapacityTracker t(100.0);          // kleine accu
    t.update(1.0, 3600.0);                   // zou 1000 mAh zijn…
    EXPECT_DOUBLE_EQ(t.used_mAh, 100.0);     // …maar direct geclamped op 100
    EXPECT_DOUBLE_EQ(t.left_mAh(), 0.0);
}

TEST(BatterySim, Integrate_mAh2) {
    EXPECT_NEAR(batt::integrate_mAh(1.0, 3600.0), 1000.0, 1e-6);
}