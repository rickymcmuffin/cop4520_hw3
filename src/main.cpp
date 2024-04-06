#include "concurrentqueue.h"
#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

// This code is for Problem 1
#define NUM_PRESENTS 500000
#define NUM_SERVANTS 4

typedef struct Node
{
    int tag;
    Node *next;
    std::mutex mutex;
} Node;

Node *list_head;

std::vector<int> presents(NUM_PRESENTS);
std::mutex presents_lock;

std::vector<int> thank_yous;
std::mutex thank_lock;

// Creates a new node with tag
Node *new_node(int tag)
{
    Node *ret = new Node;
    ret->tag = tag;
    ret->next = NULL;
    return ret;
}

void run_servant(int id)
{
    // when false, remove and thank you
    bool add_gift = false;

    // Only way to exit is break
    while (true)
    {
        add_gift = !add_gift;

        // Add gift to chain
        if (add_gift)
        {
            // pop presents
            presents_lock.lock();
            if (presents.size() == 0)
            {
                presents_lock.unlock();
                break;
            }
            int tag = presents.back();
            presents.pop_back();
            presents_lock.unlock();

            // Present to be added
            Node *new_present = new_node(tag);

            Node *cur_node = list_head;
            cur_node->mutex.lock();
            while (cur_node->tag < tag && cur_node->next != NULL)
            {
                // Go to next node
                Node *next_node = cur_node->next;
                next_node->mutex.lock();
                cur_node->mutex.unlock();
                cur_node = next_node;
            }
            new_present->next = cur_node->next;
            cur_node->next = new_present;
            cur_node->mutex.unlock();
        } else
        // removes first present in cahin
        {
            list_head->mutex.lock();
            if (list_head->next != NULL)
            {
                Node *second = list_head->next;
                second->mutex.lock();
                list_head->next = second->next;

                // adds to thank you list
                thank_lock.lock();
                thank_yous.push_back(second->tag);
                thank_lock.unlock();
                second->mutex.unlock();
                delete second;
            }
            list_head->mutex.unlock();
        }
    }
}

void problem1()
{
    // fill presents
    std::iota(std::begin(presents), std::end(presents), 0);

    // shuffle presents
    auto rng = std::default_random_engine{};
    std::shuffle(std::begin(presents), std::end(presents), rng);

    // initialize presents chain
    list_head = new_node(-1);

    std::vector<std::thread> threads;
    for (auto i = 0; i < NUM_SERVANTS; i++)
    {
        std::thread thread(run_servant, i);
        threads.push_back(std::move(thread));
    }

    for (auto i = 0; i < NUM_SERVANTS; i++)
    {
        threads[i].join();
    }

    std::cout << "Number of \"Thank You\" Letters: " << thank_yous.size()
              << '\n';
}

// End of problem 1 code

// Problem 2 Code

#define NUM_CORES 8

typedef struct reading
{
    int temperature;
    int time;
} reading;

// using lock-free library
moodycamel::ConcurrentQueue<reading> readings;

// gets a random temperature between -70 and 100
int get_temperature() { return rand() % 171 - 70; }

// runs each individual sensor
void run_sensor(int startTime)
{
    int time = startTime;
    while (time < 60)
    {
        reading read;
        read.temperature = get_temperature();
        read.time = time;
        readings.enqueue(read);
        // So each core runs different minutes
        time += NUM_CORES;
    }
}

// deques all readings and sorts them to find smallest difference.
void make_report()
{
    int sorted_temps[60];
    reading r;
    while (readings.try_dequeue(r))
    {
        sorted_temps[r.time] = r.temperature;
    }
    int largest_diff = INT_MIN;
    int interval_time = 0;
    for (auto i = 10; i < 60; i++)
    {
        int diff = abs(sorted_temps[i] - sorted_temps[i - 10]);
        if (diff > largest_diff)
        {
            largest_diff = diff;
            interval_time = i;
        }
    }

    std::cout << "Largest difference of 10 minutes is " << interval_time - 10
              << " to " << interval_time << " minutes\n";
    std::cout << "The difference is " << largest_diff << "\n\n";
}

void problem2()
{
    int i = 0;

    // run for 5 hours
    while (i < 5)
    {
        std::vector<std::thread> threads;

        for (auto i = 0; i < NUM_CORES; i++)
        {
            std::thread thread(run_sensor, i);
            threads.push_back(std::move(thread));
        }

        for (auto i = 0; i < NUM_CORES; i++)
        {
            threads[i].join();
        }
        make_report();
        i++;
    }
}

int main()
{
    std::cout << "Running Problem 1..." << '\n';
    problem1();
    std::cout << "\nRunning Problem 2..."<<'\n';
    problem2();
    return 0;
}
