#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

// блокируем определение byte Windows
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#ifdef byte
#undef byte
#endif
#include <string>
#include <vector>
#include <optional>

using namespace std;

// --------------------------- Константы файлов ---------------------------
const string USERS_FILE = "users.txt";
const string PROJECTS_FILE = "projects.txt"; 
const string EMPLOYEE_PROJECTS_FILE = "employee_projects.txt"; 
const string HR_USERS_FILE = "hr_users.txt"; 
const string ADMIN_USERS_FILE = "admin_users.txt";  

// --------------------------- Вспомогательные утилиты ---------------------------
string trim(const string& s);
string format_cell(const string& text, size_t width);

// Функция для вывода горизонтальной линии
void print_horizontal_line(size_t total_width);

// Функция для вывода заголовка таблицы
void print_table_header(const vector<string>& headers, const vector<size_t>& widths);

// Функция для вывода строки таблицы
void print_table_row(const vector<string>& cells, const vector<size_t>& widths);
bool isCyrillic(char ch);
vector<string> split(const string& s, char delim);

string now_string();
string& toLower(string& s);
// --------------------------- Шаблонные CRUD-утилиты для контейнеров ---------------------------
template <typename T, typename KeyFunc>
optional<size_t> find_index_by_key(const vector<T>& container, const typename invoke_result_t<KeyFunc(const T&)>::type& key, KeyFunc keyFn);

// Добавить элемент
template <typename T>
void add_item(vector<T>& container, T item);

// Удалить элемент по индексу
template <typename T>
bool remove_item_at(vector<T>& container, size_t index);

// Редактировать элемент (здесь просто заменяем)
template <typename T>
bool update_item_at(vector<T>& container, size_t index, const T& newItem);

namespace HRSystem {
    // Константы
    const string USERS_FILE = "users.txt";
    const string PROJECTS_FILE = "projects.txt";
    const string EMPLOYEE_PROJECTS_FILE = "employee_projects.txt";
    const string HR_USERS_FILE = "hr_users.txt";
    const string ADMIN_USERS_FILE = "admin_users.txt";

    // Функции (с using)
    using ::trim;
    using ::format_cell;
    using ::print_horizontal_line;
    using ::print_table_header;
    using ::print_table_row;
    using ::isCyrillic;
    using ::split;
    using ::now_string;
    using ::toLower;
}
