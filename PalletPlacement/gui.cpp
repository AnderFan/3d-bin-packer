#pragma execution_character_set("utf-8")
#include "graphics.h"
#include "functions.h"
#include "utils.h"
#include <windows.h>
#include <string>
#include <sstream>

static HWND g_hDlgWnd = NULL;
static GUIState* g_pState = NULL;

#define IDC_P_WIDTH_EDIT    1011
#define IDC_P_HEIGHT_EDIT   1012
#define IDC_P_DEPTH_EDIT    1013
#define IDC_FULL_ROTATE_CHECK 1014
#define IDC_P_MAX_MASS_EDIT 1015  

#define IDC_SET_CENTER_MASS 1016 
#define IDC_SET_MAX_VOLUME 1017

#define IDC_WIDTH_EDIT      1002
#define IDC_HEIGHT_EDIT     1003
#define IDC_DEPTH_EDIT      1004
#define IDC_QUANTITY_EDIT   1005
#define IDC_WEIGHT_EDIT     1006
#define IDC_ADD_BUTTON      1007
#define IDC_START_BUTTON    1008
#define IDC_CLEAR_BUTTON    1009
#define IDC_BOX_LIST        1010

LRESULT CALLBACK DialogWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        int x = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
        SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        if (g_pState) {
            SetDlgItemInt(hwnd, IDC_P_WIDTH_EDIT, g_pState->pallet_width, FALSE);
            SetDlgItemInt(hwnd, IDC_P_HEIGHT_EDIT, g_pState->pallet_height, FALSE);
            SetDlgItemInt(hwnd, IDC_P_DEPTH_EDIT, g_pState->pallet_depth, FALSE);
            SetDlgItemInt(hwnd, IDC_P_MAX_MASS_EDIT, g_pState->pallet_max_mass, FALSE);
            HWND hList = GetDlgItem(hwnd, IDC_BOX_LIST);
            if (hList && !g_pState->box_types.empty()) {
                for (const auto& bt : g_pState->box_types) {
                    char listBuffer[256];
                    sprintf_s(listBuffer, "Размер: %dx%dx%d мм, Вес: %d кг, Кол-во: %d%s",
                        bt.width, bt.height, bt.depth, bt.weight, bt.Quantity,
                        bt.full_rotateble ? ", Поворот" : "");
                    std::wstring wbuffer = utf8_to_wstring(listBuffer);
                    SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)wbuffer.c_str());
                }
            }
        }
        else {
            SetDlgItemInt(hwnd, IDC_P_WIDTH_EDIT, PALLET_X, FALSE);
            SetDlgItemInt(hwnd, IDC_P_HEIGHT_EDIT, PALLET_Y, FALSE);
            SetDlgItemInt(hwnd, IDC_P_DEPTH_EDIT, PALLET_Z, FALSE);
            SetDlgItemInt(hwnd, IDC_P_MAX_MASS_EDIT, PALLET_MAX_MASS, FALSE);
        }

        SetDlgItemInt(hwnd, IDC_WIDTH_EDIT, 300, FALSE);
        SetDlgItemInt(hwnd, IDC_HEIGHT_EDIT, 300, FALSE);
        SetDlgItemInt(hwnd, IDC_DEPTH_EDIT, 300, FALSE);
        SetDlgItemInt(hwnd, IDC_QUANTITY_EDIT, 10, FALSE);
        SetDlgItemInt(hwnd, IDC_WEIGHT_EDIT, 20, FALSE);

        CheckDlgButton(hwnd, IDC_FULL_ROTATE_CHECK, BST_UNCHECKED);

        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_ADD_BUTTON: {
            if (!g_pState) break;

            char buffer[32];

            GetDlgItemTextA(hwnd, IDC_WIDTH_EDIT, buffer, 32);
            int width = atoi(buffer);
            if (width <= 0) {
                MessageBoxW(hwnd, utf8_to_wstring("Введите корректную ширину (положительное число)").c_str(),
                    utf8_to_wstring("Ошибка").c_str(), MB_OK | MB_ICONERROR);
                break;
            }

            GetDlgItemTextA(hwnd, IDC_HEIGHT_EDIT, buffer, 32);
            int height = atoi(buffer);
            if (height <= 0) {
                MessageBoxW(hwnd, utf8_to_wstring("Введите корректную высоту (положительное число)").c_str(),
                    utf8_to_wstring("Ошибка").c_str(), MB_OK | MB_ICONERROR);
                break;
            }

            GetDlgItemTextA(hwnd, IDC_DEPTH_EDIT, buffer, 32);
            int depth = atoi(buffer);
            if (depth <= 0) {
                MessageBoxW(hwnd, utf8_to_wstring("Введите корректную глубину (положительное число)").c_str(),
                    utf8_to_wstring("Ошибка").c_str(), MB_OK | MB_ICONERROR);
                break;
            }

            GetDlgItemTextA(hwnd, IDC_QUANTITY_EDIT, buffer, 32);
            int quantity = atoi(buffer);
            if (quantity <= 0) {
                MessageBoxW(hwnd, utf8_to_wstring("Введите корректное количество (положительное число)").c_str(),
                    utf8_to_wstring("Ошибка").c_str(), MB_OK | MB_ICONERROR);
                break;
            }

            GetDlgItemTextA(hwnd, IDC_WEIGHT_EDIT, buffer, 32);
            int weight = atoi(buffer);
            if (weight <= 0) {
                MessageBoxW(hwnd, utf8_to_wstring("Введите корректный вес (положительное число)").c_str(),
                    utf8_to_wstring("Ошибка").c_str(), MB_OK | MB_ICONERROR);
                break;
            }

            bool full_rotate = IsDlgButtonChecked(hwnd, IDC_FULL_ROTATE_CHECK) == BST_CHECKED;

            box_property new_box = { quantity, width, height, depth, weight, full_rotate };
            g_pState->box_types.push_back(new_box);

            HWND hList = GetDlgItem(hwnd, IDC_BOX_LIST);
            char listBuffer[256];
            sprintf_s(listBuffer, "Размер: %dx%dx%d мм, Вес: %d кг, Кол-во: %d%s",
                width, height, depth, weight, quantity, full_rotate ? ", Поворот" : "");
            std::wstring wbuffer = utf8_to_wstring(listBuffer);
            SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)wbuffer.c_str());

            break;
        }

        case IDC_CLEAR_BUTTON: {
            if (!g_pState) break;

            g_pState->box_types.clear();

            HWND hList = GetDlgItem(hwnd, IDC_BOX_LIST);
            SendMessage(hList, LB_RESETCONTENT, 0, 0);

            break;
        }

        case IDC_START_BUTTON: {
            if (!g_pState) break;

            if (g_pState->box_types.empty()) {
                MessageBoxW(hwnd, utf8_to_wstring("Добавьте хотя бы один тип коробок!").c_str(),
                    utf8_to_wstring("Ошибка").c_str(), MB_OK | MB_ICONWARNING);
                break;
            }

            BOOL success;
            int pW = GetDlgItemInt(hwnd, IDC_P_WIDTH_EDIT, &success, FALSE);
            if (!success || pW <= 0) {
                MessageBoxW(hwnd, utf8_to_wstring("Некорректная ширина паллеты").c_str(),
                    utf8_to_wstring("Ошибка").c_str(), MB_OK | MB_ICONERROR);
                break;
            }
            int pH = GetDlgItemInt(hwnd, IDC_P_HEIGHT_EDIT, &success, FALSE);
            if (!success || pH <= 0) {
                MessageBoxW(hwnd, utf8_to_wstring("Некорректная высота паллеты").c_str(),
                    utf8_to_wstring("Ошибка").c_str(), MB_OK | MB_ICONERROR);
                break;
            }
            int pD = GetDlgItemInt(hwnd, IDC_P_DEPTH_EDIT, &success, FALSE);
            if (!success || pD <= 0) {
                MessageBoxW(hwnd, utf8_to_wstring("Некорректная глубина паллеты").c_str(),
                    utf8_to_wstring("Ошибка").c_str(), MB_OK | MB_ICONERROR);
                break;
            }
            int pMaxMass = GetDlgItemInt(hwnd, IDC_P_MAX_MASS_EDIT, &success, FALSE);
            if (!success || pMaxMass <= 0) {
                MessageBoxW(hwnd, utf8_to_wstring("Некорректный максимальный вес паллеты").c_str(),
                    utf8_to_wstring("Ошибка").c_str(), MB_OK | MB_ICONERROR);
                break;
            }
            bool center_mass_checked = IsDlgButtonChecked(hwnd, IDC_SET_CENTER_MASS) == BST_CHECKED;
            bool max_volume_checked = IsDlgButtonChecked(hwnd, IDC_SET_MAX_VOLUME) == BST_CHECKED;
            if (!center_mass_checked && !max_volume_checked) {
                MessageBoxW(hwnd, utf8_to_wstring("Выберите хотя бы один метод укладки").c_str(),
                    utf8_to_wstring("Ошибка выбора метода").c_str(), MB_OK | MB_ICONWARNING);
                break;
            }


            g_pState->pallet_width = pW;
            g_pState->pallet_height = pH;
            g_pState->pallet_depth = pD;
            g_pState->pallet_max_mass = pMaxMass;
			g_pState->use_center_mass = center_mass_checked;
			g_pState->use_max_volume = max_volume_checked;

            ShowWindow(hwnd, SW_HIDE);
            g_pState->show_input_window = false;
            g_pState->calculation_done = true;
            break;
        }
        }
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HWND CreateInputDialog(HINSTANCE hInstance, HWND hwndParent, GUIState* state) {
    g_pState = state;

    HWND hwndDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"STATIC",
        utf8_to_wstring("Настройка паллетизации").c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, 500, 760,  // Увеличили высоту окна
        hwndParent,
        NULL,
        hInstance,
        NULL
    );

    if (!hwndDlg) return NULL;

    RECT rc;
    GetWindowRect(hwndDlg, &rc);
    int x = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
    SetWindowPos(hwndDlg, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    int yPos = 20;
    int xLabel = 20;
    int xEdit = 180;
    int labelWidth = 150;
    int editWidth = 280;
    int lineHeight = 30;

    CreateWindowW(L"STATIC", utf8_to_wstring("Параметры паллеты (мм):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, 300, 20,
        hwndDlg, NULL, hInstance, NULL);
    yPos += 25;

    CreateWindowW(L"STATIC", utf8_to_wstring("Ширина (X):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20,
        hwndDlg, NULL, hInstance, NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        hwndDlg, (HMENU)IDC_P_WIDTH_EDIT, hInstance, NULL);
    yPos += lineHeight;

    CreateWindowW(L"STATIC", utf8_to_wstring("Высота (Y):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20,
        hwndDlg, NULL, hInstance, NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        hwndDlg, (HMENU)IDC_P_HEIGHT_EDIT, hInstance, NULL);
    yPos += lineHeight;

    CreateWindowW(L"STATIC", utf8_to_wstring("Глубина (Z):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20,
        hwndDlg, NULL, hInstance, NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        hwndDlg, (HMENU)IDC_P_DEPTH_EDIT, hInstance, NULL);
    yPos += lineHeight;

    // Новое поле: Максимальный вес паллеты
    CreateWindowW(L"STATIC", utf8_to_wstring("Макс. вес (кг):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20,
        hwndDlg, NULL, hInstance, NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        hwndDlg, (HMENU)IDC_P_MAX_MASS_EDIT, hInstance, NULL);
    yPos += lineHeight + 10;

    CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        xLabel, yPos, 440, 2, hwndDlg, NULL, hInstance, NULL);
    yPos += 15;

    CreateWindowW(L"STATIC", utf8_to_wstring("Добавление коробок:").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, 300, 20,
        hwndDlg, NULL, hInstance, NULL);
    yPos += 25;

    CreateWindowW(L"STATIC", utf8_to_wstring("Ширина (мм):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20,
        hwndDlg, NULL, hInstance, NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"3",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        hwndDlg, (HMENU)IDC_WIDTH_EDIT, hInstance, NULL);
    yPos += lineHeight;

    CreateWindowW(L"STATIC", utf8_to_wstring("Высота (мм):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20,
        hwndDlg, NULL, hInstance, NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"3",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        hwndDlg, (HMENU)IDC_HEIGHT_EDIT, hInstance, NULL);
    yPos += lineHeight;

    CreateWindowW(L"STATIC", utf8_to_wstring("Глубина (мм):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20,
        hwndDlg, NULL, hInstance, NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"3",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        hwndDlg, (HMENU)IDC_DEPTH_EDIT, hInstance, NULL);
    yPos += lineHeight;

    CreateWindowW(L"STATIC", utf8_to_wstring("Количество:").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20,
        hwndDlg, NULL, hInstance, NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        hwndDlg, (HMENU)IDC_QUANTITY_EDIT, hInstance, NULL);
    yPos += lineHeight;

    CreateWindowW(L"STATIC", utf8_to_wstring("Вес (кг):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20,
        hwndDlg, NULL, hInstance, NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"2",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        hwndDlg, (HMENU)IDC_WEIGHT_EDIT, hInstance, NULL);
    yPos += lineHeight;

    CreateWindowW(L"BUTTON", utf8_to_wstring("Полный поворот коробок?").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        xEdit, yPos, 200, 20,
        hwndDlg, (HMENU)IDC_FULL_ROTATE_CHECK, hInstance, NULL);
    yPos += lineHeight + 15;

    CreateWindowW(L"BUTTON", utf8_to_wstring("Добавить коробки").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        xLabel, yPos, 140, 30,
        hwndDlg, (HMENU)IDC_ADD_BUTTON, hInstance, NULL);

    CreateWindowW(L"BUTTON", utf8_to_wstring("Очистить список").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        xLabel + 150, yPos, 140, 30,
        hwndDlg, (HMENU)IDC_CLEAR_BUTTON, hInstance, NULL);

    yPos += 40;

    CreateWindowW(L"STATIC", utf8_to_wstring("Добавленные типы коробок:").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, 300, 20,
        hwndDlg, NULL, hInstance, NULL);
    yPos += 20;

    CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        xLabel, yPos, 440, 130,
        hwndDlg, (HMENU)IDC_BOX_LIST, hInstance, NULL);
    yPos += 125;

    CreateWindowW(L"STATIC", utf8_to_wstring("Установка:").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, 300, 20,
        hwndDlg, NULL, hInstance, NULL);
    yPos += 25;

    CreateWindowW(L"BUTTON", utf8_to_wstring("По центру масс").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        xLabel, yPos, 200, 20,
        hwndDlg, (HMENU)IDC_SET_CENTER_MASS, hInstance, NULL);

    CreateWindowW(L"BUTTON", utf8_to_wstring("По максимальному объёму").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        xLabel + 200 + 30, yPos, 200, 20,
        hwndDlg, (HMENU)IDC_SET_MAX_VOLUME, hInstance, NULL);

    yPos += lineHeight;

    CreateWindowW(L"BUTTON", utf8_to_wstring("Запустить расчет укладки").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
        xLabel, yPos, 440, 40,
        hwndDlg, (HMENU)IDC_START_BUTTON, hInstance, NULL);

    return hwndDlg;
}

bool show_input_dialog(GUIState* state) {
    g_hDlgWnd = CreateInputDialog(GetModuleHandle(NULL), NULL, state);

    if (!g_hDlgWnd) {
        MessageBoxW(NULL, utf8_to_wstring("Не удалось создать окно ввода!").c_str(),
            utf8_to_wstring("Ошибка").c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    SetWindowLongPtr(g_hDlgWnd, GWLP_WNDPROC, (LONG_PTR)DialogWindowProc);

    SendMessage(g_hDlgWnd, WM_INITDIALOG, (WPARAM)g_hDlgWnd, 0);

    ShowWindow(g_hDlgWnd, SW_SHOW);
    UpdateWindow(g_hDlgWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(g_hDlgWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!IsWindowVisible(g_hDlgWnd)) {
            break;
        }
    }

    return true;
}