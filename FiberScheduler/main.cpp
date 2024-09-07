#include <iostream>
#include "FiberScheduler.h"

void task1()
{
	while (true)
	{
		std::cout << "Task 1: Step 1\n";
		Fiber::SleepCurrentFiber(500);
		std::cout << "Task 1: Step 2\n";
	}
}

void task2()
{
	while (true)
	{
		std::cout << "Task 2: Step 1\n";
		Fiber::YieldCurrentFiber();
		std::cout << "Task 2: Step 2\n";
	}
}

void task3()
{
	while (true)
	{
		std::cout << "Task 2: Step 1\n";
		Fiber::SleepCurrentFiber(500);
		std::cout << "Task 2: Step 2\n";
	}
}

FiberScheduler fiberScheduler;

int main()
{
	fiberScheduler.SetMainFiberTermination(true);
	fiberScheduler.SetFiberShuffle(false);

	fiberScheduler.AddFunctionToQueue(task1);
	fiberScheduler.AddFunctionToQueue(task2);
	fiberScheduler.AddFunctionToQueue(task3);

	fiberScheduler.Start();

	//Never reached if SetMainFiberTermination(true)
	std::cout << "Bye" << std::endl;

	return 0;
}