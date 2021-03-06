// Copyright (c) 2018-2020 FRC Team 3512. All Rights Reserved.

#ifndef LIVEGRAPHER_TEST_SRC_PROFILEBASE_HPP_
#define LIVEGRAPHER_TEST_SRC_PROFILEBASE_HPP_

#include <mutex>

typedef enum { distance, velocity } SetpointMode;

class ProfileBase {
public:
    ProfileBase();
    virtual ~ProfileBase();

    virtual double updateSetpoint(double curTime) = 0;

    // Should return initial setpoint for start of profile
    virtual double setGoal(double t, double goal, double curSource) = 0;
    virtual bool atGoal();

    double getGoal() const;
    double getSetpoint() const;

    virtual void resetProfile();

    // Tells algorithm whether to use distance or velocity as setpoint
    void setMode(SetpointMode mode);
    SetpointMode getMode() const;

protected:
    // Use this to make updateSetpoint() and setGoal() thread-safe
    std::recursive_mutex m_varMutex;

    double m_goal;
    double m_setpoint;
    double m_lastTime;
    double m_timeTotal;

    SetpointMode m_mode;
};

#endif  // LIVEGRAPHER_TEST_SRC_PROFILEBASE_HPP_
