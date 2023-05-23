#include <chrono>

#define BENCHMARK(func, iterations, resultVariable)                             \
    do                                                                          \
    {                                                                           \
        auto start = std::chrono::high_resolution_clock::now();                 \
        for (int i = 0; i < iterations; ++i)                                    \
        {                                                                       \
            func;                                                               \
        }                                                                       \
        auto end = std::chrono::high_resolution_clock::now();                   \
        auto duration =                                                         \
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start); \
        resultVariable = duration.count();                                      \
        MESSAGE(#func " benchmark: " << resultVariable << "ms");                \
    } while (false)
    