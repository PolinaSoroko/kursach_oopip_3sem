#include <iostream>
#include <windows.h>
#include <algorithm> 
#include <cctype>
#include <iomanip> 
#include "UserClasses.h"
#include "Projects.h" 
#include <functional>
#include <conio.h>
#include <locale>
#include <codecvt>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

// --------------------------- Функции для работы с паролями ---------------------------

// Функция для безопасного ввода пароля (со звездочками)
string input_password(const string& prompt) {
    cout << prompt;
    string password;

    char ch;
    while ((ch = _getch()) != '\r') { 
        if (ch == '\b') { 
            if (!password.empty()) {
                password.pop_back();
                cout << "\b \b";
            }
        }
        else if (ch >= 32 && ch <= 126) { 
            password.push_back(ch);
            cout << '*';
        }
    }


    cout << endl;
    return password;
}

// Простая функция хеширования пароля
string hash_password(const string& password) {
    size_t hashed = std::hash<string>{}(password);
    return to_string(hashed);
}


bool verify_password(const string& input_password, const string& stored_hash) {
    string input_hash = hash_password(input_password);
    return input_hash == stored_hash;
}

void User::view_profile() const {
    cout << "---- Профиль ----\n";
    cout << "ФИО: " << fullname_ << "\n";
    cout << "Отдел: " << department_ << "\n";
    cout << "Роль: " << role_to_string(role_) << "\n";
}

ostream& operator<<(ostream& os, const User& u) {
    return os << u.serialize();
}
ostream& operator<<(ostream& os, const EmployeeProject& e) {
    return os << e.serialize();
}
ostream& operator<<(ostream& os, const Project& p) {
    return os << p.serialize();
}

void Admin::admin_assign_hr_role(const string& username) {}

void HRManagerUser::hr_add_employee() {}

void HRManagerUser::hr_add_project() {}

void HRManagerUser::hr_calc_performance(const string& username) {
    cout << "[HR] Расчет эффективности для сотрудника '" << username << "'\n";
}

void HRManagerUser::hr_generate_report(const string& username) {
    cout << "[HR] Генерация отчета для сотрудника '" << username << "'\n";
    cout << "Используйте пункт меню 'Сгенерировать отчет сотрудника'\n";
}

void HRManagerUser::employee_view_reports() const {}

void EmployeeUser::employee_view_reports() const {}

void EmployeeUser::employee_view_rating() const {
    cout << "[EMPLOYEE] Просмотр моего рейтинга\n";
    cout << "Используйте пункт меню 'Посмотреть мой рейтинг'\n";
}

// --------------------------- Хранилище пользователей ---------------------------

// Загружает пользователей из файла в память
void UserStore::load_from_file() {
    users_.clear();
    hr_users_.clear();
    admin_user_.reset();

    ifstream in(USERS_FILE);
    if (in) {
        string line;
        while (getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            auto u = UserFactory::create_user_from_record(line);
            if (u && u->role() != Role::HR && u->role() != Role::ADMIN) {
                users_.push_back(move(u));
            }
        }
    }

    load_hr_from_file();

    load_admin_from_file();
}

void UserStore::load_hr_from_file() {
    ifstream in(HR_USERS_FILE);
    if (!in) {
        ofstream out(HR_USERS_FILE, ios::app);
        return;
    }

    string line;
    while (getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        auto parts = split(line, '|');
        if (parts.size() >= 5) {
            auto u = make_unique<HRManagerUser>(
                trim(parts[0]),
                trim(parts[1]), 
                trim(parts[2]), 
                trim(parts[3])  
            );
            hr_users_.push_back(move(u));
        }
    }
}

void UserStore::save_to_file() {
    ofstream out(USERS_FILE, ios::trunc);
    if (!out) {
        cerr << "Unable to open users file for writing: " << USERS_FILE << "\n";
        return;
    }

    for (const auto& uptr : users_) {
        if (uptr->role() != Role::HR) {
            out << uptr->serialize() << "\n";
        }
    }
}

User* UserStore::find_by_username(const string& username) {
   
    if (admin_user_ && admin_user_->username() == username) {
        return admin_user_.get();
    }

    for (auto& hr : hr_users_) {
        if (hr->username() == username) return hr.get();
    }

    for (auto& up : users_) {
        if (up->username() == username) return up.get();
    }

    return nullptr;
}

// Добавление нового пользователя (в память и запись в файл)
bool UserStore::add_user(unique_ptr<User> user) {
    if (!user) return false;
    if (find_by_username(user->username()) != nullptr) return false;

    if (user->role() == Role::HR) {
        auto hr_user = make_unique<HRManagerUser>(
            user->username(),
            user->password(),
            user->fullname(),
            user->department()
        );
        hr_users_.push_back(move(hr_user));
    }
    else {
        users_.push_back(move(user));
    }

    save_all_files();
    return true;
}

// Удалить пользователя по логину
bool UserStore::remove_user_by_username(const string& username) {
    for (size_t i = 0; i < users_.size(); ++i) {
        if (users_[i]->username() == username) {
            users_.erase(users_.begin() + i);
            save_to_file();
            return true;
        }
    }
    return false;
}

bool UserStore::remove_hr_user_by_username(const string& username) {
    for (size_t i = 0; i < hr_users_.size(); ++i) {
        if (hr_users_[i]->username() == username) {
            hr_users_.erase(hr_users_.begin() + i);
            save_hr_to_file();
            return true;
        }
    }
    return false;
}

bool UserStore::update_user(unique_ptr<User> updated) {
    if (!updated) return false;
    for (size_t i = 0; i < users_.size(); ++i) {
        if (users_[i]->username() == updated->username()) {
            users_[i] = move(updated);
            save_to_file();
            return true;
        }
    }
    return false;
}

bool UserStore::update_employee(const string & username, const string & new_fullname, const string & new_department) {
    for (auto& user : users_) {
        if (user->username() == username) {
            user->set_fullname(new_fullname);
            user->set_department(new_department);
            save_to_file();
            return true;
        }
    }
    return false;
}

vector<const User*> UserStore::get_all_employees() const {
    vector<const User*> result;

    for (const auto& user : users_) {
        if (user->role() == Role::EMPLOYEE || user->role() == Role::PENDING) {
            result.push_back(user.get());
        }
    }

    return result;
}

vector<const User*> UserStore::get_pending_users() const {
    vector<const User*> result;
    for (const auto& user : users_) {
        if (user->role() == Role::PENDING) {
            result.push_back(user.get());
        }
    }
    return result;
}

User* UserStore::get_employee(const string& username) const {
    for (const auto& user : users_) {
        if ((user->role() == Role::EMPLOYEE || user->role() == Role::PENDING) &&
            user->username() == username) {
            return user.get();
        }
    }
    return nullptr;
}

vector<const User*> UserStore::search_employees_by_name(const string& keyword) const {
    vector<const User*> result;
    string keyword_lower = keyword;
    transform(keyword_lower.begin(), keyword_lower.end(), keyword_lower.begin(), ::tolower);

    for (const auto& user : users_) {
        if (user->role() == Role::EMPLOYEE || user->role() == Role::PENDING) {
            string fullname_lower = user->fullname();
            transform(fullname_lower.begin(), fullname_lower.end(), fullname_lower.begin(), ::tolower);

            string username_lower = user->username();
            transform(username_lower.begin(), username_lower.end(), username_lower.begin(), ::tolower);

            if (fullname_lower.find(keyword_lower) != string::npos ||
                username_lower.find(keyword_lower) != string::npos) {
                result.push_back(user.get());
            }
        }
    }
    return result;
}

// Сортировка сотрудников
vector<const User*> UserStore::get_employees_sorted_by_name(bool ascending) const {
    vector<const User*> result = get_all_employees();

    sort(result.begin(), result.end(), [ascending](const User* a, const User* b) {
        if (ascending) {
            return a->fullname() < b->fullname();
        }
        else {
            return a->fullname() > b->fullname();
        }
        });

    return result;
}

// Метод для получения всех обычных пользователей
vector<User*> UserStore::get_employees() const {
    vector<User*> employees;

    for (const auto& user : users_) {
        if (user->role() == Role::EMPLOYEE || user->role() == Role::PENDING) {
            employees.push_back(user.get());
        }
    }
    return employees;
}

// Получить всех сотрудников отсортированных по рейтингу
vector<pair<const User*, double>> UserStore::get_employees_sorted_by_rating(SystemConfig* config) const {
    vector<pair<const User*, double>> employees_with_ratings;

    for (const auto& user : users_) {
        if (user->role() == Role::EMPLOYEE || user->role() == Role::PENDING) {
            double rating = config->getPerformanceScore(user->username());
            if (rating >= 0) {
                rating = round(rating * 100) / 100;
            }
            employees_with_ratings.push_back({ user.get(), rating });
        }
    }

    sort(employees_with_ratings.begin(), employees_with_ratings.end(),
        [](const pair<const User*, double>& a, const pair<const User*, double>& b) {

            if (a.second >= 0 && b.second >= 0) {
                if (a.second != b.second) {
                    return a.second > b.second;
                }
               
                return a.first->fullname() < b.first->fullname();
            }
            else if (a.second >= 0) {
                return true;
            }

            else if (b.second >= 0) {
                return false;
            }

            return a.first->fullname() < b.first->fullname();
        });

    return employees_with_ratings;
}

// Получить место сотрудника в рейтинге
pair<int, int> UserStore::get_employee_rank(const string& username, SystemConfig* config) const {
    auto sorted_employees = get_employees_sorted_by_rating(config);

    int total_employees = sorted_employees.size();
    int position = -1;

    for (size_t i = 0; i < sorted_employees.size(); ++i) {
        if (sorted_employees[i].first->username() == username) {
            position = static_cast<int>(i) + 1;
            break;
        }
    }

    return { position, total_employees };
}

void UserStore::load_admin_from_file() {
    admin_user_.reset();

    ifstream in(ADMIN_USERS_FILE);
    if (!in) {
        cout << "\nФайл администратора не найден. Создание администратора по умолчанию...\n";
        create_default_admin();
        return;
    }

    in.seekg(0, ios::end);
    if (in.tellg() == 0) {
        in.close();
        cout << "\nФайл администратора пустой. Создание администратора по умолчанию...\n";
        create_default_admin();
        return;
    }

    in.seekg(0, ios::beg);

    string line;
    if (getline(in, line)) {
        line = trim(line);
        if (!line.empty()) {
            auto parts = split(line, '|');
            if (parts.size() >= 5) {
                admin_user_ = make_unique<Admin>(
                    trim(parts[0]),  
                    trim(parts[1]),
                    trim(parts[2]) 
                );
                return;
            }
        }
    }

    cout << "\nФайл администратора поврежден. Создание администратора по умолчанию...\n";
    create_default_admin();
}

void UserStore::create_default_admin() {
    // Создаем администратора с дефолтным хешированным паролем "admin123"
    string default_password = "admin123";
    string password_hash = hash_password(default_password);

    admin_user_ = make_unique<Admin>("admin", password_hash, "Системный администратор");

    cout << "\n" << string(50, '=') << "\n";
    cout << "Внимание: создан администратор по умолчанию!\n\n";
    cout << "Логин: admin\n";
    cout << "Пароль: admin123\n";
    cout << "Роль: Системный администратор\n\n";
    cout << "Рекомендуется сменить пароль после первого входа!\n";
    cout << string(50, '=') << "\n\n";

    save_admin_to_file();
}

void UserStore::save_admin_to_file() {
    ofstream out(ADMIN_USERS_FILE, ios::trunc);
    if (!out) {
        cerr << "Unable to open admin users file for writing: " << ADMIN_USERS_FILE << "\n";
        return;
    }

    if (admin_user_) {
        out << admin_user_->serialize() << "\n";
    }
}

bool UserStore::move_user_to_hr(const string& username) {
    for (size_t i = 0; i < users_.size(); ++i) {
        if (users_[i]->username() == username && users_[i]->role() == Role::PENDING) {
            // Создаем HR версию пользователя
            auto hr_user = make_unique<HRManagerUser>(
                users_[i]->username(),
                users_[i]->password(),
                users_[i]->fullname(),
                users_[i]->department()
            );

            // Добавляем в HR контейнер
            hr_users_.push_back(move(hr_user));

            // Удаляем из обычных пользователей
            users_.erase(users_.begin() + i);

            // Сохраняем изменения в файлах
            save_all_files();
            return true;
        }
    }
    return false;
}

bool UserStore::is_hr_user(const string& username) const {
    for (const auto& hr : hr_users_) {
        if (hr->username() == username) return true;
    }
    return false;
}

void UserStore::save_hr_to_file() {
    ofstream out(HR_USERS_FILE, ios::trunc);
    if (!out) {
        cerr << "Unable to open HR users file for writing: " << HR_USERS_FILE << "\n";
        return;
    }

    for (const auto& hr : hr_users_) {
        out << hr->serialize() << "\n";
    }
}
void UserStore::save_all_files() {
    save_to_file();      
    save_hr_to_file();    
    save_admin_to_file(); 
}

vector<const User*> UserStore::all_users() const {
    vector<const User*> result;

    if (admin_user_) {
        result.push_back(admin_user_.get());
    }

    for (const auto& user : users_) {
        result.push_back(user.get());
    }
    for (const auto& hr : hr_users_) {
        result.push_back(hr.get());
    }

    return result;
}
bool UserStore::is_admin_user(const string& username) const {
    return admin_user_ && admin_user_->username() == username;
}

// --------------------------- Интерфейс консоли и рабочий поток ---------------------------

void Application::run() {
    
    bool running = true;
    while (running) {
        system("cls");
        show_main_menu();
        int choice = input_int("Выберите: ");
        bool has_admin = (store_->find_by_username("admin") != nullptr);
        switch (choice) {
        case 1:
            handle_login();
            break;
        case 2:
            handle_register();
            break;
        
        case 0:
            cout << "Выход... До свидания\n";
            running = false;
            break;
        default:
            cout << "Неверный выбор.\n";
        }
    }
    
}

void Application::show_main_menu() {
    cout << "\n====== АИС HR-менеджмента для IT-предприятия ======\n";
    cout << "1) Войти\n";
    cout << "2) Зарегистрироваться\n";
    cout << "0) Выход\n";
}

//------------------------------------ Корректность ввода (в Application)---------------------------------------
int Application::input_int(const string& prompt) {
    while (true) {
        cout << prompt;
        string s;
        getline(cin, s);
        try {
            int v = stoi(s);
            return v;
        }
        catch (...) {
            cout << "Пожалуйста, введите число...\n";
        }
    }
}

string Application::input_line(const string& prompt, bool allow_empty) {
    string s;
    bool hasLetter, hasDigit, hasSymbol;

    while (true) {
        cout << prompt;
        if (!getline(cin, s)) {    
            cin.clear(); 
            cin.ignore(10, '\n');
            cout << "\033[31mОшибка ввода! Повторите попытку.\033[0m\n";
            continue;
        }

        s = trim(s);
        // Разрешаем пустую строку если allow_empty = true
        if (allow_empty && s.empty()) {
            return s;
        }
        toLower(s);
        hasLetter = false;
        hasDigit = false;
        hasSymbol = false;

        for (char ch : s) {
            if (isalpha(ch) || isCyrillic(ch)) hasLetter = true;
            else if (isdigit(ch)) hasDigit = true;
            else if (ispunct(ch)) hasSymbol = true;
        }

        if (s.empty()) {
            cout << "\033[31mОшибка: пустая строка!\033[0m\n";
            continue;
        }

        if (!hasLetter) {
            cout << "\033[31mОшибка: строка должна содержать хотя бы одну букву!\033[0m\n";
            continue;
        }

        return s;
    }
}


// ----------- Login flow -----------

void Application::handle_login() {
    string login = input_line("Логин: ");
    if (login.empty()) {
        cout << "Вы ввели пустой логин, возврат в главное меню...\n";
        return;
    }

    User* user = store_->find_by_username(login);
    if (!user) {
        cout << "Пользователь не найден. Хотите зарегистрироваться? (y/n): ";
        string ans; getline(cin, ans);
        if (!ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) {
            handle_register_with_login(login);
        }
        else {
            cout << "Возврат в главное меню...\n";
        }
        return;
    }

    bool ok = attempt_password(user->password());
    if (!ok) {
        cout << "Слишком много неудачных попыток. Возврат в главное меню...\n";
        return;
    }

    Role r = user->role();
    if (r == Role::ADMIN) {
        Admin admin(user->username(), user->password(), user->fullname());
        admin_session(admin);
    }
    else if (r == Role::HR) {
        HRManagerUser hr(user->username(), user->password(), user->fullname(), user->department());
        hr_session(hr);
    }
    else if (r == Role::PENDING) {
        cout << "Ваша учетная запись находится в стадии рассмотрения. для получения прав HR требуется одобрение администратора.\n";
        EmployeeUser emp(user->username(), user->password(), user->fullname(), user->department());
        employee_session(emp);
    }
    else {
        EmployeeUser emp(user->username(), user->password(), user->fullname(), user->department());
        employee_session(emp);
    }
}

// ----------- Registration flow -----------

bool Application::attempt_password(const string& true_password_hash) {
    const int MAX_ATTEMPTS = 3;
    for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {
        string pw = input_password("Пароль: ");

        if (verify_password(pw, true_password_hash)) {
            cout << "Аутентификация прошла успешно (" << now_string() << ")\n";
            return true;
        }
        else {
            cout << "Неверный пароль. Осталось попыток: " << (MAX_ATTEMPTS - attempt) << "\n";
        }
    }
    return false;
}
bool UserStore::is_password_already_used(const string& password_hash) const {
    if (admin_user_ && admin_user_->password() == password_hash) {
        return true;
    }

    for (const auto& hr : hr_users_) {
        if (hr->password() == password_hash) {
            return true;
        }
    }

    for (const auto& user : users_) {
        if (user->password() == password_hash) {
            return true;
        }
    }

    return false;
}

void Application::handle_register() {
    string login = input_line("Логин: ");
    if (login.empty()) {
        cout << "Вы ввели пустой логин, возврат в главное меню...\n";
        return;
    }
    handle_register_with_login(login);
}

void Application::handle_register_with_login(const string& desired_login) {
    string login = desired_login;
    while (store_->find_by_username(login) != nullptr) {
        cout << "Логин '" << login << "' уже существует. Введите другой логин или введите 'q' для отмены: ";
        string alt; getline(cin, alt);
        alt = trim(alt);
        if (alt.empty() || alt == "q" || alt == "Q") {
            cout << "Регистрация отменена. Возврат в главное меню...\n";
            return;
        }
        login = alt;
    }

    // Проверка на зарезервированные логины 
    if (login == "admin" || login == "administrator" || login == "root") {
        cout << "Данный логин зарезервирован системой. Выберите другой логин.\n";
        return handle_register(); 
    }

    string password;
    while (true) {
        password = input_password("Пароль: ");
        if (password.empty()) {
            cout << "Пароль не может быть пустым.\n";
            continue;
        }
        if (password.length() < 6) {
            cout << "Пароль должен содержать минимум 6 символов.\n";
            continue;
        }
        // Хешируем пароль 
        string password_hash = hash_password(password);

        // Проверяем, используется ли уже такой пароль
        if (store_->is_password_already_used(password_hash)) {
            cout << "\033[31mОшибка: этот пароль уже используется другим пользователем!\033[0m\n";
            cout << "Пожалуйста, придумайте другой пароль.\n";
            continue;
        }

        string password2 = input_password("Подтвердите пароль: ");
        if (password != password2) {
            cout << "Пароли не совпадают. Попробуйте заново.\n";
            continue;
        }
        break;
    }

    string fullname = input_line("Ваше ФИО: ");
    string department = choose_department();

    Role role = Role::EMPLOYEE;
    if (!department.empty()) {
        string dep_upper = department;
        transform(dep_upper.begin(), dep_upper.end(), dep_upper.begin(), ::toupper);
        if (dep_upper == "HR") {
            role = Role::PENDING;
        }
    }

    string password_hash = hash_password(password);

    auto u_ptr = UserFactory::create_user(login, password_hash, fullname, department, role);
    if (!u_ptr) {
        cout << "Ошибка создания пользователя\n";
        return;
    }

    if (store_->add_user(move(u_ptr))) {
        cout << "Регистрация прошла успешно, теперь вы можете войти в систему.\n";
        store_->load_from_file();
        if (role == Role::PENDING) {
            cout << "Для подтверждения роли HR требуется одобрение администратора.\n";
        }
       
        cout << "\nХотите войти в систему сейчас? (y/n): ";
        string answer;
        getline(cin, answer);

        if (!answer.empty() && (answer[0] == 'y' || answer[0] == 'Y')) {
            
            bool ok = attempt_password(password_hash); 

            if (ok) {
                Role r = role;
                if (r == Role::ADMIN) {
                    Admin admin(login, password_hash, fullname);
                    admin_session(admin);
                }
                else if (r == Role::HR) {
                    HRManagerUser hr(login, password_hash, fullname, department);
                    hr_session(hr);
                }
                else if (r == Role::PENDING) {
                    cout << "Ваша учетная запись находится в стадии рассмотрения.\n";
                    EmployeeUser emp(login, password_hash, fullname, department);
                    employee_session(emp);
                }
                else {
                    EmployeeUser emp(login, password_hash, fullname, department);
                    employee_session(emp);
                }
            }
        }
    }

    
    else {
        cout << "Регистрация не удалась (возможно логин занят), возвращаемся в главное меню.\n";
    }
}

string Application::choose_department() {
    vector<string> depts = {
        "HR", "Разработка", "QA", "Дизайн", "Маркетинг", "Продажи", "Поддержка"
    };
    cout << "Выберите отдел:\n";
    for (size_t i = 0; i < depts.size(); ++i) {
        cout << (i + 1) << ") " << depts[i] << "\n";
    }

    int sel = input_int("Ваш выбор: ");
    if (sel >= 1 && static_cast<size_t>(sel) <= depts.size()) return depts[sel - 1];
    else {
        do {
            cout << "Неверный выбор. Введите корректное число." << endl;
            sel = input_int("Ваш выбор: ");
        } while (sel < 1 || static_cast<size_t>(sel) > depts.size());
        return depts[sel - 1];
    }
}

// ----------- Сессии пользователей (меню после входа) -----------
void Application::admin_session(Admin& admin) {
    
    bool in_session = true;
    while (in_session) {
        system("cls");
        cout << "\n--- Меню Администратора ---\n";
        cout << "1) Просмотреть всех пользователей\n";
        cout << "2) Назначить роль HR ожидающему пользователю\n";
        cout << "3) Удалить пользователя\n";
        cout << "4) Настроить параметры оценки\n";
        cout << "5) Выход\n";
        int cmd = input_int("Ваш выбор: ");
        switch (cmd) {
        case 1:
            list_all_users();
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        case 2:
            assign_hr_role_flow();
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        case 3: {
            string uname = input_line("Введите логин пользователя для удаления: ");

            if (uname == admin.username()) {
                cout << "Нельзя удалить текущего администратора.\n";
                break;
            }

            if (store_->remove_user_by_username(uname)) {
                cout << "Пользователь удален успешно. Данные в файлах обновлены.\n";

                auto emp_projects = project_store_->all_employee_projects();
                for (const auto& emp_proj : emp_projects) {
                    if (emp_proj->username() == uname) {
                        project_store_->remove_employee_from_project(uname, emp_proj->project_name());
                    }
                }
            }
            
            if (store_->remove_hr_user_by_username(uname)) {
                cout << "Пользователь удален успешно. Данные в файлах обновлены.\n";

                
            }
            else {
                cout << "Пользователь не найден.\n";
            }
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        }
        case 4:
            admin_configure_system();
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        case 5:
            in_session = false;
            cout << "Администратор вышел из системы...";
            
            Sleep(1000);
            
            break;
        default:
            cout << "Неверный выбор.\n";
            Sleep(1000);
        }
    }
    
}

// Добавляем метод для настройки системы (для администратора)
void Application::admin_configure_system() {
    cout << "\n=== Настройка параметров системы ===\n";
    system_config_->configMenu();
}


// Реализация расчета эффективности с учетом отдела сотрудника
void Application::hr_calculate_performance() {
    cout << "\n=== РАСЧЕТ ЭФФЕКТИВНОСТИ СОТРУДНИКА ===\n";

    auto employees = store_->get_all_employees();

    if (employees.empty()) {
        cout << "Нет сотрудников для расчета.\n";
        return;
    }

    cout << "\n--- ВЫБЕРИТЕ СОТРУДНИКА ---\n\n";

    const int NUM_WIDTH = 5;
    const int FULLNAME_WIDTH = 35;
    const int DEPARTMENT_WIDTH = 20;
    const int RATING_WIDTH = 15;

    cout << left
        << setw(NUM_WIDTH) << "№"
        << setw(FULLNAME_WIDTH) << "ФИО"
        << setw(DEPARTMENT_WIDTH) << "ОТДЕЛ"
        << setw(RATING_WIDTH) << "ТЕКУЩАЯ ОЦЕНКА"
        << "\n";

    int total_width = NUM_WIDTH + FULLNAME_WIDTH + DEPARTMENT_WIDTH + RATING_WIDTH;
    cout << string(total_width, '=') << "\n";

    for (size_t i = 0; i < employees.size(); ++i) {
        const auto& emp = employees[i];

        double current_score = system_config_->getPerformanceScore(emp->username());
        string rating_str;

        if (current_score >= 0) {
            rating_str = to_string(current_score);
  
            size_t dot_pos = rating_str.find('.');
            if (dot_pos != string::npos && rating_str.length() > dot_pos + 3) {
                rating_str = rating_str.substr(0, dot_pos + 3);
            }
            rating_str += "/100";
        }
        else {
            rating_str = "НЕ ОЦЕНЕН";
        }

        string fullname = emp->fullname();
        if (fullname.length() > FULLNAME_WIDTH - 2) {
            fullname = fullname.substr(0, FULLNAME_WIDTH - 5) + "...";
        }

        string department = emp->department();
        if (department.length() > DEPARTMENT_WIDTH - 2) {
            department = department.substr(0, DEPARTMENT_WIDTH - 5) + "...";
        }

        cout << left
            << setw(NUM_WIDTH) << (i + 1)
            << setw(FULLNAME_WIDTH) << fullname
            << setw(DEPARTMENT_WIDTH) << department
            << setw(RATING_WIDTH) << rating_str
            << "\n";
    }

    cout << "\n";

    int choice;
    while (true) {
        cout << "Введите номер сотрудника для расчета (0 - отмена): ";
        string input;
        getline(cin, input);

        try {
            choice = stoi(input);

            if (choice == 0) {
                cout << "Отмена расчета.\n";
                return;
            }
            if (choice >= 1 && choice <= static_cast<int>(employees.size())) {
                break;
            }
            else {
                cout << "Неверный номер. Пожалуйста, выберите номер из списка (1-"
                    << employees.size() << ").\n";
            }
        }
        catch (const invalid_argument&) {
            cout << "Ошибка: введите корректный номер (целое число).\n";
        }
        catch (const out_of_range&) {
            cout << "Ошибка: введенное число слишком большое.\n";
        }
    }

    const User* selected_employee = employees[choice - 1];
    string username = selected_employee->username();
    User* employee = store_->get_employee(username);

    if (!employee) {
        cout << "Ошибка: сотрудник не найден.\n";
        return;
    }

    cout << "\n";
    printHorizontalLine('=', 80);
    cout << "|" << centerAlign("ВЫБРАН СОТРУДНИК: " + employee->fullname(), 78) << "|\n";
    cout << "|" << centerAlign("ЛОГИН: " + username + " | ОТДЕЛ: " + employee->department(), 78) << "|\n";
    printHorizontalLine('=', 80);

    // ------------------ РАСЧЕТ ЭФФЕКТИВНОСТИ ------------------

    string department = employee->department();
    string deptParamName = system_config_->getDepartmentParameterName(department);

    cout << "\nСпециализированный параметр: " << deptParamName << "\n\n";

    // 1. Получаем информацию о проектах сотрудника
    auto projects = project_store_->get_employee_projects(username);
    if (projects.empty()) {
        cout << "Сотрудник не участвует в проектах.\n";
        cout << "Эффективность: Н/Д (недостаточно данных)\n";
        return;
    }

    cout << "=== Анализ проектов ===\n";
    int total_projects = projects.size();
    int active_projects = 0;
    int completed_projects = 0;
    int leadership_count = 0;

    for (const auto& project : projects) {
        if (project->status() == "Активный" || project->status() == "активный") {
            active_projects++;
        }
        else if (project->status() == "Завершенный" || project->status() == "завершенный") {
            completed_projects++;
        }

        // Проверяем лидерскую роль
        auto all_emp_projects = project_store_->all_employee_projects();
        for (const auto& emp_proj : all_emp_projects) {
            if (emp_proj->username() == username &&
                emp_proj->project_name() == project->name()) {
                string role = emp_proj->role();
                if (role == "Руководитель" || role == "руководитель" ||
                    role == "Менеджер" || role == "менеджер" ||
                    role == "Ведущий разработчик" || role == "ведущий разработчик") {
                    leadership_count++;
                }
                break;
            }
        }
    }

    cout << "Всего проектов: " << total_projects << "\n";
    cout << "Активных проектов: " << active_projects << "\n";
    cout << "Завершенных проектов: " << completed_projects << "\n";
    cout << "Проектов с руководящей ролью: " << leadership_count << "\n\n";

    // 2. Запрашиваем данные для расчета с учетом отдела
    cout << "=== Введите данные для расчета ===\n";

    double departmentScore = 0;
    while (true) {  
        try {
            cout << "Оценка " << deptParamName << " (0-100): ";
            string input;
            getline(cin, input);
            departmentScore = stod(input);
            if (departmentScore >= 0 && departmentScore <= 100) {
                break;
            }
            else {
                cout << "Ошибка: оценка должна быть от 0 до 100\n";
            }
        }
        catch (const invalid_argument&) {
            cout << "Ошибка: введите корректное число\n";
        }
        catch (const out_of_range&) {
            cout << "Ошибка: введенное число слишком большое\n";
        }
        catch (...) {
            cout << "Ошибка: введите число\n";
        }
    }

    // Примеры подсказок для разных отделов
    string departmentLower = department;
    transform(departmentLower.begin(), departmentLower.end(), departmentLower.begin(),
        [](unsigned char c) { return tolower(c); });

    if (departmentLower.find("разработ") != string::npos || departmentLower.find("dev") != string::npos) {
        cout << "Подсказка: качество кода оценивается по:\n";
        cout << "- Количество критических багов\n";
        cout << "- Соблюдение стандартов кодирования\n";
        cout << "- Эффективность и читаемость кода\n";
    }
    else if (departmentLower.find("дизайн") != string::npos || departmentLower.find("design") != string::npos) {
        cout << "Подсказка: креативность оценивается по:\n";
        cout << "- Уникальность дизайн-решений\n";
        cout << "- Соответствие трендам\n";
        cout << "- Визуальная привлекательность\n";
    }
    else if (departmentLower.find("маркетинг") != string::npos || departmentLower.find("marketing") != string::npos) {
        cout << "Подсказка: ROI кампаний оценивается по:\n";
        cout << "- Возврат на инвестиции\n";
        cout << "- Эффективность рекламных кампаний\n";
        cout << "- Привлечение новых клиентов\n";
    }
    else if (departmentLower.find("продаж") != string::npos || departmentLower.find("sales") != string::npos) {
        cout << "Подсказка: конверсия продаж оценивается по:\n";
        cout << "- Процент успешных сделок\n";
        cout << "- Объем продаж\n";
        cout << "- Удержание клиентов\n";
    }
    else if (departmentLower.find("поддерж") != string::npos || departmentLower.find("support") != string::npos) {
        cout << "Подсказка: удовлетворенность клиентов оценивается по:\n";
        cout << "- Оценки клиентов после обращений\n";
        cout << "- Скорость решения проблем\n";
        cout << "- Профессионализм общения\n";
    }
    else if (departmentLower.find("qa") != string::npos ||
        departmentLower.find("тестиров") != string::npos) {
        cout << "Подсказка: обнаружение багов оценивается по:\n";
        cout << "- Количество найденных критических багов\n";
        cout << "- Эффективность тест-кейсов\n";
        cout << "- Качество баг-репортов\n";
    }

    cout << "\n";

    double teamworkScore = 0;
    while (true) {
        try {
            cout << "Оценка командной работы (0-100): ";
            string input;
            getline(cin, input);
            teamworkScore = stod(input);

            if (teamworkScore >= 0 && teamworkScore <= 100) {
                break;
            }
            else {
                cout << "Ошибка: оценка должна быть от 0 до 100\n";
            }
        }
        catch (const invalid_argument&) {
            cout << "Ошибка: введите корректное число\n";
        }
        catch (const out_of_range&) {
            cout << "Ошибка: введенное число слишком большое\n";
        }
        catch (...) {
            cout << "Ошибка: введите число\n";
        }

    }

    // 3. Рассчитываем оценку выполненных задач на основе проектов
    double tasksScore = 0;
    if (total_projects > 0) {
        double completionRate = (static_cast<double>(completed_projects) / total_projects) * 100;
        double leadershipBonus = leadership_count * 10;
        double activityBonus = (static_cast<double>(active_projects) / total_projects) * 20;
        tasksScore = min(100.0, completionRate + leadershipBonus + activityBonus);
    }

    cout << "\n=== Коэффициенты для отдела '" << department << "' ===\n";
    cout << "Вес " << deptParamName << ": " << system_config_->getDepartmentWeight(department) << "%\n";
    cout << "Вес командной работы: " << system_config_->getTeamworkWeight() << "%\n";
    cout << "Вес выполненных задач: " << system_config_->getTasksWeight() << "%\n";

    if (!system_config_->validateDepartmentWeights(department)) {
        cout << "Внимание: сумма весов для отдела '" << department << "' не равна 100%!\n";
        cout << "Рекомендуется настроить параметры в меню администратора.\n";
    }

    cout << "\n=== Результаты расчета ===\n";

    // Округляем до сотых для вывода
    cout << deptParamName << ": " << fixed << setprecision(2) << departmentScore << "/100\n";
    cout << "Оценка командной работы: " << fixed << setprecision(2) << teamworkScore << "/100\n";
    cout << "Оценка выполненных задач: " << fixed << setprecision(2) << tasksScore << "/100 (на основе "
        << total_projects << " проектов)\n";

    // 4. Рассчитываем итоговую эффективность с учетом отдела
    double finalScore = system_config_->calculatePerformanceForDepartment(
        department,
        departmentScore,
        teamworkScore,
        tasksScore
    );

    // Округляем итоговую оценку до сотых
    finalScore = round(finalScore * 100) / 100;

    cout << "\n=== ИТОГОВАЯ ЭФФЕКТИВНОСТЬ ===\n";
    cout << "Общий балл: " << fixed << setprecision(2) << finalScore << "/100\n";
    system_config_->savePerformanceScore(username, finalScore);
    cout << "Оценка сохранена в системе.\n";

    // Определяем уровень с учетом отдела (используем округленное значение)
    string level;
    string recommendations;

    if (finalScore >= 90) {
        level = "ЭКСПЕРТ";
        recommendations = "Кандидат на повышение/премию";
    }
    else if (finalScore >= 75) {
        level = "ОПЫТНЫЙ СПЕЦИАЛИСТ";
        recommendations = "Стабильно высокие показатели";
    }
    else if (finalScore >= 60) {
        level = "КОМПЕТЕНТНЫЙ СОТРУДНИК";
        recommendations = "Соответствует ожиданиям";
    }
    else if (finalScore >= 40) {
        level = "НАЧИНАЮЩИЙ СПЕЦИАЛИСТ";
        recommendations = "Требуется наставничество";
    }
    else {
        level = "ТРЕБУЕТСЯ ПОВЫШЕНИЕ КВАЛИФИКАЦИИ";
        recommendations = "Необходим план развития";
    }

    cout << "Уровень: " << level << "\n";
    cout << "Рекомендации: " << recommendations << "\n";

    cout << resetiosflags(ios_base::floatfield);
}

void Application::hr_session(HRManagerUser& hr) {
    
    bool in_session = true;
    while (in_session) {
        system("cls");
        cout << "\n--- Меню HR-менеджера ---\n";
        cout << "1) Просмотреть мой профиль\n";
        cout << "2) Управление проектами\n";
        cout << "3) Управление сотрудниками\n";
        
        cout << "0) Выход\n";
        int cmd = input_int("Ваш выбор: ");
        switch (cmd) {
        case 1:
            hr.view_profile();
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        case 2:
            hr_manage_projects(hr);
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        case 3:
            hr_manage_employees(hr);
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        
        
        
        case 0:
            in_session = false;
            cout << "HR-менеджер вышел из системы...\n";
            Sleep(1000);
            break;
        default:
            cout << "Неверный выбор.\n";
            Sleep(1000);
        }
    }
}

void Application::hr_manage_projects(HRManagerUser& hr) {
    system("cls");
    bool in_menu = true;
    while (in_menu) {
        cout << "\n--- Управление проектами ---\n";
        cout << "1) Добавить новый проект\n";
        cout << "2) Просмотреть все проекты\n";
        cout << "3) Назначить сотрудника на проект\n";
        cout << "4) Просмотреть детали проекта\n";
        cout << "5) Редактировать проект\n";
        cout << "6) Удалить проект\n";
        cout << "7) Поиск проектов\n";
        cout << "8) Фильтр проектов по статусу\n";
        cout << "9) Сортировка проектов\n";
        cout << "0) Вернуться в меню HR\n";

        int choice = input_int("Выберите действие: ");
        switch (choice) {
        case 1:
            hr_add_project();
            break;
        case 2:
            hr_view_all_projects();
            break;
        case 3:
            hr_assign_employee_to_project();
            break;
        case 4:
            hr_view_project_details();
            break;
        case 5:
            hr_edit_project();
            break;
        case 6:
            hr_delete_project();
            break;
        case 7:
            hr_search_projects();
            break;
        case 8:
            hr_filter_projects_by_status();
            break;
        case 9:
            hr_sort_projects();
            break;
        case 0:
            in_menu = false;
            break;
        default:
            cout << "Неверный выбор.\n";
        }
    }
}

void Application::employee_session(EmployeeUser& emp) {
    
    bool in_session = true;
    while (in_session) {
        system("cls");
        cout << "\n--- Меню сотрудника ---\n";
        cout << "1) Посмотреть мой профиль\n";
        cout << "2) Посмотреть мои проекты\n";
        cout << "3) Посмотреть мой отчет\n";
        cout << "4) Посмотреть мою оценку эффективности\n";
        cout << "5) Посмотреть мой рейтинг\n";
        cout << "6) Выйти\n";
        int cmd = input_int("Ваш выбор: ");
        switch (cmd) {
        case 1:
            emp.view_profile();
            employee_view_my_projects(emp);
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        case 2:
            employee_view_my_projects(emp);
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        case 3:
            hr_view_saved_report(emp.username());
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        case 4:
            show_employee_performance_score(emp.username());
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        case 5:
            show_my_rating(emp.username());
            cout << "Нажмите Enter для продолжения...";
            cin.ignore();
            break;
        
        case 6:
            in_session = false;
            cout << "Сотрудник вышел из системы...\n";
            Sleep(1000);
            break;
        default:
            cout << "Неверный выбор.\n";
            Sleep(1000);
        }
    }
}

void Application::show_employee_performance_score(const string& username) {
    cout << "\n=== ВАША ОЦЕНКА ЭФФЕКТИВНОСТИ ===\n";

    if (system_config_->hasPerformanceScore(username)) {
        double score = system_config_->getPerformanceScore(username);
        cout << "Текущая оценка: " << fixed << setprecision(1) << score << "/100\n";

        // Определяем уровень
        string level;
        if (score >= 90) level = "ОТЛИЧНО";
        else if (score >= 75) level = "ХОРОШО";
        else if (score >= 60) level = "УДОВЛЕТВОРИТЕЛЬНО";
        else if (score >= 40) level = "ТРЕБУЕТСЯ УЛУЧШЕНИЕ";
        else level = "НЕДОСТАТОЧНО";

        cout << "Уровень: " << level << "\n";
        cout << "Оценка обновлена: " << now_string() << "\n";
    }
    else {
        cout << "Оценка эффективности еще не рассчитана HR-менеджером.\n";
        cout << "Обратитесь к HR для проведения оценки.\n";
    }
}

// Добавим метод для просмотра сохраненного отчета:
void Application::hr_view_saved_report(const string& username) {

    vector<string> report_files;
    string search_pattern = "HR_REPORT_" + username + "_*.txt";

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(search_pattern.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        cout << "\n=== ОТЧЕТ НЕ НАЙДЕН ===\n";
        cout << "Для вас еще не создан отчет HR-менеджером.\n";
        cout << "Отчет будет доступен после его генерации HR-менеджером.\n";
        return;
    }

    string latest_file;
    SYSTEMTIME latest_time = { 0 };

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            string filename = findFileData.cFileName;
            report_files.push_back(filename);
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);

    if (report_files.empty()) {
        cout << "\n=== ОТЧЕТ НЕ НАЙДЕН ===\n";
        cout << "Для вас еще не создан отчет HR-менеджером.\n";
        return;
    }
    string latest_report = report_files[0];
    for (const auto& file : report_files) {
        if (file > latest_report) {
            latest_report = file;
        }
    }

    cout << "\n";
    printHorizontalLine('=', 80);
    cout << "|" << centerAlign("ВАШ ОТЧЕТ HR: " + latest_report, 78) << "|\n";
    printHorizontalLine('=', 80);

    ifstream report_file(latest_report);
    if (report_file) {
        string line;
        int line_count = 0;
        while (getline(report_file, line) && line_count < 100) {
            cout << line << "\n";
            line_count++;
        }
        if (line_count >= 100) {
            cout << string(80, '.') << "\n";
            cout << "Показаны первые 100 строк отчета.\n";
        }
        printHorizontalLine('=', 80);
        report_file.close();
    }
    else {
        cout << "Ошибка открытия файла отчета.\n";
    }
}

void Application::employee_view_my_projects(EmployeeUser& emp) {
    auto projects = project_store_->get_employee_projects(emp.username());

    if (projects.empty()) {
        cout << "Вы не участвуете ни в одном проекте.\n";
        return;
    }

    cout << "\n--- Ваши проекты (" << projects.size() << ") ---\n";
    for (size_t i = 0; i < projects.size(); ++i) {
        const auto& project = projects[i];
        string role = "Участник";
        auto all_emp_projects = project_store_->all_employee_projects();
        for (const auto& emp_proj : all_emp_projects) {
            if (emp_proj->username() == emp.username() &&
                emp_proj->project_name() == project->name()) {
                role = emp_proj->role();
                break;
            }
        }

        cout << (i + 1) << ") " << project->name()
            << "\n   Описание: " << project->description()
            << "\n   Статус: " << project->status()
            << "\n   Ваша роль: " << role
            << "\n   Дата создания проекта: " << project->created_date() << "\n\n";
    }
}

void Application::show_employees_sorted_by_name() {
    cout << "\n=== СПИСОК СОТРУДНИКОВ (сортировка по ФИО) ===\n\n";
    auto employees = store_->get_employees_sorted_by_name(true);

    if (employees.empty()) {
        cout << "Сотрудников не найдено.\n";
        return;
    }

    cout << left << setw(30) << "ФИО"
        << setw(25) << "Должность"
        << setw(15) << "Отдел"
        << setw(15) << "Дата найма" << "\n";

    cout << string(105, '=') << "\n";

    for (const auto& employee : employees) {
        cout << left << setw(30) << (employee->fullname().length() > 29 ?
            employee->fullname().substr(0, 27) + ".." : employee->fullname())
            << setw(25) << employee->department()
            << setw(15) << employee->department()
            << setw(15) << now_string() << "\n";
    }
    cout << "\nВсего сотрудников: " << employees.size() << "\n";
}

// Показать рейтинг сотрудников по эффективности
void Application::show_employees_rating() {
    cout << "\n=== РЕЙТИНГ СОТРУДНИКОВ ПО ЭФФЕКТИВНОСТИ ===\n\n";

    auto employees_with_ratings = store_->get_employees_sorted_by_rating(system_config_.get());

    if (employees_with_ratings.empty()) {
        cout << "Сотрудников не найдено.\n";
        return;
    }
    
    cout << left << setw(5) << "№"
        << setw(30) << "ФИО"
        << setw(15) << "Отдел"
        << setw(15) << "Оценка"
        << setw(20) << "Статус" << "\n";

    cout << string(95, '=') << "\n";

    int rated_count = 0;
    double total_rating = 0;
    double max_rating = 0;

    for (size_t i = 0; i < employees_with_ratings.size(); ++i) {
        const auto& [employee, rating] = employees_with_ratings[i];

        string status;
        
        if (rating >= 0) {
            rated_count++;
            total_rating += rating;
            if (rating > max_rating) max_rating = rating;

            if (rating >= 90) status = "ЭКСПЕРТ";
            else if (rating >= 75) status = "ОПЫТНЫЙ";
            else if (rating >= 60) status = "КОМПЕТЕНТНЫЙ";
            else if (rating >= 40) status = "НАЧИНАЮЩИЙ";
            else status = "ТРЕБУЕТ РАЗВИТИЯ";
            cout << left << setw(5) << (i + 1)
                << setw(30) << (employee->fullname().length() > 29 ?
                    employee->fullname().substr(0, 27) + ".." : employee->fullname())
                
                << setw(15) << employee->department()
                << setw(15) << fixed << setprecision(2) << rating
                << setw(20) << status << "\n";
        }
        else {
            status = "НЕ ОЦЕНЕН";
            cout << left << setw(5) << (i + 1)
                << setw(30) << (employee->fullname().length() > 29 ?
                    employee->fullname().substr(0, 27) + ".." : employee->fullname())
                
                << setw(15) << employee->department()
                << setw(15) << "0.0"
                << setw(20) << status << "\n";
        }
        
    }

    // Статистика
    cout << "\n=== СТАТИСТИКА ===\n";
    cout << "Всего сотрудников: " << employees_with_ratings.size() << "\n";
    cout << "Оценено: " << rated_count << "\n";
    cout << "Не оценено: " << (employees_with_ratings.size() - rated_count) << "\n";

    if (rated_count > 0) {
        double avg_rating = total_rating / rated_count;
        cout << fixed << setprecision(2);
        cout << "Средняя оценка: " << avg_rating << "\n";
        cout << "Максимальная оценка: " << max_rating << "\n";

        if (rated_count > 1) {
            cout << "Лучшие 10%: " << (max_rating * 0.9) << "+ баллов\n";
            cout << "Лучшие 25%: " << (max_rating * 0.75) << "+ баллов\n";
            cout << "Средние 50%: " << (avg_rating) << "± баллов\n";
        }
    }

    cout << resetiosflags(ios_base::floatfield);
}

// Показать мой рейтинг
void Application::show_my_rating(const string& username) {
    User* user = store_->find_by_username(username);
    if (!user) {
        cout << "Ошибка: пользователь не найден.\n";
        return;
    }

    cout << "\n=== МОЙ РЕЙТИНГ ===\n\n";

    auto [position, total] = store_->get_employee_rank(username, system_config_.get());
    double rating = system_config_->getPerformanceScore(username);

    cout << "Сотрудник: " << user->fullname() << "\n";
    cout << "Логин: " << user->username() << "\n";
    cout << "Отдел: " << user->department() << "\n";

    if (rating >= 0) {
        cout << fixed << setprecision(2);
        cout << "\nОценка эффективности: " << rating << "/100\n";

        if (position > 0) {
            double percentile = 100.0 - ((static_cast<double>(position - 1) / total) * 100);

            cout << "Место в рейтинге: " << position << "/" << total << "\n";
            cout << "Процент: " << fixed << setprecision(1) << percentile << "%\n";

            string level;
            if (rating >= 90) level = "ЭКСПЕРТ";
            else if (rating >= 75) level = "ОПЫТНЫЙ СПЕЦИАЛИСТ";
            else if (rating >= 60) level = "КОМПЕТЕНТНЫЙ СОТРУДНИК";
            else if (rating >= 40) level = "НАЧИНАЮЩИЙ СПЕЦИАЛИСТ";
            else level = "ТРЕБУЕТСЯ ПОВЫШЕНИЕ КВАЛИФИКАЦИИ";

            cout << "Уровень: " << level << "\n";

            if (position <= 1) {
                cout << "Вы занимаете ПЕРВОЕ место в рейтинге! Поздравляем!\n";
            }
            else if (position <= 3) {
                cout << "Вы входите в ТОП-3 сотрудников! Отличный результат!\n";
            }
            else if (position <= total * 0.1) {
                cout << "Вы входите в ТОП-10% сотрудников компании!\n";
            }
            else if (position <= total * 0.25) {
                cout << "Вы входите в четверть лучших сотрудников!\n";
            }
            else if (position <= total * 0.5) {
                cout << "Ваши показатели выше среднего.\n";
            }
            else {
                cout << "Есть потенциал для роста. Продолжайте работать над собой!\n";
            }
        }
    }
    else {
        cout << "\nВаша эффективность еще не рассчитана.\n";
        cout << "Обратитесь к другому HR-менеджеру для проведения оценки.\n";
    }

    

    cout << resetiosflags(ios_base::floatfield);
}

// ----------- Административные утилиты -----------
void Application::list_all_users() {
    auto all_users = store_->all_users();

    if (all_users.empty()) {
        cout << "\nПользователи не найдены.\n";
        return;
    }

    cout << "\n=== СПИСОК ПОЛЬЗОВАТЕЛЕЙ (" << all_users.size() << ") ===\n\n";

    const int NUM_WIDTH = 5;
    const int USERNAME_WIDTH = 20;
    const int FULLNAME_WIDTH = 25;
    const int DEPARTMENT_WIDTH = 20;
    const int ROLE_WIDTH = 15;

    cout << left
        << setw(NUM_WIDTH) << "№"
        << setw(USERNAME_WIDTH) << "ЛОГИН"
        << setw(FULLNAME_WIDTH) << "ФИО"
        << setw(DEPARTMENT_WIDTH) << "ОТДЕЛ"
        << setw(ROLE_WIDTH) << "РОЛЬ"
        << "\n";

    int total_width = NUM_WIDTH + USERNAME_WIDTH + FULLNAME_WIDTH + DEPARTMENT_WIDTH + ROLE_WIDTH;
    cout << string(total_width, '=') << "\n";

    for (size_t i = 0; i < all_users.size(); ++i) {
        const auto& u = all_users[i];

        string username = u->username();
        if (username.length() > USERNAME_WIDTH - 2) {
            username = username.substr(0, USERNAME_WIDTH - 5) + "...";
        }

        string fullname = u->fullname();
        if (fullname.length() > FULLNAME_WIDTH - 2) {
            fullname = fullname.substr(0, FULLNAME_WIDTH - 5) + "...";
        }

        string department = u->department();
        if (department.length() > DEPARTMENT_WIDTH - 2) {
            department = department.substr(0, DEPARTMENT_WIDTH - 5) + "...";
        }

        string role_str = role_to_string(u->role());

        cout << left
            << setw(NUM_WIDTH) << (i + 1)
            << setw(USERNAME_WIDTH) << username
            << setw(FULLNAME_WIDTH) << fullname
            << setw(DEPARTMENT_WIDTH) << department
            << setw(ROLE_WIDTH) << role_str
            << "\n";
    }

    cout << "\n";
}

void Application::assign_hr_role_flow() {

    vector<User*> pending;
    const auto& users = store_->users();

    for (const auto& user : users) {
        if (user->role() == Role::PENDING) {
            pending.push_back(user.get());
        }
    }

    if (pending.empty()) {
        cout << "Новых запросов на подтверждение роли HR нет.\n";
        return;
    }

    cout << "Запросы на подтверждение роли HR:\n";
    for (size_t i = 0; i < pending.size(); ++i) {
        cout << (i + 1) << ") " << pending[i]->username()
            << " (" << pending[i]->fullname() << ")\n";
    }

    int sel = input_int("Выберите пользователя для одобрения (0 - отмена): ");
    if (sel <= 0 || static_cast<size_t>(sel) > pending.size()) {
        cout << "Отменено\n";
        return;
    }

    User* target = pending[sel - 1];
    const string target_username = target->username();

    if (store_->move_user_to_hr(target_username)) {
        cout << "Пользователь " << target_username
            << " теперь HR. Данные перемещены в отдельный файл.\n";
    }
    else {
        cout << "Ошибка перемещения пользователя в HR.\n";
    }
}


// ----------- Управление сотрудниками -----------
void Application::hr_manage_employees(HRManagerUser& hr) {
    system("cls");
    bool in_menu = true;
    while (in_menu) {
        cout << "\n--- Управление сотрудниками ---\n";
        cout << "1) Добавить нового сотрудника\n";
        cout << "2) Просмотреть всех сотрудников\n";
        cout << "3) Просмотреть детали сотрудника\n";
        cout << "4) Редактировать данные сотрудника\n";
        cout << "5) Удалить сотрудника\n";
        cout << "6) Поиск сотрудников\n";
        cout << "7) Сортировка сотрудников по ФИО\n"; 
		cout << "8) Назначить сотрудника на проект\n"; 
        cout << "9) Рассчитать эффективность сотрудника\n";
        cout << "10) Рейтинг сотрудников по эффективности\n"; 
        cout << "11) Сгенерировать отчет сотрудника\n"; 
        cout << "0) Вернуться в меню HR\n";

        int choice = input_int("Выберите действие: ");
        switch (choice) {
        case 1:
            hr_add_employee();
            break;
        case 2:
            hr_view_all_employees();
            break;
        case 3:
            hr_view_employee_details();
            break;
        case 4:
            hr_edit_employee();
            break;
        case 5:
            hr_delete_employee();
            break;
        case 6:
            hr_search_employees();
            break;
        case 7:
            show_employees_sorted_by_name();
            break;
        case 8:
            hr_assign_employee_to_project();
			break;
        case 9: {
            hr_calculate_performance();
            break;
        }
        case 10: {
            show_employees_rating();
            break;
        }

        case 11:
            hr_generate_report();
            break;
        case 0:
            in_menu = false;
            break;
        default:
            cout << "Неверный выбор.\n";
        }
    }
}

void Application::hr_add_employee() {
    cout << "\n--- Добавление нового сотрудника ---\n";

    string username = input_line("Логин сотрудника: ");
    if (username.empty()) {
        cout << "Логин не может быть пустым.\n";
        return;
    }

    if (store_->find_by_username(username) != nullptr) {
        cout << "Сотрудник с таким логином уже существует.\n";
        return;
    }

    if (username == "admin" || username == "administrator" || username == "root") {
        cout << "Данный логин зарезервирован системой. Выберите другой логин.\n";
        return;
    }

    string password;
    string password_hash;
    while (true) {
        password = input_password("Пароль: ");

        if (password.empty()) {
            cout << "Пароль не может быть пустым.\n";
            continue;
        }

        if (password.length() < 6) {
            cout << "Пароль должен содержать минимум 6 символов.\n";
            continue;
        }

        password_hash = hash_password(password);

        if (store_->is_password_already_used(password_hash)) {
            cout << "\033[31mОшибка: этот пароль уже используется другим пользователем!\033[0m\n";
            cout << "Пожалуйста, придумайте другой пароль.\n";
            continue;
        }

        string password2 = input_password("Подтвердите пароль: ");

        if (password != password2) {
            cout << "Пароли не совпадают. Попробуйте заново.\n";
            continue;
        }

        break;
    }

    string fullname = input_line("ФИО сотрудника: ");
    if (fullname.empty()) {
        cout << "ФИО не может быть пустым.\n";
        return;
    }

    string department = choose_department();

    auto employee = UserFactory::create_user(username, password_hash, fullname, department, Role::EMPLOYEE);
    if (store_->add_user(move(employee))) {
        cout << "\033[32mСотрудник '" << fullname << "' успешно добавлен.\033[0m\n";
    }
    else {
        cout << "\033[31mОшибка добавления сотрудника.\033[0m\n";
    }
}

void Application::hr_view_all_employees() {
    auto employees = store_->get_all_employees();

    if (employees.empty()) {
        cout << "\nНет сотрудников.\n";
        return;
    }

    cout << "\n=== СПИСОК СОТРУДНИКОВ ===\n\n";
    const int NUM_WIDTH = 4;
    const int FULLNAME_WIDTH = 25;
    const int DEPARTMENT_WIDTH = 15;

    cout << left
        << setw(NUM_WIDTH) << "№"
        << setw(FULLNAME_WIDTH) << "ФИО"
        << setw(DEPARTMENT_WIDTH) << "ОТДЕЛ"
        << "\n";

    int total_width = NUM_WIDTH  + FULLNAME_WIDTH + DEPARTMENT_WIDTH;
    cout << string(total_width, '-') << "\n";

    for (size_t i = 0; i < employees.size(); ++i) {
        const auto& emp = employees[i];

        string fullname = emp->fullname();
        if (fullname.length() > FULLNAME_WIDTH - 1) {
            fullname = fullname.substr(0, FULLNAME_WIDTH - 4) + "...";
        }

        string department = emp->department();
        if (department.length() > DEPARTMENT_WIDTH - 1) {
            department = department.substr(0, DEPARTMENT_WIDTH - 4) + "...";
        }

        cout << left
            << setw(NUM_WIDTH) << (i + 1)
            << setw(FULLNAME_WIDTH) << fullname
            << setw(DEPARTMENT_WIDTH) << department;

        cout << "\n";
    }

    cout << "\nВсего: " << employees.size() << " сотрудников\n";
}

void Application::hr_view_employee_details() {
    cout << "\n--- Детали сотрудника ---\n";

    auto employees = store_->get_all_employees();
    if (employees.empty()) {
        cout << "Нет сотрудников.\n";
        return;
    }

    cout << "Список сотрудников:\n";
    for (size_t i = 0; i < employees.size(); ++i) {
        cout << (i + 1) << ") " << employees[i]->fullname()<< "\n";
    }

    int choice = input_int("Выберите сотрудника для просмотра: ");
    if (choice < 1 || choice > static_cast<int>(employees.size())) {
        cout << "Неверный выбор.\n";
        return;
    }

    const auto& emp = employees[choice - 1];

    cout << "\n--- Информация о сотруднике ---\n";
    emp->view_profile();

    // Показать проекты сотрудника
    auto projects = project_store_->get_employee_projects(emp->username());
    if (!projects.empty()) {
        cout << "\nПроекты сотрудника:\n";
        for (const auto& project : projects) {
            cout << "  - " << project->name()
                << " | Статус: " << project->status()
                << " | Роль: ";

            // Получаем роль сотрудника в проекте
            auto all_emp_projects = project_store_->all_employee_projects();
            for (const auto& emp_proj : all_emp_projects) {
                if (emp_proj->username() == emp->username() &&
                    emp_proj->project_name() == project->name()) {
                    cout << emp_proj->role();
                    break;
                }
            }
            cout << "\n";
        }
    }
    else {
        cout << "\nСотрудник не участвует ни в одном проекте.\n";
    }
}

void Application::hr_edit_employee() {
    cout << "\n--- Редактирование данных сотрудника ---\n";

    auto employees = store_->get_all_employees();
    if (employees.empty()) {
        cout << "Нет сотрудников.\n";
        return;
    }

    cout << "Список сотрудников:\n";
    for (size_t i = 0; i < employees.size(); ++i) {
        cout << (i + 1) << ") " << employees[i]->fullname()<< "\n";
    }

    int choice = input_int("Выберите сотрудника для редактирования: ");
    if (choice < 1 || choice > static_cast<int>(employees.size())) {
        cout << "Неверный выбор.\n";
        return;
    }

    const string username = employees[choice - 1]->username();

    cout << "\nТекущая информация:\n";
    employees[choice - 1]->view_profile();

    cout << "\nВведите новые данные (оставьте пустым, чтобы не менять):\n";

    string new_fullname = input_line("Новое ФИО: ", true);
    if (new_fullname.empty()) {
        new_fullname = employees[choice - 1]->fullname();
    }

    cout << "\nВыберите новый отдел (нажмите Enter, чтобы оставить текущий):\n";
    vector<string> depts = {
        "HR", "Разработка", "QA", "Дизайн", "Маркетинг", "Продажи", "Поддержка"
    };

    for (size_t i = 0; i < depts.size(); ++i) {
        cout << (i + 1) << ") " << depts[i] << "\n";
    }

    string new_department;
    while (true) {
        cout << "\nВаш выбор (1-" << depts.size() << ") или Enter для текущего отдела: ";
        string input;
        getline(cin, input);

        // Если нажали Enter - оставляем текущий отдел
        if (input.empty()) {
            new_department = employees[choice - 1]->department();
            break;
        }

        try {
            int dept_choice = stoi(input);
            if (dept_choice >= 1 && static_cast<size_t>(dept_choice) <= depts.size()) {
                new_department = depts[dept_choice - 1];
                break;
            }
            else {
                cout << "Неверный выбор. Пожалуйста, введите число от 1 до " << depts.size() << ".\n";
            }
        }
        catch (const exception&) {
            cout << "Пожалуйста, введите число или нажмите Enter.\n";
        }
    }

    if (store_->update_employee(username, new_fullname, new_department)) {
        cout << "Данные сотрудника обновлены.\n";
    }
    else {
        cout << "Ошибка обновления данных.\n";
    }
}

void Application::hr_delete_employee() {
    cout << "\n--- Удаление сотрудника ---\n";

    auto employees = store_->get_all_employees();
    if (employees.empty()) {
        cout << "Нет сотрудников.\n";
        return;
    }

    cout << "Список сотрудников:\n";
    for (size_t i = 0; i < employees.size(); ++i) {
        cout << (i + 1) << ") " << employees[i]->fullname() << "\n";
    }

    int choice = input_int("Выберите сотрудника для удаления: ");
    if (choice < 1 || choice > static_cast<int>(employees.size())) {
        cout << "Неверный выбор.\n";
        return;
    }

    const string username = employees[choice - 1]->username();

    cout << "Вы уверены, что хотите удалить сотрудника '" << employees[choice - 1]->fullname() << "'? (y/n): ";
    string confirm = input_line("");
    if (confirm == "y" || confirm == "Y") {
        auto emp_projects = project_store_->all_employee_projects();
        for (const auto& emp_proj : emp_projects) {
            if (emp_proj->username() == username) {
                project_store_->remove_employee_from_project(username, emp_proj->project_name());
            }
        }

        if (store_->remove_user_by_username(username)) {
            cout << "Сотрудник удален.\n";
        }
        else {
            cout << "Ошибка удаления сотрудника.\n";
        }
    }
    else {
        cout << "Удаление отменено.\n";
    }
}

void Application::hr_search_employees() {
    cout << "\n--- Поиск сотрудников ---\n";
    string keyword = input_line("Введите ключевое слово для поиска (ФИО): ");
    auto results = store_->search_employees_by_name(keyword);

    if (results.empty()) {
        cout << "Сотрудники не найдены.\n";
    }
    else {
        cout << "\n--- Результаты поиска (" << results.size() << ") ---\n";
        for (const auto& emp : results) {
            cout << "ФИО: " << emp->fullname()
                << " | Отдел: " << emp->department()
                << " | Роль: " << role_to_string(emp->role()) << "\n";
        }
    }
}

void Application::hr_sort_employees() {
    
    vector<const User*> sorted_employees;
 
    sorted_employees = store_->get_employees_sorted_by_name(true);
    cout << "\n--- Сотрудники отсортированные по ФИО (А-Я) ---\n";

    if (sorted_employees.empty()) {
        cout << "Нет сотрудников.\n";
        return;
    }

    for (const auto& emp : sorted_employees) {
        cout << "ФИО: " << emp->fullname()
            << " | Логин: " << emp->username()
            << " | Отдел: " << emp->department()
            << " | Роль: " << role_to_string(emp->role()) << "\n";
    }
}

// Метод для генерации отчета сотрудника (вывод на экран + сохранение в файл)
void Application::hr_generate_report() {
    const int TABLE_WIDTH = 80;

    cout << "\n";
    printHorizontalLine('=', TABLE_WIDTH);
    cout << "|" << centerAlign("ГЕНЕРАЦИЯ ОТЧЕТА О СОТРУДНИКЕ", TABLE_WIDTH - 2) << "|\n";
    printHorizontalLine('=', TABLE_WIDTH);

    auto employees = store_->get_all_employees();
    if (employees.empty()) {
        cout << "|" << centerAlign("Нет сотрудников для генерации отчетов", TABLE_WIDTH - 2) << "|\n";
        printHorizontalLine('=', TABLE_WIDTH);
        return;
    }

    cout << "|" << centerAlign("Список сотрудников", TABLE_WIDTH - 2) << "|\n";
    printHorizontalLine('-', TABLE_WIDTH);

    for (size_t i = 0; i < employees.size(); ++i) {
        string employee_info = to_string(i + 1) + ") " + employees[i]->fullname() + " - "+
            employees[i]->department();

        // Обрезаем если слишком длинное
        if (employee_info.length() > TABLE_WIDTH - 4) {
            employee_info = employee_info.substr(0, TABLE_WIDTH - 7) + "...";
        }

        cout << "| " << left << setw(TABLE_WIDTH - 3) << employee_info << "|\n";
    }

    printHorizontalLine('=', TABLE_WIDTH);

    int choice = input_int("\nВыберите сотрудника для генерации отчета (0 - отмена): ");
    if (choice == 0) {
        cout << "\n";
        printHorizontalLine('=', TABLE_WIDTH);
        cout << "|" << centerAlign("Операция отменена", TABLE_WIDTH - 2) << "|\n";
        printHorizontalLine('=', TABLE_WIDTH);
        return;
    }

    if (choice < 1 || choice > static_cast<int>(employees.size())) {
        cout << "\n";
        printHorizontalLine('=', TABLE_WIDTH);
        cout << "|" << centerAlign("Неверный выбор", TABLE_WIDTH - 2) << "|\n";
        printHorizontalLine('=', TABLE_WIDTH);
        return;
    }

    string username = employees[choice - 1]->username();
    User* employee = store_->get_employee(username);

    if (!employee) {
        cout << "\n";
        printHorizontalLine('=', TABLE_WIDTH);
        cout << "|" << centerAlign("Ошибка: сотрудник не найден", TABLE_WIDTH - 2) << "|\n";
        printHorizontalLine('=', TABLE_WIDTH);
        return;
    }

    generate_detailed_report(username, employee);
}

// Метод для создания подробного отчета с сохранением в файл
void Application::generate_detailed_report(const string& username, User* employee) {
    const int TABLE_WIDTH = 80;

    cout << "\n";
    printHorizontalLine('=', TABLE_WIDTH);
    cout << "|" << centerAlign("ОТЧЕТ О СОТРУДНИКЕ", TABLE_WIDTH - 2) << "|\n";
    printHorizontalLine('=', TABLE_WIDTH);

    // Основная информация
    string department = employee->department();
    string deptParamName = system_config_->getDepartmentParameterName(department);

    // Получаем информацию о проектах
    auto projects = project_store_->get_employee_projects(username);
    int total_projects = projects.size();
    int active_projects = 0;
    int completed_projects = 0;
    int leadership_count = 0;
    vector<string> project_details;

    for (const auto& project : projects) {
        if (project->status() == "Активный" || project->status() == "Active") {
            active_projects++;
        }
        else if (project->status() == "Завершенный" || project->status() == "Completed") {
            completed_projects++;
        }

        // Получаем роль сотрудника в проекте
        string role = "Участник";
        string assigned_date = "";
        auto all_emp_projects = project_store_->all_employee_projects();
        for (const auto& emp_proj : all_emp_projects) {
            if (emp_proj->username() == username &&
                emp_proj->project_name() == project->name()) {
                role = emp_proj->role();
                assigned_date = emp_proj->assigned_date();
                if (role == "Руководитель" || role == "Team Lead" ||
                    role == "Менеджер" || role == "Manager") {
                    leadership_count++;
                }
                break;
            }
        }

        project_details.push_back(project->name() + "|" + project->status() + "|" +
            role + "|" + assigned_date + "|" + project->created_date());
    }

    // Основная информация таблица
    cout << "|" << centerAlign("ОСНОВНАЯ ИНФОРМАЦИЯ", TABLE_WIDTH - 2) << "|\n";
    printHorizontalLine('-', TABLE_WIDTH);

    int label_width = 20;
    int value_width = TABLE_WIDTH - label_width - 7;

    
    cout << "| " << left << setw(label_width) << "ФИО:"
        << " | " << left << setw(value_width) << employee->fullname() << " |\n";
    cout << "| " << left << setw(label_width) << "Отдел:"
        << " | " << left << setw(value_width) << department << " |\n";
    cout << "| " << left << setw(label_width) << "Дата генерации:"
        << " | " << left << setw(value_width) << now_string() << " |\n";
    
    printHorizontalLine('=', TABLE_WIDTH);

    // Проектная активность
    cout << "|" << centerAlign("ПРОЕКТНАЯ АКТИВНОСТЬ", TABLE_WIDTH - 2) << "|\n";
    printHorizontalLine('-', TABLE_WIDTH);

    int label_width2 = 30;
    int value_width2 = TABLE_WIDTH - label_width2 - 7;

    cout << "| " << left << setw(label_width2) << "Всего проектов:"
        << " | " << right << setw(value_width2) << to_string(total_projects) << " |\n";
    cout << "| " << left << setw(label_width2) << "Активных проектов:"
        << " | " << right << setw(value_width2) << to_string(active_projects) << " |\n";
    cout << "| " << left << setw(label_width2) << "Завершенных проектов:"
        << " | " << right << setw(value_width2) << to_string(completed_projects) << " |\n";
    cout << "| " << left << setw(label_width2) << "Лидерских ролей:"
        << " | " << right << setw(value_width2) << to_string(leadership_count) << " |\n";

    if (total_projects > 0) {
        double completion_rate = (static_cast<double>(completed_projects) / total_projects) * 100;
        double activity_rate = (static_cast<double>(active_projects) / total_projects) * 100;
        double leadership_rate = (static_cast<double>(leadership_count) / total_projects) * 100;

        cout << "| " << left << setw(label_width2) << "Процент завершения:"
            << " | " << right << setw(value_width2) << fixed << setprecision(1) << completion_rate << "%|\n";
        cout << "| " << left << setw(label_width2) << "Процент активности:"
            << " | " << right << setw(value_width2) << fixed << setprecision(1) << activity_rate << "%|\n";
        cout << "| " << left << setw(label_width2) << "Процент лидерства:"
            << " | " << right << setw(value_width2) << fixed << setprecision(1) << leadership_rate << "%|\n";
    }

    printHorizontalLine('=', TABLE_WIDTH);

    // Детали проектов
    if (!project_details.empty()) {
        cout << "|" << centerAlign("ДЕТАЛИ ПРОЕКТОВ", TABLE_WIDTH - 2) << "|\n";
        printHorizontalLine('-', TABLE_WIDTH);

        // Рассчитываем ширину колонок для таблицы проектов
        int col1_width = 30; // Название проекта
        int col2_width = 15; // Статус
        int col3_width = 20; // Роль
        int col4_width = 25; // Дата
        int total_col_width = col1_width + col2_width + col3_width + col4_width + 9;

        // Заголовки столбцов
        cout << "| " << left << setw(col1_width) << "Название проекта"
            << " | " << left << setw(col2_width) << "Статус"
            
            << " | " << left << setw(col4_width) << "Дата" << " |\n";
        printHorizontalLine('-', TABLE_WIDTH);

        for (size_t i = 0; i < min(project_details.size(), static_cast<size_t>(10)); ++i) {
            vector<string> parts = split(project_details[i], '|');
            if (parts.size() >= 5) {
                string project_name = parts[0];
                if (project_name.length() > col1_width - 2) {
                    project_name = project_name.substr(0, col1_width - 3) + "...";
                }

                string status = parts[1];
                if (status.length() > col2_width - 2) {
                    status = status.substr(0, col2_width - 3) + "...";
                }

                string role = parts[2];
                if (role.length() > col3_width - 2) {
                    role = role.substr(0, col3_width - 3) + "...";
                }

                string short_date = parts[3].length() > col4_width ? parts[3].substr(0, col4_width) : parts[3];

                cout << "| " << left << setw(col1_width) << project_name
                    << " | " << left << setw(col2_width) << status
                    << " | " << left << setw(col4_width) << short_date << " |\n";
            }
        }

        if (project_details.size() > 10) {
            cout << "| " << left << setw(TABLE_WIDTH - 4)
                << "... и еще " + to_string(project_details.size() - 10) + " проектов" << " |\n";
        }

        printHorizontalLine('=', TABLE_WIDTH);
    }

    cout << "|" << centerAlign("ОЦЕНКА ЭФФЕКТИВНОСТИ", TABLE_WIDTH - 2) << "|\n";
    printHorizontalLine('-', TABLE_WIDTH);

    if (system_config_->hasPerformanceScore(username)) {
        double score = system_config_->getPerformanceScore(username);

        cout << "| " << left << setw(40) << "Текущая оценка:"
            << " | " << right << setw(value_width2-14) << fixed << setprecision(2) << score << "/100 |\n";

        cout << "| " << left << setw(40) << "Уровень:"
            << " | " << left << setw(value_width2-10)
            << (score >= 90 ? "ОТЛИЧНО" :
                score >= 75 ? "ХОРОШО" :
                score >= 60 ? "УДОВЛЕТВОРИТЕЛЬНО" :
                score >= 40 ? "ТРЕБУЕТСЯ УЛУЧШЕНИЕ" : "НЕДОСТАТОЧНО") << " |\n";

        
    }
    else {
        cout << "| " << left << setw(TABLE_WIDTH - 4)
            << "Оценка эффективности не рассчитана. Используйте пункт меню" << " |\n";
        cout << "| " << left << setw(TABLE_WIDTH - 4)
            << "'Рассчитать эффективность сотрудника' для проведения оценки." << " |\n";
    }
    

    printHorizontalLine('=', TABLE_WIDTH);

    // Рекомендации
    vector<string> recommendations = generate_recommendations(total_projects, active_projects,
        completed_projects, leadership_count);

    cout << "|" << centerAlign("РЕКОМЕНДАЦИИ ДЛЯ РАЗВИТИЯ", TABLE_WIDTH - 2) << "|\n";
    printHorizontalLine('-', TABLE_WIDTH);

    for (size_t i = 0; i < recommendations.size(); ++i) {
        string rec_line = to_string(i + 1) + ". " + recommendations[i];
        if (rec_line.length() > TABLE_WIDTH - 4) {
            rec_line = rec_line.substr(0, TABLE_WIDTH - 7) + "...";
        }
        cout << "| " << left << setw(TABLE_WIDTH - 4) << rec_line << " |\n";
    }

    printHorizontalLine('=', TABLE_WIDTH);

   

    // Подпись
    int sign_width = 40;
    int date_width = TABLE_WIDTH - sign_width - 7;
    cout << "| " << left << setw(sign_width) << "Подпись HR-менеджера: ___________________"
        << " | " << left << setw(date_width) << "Дата: ___________________" << "|\n";
    printHorizontalLine('=', TABLE_WIDTH);

    // Теперь сохраняем отчет в файл
    save_report_to_file(username, employee, department, deptParamName,
        total_projects, active_projects, completed_projects,
        leadership_count, project_details, recommendations);
}
// Вспомогательные функции для форматирования таблиц
string Application::centerAlign(const string& text, int width) {
    if (text.length() >= width) return text.substr(0, width);

    int left_padding = (width - text.length()) / 2;
    int right_padding = width - text.length() - left_padding;

    return string(left_padding, ' ') + text + string(right_padding, ' ');
}


void Application::printHorizontalLine(char symbol, int width) {
    cout << string(width, symbol) << "\n";
}

// Генерация рекомендаций
vector<string> Application::generate_recommendations(int total_projects, int active_projects,
    int completed_projects, int leadership_count) {
    vector<string> recommendations;

    if (total_projects == 0) {
        recommendations.push_back("Включить сотрудника в активные проекты");
        recommendations.push_back("Назначить наставника для адаптации");
        recommendations.push_back("Определить зону ответственности");
    }
    else if (leadership_count == 0 && total_projects >= 3) {
        recommendations.push_back("Рассмотреть возможность назначения на лидерскую роль");
        recommendations.push_back("Развивать управленческие навыки");
        recommendations.push_back("Поручить менторство новым сотрудникам");
    }
    else if (completed_projects == 0 && active_projects > 0) {
        recommendations.push_back("Сфокусироваться на завершении текущих проектов");
        recommendations.push_back("Улучшить навыки тайм-менеджмента");
        recommendations.push_back("Установить четкие дедлайны");
    }
    else if (completed_projects >= 3 && leadership_count >= 1) {
        recommendations.push_back("Рассмотреть кандидатуру для повышения");
        recommendations.push_back("Поручить более сложные и ответственные задачи");
        recommendations.push_back("Включить в процессы принятия решений");
    }
    else {
        recommendations.push_back("Продолжать текущую деятельность");
        recommendations.push_back("Развивать профессиональные навыки");
        recommendations.push_back("Участвовать в кросс-функциональных проектах");
    }

    // Общие рекомендации
    recommendations.push_back("Проводить регулярные one-to-one встречи");
    recommendations.push_back("Отслеживать прогресс по проектам");
    recommendations.push_back("Обеспечить доступ к обучению и развитию");

    return recommendations;
}

// Метод для сохранения отчета в файл (теперь в том же формате)
void Application::save_report_to_file(const string& username, User* employee,
    const string& department, const string& deptParamName,
    int total_projects, int active_projects, int completed_projects,
    int leadership_count, const vector<string>& project_details,
    const vector<string>& recommendations) {
    
    // Генерация имени файла
    string timestamp = now_string();
    replace(timestamp.begin(), timestamp.end(), ':', '-');
    replace(timestamp.begin(), timestamp.end(), ' ', '_');

    // Сначала удаляем старые отчеты для этого сотрудника
    string search_pattern = "HR_REPORT_" + username + "_*.txt";
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(search_pattern.c_str(), &findFileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                string old_file = findFileData.cFileName;
                remove(old_file.c_str());

                cout << "Старый отчет '" << old_file << "' удален.\n";
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);
        FindClose(hFind);
    }

    string filename = "HR_REPORT_" + username + "_" + timestamp + ".txt";

    ofstream file(filename);
    if (!file) {
        cout << "\n";
        printHorizontalLine('=', 80);
        cout << "|" << centerAlign("ОШИБКА СОХРАНЕНИЯ ФАЙЛА", 78) << "|\n";
        printHorizontalLine('=', 80);
        return;
    }

    const int FILE_WIDTH = 80;

    // Заголовок отчета в файл
    file << string(FILE_WIDTH, '=') << "\n";
    file << "|" << centerAlign("ОТЧЕТ HR О СОТРУДНИКЕ", FILE_WIDTH - 2) << "|\n";
    file << string(FILE_WIDTH, '=') << "\n";
    file << "| " << left << setw(20) << "Дата генерации:"
        << " | " << left << setw(FILE_WIDTH - 27) << now_string() << " |\n";
    file << "| " << left << setw(20) << "Сгенерировано:"
        << " | " << left << setw(FILE_WIDTH - 27) << "HR-менеджером" << " |\n";
    file << string(FILE_WIDTH, '=') << "\n\n";

    // Основная информация
    file << string(FILE_WIDTH, '=') << "\n";
    file << "|" << centerAlign("ОСНОВНАЯ ИНФОРМАЦИЯ", FILE_WIDTH - 2) << "|\n";
    file << string(FILE_WIDTH, '-') << "\n";

    int label_width = 20;
    int value_width = FILE_WIDTH - label_width - 7;

    file << "| " << left << setw(label_width) << "ФИО:"
        << " | " << left << setw(value_width) << employee->fullname() << " |\n";
    file << "| " << left << setw(label_width) << "Отдел:"
        << " | " << left << setw(value_width) << department << " |\n";
    
    file << string(FILE_WIDTH, '=') << "\n\n";

    file << string(FILE_WIDTH, '=') << "\n";
    file << "|" << centerAlign("ПРОЕКТНАЯ АКТИВНОСТЬ", FILE_WIDTH - 2) << "|\n";
    file << string(FILE_WIDTH, '-') << "\n";

    int label_width2 = 30;
    int value_width2 = FILE_WIDTH - label_width2 - 7;

    file << "| " << left << setw(label_width2) << "Всего проектов:"
        << " | " << right << setw(value_width2) << to_string(total_projects) << " |\n";
    file << "| " << left << setw(label_width2) << "Активных проектов:"
        << " | " << right << setw(value_width2) << to_string(active_projects) << " |\n";
    file << "| " << left << setw(label_width2) << "Завершенных проектов:"
        << " | " << right << setw(value_width2) << to_string(completed_projects) << " |\n";
    file << "| " << left << setw(label_width2) << "Лидерских ролей:"
        << " | " << right << setw(value_width2) << to_string(leadership_count) << " |\n";

    if (total_projects > 0) {
        double completion_rate = (static_cast<double>(completed_projects) / total_projects) * 100;
        double activity_rate = (static_cast<double>(active_projects) / total_projects) * 100;
        double leadership_rate = (static_cast<double>(leadership_count) / total_projects) * 100;

        file << "| " << left << setw(label_width2) << "Процент завершения:"
            << " | " << right << setw(value_width2) << fixed << setprecision(1) << completion_rate << "%|\n";
        file << "| " << left << setw(label_width2) << "Процент активности:"
            << " | " << right << setw(value_width2) << fixed << setprecision(1) << activity_rate << "%|\n";
        file << "| " << left << setw(label_width2) << "Процент лидерства:"
            << " | " << right << setw(value_width2) << fixed << setprecision(1) << leadership_rate << "%|\n";
    }

    file << string(FILE_WIDTH, '=') << "\n\n";

    // Детали проектов
    if (!project_details.empty()) {
        file << string(FILE_WIDTH, '=') << "\n";
        file << "|" << centerAlign("ДЕТАЛИ ПРОЕКТОВ", FILE_WIDTH - 2) << "|\n";
        file << string(FILE_WIDTH, '-') << "\n";

        int col1_width = 30;
        int col2_width = 15;
        int col3_width = 20;
        int col4_width = 25;

        file << "| " << left << setw(col1_width) << "Название проекта"
            << " | " << left << setw(col2_width) << "Статус"
            
            << " | " << left << setw(col4_width) << "Дата назначения" << " |\n";
        file << string(FILE_WIDTH, '-') << "\n";

        for (const auto& project_detail : project_details) {
            vector<string> parts = split(project_detail, '|');
            if (parts.size() >= 5) {
                string project_name = parts[0];
                if (project_name.length() > col1_width - 2) {
                    project_name = project_name.substr(0, col1_width - 3) + "...";
                }

                string status = parts[1];
                if (status.length() > col2_width - 2) {
                    status = status.substr(0, col2_width - 3) + "...";
                }

                string role = parts[2];
                if (role.length() > col3_width - 2) {
                    role = role.substr(0, col3_width - 3) + "...";
                }

                file << "| " << left << setw(col1_width) << project_name
                    << " | " << left << setw(col2_width) << status
                    
                    << " | " << left << setw(col4_width) << parts[3] << " |\n";
            }
        }

        file << string(FILE_WIDTH, '=') << "\n\n";
    }

    file << "|" << centerAlign("ОЦЕНКА ЭФФЕКТИВНОСТИ", FILE_WIDTH - 2) << "|\n";
    file << string(FILE_WIDTH, '-') << "\n";

    if (system_config_->hasPerformanceScore(username)) {
        double score = system_config_->getPerformanceScore(username);

        file << "| " << left << setw(40) << "Текущая оценка:"
            << " | " << right << setw(value_width2-14) << fixed << setprecision(2) << score << "/100 |\n";

        file << "| " << left << setw(40) << "Уровень:"
            << " | " << left << setw(value_width2-10)
            << (score >= 90 ? "ОТЛИЧНО" :
                score >= 75 ? "ХОРОШО" :
                score >= 60 ? "УДОВЛЕТВОРИТЕЛЬНО" :
                score >= 40 ? "ТРЕБУЕТСЯ УЛУЧШЕНИЕ" : "НЕДОСТАТОЧНО") << " |\n";

        
    }
    else {
        file << "| " << left << setw(FILE_WIDTH - 4)
            << "Оценка эффективности не рассчитана. Используйте пункт меню" << " |\n";
        file << "| " << left << setw(FILE_WIDTH - 4)
            << "'Рассчитать эффективность сотрудника' для проведения оценки." << " |\n";
    }
    

    printHorizontalLine('=', FILE_WIDTH);

    file << string(FILE_WIDTH, '=') << "\n";
    file << "|" << centerAlign("РЕКОМЕНДАЦИИ ДЛЯ РАЗВИТИЯ", FILE_WIDTH - 2) << "|\n";
    file << string(FILE_WIDTH, '-') << "\n";

    for (size_t i = 0; i < recommendations.size(); ++i) {
        file << "| " << left << setw(FILE_WIDTH - 4)
            << to_string(i + 1) + ". " + recommendations[i] << " |\n";
    }

    file << string(FILE_WIDTH, '=') << "\n\n";

    int sign_width = 40;
    int date_width = FILE_WIDTH - sign_width - 7;
    file << string(FILE_WIDTH, '=') << "\n";
    file << "| " << left << setw(sign_width) << "Подпись HR-менеджера: ___________________"
        << " | " << left << setw(date_width) << "Дата: ___________________" << "|\n";
    file << string(FILE_WIDTH, '=') << "\n";

    file.close();

    cout << "\n";
    printHorizontalLine('=', 80);
    cout << "|" << centerAlign("ОТЧЕТ УСПЕШНО СОХРАНЕН!", 78) << "|\n";

    int label_width4 = 20;
    int value_width4 = 80 - label_width4 - 7;

    cout << "| " << left << setw(label_width4) << "Файл:"
        << " | " << left << setw(value_width4) << filename << " |\n";
    cout << "| " << left << setw(label_width4) << "Размер:"
        << " | " << left << setw(value_width4) << "Полный отчет доступен для печати" << " |\n";
    cout << "| " << left << setw(label_width4) << "Путь:"
        << " | " << left << setw(value_width4) << "Текущая директория программы" << " |\n";
    printHorizontalLine('=', 80);

    cout << "\nОткрыть сохраненный файл? (y/n): ";
    string openFile = input_line("");
    if (openFile == "y" || openFile == "Y") {
        cout << "\n";
        printHorizontalLine('=', 80);
        cout << "|" << centerAlign("СОДЕРЖИМОЕ ФАЙЛА: " + filename, 78) << "|\n";
        printHorizontalLine('=', 80);

        ifstream read_file(filename); 
        if (read_file) {
            string line;
            int line_count = 0;
            while (getline(read_file, line) && line_count < 100) {
                cout << line << "\n";
                line_count++;
            }
            if (line_count >= 100) {
                cout << string(80, '.') << "\n";
                cout << "Показаны первые 100 строк файла. Полный файл: " << filename << "\n";
            }
            printHorizontalLine('=', 80);
            read_file.close();
        }
    }
}


