#include "FiberScheduler.h"
#include <random>

FiberScheduler::FiberScheduler()
{
	m_pMainFiber = ConvertThreadToFiber(nullptr);
}

FiberScheduler::~FiberScheduler()
{
	ConvertFiberToThread();
	for (const Fiber* fiber : m_Fibers)
	{
		//Kill all fibers
		DeleteFiber(fiber->GetFiberAddress());
		delete fiber;
	}
}

void FiberScheduler::PushFiber(Fiber* fiber)
{
	m_FiberQueue.push_back(fiber->GetFiberAddress());
}

void FiberScheduler::AddFunctionToQueue(std::function<void()> func)
{
	Fiber* fiber = new Fiber(std::move(func), this);
	PushFiber(fiber);
	m_Fibers.emplace_back(fiber);
}

LPVOID FiberScheduler::GetNextFiber()
{
	if (m_FiberQueue.empty())
	{
		if (m_pMainFiber)
		{
			return m_pMainFiber;
		}

		//Out of work, end the current thread
		Fiber* CurrentFiber = reinterpret_cast<Fiber*>(GetFiberData());
		DeleteFiber(CurrentFiber->GetFiberAddress());
	}

	if (m_bShuffleQueue)
	{
		ShuffleQueue();
	}

#ifdef FIBER_SUPPORT_SLEEP
	unsigned long nShortestSleepTime = ULONG_MAX;
	size_t nShortestIndex = SIZE_MAX;
	Fiber* pShortestFiber = nullptr;
	std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();


	for (size_t nFiberIndex = 0; nFiberIndex < m_FiberQueue.size(); ++nFiberIndex)
	{
		const LPVOID currentFiberAddress = m_FiberQueue[nFiberIndex];
		Fiber* currentFiber = GetFiberByAddress(currentFiberAddress);
		if (!currentFiber)
		{
			continue;
		}

		if (currentFiber->IsSleeping())
		{
			if (now >= currentFiber->GetSleepEndTime())
			{
				currentFiber->SetIsSleeping(false);
				m_FiberQueue.erase(m_FiberQueue.begin() + nFiberIndex);
				return currentFiberAddress;
			}

			auto sleepDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentFiber->GetSleepEndTime() - now).count();
			if (sleepDuration < nShortestSleepTime)
			{
				nShortestSleepTime = sleepDuration;
				pShortestFiber = currentFiber;
				nShortestIndex = nFiberIndex;
			}

		}
		else
		{
			m_FiberQueue.erase(m_FiberQueue.begin() + nFiberIndex);
			return currentFiberAddress;
		}
	}

	if (pShortestFiber)
	{
		Sleep(nShortestSleepTime);
		pShortestFiber->SetIsSleeping(false);
		if (nShortestIndex < m_FiberQueue.size())
		{
			m_FiberQueue.erase(m_FiberQueue.begin() + nShortestIndex);
		}
		return pShortestFiber->GetFiberAddress();
	}

	return nullptr;

#else
	const LPVOID nextFiber = m_FiberQueue.front();
	m_FiberQueue.pop_front();
	return nextFiber;
#endif
}

LPVOID FiberScheduler::GetMainFiber()
{
	return m_pMainFiber;
}

void FiberScheduler::Start()
{
	if (!m_FiberQueue.empty())
	{
		const LPVOID firstFiber = GetNextFiber();
		SwitchToFiber(firstFiber);
	}
}

void FiberScheduler::ShuffleQueue()
{
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(m_FiberQueue.begin(), m_FiberQueue.end(), g);
}

void FiberScheduler::KillMainFiber()
{
	//Bye bye main fiber
	if (GetMainFiber())
	{
		DeleteFiber(GetMainFiber());
		m_pMainFiber = nullptr;
	}
}

Fiber* FiberScheduler::GetFiberByAddress(LPVOID Address)
{
	for (const auto Fiber : m_Fibers)
	{
		if (Fiber->GetFiberAddress() == Address)
		{
			return Fiber;
		}
	}
	return nullptr;
}

Fiber::Fiber(std::function<void()> func, FiberScheduler* sched) : m_pFiber(nullptr), m_BoundFunction(std::move(func)), m_pScheduler(sched)
{
	m_pFiber = CreateFiber(0, Fiber::Run, this);
}

void Fiber::SleepCurrentFiber(unsigned long milliseconds)
{
#ifdef FIBER_SUPPORT_SLEEP
	Fiber* CurrentFiber = static_cast<Fiber*>(GetFiberData());
	CurrentFiber->Sleep(milliseconds);
#endif
	YieldCurrentFiber();
}

#ifdef FIBER_SUPPORT_SLEEP
void Fiber::Sleep(unsigned long milliseconds)
{
	m_bSleeping = true;
	m_SleepEndTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
}
#endif

void Fiber::Run(LPVOID lpParameter)
{
	const Fiber* fiber = static_cast<Fiber*>(lpParameter);
	fiber->m_BoundFunction();
	SwitchToFiber(fiber->m_pScheduler->GetNextFiber());
}

void Fiber::YieldCurrentFiber()
{
	Fiber* CurrentFiber = static_cast<Fiber*>(GetFiberData());

	if (FiberScheduler* Scheduler = CurrentFiber->GetScheduler())
	{
		//Add current execution into queue
		Scheduler->PushFiber(CurrentFiber);

		if (Scheduler->IsMainFiberTerminationRequested())
		{
			Scheduler->KillMainFiber();
		}

		const LPVOID NextFiber = Scheduler->GetNextFiber();

		if (NextFiber)
		{
			SwitchToFiber(NextFiber);
		}
	}
}