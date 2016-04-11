#include "heap.h"

#include <heap/heap.h>
#include <utils/undefined.h>

#include <gtest/gtest.h>
#include <chrono>
#include <iostream>

TEST(comparsion, all)
{
    simple::heap<std::uint64_t, std::uint64_t> simple_heap;
    data::heap<std::uint64_t, std::uint64_t> heap(8);

    std::mt19937 generator;
    std::uniform_int_distribution<std::uint64_t> distribution(1, 1000);

    std::size_t size = distribution(generator);
    std::cout << "Size: " << size << " elements" << std::endl;

    std::vector<std::uint64_t> elements;
    // Generate elements sequence
    for (std::size_t i = 0; i < size; ++i)
    {
        auto x = distribution(generator);
        elements.push_back(x);
    }

    {
        // Fill buffer tree heap
        auto start = std::chrono::system_clock::now();
        for (auto x : elements)
            heap.add(x, x);
        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<int, std::milli>>(end - start);
        std::cout << "Filling buffer tree heap: " << duration.count() << " ms" << std::endl;
    }

    {
        // Fill simple heap
        auto start = std::chrono::system_clock::now();
        for (auto x : elements)
            simple_heap.add(x, x);
        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<int, std::milli>>(end - start);
        std::cout << "Filling simple heap: " << duration.count() << " ms" << std::endl;
    }

    std::vector<std::pair<std::uint64_t, std::uint64_t>> heap_elements;
    {
        // Fetch elements from buffer tree heap
        auto start = std::chrono::system_clock::now();
        while (!heap.empty())
            heap_elements.push_back(heap.remove_min());
        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<int, std::milli>>(end - start);
        std::cout << "Fetch elements from buffer tree heap: " << duration.count() << " ms" << std::endl;
    }

    std::vector<std::pair<std::uint64_t, std::uint64_t>> simple_heap_elements;
    {
        // Fetch elements from simple heap
        auto start = std::chrono::system_clock::now();
        while (!simple_heap.empty())
            simple_heap_elements.push_back(simple_heap.remove_min());
        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<int, std::milli>>(end - start);
        std::cout << "Fetch elements from simple heap: " << duration.count() << " ms" << std::endl;
    }

    EXPECT_EQ(heap_elements, simple_heap_elements);
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
