#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>
#include <cctype>

using namespace std;

class SystemConfig {
private:
    double codeQualityWeight = 30.0;           // Вес для разработчиков
    double teamworkWeight = 30.0;              // Вес командной работы (общий)
    double tasksWeight = 40.0;                 // Вес выполненных задач (общий)

    // Дополнительные параметры для разных отделов
    double designCreativityWeight = 30.0;      // Вес креативности для дизайнеров
    double marketingROIWeight = 30.0;          // Вес ROI для маркетологов
    double salesConversionWeight = 30.0;       // Вес конверсии для продавцов
    double supportSatisfactionWeight = 30.0;   // Вес удовлетворенности для поддержки
    double qaBugDetectionWeight = 30.0;        // Вес обнаружения багов для QA
    map<string, double> performance_scores_;

public:
    SystemConfig();

    // Геттеры для общих параметров
    double getCodeQualityWeight() const;
    double getTeamworkWeight() const;
    double getTasksWeight() const;

    // Геттеры для специализированных параметров
    double getDesignCreativityWeight() const;
    double getMarketingROIWeight() const;
    double getSalesConversionWeight() const;
    double getSupportSatisfactionWeight() const;
    double getQABugDetectionWeight() const;

    // Получение специализированного веса для отдела
    double getDepartmentWeight(const std::string& department) const;

    // Получение названия параметра для отдела
    string getDepartmentParameterName(const std::string& department) const;

    // Сеттеры
    void setCodeQualityWeight(double weight);
    void setTeamworkWeight(double weight);
    void setTasksWeight(double weight);
    void setDesignCreativityWeight(double weight);
    void setMarketingROIWeight(double weight);
    void setSalesConversionWeight(double weight);
    void setSupportSatisfactionWeight(double weight);
    void setQABugDetectionWeight(double weight);

    // Сохранение и загрузка конфигурации
    void saveConfig() const;
    bool loadConfig();

    // Меню настройки (для администратора)
    void configMenu();

    // Показать все параметры
    void showAllParameters() const;

    // Проверка корректности весов для конкретного отдела
    bool validateDepartmentWeights(const std::string& department) const;

    // Расчет эффективности с учетом отдела
    double calculatePerformanceForDepartment(const std::string& department,
        double departmentScore,
        double teamworkScore,
        double tasksScore) const;
    // Методы для работы с оценками эффективности
    void savePerformanceScore(const string& username, double score);
    double getPerformanceScore(const string& username) const;
    bool hasPerformanceScore(const string& username) const;
    void loadPerformanceScores();
    void savePerformanceScores() const;

private:
    // Вспомогательная функция для преобразования в нижний регистр
    string toLower(const std::string& str) const;

    // Специальный вес для HR (рассчитывается на основе других параметров)
    double getHRWeight() const;
};
namespace HRSystem {
    using ::SystemConfig;
}