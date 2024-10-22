/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003-2017 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#include "PlatformThread.h"
#include "RegisterState.h"
#include <wtf/Lock.h>
#include <wtf/Noncopyable.h>
#include <wtf/ScopedLambda.h>
#include <wtf/ThreadSpecific.h>

#if USE(PTHREADS) && !OS(WINDOWS) && !OS(DARWIN)
#include <semaphore.h>
#include <signal.h>
// Using signal.h didn't make mcontext_t and ucontext_t available on FreeBSD.
// This bug has been fixed in FreeBSD 11.0-CURRENT, so this workaround can be
// removed after FreeBSD 10.x goes EOL.
// https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=207079
#if OS(FREEBSD)
#include <ucontext.h>
#endif
#endif

namespace JSC {

class CodeBlockSet;
class ConservativeRoots;
class Heap;
class JITStubRoutineSet;

struct CurrentThreadState {
    void* stackOrigin { nullptr };
    void* stackTop { nullptr };
    RegisterState* registerState { nullptr };
};
    
class MachineThreads {
    WTF_MAKE_NONCOPYABLE(MachineThreads);
public:
    MachineThreads();
    ~MachineThreads();

    void gatherConservativeRoots(ConservativeRoots&, JITStubRoutineSet&, CodeBlockSet&, CurrentThreadState*);

    JS_EXPORT_PRIVATE void addCurrentThread(); // Only needs to be called by clients that can use the same heap from multiple threads.

    class Thread {
        WTF_MAKE_FAST_ALLOCATED;
        Thread(const PlatformThread& platThread, void* base, void* end);

    public:
        ~Thread();

        static Thread* createForCurrentThread();

        struct Registers {
            void* stackPointer() const;
#if ENABLE(SAMPLING_PROFILER)
            void* framePointer() const;
            void* instructionPointer() const;
            void* llintPC() const;
#endif // ENABLE(SAMPLING_PROFILER)
            
#if OS(DARWIN)
#if CPU(X86)
            typedef i386_thread_state_t PlatformRegisters;
#elif CPU(X86_64)
            typedef x86_thread_state64_t PlatformRegisters;
#elif CPU(PPC)
            typedef ppc_thread_state_t PlatformRegisters;
#elif CPU(PPC64)
            typedef ppc_thread_state64_t PlatformRegisters;
#elif CPU(ARM)
            typedef arm_thread_state_t PlatformRegisters;
#elif CPU(ARM64)
            typedef arm_thread_state64_t PlatformRegisters;
#else
#error Unknown Architecture
#endif
            
#elif OS(WINDOWS)
            typedef CONTEXT PlatformRegisters;
#elif USE(PTHREADS)
            struct PlatformRegisters {
                pthread_attr_t attribute;
                mcontext_t machineContext;
            };
#else
#error Need a thread register struct for this platform
#endif
            
            PlatformRegisters regs;
        };
        
        bool operator==(const PlatformThread& other) const;
        bool operator!=(const PlatformThread& other) const { return !(*this == other); }

        bool suspend();
        void resume();
        size_t getRegisters(Registers&);
        void freeRegisters(Registers&);
        std::pair<void*, size_t> captureStack(void* stackTop);

        Thread* next;
        PlatformThread platformThread;
        void* stackBase;
        void* stackEnd;
#if OS(WINDOWS)
        HANDLE platformThreadHandle;
#elif USE(PTHREADS) && !OS(DARWIN)
        sem_t semaphoreForSuspendResume;
        mcontext_t suspendedMachineContext;
        int suspendCount { 0 };
        std::atomic<bool> suspended { false };
#endif
    };

    Lock& getLock() { return m_registeredThreadsMutex; }
    Thread* threadsListHead(const AbstractLocker&) const { ASSERT(m_registeredThreadsMutex.isLocked()); return m_registeredThreads; }
    Thread* machineThreadForCurrentThread();

private:
    void gatherFromCurrentThread(ConservativeRoots&, JITStubRoutineSet&, CodeBlockSet&, CurrentThreadState&);

    void tryCopyOtherThreadStack(Thread*, void*, size_t capacity, size_t*);
    bool tryCopyOtherThreadStacks(const AbstractLocker&, void*, size_t capacity, size_t*);

    static void THREAD_SPECIFIC_CALL removeThread(void*);

    template<typename PlatformThread>
    void removeThreadIfFound(PlatformThread);

    Lock m_registeredThreadsMutex;
    Thread* m_registeredThreads;
    WTF::ThreadSpecificKey m_threadSpecificForMachineThreads;
};

#define DECLARE_AND_COMPUTE_CURRENT_THREAD_STATE(stateName) \
    CurrentThreadState stateName; \
    stateName.stackTop = &stateName; \
    stateName.stackOrigin = wtfThreadData().stack().origin(); \
    ALLOCATE_AND_GET_REGISTER_STATE(stateName ## _registerState); \
    stateName.registerState = &stateName ## _registerState

// The return value is meaningless. We just use it to suppress tail call optimization.
int callWithCurrentThreadState(const ScopedLambda<void(CurrentThreadState&)>&);

} // namespace JSC

