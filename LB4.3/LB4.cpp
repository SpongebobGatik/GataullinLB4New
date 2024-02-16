#include <iostream>
#include <thread>
#include <vector>
#include <semaphore>

std::counting_semaphore<2> max_philosophers(2);

void philosopher(int id, std::vector<std::unique_ptr<std::binary_semaphore>>& forks) {
    while (true) {
        std::cout << "Философ " << id << " думает\n";
        std::cout << "Философ " << id << " взял правую вилку\n";
        max_philosophers.acquire();
        forks[id]->acquire();
        std::cout << "Философ " << id << " взял левую вилку\n";
        forks[(id + 1) % 5]->acquire();
        std::cout << "Философ " << id << " ест\n";
        std::cout << "Философ " << id << " положил правую вилку\n";
        forks[id]->release();
        std::cout << "Философ " << id << " положил левую вилку\n";
        forks[(id + 1) % 5]->release();
        max_philosophers.release();
    }
}

int main() {
    setlocale(LC_ALL, "Rus");
    std::vector<std::unique_ptr<std::binary_semaphore>> forks;
    for (int i = 0; i < 5; ++i) {
        forks.push_back(std::make_unique<std::binary_semaphore>(1));
    }
    std::vector<std::thread> philosophers;
    for (int i = 0; i < 5; ++i) {
        philosophers.emplace_back(philosopher, i, std::ref(forks));
    }
    for (auto& philosopher : philosophers) {
        philosopher.join();
    }
    return 0;
}
