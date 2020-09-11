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
            liveGrapher.GraphData(sSetpoint, "SCurve SP");
            liveGrapher.GraphData(0.0, "Test");
            liveGrapher.GraphData(tSetpoint, "TCurve SP");

            liveGrapher.GraphData(-sSetpoint, "SCurve SP 2");
            liveGrapher.GraphData(2.0, "Test 2");
            liveGrapher.GraphData(-tSetpoint, "TCurve SP 2");

            liveGrapher.GraphData(20.0 + sSetpoint, "SCurve SP 3");
            liveGrapher.GraphData(4.0, "Test 3");
            liveGrapher.GraphData(20.0 + tSetpoint, "TCurve SP 3");

            liveGrapher.GraphData(20.0 - sSetpoint, "SCurve SP 4");
            liveGrapher.GraphData(5.0, "Test 4");
            liveGrapher.GraphData(20.0 - tSetpoint, "TCurve SP 4");

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

        std::this_thread::sleep_for(1ms);
    }
}
