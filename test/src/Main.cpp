// Copyright (c) 2018-2020 FRC Team 3512. All Rights Reserved.

#include <signal.h>
#include <stdint.h>
#include <unistd.h>

#include <chrono>
#include <iostream>

#include "SCurveProfile.hpp"
#include "TrapezoidProfile.hpp"
#include "livegrapher/LiveGrapher.hpp"

using namespace std::chrono_literals;

int main() {
    LiveGrapher liveGrapher(3513);

    // Ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    double goal = 150;
    SCurveProfile sProfile(91.26, 228.15);
    TrapezoidProfile tProfile(91.26, 0.4);
    sProfile.setGoal(0, goal);
    tProfile.setGoal(0, goal);

    using clock = std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    auto startTime =
        duration_cast<milliseconds>(clock::now().time_since_epoch());
    auto lastTime = startTime;
    auto currentTime = startTime;
    float curTime = 0;
    float sSetpoint = 0.f;
    float tSetpoint = 0.f;

    sProfile.resetProfile();
    tProfile.resetProfile();
    while (1) {
        currentTime =
            duration_cast<milliseconds>(clock::now().time_since_epoch());
        curTime = (currentTime.count() - startTime.count()) / 1000.f;
        sSetpoint = sProfile.updateSetpoint(curTime);
        tSetpoint = tProfile.updateSetpoint(curTime);

        if (currentTime - lastTime > 10ms) {
            liveGrapher.AddData("SCurve SP", sSetpoint);
            liveGrapher.AddData("Test", 0.0);
            liveGrapher.AddData("TCurve SP", tSetpoint);

            liveGrapher.AddData("SCurve SP 2", -sSetpoint);
            liveGrapher.AddData("Test 2", 2.0);
            liveGrapher.AddData("TCurve SP 2", -tSetpoint);

            liveGrapher.AddData("SCurve SP 3", 20.0 + sSetpoint);
            liveGrapher.AddData("Test 3", 4.0);
            liveGrapher.AddData("TCurve SP 3", 20.0 + tSetpoint);

            liveGrapher.AddData("SCurve SP 4", 20.0 - sSetpoint);
            liveGrapher.AddData("Test 4", 5.0);
            liveGrapher.AddData("TCurve SP 4", 20.0 - tSetpoint);

            liveGrapher.AddData("SCurve SP 5", 20.0 - sSetpoint);
            liveGrapher.AddData("Test 5", 6.0);
            liveGrapher.AddData("TCurve SP 5", 20.0 - tSetpoint);

            liveGrapher.AddData("SCurve SP 6", 20.0 - sSetpoint);
            liveGrapher.AddData("Test 6", 7.0);
            liveGrapher.AddData("TCurve SP 6", 20.0 - tSetpoint);

            liveGrapher.AddData("SCurve SP 7", 20.0 - sSetpoint);
            liveGrapher.AddData("Test 7", 8.0);
            liveGrapher.AddData("TCurve SP 7", 20.0 - tSetpoint);

            liveGrapher.AddData("SCurve SP 8", 20.0 - sSetpoint);
            liveGrapher.AddData("Test 8", 9.0);
            liveGrapher.AddData("TCurve SP 8", 20.0 - tSetpoint);

            liveGrapher.AddData("SCurve SP 9", 20.0 - sSetpoint);
            liveGrapher.AddData("Test 9", 10.0);
            liveGrapher.AddData("TCurve SP 9", 20.0 - tSetpoint);

            liveGrapher.AddData("SCurve SP 10", 20.0 - sSetpoint);
            liveGrapher.AddData("Test 10", 11.0);
            liveGrapher.AddData("TCurve SP 10", 20.0 - tSetpoint);

            liveGrapher.AddData("SCurve SP 11", 20.0 - sSetpoint);
            liveGrapher.AddData("Test 11", 12.0);
            liveGrapher.AddData("TCurve SP 11", 20.0 - tSetpoint);  // 33

            lastTime = currentTime;
        }

        if (tProfile.atGoal()) {
            startTime =
                duration_cast<milliseconds>(clock::now().time_since_epoch());

            if (sProfile.getGoal() == goal) {
                sProfile.setGoal(curTime, 0, sProfile.getGoal());
                tProfile.setGoal(curTime, 0, tProfile.getGoal());
            } else {
                sProfile.setGoal(curTime, goal, sProfile.getGoal());
                tProfile.setGoal(curTime, goal, tProfile.getGoal());
            }
        }

        std::this_thread::sleep_for(5ms);
    }
}
