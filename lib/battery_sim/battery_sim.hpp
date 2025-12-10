#pragma once
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace batt {

template <typename T>
inline T clamp(T v, T lo, T hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}    



// Punt op de ontlaadcurve: resterende capaciteit (mAh) -> spanning (V)
struct Knot {
    double mAh_left;
    double volts;
};

// Lineaire interpolatie op de curve (assume: aflopend gesorteerd op mAh_left)
inline double targetVoltageFromRemaining(double mAh_left, const std::vector<Knot>& curve) {
    if (curve.empty()) return 0.0;

   
    if (mAh_left >= curve.front().mAh_left) return curve.front().volts;
    
    if (mAh_left <= curve.back().mAh_left)  return curve.back().volts;


    for (size_t i = 1; i < curve.size(); ++i) {
        if (mAh_left >= curve[i].mAh_left) {
            const double x0 = curve[i-1].mAh_left, y0 = curve[i-1].volts;
            const double x1 = curve[i].mAh_left,   y1 = curve[i].volts;
            const double u  = (mAh_left - x1) / (x0 - x1); 
            return y1 + (y0 - y1) * u;
        }
    }
    return curve.back().volts;
}

// Integreer capaciteit: mAh += I[A] * dt[s] * 1000 / 3600
inline double integrate_mAh(double current_A, double dt_s) {
    return current_A * (dt_s / 3600.0) * 1000.0;
}


inline double rpot_from_vout(double vout, double min_den = 0.05) {
    double d = vout - 1.0;
    return 42000.0 / d;
}

// Map Rpot -> wiper (0..255) met clamp op [0, Rmax]
uint8_t wiper_from_rpot(double rpot_ohm, double rmax_ohm) {
    double w = ((10000 - rpot_ohm)/10000) * 128.0;
    
    return w;
}

// Hulpstruct voor stateful simulatie van capaciteit
struct CapacityTracker {
    double cap_total_mAh;   // begin-capaciteit
    double used_mAh = 0.0;  // verbruikt

    explicit CapacityTracker(double total_mAh) : cap_total_mAh(total_mAh) {}

    // werk capaciteit bij op basis van gemeten stroom en dt
    void update(double current_A, double dt_s) {
        used_mAh += integrate_mAh(current_A, dt_s);
        if (used_mAh < 0) used_mAh = 0;
        if (used_mAh > cap_total_mAh) used_mAh = cap_total_mAh;
    }

    double left_mAh() const { return cap_total_mAh - used_mAh; }
};

} // namespace batt
