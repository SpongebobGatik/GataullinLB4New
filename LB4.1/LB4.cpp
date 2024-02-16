#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <barrier>
#include <chrono>
#include <random>
#include <benchmark/benchmark.h>

// Количество рабочих потоков
const int NUM_THREADS = 4;

// Количество символов в каждом потоке
const int NUM_SYMBOLS = 10;

// Массив для хранения символов
char threadSymbols[NUM_THREADS][NUM_SYMBOLS];

// Определяем все синхронизационные примитивы
std::counting_semaphore<1> semaphore(1); // Семафор с ресурсом 1
std::binary_semaphore slimSemaphore(1); // Семафор с ограниченным ресурсом 1
std::mutex mtx; // Мьютекс
std::barrier threadBarrier(1); // Барьер для синхронизации потоков
std::atomic_flag spinLock = ATOMIC_FLAG_INIT; // Спинлок
std::atomic<bool> spinWait(false); // Спинвейт
std::condition_variable condVar; // Условная переменная
std::mutex condVarMutex; // Мьютекс для условной переменной
bool isReady = false; // Флаг готовности для условной переменной

// Генератор случайных чисел
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(32, 126);

// Прототипы функций
void fillSymbols(int i);
void printSymbols();
double runRace(auto sync);
void BM_runRace(benchmark::State& state, auto sync, std::string name);

int main(int argc, char** argv) {
    // Запускаем бенчмарк
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}

// Реализация функций
void fillSymbols(int i) {
    for (int j = 0; j < NUM_SYMBOLS; j++) {
        threadSymbols[i][j] = dis(gen);
    }
}

void printSymbols() {
    for (int i = 0; i < NUM_THREADS; i++) {
        for (int j = 0; j < NUM_SYMBOLS; j++) {
            std::cout << threadSymbols[i][j];
        }
        std::cout << "\n";
    }
}

double runRace(auto sync) {
    // Создаем массив потоков
    std::thread threads[NUM_THREADS];

    // Запускаем таймер
    auto start = std::chrono::steady_clock::now();

    // Запускаем потоки с функцией заполнения массива
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i] = std::thread(fillSymbols, i);
    }

    // Синхронизируем потоки с помощью переданного объекта
    sync();

    // Останавливаем потоки
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].join();
    }

    // Останавливаем таймер
    auto end = std::chrono::steady_clock::now();

    // Возвращаем результаты
    return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - start).count();
}

void BM_runRace(benchmark::State& state, auto sync, std::string name) {
    double elapsed_time = 0;
    for (auto _ : state) {
        // Здесь пишем код, который хотим измерить
        elapsed_time = runRace(sync);
    }
    state.counters["StopWatch"] = elapsed_time;
}

// Регистрируем функцию для измерения
BENCHMARK_CAPTURE(BM_runRace, mutex, [&] { std::lock_guard<std::mutex> lock(mtx); }, "mutex")->Iterations(1)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_runRace, semaphore, [&] { semaphore.acquire(); semaphore.release(); }, "semaphore")->Iterations(1)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_runRace, slimSemaphore, [&] { slimSemaphore.acquire(); slimSemaphore.release(); }, "slimSemaphore")->Iterations(1)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_runRace, threadBarrier, [&] { threadBarrier.arrive_and_wait(); }, "threadBarrier")->Iterations(1)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_runRace, spinLock, [&] { while (spinLock.test_and_set(std::memory_order_acquire)); spinLock.clear(std::memory_order_release); }, "spinLock")->Iterations(1)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_runRace, spinWait, [&] { while (spinWait.exchange(true, std::memory_order_acquire)); spinWait.store(false, std::memory_order_release); }, "spinWait")->Iterations(1)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_runRace, condVar, [&] { std::unique_lock<std::mutex> lock(condVarMutex); isReady = true; condVar.notify_one(); condVar.wait(lock, [&] { return isReady; }); }, "condVar")->Iterations(1)->Unit(benchmark::kMillisecond);