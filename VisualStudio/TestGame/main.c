
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "FyuuCore.h"

#include <time.h>
#include <Windows.h>

#if _DEBUG
#pragma comment(lib, "../x64/Debug/Engine.lib")
#elif NDEBUG
#pragma comment(lib, "../x64/Release/Engine.lib")
#endif

static void MemoryAllocTest() {

	LARGE_INTEGER startCount, endCount, freq;
	double elapsedTime;
	double elapsedMs;
	size_t iterations = 1000000;
	QueryPerformanceFrequency(&freq);

	QueryPerformanceCounter(&startCount);
	for (size_t i = 0; i < iterations; ++i) {
		void* ptr = malloc(1024);
		if (ptr) {
			free(ptr); 
		}
	}
	QueryPerformanceCounter(&endCount);
	elapsedTime = (double)(endCount.QuadPart - startCount.QuadPart) / freq.QuadPart;
	elapsedMs = elapsedTime * 1000.0;
	printf("Elapsed time: %f\n", elapsedMs);

	QueryPerformanceCounter(&startCount);
	for (size_t i = 0; i < iterations; ++i) {
		void* ptr = Fyuu_Malloc(1024);
		if (ptr) {
			Fyuu_Free(ptr);
		}
	}
	QueryPerformanceCounter(&endCount);
	elapsedTime = (double)(endCount.QuadPart - startCount.QuadPart) / freq.QuadPart;
	elapsedMs = elapsedTime * 1000.0;
	printf("Elapsed time: %f\n", elapsedMs);

}

int main(int argc, char** argv) {
	//MemoryAllocTest();
	return Fyuu_RunApplication(argc, argv);
}