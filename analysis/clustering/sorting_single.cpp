#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <string>
#include <queue>

std::vector<std::string> generateRandomIntegers(int N)
{
    std::vector<std::string> randomIntegers;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 10000);
    for (int i = 0; i < N; ++i)
        randomIntegers.push_back(std::to_string(dist(gen)));

    return randomIntegers;
}

int main()
{
    std::vector<std::string> nums = generateRandomIntegers(10000000);
    
    for (size_t i = 500; i <= 25000; i += 500)
    {
        std::vector<std::string> clone1(nums);
        std::vector<std::string> result1;

        // first timing
        auto start = std::chrono::high_resolution_clock::now();

        result1.reserve(i);
        for (const auto &num : clone1)
        {
            if (result1.size() >= i && result1.back() < num)
                continue;

            auto it = std::lower_bound(result1.begin(), result1.end(), num);
            result1.insert(it, num);
            if (result1.size() > i)
                result1.resize(i);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << duration.count() << std::endl;
        // end first timing

        std::vector<std::string> clone3(nums);
        std::priority_queue<std::string, std::vector<std::string>, std::less<std::string>> pq;
        auto start2 = std::chrono::high_resolution_clock::now();
        
        for (const auto &num : nums)
        {
            if (pq.size() < i)
                pq.push(num);
            else if (num < pq.top())
            {
                pq.pop();
                pq.push(num);
            }
        }

        std::vector<std::string> result3;
        while (!pq.empty())
        {
            result3.push_back(pq.top());
            pq.pop();
        }

        std::reverse(result3.begin(), result3.end());
        auto end2 = std::chrono::high_resolution_clock::now();
        auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
        std::cout << duration2.count() << std::endl;

        std::vector<std::string> clone2(nums);
        std::vector<std::string> result2;

        auto start1 = std::chrono::high_resolution_clock::now();
        for (const auto &num : clone2)
            result2.push_back(num);

        std::sort(result2.begin(), result2.end());
        result2.resize(i);

        auto end1 = std::chrono::high_resolution_clock::now();
        auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
        std::cout << duration1.count() << std::endl;
    }

    return 0;
}