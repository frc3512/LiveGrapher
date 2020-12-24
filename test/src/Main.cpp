// Copyright (c) 2018-2020 FRC Team 3512. All Rights Reserved.

#include <chrono>

#include "SCurveProfile.hpp"
#include "TrapezoidProfile.hpp"
#include "livegrapher/LiveGrapher.hpp"

using namespace std::chrono_literals;

int main() {
    LiveGrapher liveGrapher(3513);

    double goal = 150.0;
    SCurveProfile sProfile(91.26, 228.15);
    TrapezoidProfile tProfile(91.26, 0.4);
    sProfile.setGoal(0.0, goal);
    tProfile.setGoal(0.0, goal);

    using clock = std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    auto startTime =
        duration_cast<milliseconds>(clock::now().time_since_epoch());
    auto lastTime = startTime;
    auto currentTime = startTime;
    float curTime = 0.f;
    float sSetpoint = 0.f;
    float tSetpoint = 0.f;

    sProfile.resetProfile();
    tProfile.resetProfile();
    while (1) {
        currentTime =
            duration_cast<milliseconds>(clock::now().time_since_epoch());
        curTime = (currentTime.count() - startTime.count()) / 1000.f;
        sSetpoint = static_cast<float>(sProfile.updateSetpoint(curTime));
        tSetpoint = static_cast<float>(tProfile.updateSetpoint(curTime));

        if (currentTime - lastTime > 10ms) {
            liveGrapher.AddData("SCurve SP", sSetpoint);
            liveGrapher.AddData("Test", 0.f);
            liveGrapher.AddData("TCurve SP", tSetpoint);

            liveGrapher.AddData("SCurve SP 2", -sSetpoint);
            liveGrapher.AddData("Test 2", 2.f);
            liveGrapher.AddData("TCurve SP 2", -tSetpoint);

            liveGrapher.AddData("SCurve SP 3", 20.f + sSetpoint);
            liveGrapher.AddData("Test 3", 4.f);
            liveGrapher.AddData("TCurve SP 3", 20.f + tSetpoint);

            liveGrapher.AddData("SCurve SP 4", 20.f - sSetpoint);
            liveGrapher.AddData("Test 4", 5.f);
            liveGrapher.AddData("TCurve SP 4", 20.f - tSetpoint);

            liveGrapher.AddData("SCurve SP 5", 20.f - sSetpoint);
            liveGrapher.AddData("Test 5", 6.f);
            liveGrapher.AddData("TCurve SP 5", 20.f - tSetpoint);

            liveGrapher.AddData("SCurve SP 6", 20.f - sSetpoint);
            liveGrapher.AddData("Test 6", 7.f);
            liveGrapher.AddData("TCurve SP 6", 20.f - tSetpoint);

            liveGrapher.AddData("SCurve SP 7", 20.f - sSetpoint);
            liveGrapher.AddData("Test 7", 8.f);
            liveGrapher.AddData("TCurve SP 7", 20.f - tSetpoint);

            liveGrapher.AddData("SCurve SP 8", 20.f - sSetpoint);
            liveGrapher.AddData("Test 8", 9.f);
            liveGrapher.AddData("TCurve SP 8", 20.f - tSetpoint);

            liveGrapher.AddData("SCurve SP 9", 20.f - sSetpoint);
            liveGrapher.AddData("Test 9", 10.f);
            liveGrapher.AddData("TCurve SP 9", 20.f - tSetpoint);

            liveGrapher.AddData("SCurve SP 10", 20.f - sSetpoint);
            liveGrapher.AddData("Test 10", 11.f);
            liveGrapher.AddData("TCurve SP 10", 20.f - tSetpoint);

            liveGrapher.AddData("SCurve SP 11", 20.f - sSetpoint);
            liveGrapher.AddData("Test 11", 12.f);
            liveGrapher.AddData("TCurve SP 11", 20.f - tSetpoint);  // 33

            lastTime = currentTime;
        }

        if (tProfile.atGoal()) {
            startTime =
                duration_cast<milliseconds>(clock::now().time_since_epoch());

            if (sProfile.getGoal() == goal) {
                sProfile.setGoal(curTime, 0.0, sProfile.getGoal());
                tProfile.setGoal(curTime, 0.0, tProfile.getGoal());
            } else {
                sProfile.setGoal(curTime, goal, sProfile.getGoal());
                tProfile.setGoal(curTime, goal, tProfile.getGoal());
            }
        }

        std::this_thread::sleep_for(5ms);
    }
}
