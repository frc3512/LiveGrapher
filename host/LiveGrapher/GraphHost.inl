// Copyright (c) FRC Team 3512, Spartatroniks 2013-2016. All Rights Reserved.

template <typename Rep, typename Period>
void GraphHost::SetSendInterval(
    const std::chrono::duration<Rep, Period>& time) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    m_sendInterval = duration_cast<milliseconds>(time).count();
}
