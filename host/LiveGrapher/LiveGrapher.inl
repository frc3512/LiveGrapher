// Copyright (c) 2013-2018 FRC Team 3512. All Rights Reserved.

#pragma once

template <typename Rep, typename Period>
void LiveGrapher::SetSendInterval(
    const std::chrono::duration<Rep, Period>& time) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    m_sendInterval = duration_cast<milliseconds>(time).count();
}
