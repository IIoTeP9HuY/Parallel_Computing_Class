#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <vector>
#include <thread>

unsigned debug_flag;

void sleep(int max_delay) {
    std::this_thread::sleep_for(std::chrono::milliseconds( rand() % (max_delay + 1) ));
}

std::mutex turn_mutex;
unsigned wait_for_forks_time;

class Fork {
private:
    std::mutex m;
        bool isTaken_;

public:
    Fork(): m() {
    };

        bool isTaken() const
        {
                return isTaken_;
        }

    void take() {
        m.lock();
                isTaken_ = true;
    }

        bool try_take() {
                if (m.try_lock())
                {
                    isTaken_ = true;
                    return true;
                }
                return false;
        }

    void put() {
        m.unlock();
                isTaken_ = false;
    }
};

typedef std::chrono::milliseconds Milliseconds;
typedef std::chrono::high_resolution_clock Clock;
typedef Clock::time_point Time;

class Philosopher {
private:
    unsigned id;
    Fork* fork_left;
    Fork* fork_right;
    unsigned think_delay;
    unsigned eat_delay;
    unsigned eat_count;
    unsigned wait_time;
    Time wait_start;
    std::atomic<bool> stop_flag;
        std::atomic<bool> wants_to_eat;

    void think() {
        if (debug_flag) std::printf("[%d] thinking\n", id);
        sleep(think_delay);
        if (debug_flag) std::printf("[%d] hungry\n", id);
        wait_start = Clock::now();
    }

    void eat() {
        wait_time += std::chrono::duration_cast<Milliseconds>(Clock::now() - wait_start).count();
        if (debug_flag) std::printf("[%d] eating\n", id);
        sleep(eat_delay);
        eat_count++;
    }

public:
    Philosopher(unsigned id, Fork* fork_left, Fork* fork_right, unsigned think_delay, unsigned eat_delay) : 
        id(id), fork_left(fork_left), fork_right(fork_right), think_delay(think_delay), eat_delay(eat_delay), 
        eat_count(0), wait_time(0), stop_flag(false) {
    }

    void run() {
        while (!stop_flag) {
            think();

                        wants_to_eat = true;
                        bool success_take = false;
                        while (!success_take)
                        {
                            if (!fork_left->isTaken() && !fork_right->isTaken())
                            {
                                turn_mutex.lock();

                                bool left_fork_taken = fork_left->try_take();
                                if (debug_flag) std::printf("[%d] took left fork\n", id);

                                bool right_fork_taken = fork_right->try_take();
                                if (debug_flag) std::printf("[%d] took right fork\n", id);

                                if (left_fork_taken && right_fork_taken)
                                {
                                    success_take = true;    
                                }
                                else
                                {
                                    if (left_fork_taken)
                                    {
                                        fork_left->put();
                                        if (debug_flag) std::printf("[%d] put right fork\n", id);
                                    }
                                    if (right_fork_taken)
                                    {
                                        fork_right->put();
                                        if (debug_flag) std::printf("[%d] put left fork\n", id);
                                    }
                                }
                                turn_mutex.unlock();
                            }
                        std::this_thread::sleep_for(std::chrono::microseconds(wait_for_forks_time));
                        }

            eat();
                        wants_to_eat = false;

            fork_right->put();
            if (debug_flag) std::printf("[%d] put right fork\n", id);

            fork_left->put();
            if (debug_flag) std::printf("[%d] put left fork\n", id);
        }
    }

    void stop() {
        stop_flag = true;
    }

        bool wantsToEat() const {
            return wants_to_eat;
        }

    void printStats() const {
        std::printf("[%d] %d %dms\n", id, eat_count, wait_time);
    }

        unsigned getEatCount() const {
                return eat_count;
        }

        unsigned getWaitTime() const {
                return wait_time;
        }
};

struct GameStats
{
    GameStats(unsigned mean_eat_count, unsigned mean_wait_time):
        mean_eat_count(mean_eat_count), mean_wait_time(mean_wait_time) {}

    unsigned mean_eat_count;
    unsigned mean_wait_time;
};


GameStats run_game(unsigned N, unsigned duration, unsigned think_delay, unsigned eat_delay, bool debug_flag,
                            unsigned wait_for_forks_time_)
{
    wait_for_forks_time = wait_for_forks_time_;
    setbuf(stdout, NULL);
    srand((unsigned)time(0));

    Fork* forks[N];
    for (unsigned i=0; i<N; i++) {
            forks[i] = new Fork();
    }
    
    Philosopher* phils[N];
    for (unsigned i=0; i<N; i++) {
            phils[i] = new Philosopher(i + 1, forks[(i + 1) % N], forks[i], 
                    think_delay, eat_delay);
    }

    std::thread threads[N];
    for (unsigned i=0; i<N; i++) {
            threads[i] = std::thread(&Philosopher::run, phils[i]);
    }

    std::this_thread::sleep_for(std::chrono::seconds(duration));

    std::for_each(phils, phils + N, std::mem_fn(&Philosopher::stop));

    std::for_each(threads, threads + N, std::mem_fn(&std::thread::join));

    std::for_each(phils, phils + N, std::mem_fn(&Philosopher::printStats));
    size_t mean_eat_count = 0;
    size_t mean_wait_time = 0;
    for (const auto& phil : phils)
    {
        mean_eat_count += phil->getEatCount();
        mean_wait_time += phil->getWaitTime();
    }
    mean_eat_count /= N;
    mean_wait_time /= N;
    std::cout << "wait_for_forks_time = " << wait_for_forks_time << std::endl;
    std::cout << "mean_eat_count = " << mean_eat_count << std::endl;
    std::cout << "mean_wait_time = " << mean_wait_time << "ms" << std::endl;
    std::cout << "-------------------------" << std::endl;

    for (unsigned i=0; i<N; i++) {
            delete forks[i]; 
            delete phils[i];
    }

    return GameStats(mean_eat_count, mean_wait_time);
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
            std::printf("Usage: %s phil_count duration think_delay eat_delay debug_flag\n", argv[0]);
            return 1;
    }
    unsigned N = atoi(argv[1]);
    unsigned duration = atoi(argv[2]);
    unsigned think_delay = atoi(argv[3]);
    unsigned eat_delay = atoi(argv[4]);
    debug_flag = atoi(argv[5]);

    unsigned wait_for_forks_time_ = 30;
    run_game(N, duration, think_delay, eat_delay, debug_flag, wait_for_forks_time_);
    return 0;
}
