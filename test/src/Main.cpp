#include <signal.h>
#include <stdint.h>
#include <unistd.h>

#include <chrono>
#include <iostream>

#include "SCurveProfile.hpp"
#include "TrapezoidProfile.hpp"
#include "LiveGrapher/LiveGrapher.hpp"

using namespace std::chrono_literals;

int main() {
    LiveGrapher liveGrapher(3513);
    liveGrapher.SetSendInterval(10ms);

    // Ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    double goal = 150;
    SCurveProfile sProfile(91.26, 228.15);
    TrapezoidProfile tProfile(91.26, 0.4);
    sProfile.setGoal(0, goal);
    tProfile.setGoal(0, goal);

    using clock = std::chrono::high_resolution_clock;
    using namespace std::chrono;

    uint64_t startTime = duration_cast<milliseconds>(clock::now().time_since_epoch()).count();
    float curTime = 0;
    float sSetpoint = 0.f;
    float tSetpoint = 0.f;

    sProfile.resetProfile();
    tProfile.resetProfile();
    while (1) {
        curTime = (duration_cast<milliseconds>(clock::now().time_since_epoch()).count() - startTime) / 1000.f;
        sSetpoint = sProfile.updateSetpoint(curTime);
        tSetpoint = tProfile.updateSetpoint(curTime);

        if (liveGrapher.HasIntervalPassed()) {
            liveGrapher.GraphData(sSetpoint, "SCurve SP");
            liveGrapher.GraphData(4.5, "Test");
            liveGrapher.GraphData(tSetpoint, "TCurve SP");

            liveGrapher.ResetInterval();
        }

        if (tProfile.atGoal()) {
            startTime = duration_cast<milliseconds>(clock::now().time_since_epoch()).count();

            if (sProfile.getGoal() == goal) {
                sProfile.setGoal(curTime, 0, sProfile.getGoal());
                tProfile.setGoal(curTime, 0, tProfile.getGoal());
            }
            else {
                sProfile.setGoal(curTime, goal, sProfile.getGoal());
                tProfile.setGoal(curTime, goal, tProfile.getGoal());
            }
        }

        std::this_thread::sleep_for(1ms);
    }
}

