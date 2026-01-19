#include"UserClasses.h"
#include "Projects.h"
#include <windows.h>
//using namespace HRSystem;


// --------------------------- main ---------------------------
int main() {
    setlocale(LC_ALL, "RUS");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    HRSystem::Application app;
    app.run();
    return 0;
}