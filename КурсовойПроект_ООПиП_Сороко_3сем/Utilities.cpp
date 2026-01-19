#include "Utilities.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm> 


using namespace std;


// --------------------------- Вспомогательные утилиты ---------------------------
string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

bool isCyrillic(char ch) {
    unsigned char c = ch;
    return (c >= 0xC0 && c <= 0xFF) || c == 0xA8 || c == 0xB8;
}

string& toLower(string& s) {
    transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) {
            return std::tolower(c);
        }
    );
    return s;
}

string format_cell(const string& text, size_t width) {
    if (text.length() > width) {
        return text.substr(0, width - 3) + "...";
    }
    else {
        return text + string(width - text.length(), ' ');
    }
}

// Функция для вывода горизонтальной линии
void print_horizontal_line(size_t total_width) {
    cout << string(total_width + 2, '-') << "\n";
}   

// Функция для вывода заголовка таблицы
void print_table_header(const vector<string>& headers, const vector<size_t>& widths) {
    size_t total_width = 0;
    for (size_t w : widths) {
        total_width += w + 2;
    }
    total_width += 1;

    print_horizontal_line(total_width);

    cout << "|";
    for (size_t i = 0; i < headers.size(); ++i) {
        cout << " " << format_cell(headers[i], widths[i]) << " |";
    }
    cout << "\n";

    print_horizontal_line(total_width);
}

// Функция для вывода строки таблицы
void print_table_row(const vector<string>& cells, const vector<size_t>& widths) {
    cout << "|";
    for (size_t i = 0; i < cells.size(); ++i) {
        cout << " " << format_cell(cells[i], widths[i]) << " |";
    }
    cout << "\n";
}

vector<string> split(const string& s, char delim) {
    vector<string> out;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) out.push_back(item);
    return out;
}

string now_string() {
    time_t t = time(nullptr);
    tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    ostringstream oss;
    oss << put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// --------------------------- Шаблонные CRUD-утилиты для контейнеров ---------------------------
template <typename T, typename KeyFunc>
optional<size_t> find_index_by_key(const vector<T>& container, const typename invoke_result_t<KeyFunc(const T&)>::type& key, KeyFunc keyFn) {
    for (size_t i = 0; i < container.size(); ++i) {
        if (keyFn(container[i]) == key) return i;
    }
    return nullopt;
}

// Добавить элемент
template <typename T>
void add_item(vector<T>& container, T item) {
    container.push_back(move(item));
}

// Удалить элемент по индексу
template <typename T>
bool remove_item_at(vector<T>& container, size_t index) {
    if (index >= container.size()) return false;
    container.erase(container.begin() + index);
    return true;
}

// Редактировать элемент (здесь просто заменяем)
template <typename T>
bool update_item_at(vector<T>& container, size_t index, const T& newItem) {
    if (index >= container.size()) return false;
    container[index] = newItem;
    return true;
}