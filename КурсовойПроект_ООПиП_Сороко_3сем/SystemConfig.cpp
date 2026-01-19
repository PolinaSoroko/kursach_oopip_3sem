#include "SystemConfig.h"
#include "Utilities.h"
#include <iomanip>

using namespace std;

SystemConfig::SystemConfig() {
    loadConfig();
    loadPerformanceScores();
}

double SystemConfig::getCodeQualityWeight() const {
    return codeQualityWeight;
}

double SystemConfig::getTeamworkWeight() const {
    return teamworkWeight;
}

double SystemConfig::getTasksWeight() const {
    return tasksWeight;
}

double SystemConfig::getDesignCreativityWeight() const {
    return designCreativityWeight;
}

double SystemConfig::getMarketingROIWeight() const {
    return marketingROIWeight;
}

double SystemConfig::getSalesConversionWeight() const {
    return salesConversionWeight;
}

double SystemConfig::getSupportSatisfactionWeight() const {
    return supportSatisfactionWeight;
}

double SystemConfig::getQABugDetectionWeight() const {
    return qaBugDetectionWeight;
}

// Получение специализированного веса для отдела
double SystemConfig::getDepartmentWeight(const string& department) const {
    string dept = toLower(department);

    if (dept.find("разработ") != string::npos || dept.find("dev") != string::npos) {
        return codeQualityWeight;
    }
    else if (dept.find("дизайн") != string::npos || dept.find("design") != string::npos) {
        return designCreativityWeight;
    }
    else if (dept.find("маркетинг") != string::npos || dept.find("marketing") != string::npos) {
        return marketingROIWeight;
    }
    else if (dept.find("продаж") != string::npos || dept.find("sales") != string::npos) {
        return salesConversionWeight;
    }
    else if (dept.find("поддержк") != string::npos || dept.find("support") != string::npos) {
        return supportSatisfactionWeight;
    }
    else if (dept.find("qa") != string::npos || dept.find("тестиров") != string::npos) {
        return qaBugDetectionWeight;
    }
    else if (dept == "hr") {
        return getHRWeight(); // Специальный вес для HR
    }
    else {
        return codeQualityWeight; // По умолчанию
    }
}

// Получение названия параметра для отдела
string SystemConfig::getDepartmentParameterName(const string& department) const {
    string dept = toLower(department);

    if (dept.find("разработ") != string::npos || dept.find("dev") != string::npos) {
        return "Качество кода";
    }
    else if (dept.find("дизайн") != string::npos || dept.find("design") != string::npos) {
        return "Креативность";
    }
    else if (dept.find("маркетинг") != string::npos || dept.find("marketing") != string::npos) {
        return "ROI кампаний";
    }
    else if (dept.find("продаж") != string::npos || dept.find("sales") != string::npos) {
        return "Конверсия продаж";
    }
    else if (dept.find("поддержк") != string::npos || dept.find("support") != string::npos) {
        return "Удовлетворенность клиентов";
    }
    else if (dept.find("qa") != string::npos || dept.find("тестиров") != string::npos) {
        return "Обнаружение багов";
    }
    else if (dept == "hr") {
        return "Эффективность найма";
    }
    else {
        return "Специализированный параметр";
    }
}

void SystemConfig::setCodeQualityWeight(double weight) {
    codeQualityWeight = weight;
    saveConfig();
}

void SystemConfig::setTeamworkWeight(double weight) {
    teamworkWeight = weight;
    saveConfig();
}

void SystemConfig::setTasksWeight(double weight) {
    tasksWeight = weight;
    saveConfig();
}

void SystemConfig::setDesignCreativityWeight(double weight) {
    designCreativityWeight = weight;
    saveConfig();
}

void SystemConfig::setMarketingROIWeight(double weight) {
    marketingROIWeight = weight;
    saveConfig();
}

void SystemConfig::setSalesConversionWeight(double weight) {
    salesConversionWeight = weight;
    saveConfig();
}

void SystemConfig::setSupportSatisfactionWeight(double weight) {
    supportSatisfactionWeight = weight;
    saveConfig();
}

void SystemConfig::setQABugDetectionWeight(double weight) {
    qaBugDetectionWeight = weight;
    saveConfig();
}

// Сохранение конфигурации
void SystemConfig::saveConfig() const {
    ofstream fout("config.txt");
    if (fout) {
        fout << codeQualityWeight << "\n";
        fout << teamworkWeight << "\n";
        fout << tasksWeight << "\n";
        fout << designCreativityWeight << "\n";
        fout << marketingROIWeight << "\n";
        fout << salesConversionWeight << "\n";
        fout << supportSatisfactionWeight << "\n";
        fout << qaBugDetectionWeight << "\n";
    }
}

// Загрузка конфигурации
bool SystemConfig::loadConfig() {
    ifstream fin("config.txt");
    if (!fin) {
        // Если файла нет, создаем с значениями по умолчанию
        saveConfig();
        return false;
    }

    fin >> codeQualityWeight;
    fin >> teamworkWeight;
    fin >> tasksWeight;

    // Загружаем дополнительные параметры, если они есть
    if (!fin.eof()) {
        fin >> designCreativityWeight;
    }
    if (!fin.eof()) {
        fin >> marketingROIWeight;
    }
    if (!fin.eof()) {
        fin >> salesConversionWeight;
    }
    if (!fin.eof()) {
        fin >> supportSatisfactionWeight;
    }
    if (!fin.eof()) {
        fin >> qaBugDetectionWeight;
    }

    return true;
}

// Меню настройки (для администратора)
void SystemConfig::configMenu() {
    int choice;
    string input;

    do {
        cout << "\n=== Настройка параметров системы ===\n";
        cout << "=== Общие параметры ===\n";
        cout << "1. Показать все текущие параметры\n";
        cout << "2. Изменить вес командной работы (для всех)\n";
        cout << "3. Изменить вес выполненных задач (для всех)\n";
        cout << "\n=== Специализированные параметры для отделов ===\n";
        cout << "4. Изменить вес качества кода (Разработка)\n";
        cout << "5. Изменить вес креативности (Дизайн)\n";
        cout << "6. Изменить вес ROI кампаний (Маркетинг)\n";
        cout << "7. Изменить вес конверсии продаж (Продажи)\n";
        cout << "8. Изменить вес удовлетворенности клиентов (Поддержка)\n";
        cout << "9. Изменить вес обнаружения багов (QA)\n";
        cout << "\n10. Сохранить изменения\n";
        cout << "0. Выход в главное меню\n";
        cout << "Выберите пункт: ";
        getline(cin, input);

        try {
            choice = stoi(input);
        }
        catch (...) {
            choice = -1;
        }

        switch (choice) {
        case 1:
            showAllParameters();
            break;

        case 2: {
            double newWeight;
            cout << "Текущий вес командной работы: " << teamworkWeight << "\n";
            cout << "Введите новый вес: ";
            getline(cin, input);
            try {
                newWeight = stod(input);
                teamworkWeight = newWeight;
                cout << "Вес командной работы изменен на: " << teamworkWeight << "\n";
            }
            catch (...) {
                cout << "Ошибка: введите корректное число\n";
            }
            break;
        }

        case 3: {
            double newWeight;
            cout << "Текущий вес выполненных задач: " << tasksWeight << "\n";
            cout << "Введите новый вес: ";
            getline(cin, input);
            try {
                newWeight = stod(input);
                tasksWeight = newWeight;
                cout << "Вес выполненных задач изменен на: " << tasksWeight << "\n";
            }
            catch (...) {
                cout << "Ошибка: введите корректное число\n";
            }
            break;
        }

        case 4: {
            double newWeight;
            cout << "Текущий вес качества кода (для разработки): " << codeQualityWeight << "\n";
            cout << "Введите новый вес: ";
            getline(cin, input);
            try {
                newWeight = stod(input);
                codeQualityWeight = newWeight;
                cout << "Вес качества кода изменен на: " << codeQualityWeight << "\n";
            }
            catch (...) {
                cout << "Ошибка: введите корректное число\n";
            }
            break;
        }

        case 5: {
            double newWeight;
            cout << "Текущий вес креативности (для дизайна): " << designCreativityWeight << "\n";
            cout << "Введите новый вес: ";
            getline(cin, input);
            try {
                newWeight = stod(input);
                designCreativityWeight = newWeight;
                cout << "Вес креативности изменен на: " << designCreativityWeight << "\n";
            }
            catch (...) {
                cout << "Ошибка: введите корректное число\n";
            }
            break;
        }

        case 6: {
            double newWeight;
            cout << "Текущий вес ROI кампаний (для маркетинга): " << marketingROIWeight << "\n";
            cout << "Введите новый вес: ";
            getline(cin, input);
            try {
                newWeight = stod(input);
                marketingROIWeight = newWeight;
                cout << "Вес ROI кампаний изменен на: " << marketingROIWeight << "\n";
            }
            catch (...) {
                cout << "Ошибка: введите корректное число\n";
            }
            break;
        }

        case 7: {
            double newWeight;
            cout << "Текущий вес конверсии продаж (для продаж): " << salesConversionWeight << "\n";
            cout << "Введите новый вес: ";
            getline(cin, input);
            try {
                newWeight = stod(input);
                salesConversionWeight = newWeight;
                cout << "Вес конверсии продаж изменен на: " << salesConversionWeight << "\n";
            }
            catch (...) {
                cout << "Ошибка: введите корректное число\n";
            }
            break;
        }

        case 8: {
            double newWeight;
            cout << "Текущий вес удовлетворенности клиентов (для поддержки): " << supportSatisfactionWeight << "\n";
            cout << "Введите новый вес: ";
            getline(cin, input);
            try {
                newWeight = stod(input);
                supportSatisfactionWeight = newWeight;
                cout << "Вес удовлетворенности клиентов изменен на: " << supportSatisfactionWeight << "\n";
            }
            catch (...) {
                cout << "Ошибка: введите корректное число\n";
            }
            break;
        }

        case 9: {
            double newWeight;
            cout << "Текущий вес обнаружения багов (для QA): " << qaBugDetectionWeight << "\n";
            cout << "Введите новый вес: ";
            getline(cin, input);
            try {
                newWeight = stod(input);
                qaBugDetectionWeight = newWeight;
                cout << "Вес обнаружения багов изменен на: " << qaBugDetectionWeight << "\n";
            }
            catch (...) {
                cout << "Ошибка: введите корректное число\n";
            }
            break;
        }

        case 10:
            saveConfig();
            cout << "Настройки сохранены!\n";
            break;

        case 0:
            cout << "Выход из настроек...\n";
            break;

        default:
            cout << "Неверный выбор. Попробуйте снова.\n";
        }

    } while (choice != 0);
}

// Показать все параметры
void SystemConfig::showAllParameters() const {
    cout << "\n=== Текущие параметры системы ===\n";
    cout << "\nОбщие параметры (для всех отделов):\n";
    cout << "Вес командной работы: " << teamworkWeight << "\n";
    cout << "Вес выполненных задач: " << tasksWeight << "\n";

    cout << "\nСпециализированные параметры по отделам:\n";
    cout << "Вес качества кода (Разработка): " << codeQualityWeight << "\n";
    cout << "Вес креативности (Дизайн): " << designCreativityWeight << "\n";
    cout << "Вес ROI кампаний (Маркетинг): " << marketingROIWeight << "\n";
    cout << "Вес конверсии продаж (Продажи): " << salesConversionWeight << "\n";
    cout << "Вес удовлетворенности клиентов (Поддержка): " << supportSatisfactionWeight << "\n";
    cout << "Вес обнаружения багов (QA): " << qaBugDetectionWeight << "\n";

    double total = teamworkWeight + tasksWeight;
    cout << "\nОбщий вес (должен быть 100% для каждого отдела):\n";
    cout << "Разработка: " << (codeQualityWeight + total) << "%\n";
    cout << "Дизайн: " << (designCreativityWeight + total) << "%\n";
    cout << "Маркетинг: " << (marketingROIWeight + total) << "%\n";
    cout << "Продажи: " << (salesConversionWeight + total) << "%\n";
    cout << "Поддержка: " << (supportSatisfactionWeight + total) << "%\n";
    cout << "QA: " << (qaBugDetectionWeight + total) << "%\n";
}

// Проверка корректности весов для конкретного отдела
bool SystemConfig::validateDepartmentWeights(const string& department) const {
    double deptWeight = getDepartmentWeight(department);
    return (deptWeight + teamworkWeight + tasksWeight) == 100.0;
}

// Расчет эффективности с учетом отдела
double SystemConfig::calculatePerformanceForDepartment(const string& department,
    double departmentScore,
    double teamworkScore,
    double tasksScore) const {
    double deptWeight = getDepartmentWeight(department);

    return (departmentScore * deptWeight / 100.0) +
        (teamworkScore * teamworkWeight / 100.0) +
        (tasksScore * tasksWeight / 100.0);
}

// Вспомогательная функция для преобразования в нижний регистр
string SystemConfig::toLower(const string& str) const {
    string result = str;
    transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return tolower(c); });
    return result;
}

// Специальный вес для HR (рассчитывается на основе других параметров)
double SystemConfig::getHRWeight() const {
    // Для HR используем среднее из всех специализированных параметров
    return (codeQualityWeight + designCreativityWeight + marketingROIWeight +
        salesConversionWeight + supportSatisfactionWeight + qaBugDetectionWeight) / 6.0;
}

// Загрузка сохраненных оценок
void SystemConfig::loadPerformanceScores() {
    ifstream fin("performance_scores.txt");
    if (!fin) return;

    string line;
    while (getline(fin, line)) {
        auto parts = split(line, '|');
        if (parts.size() >= 2) {
            string username = trim(parts[0]);
            try {
                double score = stod(trim(parts[1]));
                performance_scores_[username] = score;
            }
            catch (...) {
                // Пропускаем некорректные строки
            }
        }
    }
}

// Сохранение оценок
void SystemConfig::savePerformanceScores() const {
    ofstream fout("performance_scores.txt");
    if (fout) {
        for (const auto& [username, score] : performance_scores_) {
            fout << username << "|" << fixed << setprecision(2) << score << "\n";
        }
    }
}

// Сохранение оценки для конкретного пользователя
void SystemConfig::savePerformanceScore(const string& username, double score) {
    performance_scores_[username] = score;
    savePerformanceScores();
}

// Получение оценки пользователя
double SystemConfig::getPerformanceScore(const string& username) const {
    auto it = performance_scores_.find(username);
    if (it != performance_scores_.end()) {
        return it->second;
    }
    return -1.0; // -1 означает, что оценка не найдена
}

// Проверка наличия оценки
bool SystemConfig::hasPerformanceScore(const string& username) const {
    return performance_scores_.find(username) != performance_scores_.end();
}