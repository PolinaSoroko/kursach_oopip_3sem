#pragma once

#include <algorithm> 
#include <fstream> 
#include <memory> 
#include <string> 
#include <vector>
#include "Utilities.h"
#include "SystemConfig.h"

class Project;
class EmployeeProject;
class ProjectStore;

using namespace std;

// --------------------------- Роли и типы ---------------------------
enum class Role { EMPLOYEE, HR, ADMIN, PENDING };

inline string role_to_string(Role r) {
    switch (r) {
    case Role::EMPLOYEE: return "EMPLOYEE";
    case Role::HR: return "HR";
    case Role::ADMIN: return "ADMIN";
    case Role::PENDING: return "PENDING";
    }
    return "UNKNOWN";
}

inline Role string_to_role(const string& s) {
    string u = s;
    transform(u.begin(), u.end(), u.begin(), ::toupper);
    if (u == "EMPLOYEE") return Role::EMPLOYEE;
    if (u == "HR") return Role::HR;
    if (u == "ADMIN") return Role::ADMIN;
    if (u == "PENDING") return Role::PENDING;
    return Role::EMPLOYEE;
}

// --------------------------- Базовый класс User ---------------------------
class User {
    friend ostream& operator<<(ostream&, const User&);
    static int total_users;
protected:
    string username_;
    string password_;
    string fullname_;
    string department_;
    Role role_ = Role::EMPLOYEE;
public:
    User() = default;
    User(string username, string password,
        string fullname, string department, Role role)
        : username_(move(username)),
        password_(move(password)),
        fullname_(move(fullname)),
        department_(move(department)),
        role_(role) {
    }

    virtual ~User() = default;

    const string& username() const { return username_; }
    const string& password() const { return password_; }
    const string& fullname() const { return fullname_; }
    const string& department() const { return department_; }
    Role role() const { return role_; }

    void set_password(const string& p) { password_ = p; }
    void set_fullname(const string& n) { fullname_ = n; }
    void set_department(const string& d) { department_ = d; }
    void set_role(Role r) { role_ = r; }

    virtual string serialize() const {
        return username_ + "|" + password_ + "|" + fullname_ + "|" + department_ + "|" + role_to_string(role_);
    }
    virtual void view_profile() const;
    virtual void admin_assign_hr_role(const string& username) = 0;
    virtual void hr_add_employee() = 0;
    virtual void hr_add_project() = 0;
    virtual void hr_calc_performance(const string& username) = 0;
    virtual void hr_generate_report(const string& username) = 0;
    virtual void employee_view_reports() const = 0;
    virtual void employee_view_rating() const = 0;

};

// Администратор — расширяет User
class Admin : public User {
public:
    Admin(string username, string password, string fullname = "Administrator")
        : User(move(username), move(password), move(fullname), "ADMIN_DEPT", Role::ADMIN) {
    }
   
    void admin_assign_hr_role(const string& username) override;
    // Админ НЕ должен выполнять HR функции
    void hr_add_employee() override {} 
    void hr_add_project() override {}
    void hr_calc_performance(const string& username) override {}
    void hr_generate_report(const string& username) override {}
    void employee_view_reports() const override {} 
    void employee_view_rating() const override {}

    string serialize() const override {
        return username_ + "|" + password_ + "|" + fullname_ + "|ADMIN|ADMIN";
    }
    
};

class HRManagerUser : public User {
public:
    HRManagerUser(string username, string password, string fullname, string department = "HR")
        : User(move(username), move(password), move(fullname), move(department), Role::HR) {
    }

    // HR НЕ может назначать других HR
    void admin_assign_hr_role(const string& username) override {}
    void employee_view_rating() const override {};

    void hr_add_employee() override;
    void hr_add_project() override;
    void hr_calc_performance(const string& username) override;
    void hr_generate_report(const string& username) override;

    void employee_view_reports() const override;
    
};

// Обычный сотрудник
class EmployeeUser : public User {
public:
    EmployeeUser(string username, string password, string fullname, string department)
        : User(move(username), move(password), move(fullname), move(department), Role::EMPLOYEE) {
    }

    // Сотрудник НЕ может выполнять HR и Admin функции
    void admin_assign_hr_role(const string& username) override {}

    void hr_add_employee() override {}
    void hr_add_project() override {}
    void hr_calc_performance(const string& username) override {}
    void hr_generate_report(const string& username) override {}

    // Сотрудник может только просматривать свои отчеты и рейтинг
    void employee_view_reports() const override;
    void employee_view_rating() const override;
};

// --------------------------- Factory для пользователей ---------------------------
class UserFactory {
public:
    static unique_ptr<User> create_user_from_record(const string& record) {
        auto parts = split(record, '|');
        if (parts.size() < 5) return nullptr;

        string username = trim(parts[0]);
        string password = trim(parts[1]);
        string fullname = trim(parts[2]);
        string department = trim(parts[3]);
        Role role = string_to_role(trim(parts[4]));

        if (role == Role::PENDING) {
            auto ptr = make_unique<EmployeeUser>(username, password, fullname, department);
            ptr->set_role(Role::PENDING);
            return ptr;
        }
        else {
            return make_unique<EmployeeUser>(username, password, fullname, department);
        }
    }

    static unique_ptr<User> create_user(const string& username,
        const string& password,
        const string& fullname,
        const string& department,
        Role role) {
        if (role == Role::ADMIN) {
            return make_unique<Admin>(username, password, fullname);
        }
        else if (role == Role::HR) {
            return make_unique<HRManagerUser>(username, password, fullname, department);
        }
        else if (role == Role::PENDING) {
            auto user = make_unique<EmployeeUser>(username, password, fullname, department);
            user->set_role(Role::PENDING);
            return user;
        }
        else {
            return make_unique<EmployeeUser>(username, password, fullname, department);
        }
    }
};

// --------------------------- Хранилище пользователей (файловое) ---------------------------
template <typename T>
class Store {
protected:
    vector<unique_ptr<T>> items;

public:
    bool add(unique_ptr<T> item) {
        items.push_back(move(item));
        return true;
    }
    const vector<unique_ptr<T>>& get_all() const {
        return items;
    }
    T* find_by_name(const string& name) const {
        for (auto& it : items)
            if (it->name() == name)
                return it.get();
        return nullptr;
    }
};

class UserStore : public Store<User> {
private:
    vector<unique_ptr<User>> users_;
    vector<unique_ptr<User>> hr_users_;
    unique_ptr<User> admin_user_;
public:
    UserStore() {
        load_from_file();
    }
    bool is_password_already_used(const string& password_hash) const;
    void load_from_file();

    void save_to_file();
    User* find_by_username(const string& username);
    bool add_user(unique_ptr<User> user);
    bool remove_user_by_username(const string& username);
    bool update_user(unique_ptr<User> updated);

    void load_hr_from_file();
    void save_hr_to_file();
    bool move_user_to_hr(const string& username);
    void save_all_files();

    // Проверка, является ли пользователь HR
    bool is_hr_user(const string& username) const;
    void create_default_admin();

    void load_admin_from_file();
    void save_admin_to_file();
    bool is_admin_user(const string& username) const;
    const vector<unique_ptr<User>>& users() const { return users_; }
    vector<const User*> all_users() const;
    vector<const User*> get_all_employees() const;
    User* get_employee(const string& username) const;
    bool update_employee(const string& username, const string& new_fullname, const string& new_department);
    
    vector<const User*> search_employees_by_name(const string& keyword) const;

    vector<const User*> get_employees_sorted_by_name(bool ascending = true) const;
    vector<const User*> get_pending_users() const;
    bool remove_hr_user_by_username(const string& username);
    vector<pair<const User*, double>> get_employees_sorted_by_rating(SystemConfig* config) const;

    // Получить место сотрудника в рейтинге
    pair<int, int> get_employee_rank(const string& username, SystemConfig* config) const;

    // Получить всех обычных пользователей (для сортировки)
    vector<User*> get_employees() const;
    
};


// --------------------------- Интерфейс консоли / рабочий поток ---------------------------
class Application {
private:
    unique_ptr<UserStore> store_;
    unique_ptr<ProjectStore> project_store_;
    unique_ptr<SystemConfig> system_config_;

    void show_main_menu();

    static int input_int(const string& prompt);

    static string input_line(const string& prompt, bool allow_empty = false);

    // ----------- Login flow -----------
    void handle_login();
    bool attempt_password(const string& true_password_hash);

    // ----------- Registration flow -----------
    void handle_register();

    void handle_register_with_login(const string& desired_login);

    string choose_department();

    // ----------- Сессии пользователей (меню после входа) -----------
    void admin_session(Admin& admin);

    void hr_session(HRManagerUser& hr);

    void employee_session(EmployeeUser& emp);

    // ----------- Административные утилиты -----------
    void list_all_users();

    void assign_hr_role_flow();

public:
    Application()
        : store_(make_unique<UserStore>()),
        project_store_(make_unique<ProjectStore>()),
        system_config_(make_unique<SystemConfig>()) { 
        setlocale(LC_ALL, "Russian");
        locale::global(std::locale("Russian"));

    }
    
    void run();
    // Методы для управления проектами (для HR менеджеров)
    void hr_manage_projects(HRManagerUser& hr);
    // Методы для работы с проектами
    void hr_add_project();
    void hr_view_all_projects();
    void hr_edit_project();
    void hr_delete_project();
    void hr_assign_employee_to_project();
    void hr_view_project_details();
    void hr_search_projects();
    void hr_filter_projects_by_status();
    void hr_sort_projects();
    void employee_view_my_projects(EmployeeUser& emp);

    void hr_manage_employees(HRManagerUser& hr);

    void hr_add_employee();
    void hr_view_all_employees();
    void hr_edit_employee();
    void hr_delete_employee();
    void hr_view_employee_details();
    void hr_search_employees();
    void hr_sort_employees();
    // метод для расчета эффективности с использованием SystemConfig
    void hr_calculate_performance();

    // Метод для администратора для настройки коэффициентов
    void admin_configure_system();

    void show_employee_performance_score(const string& username);
    void hr_view_saved_report(const string& username);
    
    void hr_generate_report();
    void generate_detailed_report(const string& username, User* employee);
    void save_report_to_file(const string& username, User* employee,
        const string& department, const string& deptParamName,
        int total_projects, int active_projects, int completed_projects,
        int leadership_count, const vector<string>& project_details,
        const vector<string>& recommendations);

    string centerAlign(const string& text, int width);
    void printHorizontalLine(char symbol, int width);
    vector<string> generate_recommendations(int total_projects, int active_projects,
        int completed_projects, int leadership_count);
    void show_employees_sorted_by_name();
    void show_employees_rating();
    void show_my_rating(const string& username);


};
namespace HRSystem {
    using ::Application;
    using ::User;
    using ::Admin;
    using ::HRManagerUser;
    using ::EmployeeUser;
    using ::UserFactory;
    using ::UserStore;
}



