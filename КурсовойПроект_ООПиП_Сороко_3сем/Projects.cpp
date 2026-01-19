#include "UserClasses.h"
#include "Projects.h"

#include<iostream>

using namespace std;

// --------------------------- ProjectStore методы ---------------------------

void ProjectStore::load_from_file() {
    load_projects_from_file();
    load_employee_projects_from_file();
}

void ProjectStore::load_projects_from_file() {
    projects_.clear();
    ifstream in(PROJECTS_FILE);
    if (!in.is_open()) {
        cerr << "Предупреждение: не удалось открыть файл проектов: " << PROJECTS_FILE
            << ". Создан новый файл.\n";
        ofstream out(PROJECTS_FILE, ios::app);
        if (!out.is_open()) {
            cerr << "Ошибка: не удалось создать файл проектов: " << PROJECTS_FILE << "\n";
        }
        return;
    }

    string line;
    while (getline(in, line)) {
        if (in.bad() || in.fail()) {
            cerr << "Ошибка чтения файла проектов. Пропущена некорректная строка.\n";
            in.clear();
            continue;
        }
        line = trim(line);
        if (line.empty()) continue;
        auto project = Project::create_from_record(line);
        if (project) {
            projects_.push_back(move(project));
        }
    }

    in.close();
}

void ProjectStore::load_employee_projects_from_file() {
    employee_projects_.clear();
    ifstream in(EMPLOYEE_PROJECTS_FILE);
    if (!in.is_open()) {
        cerr << "Предупреждение: не удалось открыть файл проектов: " << EMPLOYEE_PROJECTS_FILE
            << ". Создан новый файл.\n";
        ofstream out(EMPLOYEE_PROJECTS_FILE, ios::app);
        if (!out.is_open()) {
            cerr << "Ошибка: не удалось создать файл проектов: " << EMPLOYEE_PROJECTS_FILE << "\n";
        }
        return;
    }

    string line;
    while (getline(in, line)) {
        if (in.bad() || in.fail()) {
            cerr << "Ошибка чтения файла проектов. Пропущена некорректная строка.\n";
            in.clear();
            continue;
        }
        line = trim(line);
        if (line.empty()) continue;
        auto emp_proj = EmployeeProject::create_from_record(line);
        if (emp_proj) {
            employee_projects_.push_back(move(emp_proj));
        }
    }  

    in.close();
}

void ProjectStore::save_all_files() {
    save_projects_to_file();
    save_employee_projects_to_file();
}

void ProjectStore::save_projects_to_file() {
    ofstream out(PROJECTS_FILE, ios::trunc);
    if (!out.is_open()) {
        cerr << "Критическая ошибка: не удалось открыть файл проектов для записи: "
            << PROJECTS_FILE << "\n";
        return;
    }

    for (const auto& project : projects_) {
        if (!(out << project->serialize() << "\n")) {
            cerr << "Ошибка записи проекта в файл.\n";
            break;
        }
    }

   
    out.close();  // Явное закрытие потока

    if (!out) {
        cerr << "Ошибка при закрытии файла проектов.\n";
    }
}

void ProjectStore::save_employee_projects_to_file() {
    ofstream out(EMPLOYEE_PROJECTS_FILE, ios::trunc);
    if (!out.is_open()) {
        cerr << "Критическая ошибка: не удалось открыть файл проектов для записи: "
            << EMPLOYEE_PROJECTS_FILE << "\n";
        return;
    }

    for (const auto& emp_proj : employee_projects_) {
        if (!(out << emp_proj->serialize() << "\n")) {
            cerr << "Ошибка записи проекта в файл.\n";
            break;
        }
    }
    
    out.close();  // Явное закрытие потока

    if (!out) {
        cerr << "Ошибка при закрытии файла проектов.\n";
    }
}

bool ProjectStore::add_project(unique_ptr<Project> project) {
    if (!project || project->name().empty()) return false;

    // Проверяем, существует ли уже проект с таким именем
    if (find_project(project->name()) != nullptr) return false;

    projects_.push_back(move(project));
    save_projects_to_file();
    return true;
}

bool ProjectStore::remove_project(const string& project_name) {
    // Удаляем проект
    for (size_t i = 0; i < projects_.size(); ++i) {
        if (projects_[i]->name() == project_name) {
            projects_.erase(projects_.begin() + i);

            // Удаляем все связи сотрудников с этим проектом
            vector<size_t> to_remove;
            for (size_t j = 0; j < employee_projects_.size(); ++j) {
                if (employee_projects_[j]->project_name() == project_name) {
                    to_remove.push_back(j);
                }
            }

            // Удаляем в обратном порядке
            for (auto it = to_remove.rbegin(); it != to_remove.rend(); ++it) {
                employee_projects_.erase(employee_projects_.begin() + *it);
            }

            save_all_files();
            return true;
        }
    }
    return false;
}


bool ProjectStore::update_project(const string& project_name, const string& new_status) {
    return update_project(project_name, new_status, "");
}

bool ProjectStore::update_project(const string& project_name, const string& new_status, const string& new_description) {
    auto project = find_project(project_name);
    if (!project) return false;

    if (!new_status.empty()) {
        project->set_status(new_status);
    }

    if (!new_description.empty()) {
        project->set_description(new_description);
    }

    save_projects_to_file();
    return true;
}
Project* ProjectStore::find_project(const string& project_name) const {
    for (const auto& project : projects_) {
        if (project->name() == project_name) return project.get();
    }
    return nullptr;
}

bool ProjectStore::assign_employee_to_project(const string& username, const string& project_name) {
    // значение по умолчанию
    return assign_employee_to_project(username, project_name, "Участник");
}

bool ProjectStore::assign_employee_to_project(const string& username, const string& project_name, const string& role) {
    auto project = find_project(project_name);
    if (!project) return false;

    auto ep = make_unique<EmployeeProject>(username, project_name, role);
    employee_projects_.push_back(move(ep));

    save_employee_projects_to_file();
    return true;
}


bool ProjectStore::remove_employee_from_project(const string& username, const string& project_name) {
    for (size_t i = 0; i < employee_projects_.size(); ++i) {
        if (employee_projects_[i]->username() == username &&
            employee_projects_[i]->project_name() == project_name) {
            employee_projects_.erase(employee_projects_.begin() + i);
            save_employee_projects_to_file();
            return true;
        }
    }
    return false;
}

bool ProjectStore::update_employee_role(const string& username, const string& project_name, const string& new_role) {
    for (auto& emp_proj : employee_projects_) {
        if (emp_proj->username() == username && emp_proj->project_name() == project_name) {
            emp_proj->set_role(new_role);
            save_employee_projects_to_file();
            return true;
        }
    }
    return false;
}


vector<const Project*> ProjectStore::all_projects() const {
    vector<const Project*> result;
    for (const auto& project : projects_) {
        result.push_back(project.get());
    }
    return result;
}

vector<const EmployeeProject*> ProjectStore::all_employee_projects() const {
    vector<const EmployeeProject*> result;
    for (const auto& emp_proj : employee_projects_) {
        result.push_back(emp_proj.get());
    }
    return result;
}

vector<const Project*> ProjectStore::get_employee_projects(const string& username) const {
    vector<const Project*> result;
    for (const auto& emp_proj : employee_projects_) {
        if (emp_proj->username() == username) {
            auto project = find_project(emp_proj->project_name());
            if (project) {
                result.push_back(project);
            }
        }
    }
    return result;
}

vector<const EmployeeProject*> ProjectStore::get_project_employees(const string& project_name) const {
    vector<const EmployeeProject*> result;
    for (const auto& emp_proj : employee_projects_) {
        if (emp_proj->project_name() == project_name) {
            result.push_back(emp_proj.get());
        }
    }
    return result;
}

// Поиск и фильтрация
vector<const Project*> ProjectStore::search_projects_by_name(const string& keyword) const {
    vector<const Project*> result;
    string keyword_lower = keyword;
    transform(keyword_lower.begin(), keyword_lower.end(), keyword_lower.begin(), ::tolower);

    for (const auto& project : projects_) {
        string name_lower = project->name();
        transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

        if (name_lower.find(keyword_lower) != string::npos) {
            result.push_back(project.get());
        }
    }
    return result;
}

vector<const Project*> ProjectStore::filter_projects_by_status(const string& status) const {
    vector<const Project*> result;
    for (const auto& project : projects_) {
        if (project->status() == status) {
            result.push_back(project.get());
        }
    }
    return result;
}

// Сортировка
vector<const Project*> ProjectStore::get_projects_sorted_by_name(bool ascending) const {
    vector<const Project*> result = all_projects();

    sort(result.begin(), result.end(), [ascending](const Project* a, const Project* b) {
        if (ascending) {
            return a->name() < b->name();
        }
        else {
            return a->name() > b->name();
        }
        });

    return result;
}

vector<const Project*> ProjectStore::get_projects_sorted_by_date(bool ascending) const {
    vector<const Project*> result = all_projects();

    sort(result.begin(), result.end(), [ascending](const Project* a, const Project* b) {
        if (ascending) {
            return a->created_date() < b->created_date();
        }
        else {
            return a->created_date() > b->created_date();
        }
        });

    return result;
}

void Application::hr_add_project() {
    cout << "\n--- Добавление нового проекта ---\n";

    string name = input_line("Название проекта: ");
    if (name.empty()) {
        cout << "Название проекта не может быть пустым.\n";
        return;
    }

    string description = input_line("Описание проекта: ");
    if (description.empty()) {
        cout << "Описание проекта не может быть пустым.\n";
        return;
    }

    string status = "активный";
    cout << "Статус проекта (по умолчанию 'Активный'): ";
    string status_input = input_line("");
    if (!status_input.empty()) {
        status = status_input;
    }

    auto project = make_unique<Project>(name, description, status);
    if (project_store_->add_project(move(project))) {
        cout << "Проект '" << name << "' успешно добавлен.\n";
    }
    else {
        cout << "Ошибка: проект с таким названием уже существует.\n";
    }
}



void Application::hr_view_all_projects() {
    auto projects = project_store_->all_projects();

    if (projects.empty()) {
        cout << "\nНет проектов.\n";
        return;
    }

    cout << "\n--- Все проекты (" << projects.size() << ") ---\n";

    // Определяем ширины колонок
    vector<size_t> widths = { 4, 30, 20, 15, 20 }; // Номер, Название, Описание, Статус, Дата создания
    vector<string> headers = { "№", "Название проекта", "Описание", "Статус", "Дата создания" };

    print_table_header(headers, widths);

    for (size_t i = 0; i < projects.size(); ++i) {
        const auto& project = projects[i];
        vector<string> row = {
            to_string(i + 1),
            project->name(),
            project->description(),
            project->status(),
            project->created_date()
        };
        print_table_row(row, widths);
    }

    size_t total_width = 0;
    for (size_t w : widths) {
        total_width += w + 2;
    }
    total_width += 1;
    print_horizontal_line(total_width);
}



void Application::hr_assign_employee_to_project() {
    cout << "\n--- Назначение сотрудника на проект ---\n";

    auto employees = store_->get_all_employees();
    if (employees.empty()) {
        cout << "Нет сотрудников для назначения.\n";
        return;
    }

    cout << "Список сотрудников:\n";
    for (size_t i = 0; i < employees.size(); ++i) {
        cout << (i + 1) << ") " << employees[i]->fullname() << "\n";
    }

    int emp_choice = input_int("Выберите сотрудника: ");
    if (emp_choice < 1 || emp_choice > static_cast<int>(employees.size())) {
        cout << "Неверный выбор.\n";
        return;
    }

    const string username = employees[emp_choice - 1]->username();

    auto projects = project_store_->all_projects();
    if (projects.empty()) {
        cout << "Нет проектов.\n";
        return;
    }

    cout << "Список проектов:\n";
    for (size_t i = 0; i < projects.size(); ++i) {
        cout << (i + 1) << ") " << projects[i]->name() << "\n";
    }

    int proj_choice = input_int("Выберите проект: ");
    if (proj_choice < 1 || proj_choice > static_cast<int>(projects.size())) {
        cout << "Неверный выбор.\n";
        return;
    }

    const string project_name = projects[proj_choice - 1]->name();

    string role = input_line("Роль сотрудника в проекте: ");
    if (role.empty()) {
        role = "Участник";
    }

    if (project_store_->assign_employee_to_project(username, project_name, role)) {
        cout << "Сотрудник " << username << " успешно назначен на проект '"
            << project_name << "' с ролью '" << role << "'.\n";
    }
    else {
        cout << "Ошибка: сотрудник уже назначен на этот проект или проект не существует.\n";
    }
}

void Application::hr_view_project_details() {
    cout << "\n--- Детали проекта ---\n";

    auto projects = project_store_->all_projects();
    if (projects.empty()) {
        cout << "Нет проектов.\n";
        return;
    }

    cout << "Список проектов:\n";
    for (size_t i = 0; i < projects.size(); ++i) {
        cout << (i + 1) << ") " << projects[i]->name() << "\n";
    }

    int choice = input_int("Выберите проект для просмотра: ");
    if (choice < 1 || choice > static_cast<int>(projects.size())) {
        cout << "Неверный выбор.\n";
        return;
    }

    const auto& project = projects[choice - 1];

    cout << "\n--- Информация о проекте ---\n";
    cout << "Название: " << project->name() << "\n";
    cout << "Описание: " << project->description() << "\n";
    cout << "Статус: " << project->status() << "\n";
    cout << "Дата создания: " << project->created_date() << "\n";

    // Показать сотрудников на проекте
    auto project_employees = project_store_->get_project_employees(project->name());
    if (!project_employees.empty()) {
        cout << "\nСотрудники на проекте (" << project_employees.size() << "):\n";

        // Определяем ширины колонок для таблицы сотрудников
        vector<size_t> widths = { 20, 15, 20, 12 }; // ФИО, Логин, Роль, Дата назначения
        vector<string> headers = { "ФИО", "Логин", "Роль", "Дата назначения" };

        print_table_header(headers, widths);

        for (const auto& emp_proj : project_employees) {
            User* user = store_->find_by_username(emp_proj->username());
            if (user) {
                vector<string> row = {
                    user->fullname(),
                    user->username(),
                    emp_proj->role(),
                    emp_proj->assigned_date()
                };
                print_table_row(row, widths);
            }
        }

        size_t total_width = 0;
        for (size_t w : widths) {
            total_width += w + 2;
        }
        total_width += 1;
        print_horizontal_line(total_width);
    }
    else {
        cout << "\nНа проекте нет сотрудников.\n";
    }
}

void Application::hr_edit_project() {
    cout << "\n--- Редактирование проекта ---\n";

    auto projects = project_store_->all_projects();
    if (projects.empty()) {
        cout << "Нет проектов.\n";
        return;
    }
    do {
        cout << "Список проектов:\n";
        for (size_t i = 0; i < projects.size(); ++i) {
            cout << (i + 1) << ") " << projects[i]->name() << "\n";
        }

        int choice = input_int("Выберите проект для редактирования (0-выйти): ");
        if (choice < 1 || choice > static_cast<int>(projects.size())) {
            cout << "Неверный выбор.\n";
            return;
        }

        const string project_name = projects[choice - 1]->name();
        Project* project = project_store_->find_project(project_name);

        cout << "\nЧто вы хотите изменить?\n";
        cout << "1) Статус проекта\n";
        cout << "2) Описание проекта\n";
        cout << "3) Роль сотрудника на проекте\n";

        int edit_choice = input_int("Ваш выбор: ");

        switch (edit_choice) {
        case 1: {
            cout << "Текущий статус: " << project->status() << "\n";
            string new_status = input_line("Новый статус («активный», «завершенный», «приостановленный», «планируется»): ");

            // Создаем список допустимых статусов
            vector<string> valid_statuses = { "активный", "завершенный", "приостановленный", "планируется" };

            // Проверяем корректность статуса
            bool valid_status = false;
            for (const auto& status : valid_statuses) {
                if (new_status == status) {
                    valid_status = true;
                    break;
                }
            }

            if (!valid_status && !new_status.empty()) {
                cout << "Предупреждение: статус '" << new_status << "' не является стандартным.\n";
                cout << "Стандартные статусы: ";
                for (size_t i = 0; i < valid_statuses.size(); ++i) {
                    if (i > 0) cout << ", ";
                    cout << valid_statuses[i];
                }
                cout << "\n";

                cout << "Продолжить с пользовательским статусом? (y/n): ";
                string confirm = input_line("");
                if (confirm != "y" && confirm != "Y") {
                    cout << "Изменение статуса отменено.\n";
                    break;
                }
            }

            project->set_status(new_status);
            project_store_->save_projects_to_file();
            cout << "Статус проекта обновлен.\n";
            break;
        }
        case 2: {
            cout << "Текущее описание:\n";
            cout << project->description() << "\n\n";

            string new_desc = input_line("Новое описание (нажмите Enter для отмены): ");
            if (new_desc.empty()) {
                cout << "Изменение описания отменено.\n";
                break;
            }

            if (new_desc.length() < 10) {
                cout << "Описание слишком короткое (минимум 10 символов).\n";
                cout << "Продолжить? (y/n): ";
                string confirm = input_line("");
                if (confirm != "y" && confirm != "Y") {
                    break;
                }
            }

            project->set_description(new_desc);
            project_store_->save_projects_to_file();
            cout << "Описание проекта обновлено.\n";
            break;
        }
        case 3: {
            // Изменение роли сотрудника (существующий код)
            auto project_employees = project_store_->get_project_employees(project_name);
            if (project_employees.empty()) {
                cout << "На проекте нет сотрудников.\n";
                break;
            }

            cout << "Сотрудники на проекте:\n";
            for (size_t i = 0; i < project_employees.size(); ++i) {
                User* user = store_->find_by_username(project_employees[i]->username());
                if (user) {
                    cout << (i + 1) << ") " << user->fullname()
                        << " - Текущая роль: " << project_employees[i]->role() << "\n";
                }
            }

            int emp_choice = input_int("Выберите сотрудника: ");
            if (emp_choice < 1 || emp_choice > static_cast<int>(project_employees.size())) {
                cout << "Неверный выбор.\n";
                break;
            }

            const string username = project_employees[emp_choice - 1]->username();
            cout << "Доступные роли: Участник (по умолчанию), Руководитель, Разработчик, Тестировщик, Аналитик, Дизайнер\n";
            string new_role = input_line("Новая роль: ");

            if (new_role.empty()) {
                new_role = "участник";
            }

            if (project_store_->update_employee_role(username, project_name, new_role)) {
                cout << "Роль сотрудника обновлена.\n";
            }
            else {
                cout << "Ошибка обновления роли.\n";
            }
            break;
        }
        default:
            cout << "Неверный выбор.\n";
        }
    } while (true);
   
}

void Application::hr_delete_project() {
    cout << "\n--- Удаление проекта ---\n";

    auto projects = project_store_->all_projects();
    if (projects.empty()) {
        cout << "Нет проектов.\n";
        return;
    }

    cout << "Список проектов:\n";
    for (size_t i = 0; i < projects.size(); ++i) {
        cout << (i + 1) << ") " << projects[i]->name() << "\n";
    }

    int choice = input_int("Выберите проект для удаления: ");
    if (choice < 1 || choice > static_cast<int>(projects.size())) {
        cout << "Неверный выбор.\n";
        return;
    }

    const string project_name = projects[choice - 1]->name();

    cout << "Вы уверены, что хотите удалить проект '" << project_name << "'? (y/n): ";
    string confirm = input_line("");
    if (confirm == "y" || confirm == "Y") {
        if (project_store_->remove_project(project_name)) {
            cout << "Проект удален.\n";
        }
        else {
            cout << "Ошибка удаления проекта.\n";
        }
    }
    else {
        cout << "Удаление отменено.\n";
    }
}

void Application::hr_search_projects() {
    cout << "\n--- Поиск проектов ---\n";
    string keyword = input_line("Введите ключевое слово для поиска: ");

    auto results = project_store_->search_projects_by_name(keyword);

    if (results.empty()) {
        cout << "\nПроекты не найдены.\n";
    }
    else {
        cout << "\n--- Результаты поиска (" << results.size() << ") ---\n";

        // Определяем ширины колонок
        vector<size_t> widths = { 30, 20, 15, 20 }; // Название, Описание, Статус, Дата
        vector<string> headers = { "Название проекта", "Описание", "Статус", "Дата создания" };

        print_table_header(headers, widths);

        for (const auto& project : results) {
            vector<string> row = {
                project->name(),
                project->description(),
                project->status(),
                project->created_date()
            };
            print_table_row(row, widths);
        }

        size_t total_width = 0;
        for (size_t w : widths) {
            total_width += w + 2;
        }
        total_width += 1;
        print_horizontal_line(total_width);
    }
}

void Application::hr_filter_projects_by_status() {
    cout << "\n--- Фильтр проектов по статусу ---\n";
    cout << "Доступные статусы: Активный, Завершенный, Приостановленный\n";
    string status = input_line("Введите статус для фильтрации: ");

    auto results = project_store_->filter_projects_by_status(status);

    if (results.empty()) {
        cout << "\nПроекты с статусом '" << status << "' не найдены.\n";
    }
    else {
        cout << "\n--- Проекты со статусом '" << status << "' (" << results.size() << ") ---\n";

        // Определяем ширины колонок
        vector<size_t> widths = { 30, 25, 20 }; // Название, Описание, Дата
        vector<string> headers = { "Название проекта", "Описание", "Дата создания" };

        print_table_header(headers, widths);

        for (const auto& project : results) {
            vector<string> row = {
                project->name(),
                project->description(),
                project->created_date()
            };
            print_table_row(row, widths);
        }

        size_t total_width = 0;
        for (size_t w : widths) {
            total_width += w + 2;
        }
        total_width += 1;
        print_horizontal_line(total_width);
    }
}

void Application::hr_sort_projects() {
    cout << "\n--- Сортировка проектов ---\n";
    cout << "1) По названию (А-Я)\n";
    cout << "2) По названию (Я-А)\n";
    cout << "3) По дате создания (старые сначала)\n";
    cout << "4) По дате создания (новые сначала)\n";

    int choice = input_int("Выберите тип сортировки: ");

    vector<const Project*> sorted_projects;
    string title;

    switch (choice) {
    case 1:
        sorted_projects = project_store_->get_projects_sorted_by_name(true);
        title = "Проекты отсортированные по названию (А-Я)";
        break;
    case 2:
        sorted_projects = project_store_->get_projects_sorted_by_name(false);
        title = "Проекты отсортированные по названию (Я-А)";
        break;
    case 3:
        sorted_projects = project_store_->get_projects_sorted_by_date(true);
        title = "Проекты отсортированные по дате создания (старые сначала)";
        break;
    case 4:
        sorted_projects = project_store_->get_projects_sorted_by_date(false);
        title = "Проекты отсортированные по дате создания (новые сначала)";
        break;
    default:
        cout << "Неверный выбор.\n";
        return;
    }

    if (sorted_projects.empty()) {
        cout << "\nНет проектов.\n";
        return;
    }

    cout << "\n--- " << title << " (" << sorted_projects.size() << ") ---\n";

    // Определяем ширины колонок
    vector<size_t> widths = { 30, 20, 15, 20 }; // Название, Описание, Статус, Дата
    vector<string> headers = { "Название проекта", "Описание", "Статус", "Дата создания" };

    print_table_header(headers, widths);

    for (const auto& project : sorted_projects) {
        vector<string> row = {
            project->name(),
            project->description(),
            project->status(),
            project->created_date()
        };
        print_table_row(row, widths);
    }

    size_t total_width = 0;
    for (size_t w : widths) {
        total_width += w + 2;
    }
    total_width += 1;
    print_horizontal_line(total_width);
}