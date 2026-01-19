#pragma once
#include"Utilities.h"

class User;
class UserStore;
class Application;


// --------------------------- Класс проекта ---------------------------
class Project {
    friend ostream& operator<<(ostream&, const Project&);
   
public:
    Project() = default;
    Project(string name, string description, string status)
        : name_(move(name)), description_(move(description)), status_(move(status)) {
        created_date_ = now_string();
    }

    const string& name() const { return name_; }
    const string& description() const { return description_; }
    const string& status() const { return status_; }
    const string& created_date() const { return created_date_; }

    void set_status(const string& status) { status_ = status; }
    void set_description(const string& desc) { description_ = desc; }

    // Сериализация в строку для файла
    string serialize() const {
        return name_ + "|" + description_ + "|" + status_ + "|" + created_date_;
    }

    static unique_ptr<Project> create_from_record(const string& record) {
        auto parts = split(record, '|');
        if (parts.size() >= 4) {
            auto project = make_unique<Project>(
                trim(parts[0]),  // name
                trim(parts[1]),  // description
                trim(parts[2])   // status
            );
            return project;
        }
        return nullptr;
    }

private:
    string name_;
    string description_;
    string status_;
    string created_date_;
};

// --------------------------- Связь сотрудник-проект ---------------------------
class EmployeeProject {
    friend ostream& operator<<(ostream&, const EmployeeProject&);
public:
    EmployeeProject(string username, string project_name, string role)
        : username_(move(username)), project_name_(move(project_name)), role_(move(role)) {
        assigned_date_ = now_string();
    }
    
    const string& username() const { return username_; }
    const string& project_name() const { return project_name_; }
    const string& role() const { return role_; }
    const string& assigned_date() const { return assigned_date_; }

    void set_role(const string& role) { role_ = role; }

    // Сериализация в строку для файла
    string serialize() const {
        return username_ + "|" + project_name_ + "|" + role_ + "|" + assigned_date_;
    }

    static unique_ptr<EmployeeProject> create_from_record(const string& record) {
        auto parts = split(record, '|');
        if (parts.size() >= 4) {
            auto emp_proj = make_unique<EmployeeProject>(
                trim(parts[0]),  // username
                trim(parts[1]),  // project_name
                trim(parts[2])   // role
            );
            return emp_proj;
        }
        return nullptr;
    }

private:
    string username_;
    string project_name_;
    string role_;
    string assigned_date_;
};

// --------------------------- Хранилище проектов ---------------------------
class ProjectStore : public Store<Project> {
private:
    vector<unique_ptr<Project>> projects_;
    vector<unique_ptr<EmployeeProject>> employee_projects_;

public:
    ProjectStore() {
        load_from_file();
    }

    void load_from_file();
    void save_all_files();
    
    // CRUD операции для проектов
    bool add_project(unique_ptr<Project> project);
    bool remove_project(const string& project_name);
    bool update_project(const string& project_name, const string& new_status);
    bool update_project(const string& project_name, const string& new_status, const string& new_description);
    Project* find_project(const string& project_name) const;

    // CRUD операции для связи сотрудник-проект
    bool assign_employee_to_project(const string& username, const string& project_name);
    bool assign_employee_to_project(const string& username, const string& project_name, const string& role);

    bool remove_employee_from_project(const string& username, const string& project_name);
    bool update_employee_role(const string& username, const string& project_name, const string& new_role);

    // Получение данных
    vector<const Project*> all_projects() const;
    vector<const EmployeeProject*> all_employee_projects() const;
    vector<const Project*> get_employee_projects(const string& username) const;
    vector<const EmployeeProject*> get_project_employees(const string& project_name) const;

    // Поиск и фильтрация
    vector<const Project*> search_projects_by_name(const string& keyword) const;
    vector<const Project*> filter_projects_by_status(const string& status) const;

    // Сортировка
    vector<const Project*> get_projects_sorted_by_name(bool ascending = true) const;
    vector<const Project*> get_projects_sorted_by_date(bool ascending = true) const;

public:
    void load_projects_from_file();
    void load_employee_projects_from_file();
    void save_projects_to_file();
    void save_employee_projects_to_file();
};
namespace HRSystem {
    using ::Project;
    using ::EmployeeProject;
    using ::ProjectStore;
}