#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>

// Структура, представляющая результат сессии
struct SessionResult {
    int semester;
    std::string discipline;
    int grade;
};

// Структура, представляющая данные о студенте
struct Student {
    std::string fullName;
    std::string groupNumber;
    std::vector<SessionResult> results;
};

// Функция для вычисления средней успеваемости студентов группы за семестр
float calculateAverageGrade(const std::vector<Student>& students, const std::string& group, int semester) {
    int totalGrades = 0;
    int numStudents = 0;

    // Итерируемся по студентам
    for (const auto& student : students) {
        // Проверяем, что студент из нужной группы
        if (student.groupNumber == group) {
            // Итерируемся по результатам сессий студента
            for (const auto& result : student.results) {
                // Проверяем, что результат относится к нужному семестру
                if (result.semester == semester) {
                    totalGrades += result.grade;
                    numStudents++;
                }
            }
        }
    }

    // Вычисляем среднюю успеваемость
    if (numStudents > 0) {
        return static_cast<float>(totalGrades) / numStudents;
    }
    else {
        return 0.0f;
    }
}

// Функция, которую выполняет каждый поток
void calculateAverageGradeThread(const std::vector<Student>& students, const std::string& group, int semester, float& averageGrade) {
    averageGrade = calculateAverageGrade(students, group, semester);
}

int main() {
    setlocale(LC_ALL, "Rus");
    srand(time(NULL)); // Добавлено для инициализации генератора случайных чисел

    // Задаем данные
    int arraySize = 100;
    int numThreads = 2;
    std::string group = "Г";
    int semester = 2;

    // Создаем и заполняем массив данных о студентах
    std::vector<Student> students(arraySize);
    for (int i = 0; i < arraySize; i++) {
        students[i].fullName = "Студент " + std::to_string(i + 1);
        students[i].groupNumber = group;
        for (int j = 0; j < semester; j++) {
            SessionResult result;
            result.semester = j + 1;
            result.discipline = "Дисциплина " + std::to_string(j + 1);
            result.grade = rand() % 4 + 2; // Случайная отметка от 2 до 5
            students[i].results.push_back(result);
        }
    }

    // Вычисляем среднюю успеваемость без использования многопоточности
    auto start = std::chrono::high_resolution_clock::now();
    float averageGrade = calculateAverageGrade(students, group, semester);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Выводим результаты
    std::cout << "Средний балл для группы " << group << " в семестре " << semester << ": " << averageGrade << std::endl;
    std::cout << "Время без многопоточности: " << duration << " микросекунд" << std::endl;

    // Вычисляем среднюю успеваемость с использованием многопоточности
    start = std::chrono::high_resolution_clock::now();

    // Создаем вектор потоков
    std::vector<std::thread> threads;

    // Создаем вектор для хранения результатов
    std::vector<float> averageGrades(numThreads);

    // Разделяем массив данных на несколько частей и обрабатываем каждую часть в отдельном потоке
    int chunkSize = students.size() / numThreads;
    for (int i = 0; i < numThreads; i++) {
        int startIdx = i * chunkSize;
        int endIdx = (i == numThreads - 1) ? students.size() : startIdx + chunkSize;

        threads.emplace_back(calculateAverageGradeThread, std::ref(students), std::ref(group), semester, std::ref(averageGrades[i]));
    }

    // Ожидаем завершения всех потоков
    for (auto& thread : threads) {
        thread.join();
    }

    // Объединяем результаты
    float totalAverageGrade = 0.0f;
    for (const auto& grade : averageGrades) {
        totalAverageGrade += grade;
    }
    totalAverageGrade /= numThreads;

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Выводим результаты
    std::cout << "Средний балл для группы " << group << " в семестре " << semester << ": " << totalAverageGrade << std::endl;
    std::cout << "Время с многопоточностью: " << duration << " микросекунд" << std::endl;

    return 0;
}
