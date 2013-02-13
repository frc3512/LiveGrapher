////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2012 Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

/* !!! THIS IS AN EXTREMELY ALTERED AND PURPOSE-BUILT VERSION OF SFML !!!
 * This distribution is designed to possess only a limited subset of the
 * original library's functionality and to only build on Win32 platforms.
 * The original distribution of this software has many more features and
 * supports more platforms.
 */

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "../SFML/System/Thread.hpp"
#include "../SFML/System/Err.hpp"

#include <cassert>
#include <process.h>

namespace sf
{
////////////////////////////////////////////////////////////
Thread::~Thread()
{
    wait();
    delete m_entryPoint;
}


////////////////////////////////////////////////////////////
void Thread::launch()
{
    wait();

    m_thread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &Thread::entryPoint, this, 0, &m_threadId));

    if (!m_thread)
        err() << "Failed to create thread" << std::endl;
}


////////////////////////////////////////////////////////////
void Thread::wait()
{
    if (m_thread)
    {
        assert(m_threadId != GetCurrentThreadId()); // A thread cannot wait for itself!
        WaitForSingleObject(m_thread, INFINITE);

        CloseHandle(m_thread);
        m_thread = NULL;
    }
}


////////////////////////////////////////////////////////////
void Thread::terminate()
{
    if (m_thread) {
        TerminateThread(m_thread, 0);

        CloseHandle(m_thread);
        m_thread = NULL;
    }
}


////////////////////////////////////////////////////////////
void Thread::run()
{
    m_entryPoint->run();
}

////////////////////////////////////////////////////////////
unsigned int __stdcall Thread::entryPoint(void* userData)
{
    // The Thread instance is stored in the user data
    Thread* owner = static_cast<Thread*>(userData);

    // Forward to the owner
    owner->run();

    // Optional, but it is cleaner
    _endthreadex(0);

    return 0;
}

} // namespace sf
