#include "data.h"
#include "functions.h"
#include "types.hpp"
#include <algorithm>
#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {}

// int main()
//{
//     SetConsoleOutputCP(CP_UTF8);
//     SetConsoleCP(CP_UTF8);
//
//     cout << "========================================\n";
//     cout << "  Программа укладки коробок на паллет\n";
//     cout << "========================================\n\n";
//
//     cout << "Выберите режим работы:\n";
//     cout << "1 - Консольный режим (по умолчанию)\n";
//     cout << "2 - Графический режим с визуализацией\n";
//     cout << "Ваш выбор: ";
//
//     int mode = 1;
//     string input;
//     getline(cin, input);
//
//     if (!input.empty()) {
//         mode = atoi(input.c_str());
//     }
//
//     if (mode == 2) {
//         // Графический режим
//         cout << "\nЗапуск графического режима...\n";
//         run_graphics_mode();
//     }
//     else {
//         // Консольный режим
//         cout << "\nЗапуск консольного режима...\n\n";
//
//
//         pallet pallet1;
//         pallet_handle(&pallet1);
//
//         create_ascii_layers(&pallet1);
//         create_ascii_layers_m(&pallet1);
//     }
//
//     cout << "\nПрограмма завершена.\n";
//     return 0;
// }
