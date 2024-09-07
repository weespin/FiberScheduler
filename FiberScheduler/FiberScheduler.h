#pragma once

#define FIBER_SUPPORT_SLEEP

#include <chrono>
#include <functional>
#include <vector>
#include <windows.h>

class Fiber;
class FiberScheduler
{
public:
	FiberScheduler();
	~FiberScheduler();

	void PushFiber(Fiber* fiber);

	void AddFunctionToQueue(std::function<void()> func);

	LPVOID GetNextFiber();
	LPVOID GetMainFiber();

	void Start();
	void ShuffleQueue();

	void SetFiberShuffle(bool bEnabled) { m_bShuffleQueue = bEnabled; }
	bool IsFiberShuffleEnabled() const { return m_bShuffleQueue; }

	//Don't set main fiber termination if the scheduler is created on the stack of the thread
	void SetMainFiberTermination(bool bKill) { m_bKillMainThread = bKill; }

	bool IsMainFiberTerminationRequested() const { return m_bKillMainThread && m_pMainFiber; }
	void KillMainFiber();
	Fiber* GetFiberByAddress(LPVOID Address);

private:
	//todo: we don't really need 2 containers
#ifdef FIBER_SUPPORT_SLEEP
	std::vector<LPVOID> m_FiberQueue;
#else
	std::deque<LPVOID> m_FiberQueue;
#endif

	std::vector<Fiber*> m_Fibers;

	LPVOID m_pMainFiber;
	bool m_bKillMainThread = false;
	bool m_bShuffleQueue = true;
};

class Fiber
{
public:
	Fiber(std::function<void()> func, FiberScheduler* sched);

	LPVOID GetFiberAddress() const { return m_pFiber; }
	std::function<void()> GetBoundFunction() const { return m_BoundFunction; }
	FiberScheduler* GetScheduler() const { return m_pScheduler; }

	static void CALLBACK Run(LPVOID lpParameter);

	static void YieldCurrentFiber();
	static void SleepCurrentFiber(unsigned long Miliseconds);

#ifdef FIBER_SUPPORT_SLEEP
	void Sleep(unsigned long miliseconds);
	bool IsSleeping() const { return m_bSleeping; }
	const std::chrono::steady_clock::time_point& GetSleepEndTime() { return m_SleepEndTime; }
	void SetIsSleeping(bool bSleep) { m_bSleeping = bSleep; };
#endif

private:
	LPVOID m_pFiber;
	std::function<void()> m_BoundFunction;
	FiberScheduler* m_pScheduler;
#ifdef FIBER_SUPPORT_SLEEP
	bool m_bSleeping;
	std::chrono::steady_clock::time_point m_SleepEndTime;
#endif
};
