#pragma execution_character_set("utf-8")
#include "graphics.h"
#include "functions.h"
#include "utils.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <cmath>
#include <string>
#include <sstream>

#pragma comment(lib, "comctl32.lib")

static HWND g_hWnd = NULL;
static HWND g_hTabControl = NULL;
static HWND g_hCalcButton = NULL;
static HDC g_hDC = NULL;
static bool g_running = true;
static int g_currentTab = 0; // 0 - ввод, 1 - визуализация, 2 визуализация

// ID элементов управления
#define IDC_TAB_CONTROL 2000
#define IDC_CALC_BUTTON 2001

// ID для вкладки ввода данных
#define IDC_P_WIDTH_EDIT    3001
#define IDC_P_HEIGHT_EDIT   3002
#define IDC_P_DEPTH_EDIT    3003
#define IDC_P_MAX_MASS_EDIT 3004
#define IDC_WIDTH_EDIT      3005
#define IDC_HEIGHT_EDIT     3006
#define IDC_DEPTH_EDIT      3007
#define IDC_QUANTITY_EDIT   3008
#define IDC_WEIGHT_EDIT     3009
#define IDC_FULL_ROTATE_CHECK 3010
#define IDC_SET_CENTER_MASS 3011
#define IDC_SET_MAX_VOLUME  3012
#define IDC_ADD_BUTTON      3013
#define IDC_CLEAR_BUTTON    3014
#define IDC_BOX_LIST        3015
#define IDC_DELETE_BUTTON   3016

// Дескрипторы элементов управления для вкладки ввода
static HWND g_hPWidthEdit = NULL;
static HWND g_hPHeightEdit = NULL;
static HWND g_hPDepthEdit = NULL;
static HWND g_hPMaxMassEdit = NULL;
static HWND g_hWidthEdit = NULL;
static HWND g_hHeightEdit = NULL;
static HWND g_hDepthEdit = NULL;
static HWND g_hQuantityEdit = NULL;
static HWND g_hWeightEdit = NULL;
static HWND g_hFullRotateCheck = NULL;
static HWND g_hCenterMassCheck = NULL;
static HWND g_hMaxVolumeCheck = NULL;
static HWND g_hAddButton = NULL;
static HWND g_hClearButton = NULL;
static HWND g_hDeleteButton = NULL;
static HWND g_hBoxList = NULL;

// Статические метки
static HWND g_hPalletLabel = NULL;
static HWND g_hPWidthLabel = NULL;
static HWND g_hPHeightLabel = NULL;
static HWND g_hPDepthLabel = NULL;
static HWND g_hPMaxMassLabel = NULL;
static HWND g_hBoxesLabel = NULL;
static HWND g_hWidthLabel = NULL;
static HWND g_hHeightLabel = NULL;
static HWND g_hDepthLabel = NULL;
static HWND g_hQuantityLabel = NULL;
static HWND g_hWeightLabel = NULL;
static HWND g_hMethodLabel = NULL;
static HWND g_hBoxListLabel = NULL;

struct Point3D {
    float x, y, z;
};

struct Point2D {
    int x, y;
};

static Camera g_camera;
static GUIState g_state;
static int g_windowWidth = 1200;
static int g_windowHeight = 720;
static void hide_delete_button() {
    if (g_hDeleteButton) {
        ShowWindow(g_hDeleteButton, SW_HIDE);
    }
}

static void update_delete_button_position() {
    if (!g_hWnd || !g_hBoxList || !g_hDeleteButton) return;

    int sel = (int)SendMessageW(g_hBoxList, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR) {
        hide_delete_button();
        return;
    }

    RECT itemRc{};
    if (SendMessageW(g_hBoxList, LB_GETITEMRECT, (WPARAM)sel, (LPARAM)&itemRc) == LB_ERR) {
        hide_delete_button();
        return;
    }

    POINT pt{ itemRc.right, itemRc.top };
    ClientToScreen(g_hBoxList, &pt);
    ScreenToClient(g_hWnd, &pt);

    const int margin = 4;
    int btnW = 40;
    int btnH = itemRc.bottom - itemRc.top;
    if (btnH < 18) btnH = 18;

    int x = pt.x + margin;
    int y = pt.y;

    RECT listScreenRc{};
    GetWindowRect(g_hBoxList, &listScreenRc);
    POINT listTL{ listScreenRc.left, listScreenRc.top };
    POINT listBR{ listScreenRc.right, listScreenRc.bottom };
    ScreenToClient(g_hWnd, &listTL);
    ScreenToClient(g_hWnd, &listBR);

    if (x + btnW > listBR.x) {
        x = listTL.x - btnW - margin;
    }

    SetWindowPos(g_hDeleteButton, NULL, x, y, btnW, btnH, SWP_NOZORDER);
    ShowWindow(g_hDeleteButton, SW_SHOW);
}


void show_input_controls(bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    
    ShowWindow(g_hPalletLabel, cmd);
    ShowWindow(g_hPWidthLabel, cmd);
    ShowWindow(g_hPWidthEdit, cmd);
    ShowWindow(g_hPHeightLabel, cmd);
    ShowWindow(g_hPHeightEdit, cmd);
    ShowWindow(g_hPDepthLabel, cmd);
    ShowWindow(g_hPDepthEdit, cmd);
    ShowWindow(g_hPMaxMassLabel, cmd);
    ShowWindow(g_hPMaxMassEdit, cmd);
    
    ShowWindow(g_hBoxesLabel, cmd);
    ShowWindow(g_hWidthLabel, cmd);
    ShowWindow(g_hWidthEdit, cmd);
    ShowWindow(g_hHeightLabel, cmd);
    ShowWindow(g_hHeightEdit, cmd);
    ShowWindow(g_hDepthLabel, cmd);
    ShowWindow(g_hDepthEdit, cmd);
    ShowWindow(g_hQuantityLabel, cmd);
    ShowWindow(g_hQuantityEdit, cmd);
    ShowWindow(g_hWeightLabel, cmd);
    ShowWindow(g_hWeightEdit, cmd);
    ShowWindow(g_hFullRotateCheck, cmd);
    
    ShowWindow(g_hAddButton, cmd);
    ShowWindow(g_hClearButton, cmd);
    ShowWindow(g_hBoxListLabel, cmd);
    ShowWindow(g_hBoxList, cmd);
    
    ShowWindow(g_hMethodLabel, cmd);
    ShowWindow(g_hCenterMassCheck, cmd);
    ShowWindow(g_hMaxVolumeCheck, cmd);
	ShowWindow(g_hCalcButton, cmd);

    if (show) update_delete_button_position();
    else hide_delete_button();
}


void reset_state_for_new_packing() {

    for (auto* pal : g_state.current_pallet) {
        if (pal) {
            for (auto* box_ptr : pal->placed_boxes) {
                delete box_ptr;
            }
            pal->placed_boxes.clear();

            // Удаляем зоны
            for (auto* z : pal->zone_vector) {
                delete z;
            }
            pal->zone_vector.clear();

            for (auto* z : pal->zone_dead_vector) {
                delete z;
            }
            pal->zone_dead_vector.clear();

            pal->placed_boxes.clear();

            delete pal;
            g_state.current_pallet = { nullptr, nullptr };
        }
	}
    

    g_state.calculation_done = false;
    g_state.use_center_mass = false;
    g_state.use_max_volume = false;
}

void calculate_packing() {
    vector<box*> total_boxes;

    if (g_state.box_types.empty()) {
        MessageBoxW(g_hWnd, L"Добавьте хотя бы один тип коробок!", L"Ошибка", MB_OK | MB_ICONWARNING);
        return;
    }

    // Получаем параметры паллеты
    BOOL success;
    int pW = GetDlgItemInt(g_hWnd, IDC_P_WIDTH_EDIT, &success, FALSE);
    if (!success || pW <= 0) {
        MessageBoxW(g_hWnd, L"Некорректная ширина паллеты", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }
    int pH = GetDlgItemInt(g_hWnd, IDC_P_HEIGHT_EDIT, &success, FALSE);
    if (!success || pH <= 0) {
        MessageBoxW(g_hWnd, L"Некорректная высота паллеты", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }
    int pD = GetDlgItemInt(g_hWnd, IDC_P_DEPTH_EDIT, &success, FALSE);
    if (!success || pD <= 0) {
        MessageBoxW(g_hWnd, L"Некорректная глубина паллеты", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }
    int pMaxMass = GetDlgItemInt(g_hWnd, IDC_P_MAX_MASS_EDIT, &success, FALSE);
    if (!success || pMaxMass <= 0) {
        MessageBoxW(g_hWnd, L"Некорректный максимальный вес паллеты", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    bool center_mass_checked = IsDlgButtonChecked(g_hWnd, IDC_SET_CENTER_MASS) == BST_CHECKED;
    bool max_volume_checked = IsDlgButtonChecked(g_hWnd, IDC_SET_MAX_VOLUME) == BST_CHECKED;
    
    if (!center_mass_checked && !max_volume_checked) {
        MessageBoxW(g_hWnd, L"Выберите хотя бы один метод укладки", L"Ошибка", MB_OK | MB_ICONWARNING);
        return;
    }

    g_state.pallet_width = pW;
    g_state.pallet_height = pH;
    g_state.pallet_depth = pD;
    g_state.pallet_max_mass = pMaxMass;
    g_state.use_center_mass = center_mass_checked;
    g_state.use_max_volume = max_volume_checked;

    update_tabs();

    //// Создаём коробки из введённых типов
    //int box_len = 0;
    //for (const auto& box_type : g_state.box_types) {
    //    box_len += box_type.Quantity;
    //}
    //total_boxes.reserve(box_len);

    //for (const auto& box_type : g_state.box_types) {
    //    for (int i = 0; i < box_type.Quantity; ++i) {
    //        box* new_box = new box();
    //        new_box->xyz_size[0][0] = box_type.width;
    //        new_box->xyz_size[0][1] = box_type.height;
    //        new_box->xyz_size[0][2] = box_type.depth;

    //        new_box->xyz_size[1][2] = box_type.width;
    //        new_box->xyz_size[1][1] = box_type.height;
    //        new_box->xyz_size[1][0] = box_type.depth;

    //        new_box->xyz_size[2][0] = box_type.width;
    //        new_box->xyz_size[2][2] = box_type.height;
    //        new_box->xyz_size[2][1] = box_type.depth;

    //        new_box->mass = box_type.weight;
    //        new_box->placed = false;
    //        new_box->full_rotateble = box_type.full_rotateble;

    //        total_boxes.push_back(new_box);
    //    }
    //}
    auto create_box_copies = [&]() -> vector<box*> {
        vector<box*> boxes;
        int box_len = 0;
        for (const auto& box_type : g_state.box_types) {
            box_len += box_type.Quantity;
        }
        boxes.reserve(box_len);

        for (const auto& box_type : g_state.box_types) {
            for (int i = 0; i < box_type.Quantity; ++i) {
                box* new_box = new box();
                new_box->xyz_size[0][0] = box_type.width;
                new_box->xyz_size[0][1] = box_type.height;
                new_box->xyz_size[0][2] = box_type.depth;

                new_box->xyz_size[1][2] = box_type.width;
                new_box->xyz_size[1][1] = box_type.height;
                new_box->xyz_size[1][0] = box_type.depth;

                new_box->xyz_size[2][0] = box_type.width;
                new_box->xyz_size[2][2] = box_type.height;
                new_box->xyz_size[2][1] = box_type.depth;

                new_box->mass = box_type.weight;
                new_box->placed = false;
                new_box->full_rotateble = box_type.full_rotateble;

                boxes.push_back(new_box);
            }
        }
        return boxes;
        };
    
    if (g_state.use_center_mass) {
        cout << "Метод укладки: Центр масс\n";
		int center_mass_or_max_volume = 0;
        g_state.current_pallet[0] = (new pallet(
            g_state.pallet_width,
            g_state.pallet_height,
            g_state.pallet_depth,
            g_state.pallet_max_mass,
            center_mass_or_max_volume
        ));
    }
    if (g_state.use_max_volume) {
        cout << "Метод укладки: Максимальный объем\n";
        int center_mass_or_max_volume = 1;
        g_state.current_pallet[1] = (new pallet(
            g_state.pallet_width,
            g_state.pallet_height,
            g_state.pallet_depth,
            g_state.pallet_max_mass,
            center_mass_or_max_volume
        ));
    }



    for (auto* pal : g_state.current_pallet) {
        vector<box*> boxs = total_boxes;
        if (pal == nullptr) {
            continue;
        }
		int index = 0;

        vector<box*> total_boxes = create_box_copies();

        g_camera.targetX = pal->xyz_size[0] / 2.0f;
        g_camera.targetY = pal->xyz_size[1] / 2.0f;
        g_camera.targetZ = pal->xyz_size[2] / 2.0f;
        g_camera.distance = pal->xyz_size[0] * 2.0f;

        // Запускаем расчёт
        cout << "\nЗапуск расчёта укладки...\n";
        cout << "Макс. вес паллеты: " << pal->max_mass << " кг\n";
        pallet_handle(pal, total_boxes);
        cout << "Расчёт завершён! Размещено коробок: " << pal->placed_boxes.size() << " для " << index++ << "паллеты." << endl;
        // Освобождаем память
        for (auto* box_ptr : total_boxes) {
            if (box_ptr && !box_ptr->placed) {
                delete box_ptr;
            }
        }
    }
    g_state.calculation_done = true;


    // Переключаемся на вкладку визуализации
    g_currentTab = 1;
    TabCtrl_SetCurSel(g_hTabControl, g_currentTab);
    show_input_controls(false);
    
    InvalidateRect(g_hWnd, NULL, TRUE);
}

void start_new_packing() {
    reset_state_for_new_packing();
    
    //SendMessage(g_hBoxList, LB_RESETCONTENT, 0, 0);
    
    InvalidateRect(g_hWnd, NULL, TRUE);
}

Point2D project_3d_to_2d(float x, float y, float z, Camera* cam)  {
    float angleXRad = cam->angleX * 3.14159f / 180.0f;
    float angleYRad = cam->angleY * 3.14159f / 180.0f;

    float cx = x - cam->targetX;
    float cy = y - cam->targetY;
    float cz = z - cam->targetZ;

    float tx = cx * cos(angleYRad) - cz * sin(angleYRad);
    float tz = cx * sin(angleYRad) + cz * cos(angleYRad);
    cx = tx;
    cz = tz;

    float ty = cy * cos(angleXRad) - cz * sin(angleXRad);
    tz = cy * sin(angleXRad) + cz * cos(angleXRad);
    cy = ty;
    cz = tz;

    cz += cam->distance;

    float scale = 800.0f / (cz + 0.1f);

    Point2D result;
    result.x = (int)(g_windowWidth / 2 + cx * scale);
    result.y = (int)(g_windowHeight / 2 - cy * scale);

    return result;
}

void draw_line_3d(HDC hdc, float x1, float y1, float z1, float x2, float y2, float z2, Camera* cam, COLORREF color, float size = 1.0) {
    Point2D p1 = project_3d_to_2d(x1, y1, z1, cam);
    Point2D p2 = project_3d_to_2d(x2, y2, z2, cam);

    HPEN hPen = CreatePen(PS_SOLID, size, color);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    MoveToEx(hdc, p1.x, p1.y, NULL);
    LineTo(hdc, p2.x, p2.y);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void fill_rect_3d(HDC hdc, float x1, float y1, float z1, float x2, float y2, float z2,
    float x3, float y3, float z3, float x4, float y4, float z4,
    Camera* cam, COLORREF color) {
    Point2D p1 = project_3d_to_2d(x1, y1, z1, cam);
    Point2D p2 = project_3d_to_2d(x2, y2, z2, cam);
    Point2D p3 = project_3d_to_2d(x3, y3, z3, cam);
    Point2D p4 = project_3d_to_2d(x4, y4, z4, cam);

    POINT points[4] = {
        {p1.x, p1.y},
        {p2.x, p2.y},
        {p3.x, p3.y},
        {p4.x, p4.y}
    };

    HBRUSH hBrush = CreateSolidBrush(color);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    Polygon(hdc, points, 4);

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

void draw_box_3d(HDC hdc, box* box_ptr, int index, Camera* cam) {
    float x = (float)box_ptr->xyz[0];
    float y = (float)box_ptr->xyz[1];
    float z = (float)box_ptr->xyz[2];

    int r = box_ptr->rotate;
    float w = (float)box_ptr->xyz_size[r][0];
    float h = (float)box_ptr->xyz_size[r][1];
    float d = (float)box_ptr->xyz_size[r][2];

    BYTE red = (BYTE)((index * 67) % 200 + 55);
    BYTE green = (BYTE)((index * 131) % 200 + 55);
    BYTE blue = (BYTE)((index * 197) % 200 + 55);
    COLORREF color = RGB(red, green, blue);

    fill_rect_3d(hdc, x, y, z, x + w, y, z, x + w, y + h, z, x, y + h, z, cam, color);
    fill_rect_3d(hdc, x, y, z + d, x + w, y, z + d, x + w, y + h, z + d, x, y + h, z + d, cam, color);

    COLORREF colorTop = RGB(min(255, red + 30), min(255, green + 30), min(255, blue + 30));
    fill_rect_3d(hdc, x, y + h, z, x + w, y + h, z, x + w, y + h, z + d, x, y + h, z + d, cam, colorTop);

    COLORREF colorBot = RGB(max(0, red - 30), max(0, green - 30), max(0, blue - 30));
    fill_rect_3d(hdc, x, y, z, x + w, y, z, x + w, y, z + d, x, y, z + d, cam, colorBot);

    fill_rect_3d(hdc, x, y, z, x, y + h, z, x, y + h, z + d, x, y, z + d, cam, color);
    fill_rect_3d(hdc, x + w, y, z, x + w, y + h, z, x + w, y + h, z + d, x + w, y, z + d, cam, color);
}

void draw_pallet_base(HDC hdc, Camera* cam, pallet* pal_ptr) {
    COLORREF palletColor = RGB(139, 90, 43);

    float px = (float)pal_ptr->xyz_size[0];
    float pz = (float)pal_ptr->xyz_size[2];

    fill_rect_3d(hdc, 0, -0.5f, 0, px, -0.5f, 0,
        px, 0, 0, 0, 0, 0, cam, palletColor);
    fill_rect_3d(hdc, 0, -0.5f, pz, px, -0.5f, pz,
        px, 0, pz, 0, 0, pz, cam, palletColor);
    fill_rect_3d(hdc, 0, -0.5f, 0, 0, 0, 0,
        0, 0, pz, 0, -0.5f, pz, cam, palletColor);
    fill_rect_3d(hdc, px, -0.5f, 0, px, 0, 0,
        px, 0, pz, px, -0.5f, pz, cam, palletColor);
}

void draw_grid(HDC hdc, Camera* cam, pallet* pal_ptr) {
    COLORREF gridColor = RGB(200, 200, 200);

    int px = pal_ptr->xyz_size[0];
    int pz = pal_ptr->xyz_size[2];

    for (int i = 0; i <= px; i++) {
        draw_line_3d(hdc, (float)i, 0, 0, (float)i, 0, (float)pz, cam, gridColor);
    }

    for (int i = 0; i <= pz; i++) {
        draw_line_3d(hdc, 0, 0, (float)i, (float)px, 0, (float)i, cam, gridColor);
    }
}

void draw_sphere_3d(HDC hdc, float cx, float cy, float cz, float radius, Camera* cam, COLORREF color) {
    const int segments = 12;
    
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i * 3.14159f * 2.0f / segments;
        float angle2 = (float)(i + 1) * 3.14159f * 2.0f / segments;
        
        float x1 = cx + radius * cos(angle1);
        float y1 = cy + radius * sin(angle1);
        float x2 = cx + radius * cos(angle2);
        float y2 = cy + radius * sin(angle2);
        
        draw_line_3d(hdc, x1, y1, cz, x2, y2, cz, cam, color);
    }
    
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i * 3.14159f * 2.0f / segments;
        float angle2 = (float)(i + 1) * 3.14159f * 2.0f / segments;
        
        float x1 = cx + radius * cos(angle1);
        float z1 = cz + radius * sin(angle1);
        float x2 = cx + radius * cos(angle2);
        float z2 = cz + radius * sin(angle2);
        
        draw_line_3d(hdc, x1, cy, z1, x2, cy, z2, cam, color);
    }
    
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i * 3.14159f * 2.0f / segments;
        float angle2 = (float)(i + 1) * 3.14159f * 2.0f / segments;
        
        float y1 = cy + radius * cos(angle1);
        float z1 = cz + radius * sin(angle1);
        float y2 = cy + radius * cos(angle2);
        float z2 = cz + radius * sin(angle2);
        
        draw_line_3d(hdc, cx, y1, z1, cx, y2, z2, cam, color);
    }
}

void draw_center_of_mass(HDC hdc, pallet* pal_ptr, Camera* cam) {
    if (!pal_ptr || pal_ptr->placed_boxes.empty()) return;
    
    float com_x = (float)pal_ptr->xyz_mass_centre[0];
    float com_y = (float)pal_ptr->xyz_mass_centre[1];
    float com_z = (float)pal_ptr->xyz_mass_centre[2];
    
    draw_sphere_3d(hdc, com_x, com_y, com_z, 15.0f, cam, RGB(255, 0, 0));
    draw_line_3d(hdc, com_x, 0, com_z, com_x, com_y, com_z, cam, RGB(255, 100, 100));
    
    const int circle_segments = 16;
    float base_radius = 20.0f;
    for (int i = 0; i < circle_segments; i++) {
        float angle1 = (float)i * 3.14159f * 2.0f / circle_segments;
        float angle2 = (float)(i + 1) * 3.14159f * 2.0f / circle_segments;
        
        float x1 = com_x + base_radius * cos(angle1);
        float z1 = com_z + base_radius * sin(angle1);
        float x2 = com_x + base_radius * cos(angle2);
        float z2 = com_z + base_radius * sin(angle2);
        
        draw_line_3d(hdc, x1, 0.1f, z1, x2, 0.1f, z2, cam, RGB(255, 150, 150));
    }
    
    float ideal_x = (float)pal_ptr->ideal_cx;
    float ideal_z = (float)pal_ptr->ideal_cz;
    
    float ideal_radius = 15.0f;
    for (int i = 0; i < circle_segments; i++) {
        float angle1 = (float)i * 3.14159f * 2.0f / circle_segments;
        float angle2 = (float)(i + 1) * 3.14159f * 2.0f / circle_segments;
        
        float x1 = ideal_x + ideal_radius * cos(angle1);
        float z1 = ideal_z + ideal_radius * sin(angle1);
        float x2 = ideal_x + ideal_radius * cos(angle2);
        float z2 = ideal_z + ideal_radius * sin(angle2);
        
        draw_line_3d(hdc, x1, 0.1f, z1, x2, 0.1f, z2, cam, RGB(0, 200, 0));
    }
}

void draw_axes(HDC hdc, Camera* cam, pallet* pal_ptr) {
    float px = (float)pal_ptr->xyz_size[0];
    float py = (float)pal_ptr->xyz_size[1];
    float pz = (float)pal_ptr->xyz_size[2];

    draw_line_3d(hdc, 0, 0, 0, px * 2, 0, 0, cam, RGB(255, 0, 0), 1.5);
    draw_line_3d(hdc, 0, 0, 0, 0, py * 2, 0, cam, RGB(0, 255, 0), 1.5);
    draw_line_3d(hdc, 0, 0, 0, 0, 0, pz * 2, cam, RGB(0, 0, 255), 1.5);
}

void render_3d_scene(HDC hdc, pallet* pal_ptr, Camera* cam) {
    if (!pal_ptr) return;

    draw_axes(hdc, cam, pal_ptr);
    draw_grid(hdc, cam, pal_ptr);
    draw_pallet_base(hdc, cam, pal_ptr);

    int index = 0;
    for (auto& box_ptr : pal_ptr->placed_boxes) {
        draw_box_3d(hdc, box_ptr, index++, cam);
    }
    
    draw_center_of_mass(hdc, pal_ptr, cam);
}

void draw_text_line(HDC hdc, int x, int y, const char* text, COLORREF color = RGB(0, 0, 0)) {
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);

    std::wstring wtext = utf8_to_wstring(text);
    TextOutW(hdc, x, y, wtext.c_str(), (int)wtext.length());
}

void render_stats_panel(HDC hdc, pallet* pal_ptr, RECT* panelRect) {
    if (!pal_ptr || !pal_ptr->placed_boxes.size()) return;

    HBRUSH hBrush = CreateSolidBrush(RGB(250, 250, 250));
    FillRect(hdc, panelRect, hBrush);
    DeleteObject(hBrush);

    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    Rectangle(hdc, panelRect->left, panelRect->top, panelRect->right, panelRect->bottom);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    int yPos = panelRect->top + 20;
    int xPos = panelRect->left + 20;

    HFONT hFont = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    draw_text_line(hdc, xPos, yPos, "Статистика укладки", RGB(0, 128, 0));
    yPos += 40;

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    hOldFont = (HFONT)SelectObject(hdc, hFont);

    char buffer[256];

    sprintf_s(buffer, "Размещено коробок: %d", (int)pal_ptr->placed_boxes.size());
    draw_text_line(hdc, xPos, yPos, buffer);
    yPos += 25;

    sprintf_s(buffer, "Макс. высота: %d см", pal_ptr->max_height_box);
    draw_text_line(hdc, xPos, yPos, buffer);
    yPos += 25;

    sprintf_s(buffer, "Вес: %d / %d кг", pal_ptr->total_mass, pal_ptr->max_mass);
    draw_text_line(hdc, xPos, yPos, buffer);
    yPos += 25;

    int total_volume = pal_ptr->xyz_size[0] * pal_ptr->xyz_size[1] * pal_ptr->xyz_size[2];
    int used_volume = 0;
    for (const auto& box_ptr : pal_ptr->placed_boxes) {
        int r = box_ptr->rotate;
        used_volume += box_ptr->xyz_size[r][0] * box_ptr->xyz_size[r][1] * box_ptr->xyz_size[r][2];
    }

    sprintf_s(buffer, "Заполнено: %.1f%%", (used_volume * 100.0) / total_volume);
    draw_text_line(hdc, xPos, yPos, buffer);
    yPos += 25;

    sprintf_s(buffer, "Объем: %d / %d м³", used_volume / 1000, total_volume / 1000);
    draw_text_line(hdc, xPos, yPos, buffer);
    yPos += 30;

    draw_text_line(hdc, xPos, yPos, "Центр масс:", RGB(0, 0, 128));
    yPos += 25;

    sprintf_s(buffer, "X: %d мм (идеал: %d мм)", (int)pal_ptr->xyz_mass_centre[0], (int)pal_ptr->ideal_cx);
    draw_text_line(hdc, xPos + 10, yPos, buffer);
    yPos += 22;

    sprintf_s(buffer, "Y: %d мм (идеал: %d мм)", (int)pal_ptr->xyz_mass_centre[1], (int)pal_ptr->ideal_cy);
    draw_text_line(hdc, xPos + 10, yPos, buffer);
    yPos += 22;

    sprintf_s(buffer, "Z: %d мм (идеал: %d мм)", (int)pal_ptr->xyz_mass_centre[2], (int)pal_ptr->ideal_cz);
    draw_text_line(hdc, xPos + 10, yPos, buffer);
    yPos += 30;

    draw_text_line(hdc, xPos, yPos, "Управление:", RGB(0, 0, 128));
    yPos += 25;
    draw_text_line(hdc, xPos + 10, yPos, "• Колесико мыши - зум");
    yPos += 22;
    draw_text_line(hdc, xPos + 10, yPos, "• ПКМ + движение - поворот");

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static bool isDragging = false;
    static POINT lastMousePos;

    switch (uMsg) {
    case WM_CLOSE:
        g_running = false;
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CALC_BUTTON) {
            if (g_currentTab == 0) {
                if (g_state.calculation_done) {
                    int result = MessageBoxW(hwnd, L"Укладка уже рассчитана. Хотите начать новую укладку?", L"Подтверждение", MB_YESNO | MB_ICONQUESTION);
                    if (result == IDYES) {
                        reset_state_for_new_packing();
                        calculate_packing();
                    }
                }
                else {
                    calculate_packing();
                }
                return 0;
            }
        }
        else if (LOWORD(wParam) == IDC_ADD_BUTTON) {
            // Добавить коробку
            char buffer[32];

            GetDlgItemTextA(hwnd, IDC_WIDTH_EDIT, buffer, 32);
            int width = atoi(buffer);
            if (width <= 0) {
                MessageBoxW(hwnd, L"Введите корректную ширину", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }

            GetDlgItemTextA(hwnd, IDC_HEIGHT_EDIT, buffer, 32);
            int height = atoi(buffer);
            if (height <= 0) {
                MessageBoxW(hwnd, L"Введите корректную высоту", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }

            GetDlgItemTextA(hwnd, IDC_DEPTH_EDIT, buffer, 32);
            int depth = atoi(buffer);
            if (depth <= 0) {
                MessageBoxW(hwnd, L"Введите корректную глубину", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }

            GetDlgItemTextA(hwnd, IDC_QUANTITY_EDIT, buffer, 32);
            int quantity = atoi(buffer);
            if (quantity <= 0) {
                MessageBoxW(hwnd, L"Введите корректное количество", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }

            GetDlgItemTextA(hwnd, IDC_WEIGHT_EDIT, buffer, 32);
            int weight = atoi(buffer);
            if (weight <= 0) {
                MessageBoxW(hwnd, L"Введите корректный вес", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }

            bool full_rotate = IsDlgButtonChecked(hwnd, IDC_FULL_ROTATE_CHECK) == BST_CHECKED;

            box_property new_box = { quantity, width, height, depth, weight, full_rotate };
            g_state.box_types.push_back(new_box);

            char listBuffer[256];
            sprintf_s(listBuffer, "Размер: %dx%dx%d мм, Вес: %d кг, Кол-во: %d%s",
                width, height, depth, weight, quantity, full_rotate ? ", Поворот" : "");
            std::wstring wbuffer = utf8_to_wstring(listBuffer);
            SendMessageW(g_hBoxList, LB_ADDSTRING, 0, (LPARAM)wbuffer.c_str());

            return 0;
        }
        else if (LOWORD(wParam) == IDC_CLEAR_BUTTON) {
            g_state.box_types.clear();
            SendMessage(g_hBoxList, LB_RESETCONTENT, 0, 0);
            hide_delete_button();
            return 0;
        }
        else if (LOWORD(wParam) == IDC_BOX_LIST && HIWORD(wParam) == LBN_SELCHANGE) {
            update_delete_button_position();
            return 0;
        }
		else if (LOWORD(wParam) == IDC_DELETE_BUTTON) { // Удаление выбранной коробки
            int sel = (int)SendMessageW(g_hBoxList, LB_GETCURSEL, 0, 0);
            if (sel == LB_ERR) {
                MessageBoxW(hwnd, L"Выберите строку в списке, чтобы удалить.", L"Удаление", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            if (sel >= 0 && sel < (int)g_state.box_types.size()) {
                g_state.box_types.erase(g_state.box_types.begin() + sel);
            }

            SendMessageW(g_hBoxList, LB_DELETESTRING, (WPARAM)sel, 0);

            int newCount = (int)SendMessageW(g_hBoxList, LB_GETCOUNT, 0, 0);
            if (newCount > 0) {
                int newSel = min(sel, newCount - 1);
                SendMessageW(g_hBoxList, LB_SETCURSEL, (WPARAM)newSel, 0);
            }
            update_delete_button_position();
            return 0;
        }
        break;

    case WM_NOTIFY: {
        LPNMHDR pnmhdr = (LPNMHDR)lParam;
        if (pnmhdr->idFrom == IDC_TAB_CONTROL && pnmhdr->code == TCN_SELCHANGE) {
            g_currentTab = TabCtrl_GetCurSel(g_hTabControl);
            
            if (g_currentTab == 0) {
                // Вкладка ввода
                show_input_controls(true);
            } else {
                // Вкладка визуализации
                show_input_controls(false);
            }
            
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        if (g_currentTab != 0) { // Только на вкладке визуализации
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            g_camera.distance -= delta / 10.0f * 2.0f;
            if (g_camera.distance < 100) g_camera.distance = 100;
            if (g_camera.distance > 5000) g_camera.distance = 5000;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_RBUTTONDOWN:
        if (g_currentTab != 0) {
            isDragging = true;
            lastMousePos.x = GET_X_LPARAM(lParam);
            lastMousePos.y = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
        }
        return 0;

    case WM_RBUTTONUP:
        isDragging = false;
        ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE:
        if (isDragging && g_currentTab != 0) {
            POINT currentPos;
            currentPos.x = GET_X_LPARAM(lParam);
            currentPos.y = GET_Y_LPARAM(lParam);

            int dx = currentPos.x - lastMousePos.x;
            int dy = currentPos.y - lastMousePos.y;

            g_camera.angleY += dx * 0.5f;
            g_camera.angleX += dy * 0.5f;

            if (g_camera.angleX > 89.0f) g_camera.angleX = 89.0f;
            if (g_camera.angleX < -89.0f) g_camera.angleX = -89.0f;

            lastMousePos = currentPos;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, g_windowWidth, g_windowHeight);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdcMem, &clientRect, hBrush);
        DeleteObject(hBrush);

        // Отрисовка в зависимости от активной вкладки
        if (g_currentTab == 1 && g_state.current_pallet[0]) {
            RECT renderRect = clientRect;
            renderRect.top = 55;

            FillRect(hdcMem, &renderRect, hBrush);

            HRGN hRgn = CreateRectRgn(renderRect.left, renderRect.top, renderRect.right, renderRect.bottom);
            SelectClipRgn(hdcMem, hRgn);

            render_3d_scene(hdcMem, g_state.current_pallet[0], &g_camera);

            if (g_state.calculation_done) {
                RECT statsPanel = { g_windowWidth - 380, 60, g_windowWidth - 10, 520 };
                render_stats_panel(hdcMem, g_state.current_pallet[0], &statsPanel);
            }

            SelectClipRgn(hdcMem, NULL);
            DeleteObject(hRgn);
        }
        if (g_currentTab == 2 && g_state.current_pallet[1]) {
            RECT renderRect = clientRect;
            renderRect.top = 55;

            FillRect(hdcMem, &renderRect, hBrush);

            HRGN hRgn = CreateRectRgn(renderRect.left, renderRect.top, renderRect.right, renderRect.bottom);
            SelectClipRgn(hdcMem, hRgn);

            render_3d_scene(hdcMem, g_state.current_pallet[1], &g_camera);

            if (g_state.calculation_done) {
                RECT statsPanel = { g_windowWidth - 380, 60, g_windowWidth - 10, 520 };
                render_stats_panel(hdcMem, g_state.current_pallet[1], &statsPanel);
            }

            SelectClipRgn(hdcMem, NULL);
            DeleteObject(hRgn);
        }
        BitBlt(hdc, 0, 0, g_windowWidth, g_windowHeight, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hwnd, &ps);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void update_tabs() {
    while (TabCtrl_GetItemCount(g_hTabControl) > 1) {
        TabCtrl_DeleteItem(g_hTabControl, 1);
    }

    TCITEMW tie;
    tie.mask = TCIF_TEXT;

    int tabIndex = 1;

    if (g_state.use_center_mass) {
        tie.pszText = (LPWSTR)L"Центр масс";
        TabCtrl_InsertItem(g_hTabControl, tabIndex++, &tie);
    }

    if (g_state.use_max_volume) {
        tie.pszText = (LPWSTR)L"Макс объём";
        TabCtrl_InsertItem(g_hTabControl, tabIndex++, &tie);
    }
}

bool init_graphics(int width, int height, const char* title) {
    g_windowWidth = width;
    g_windowHeight = height;

    // Инициализация Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"PalletVisualizerClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW; // Добавьте эту строку

    RegisterClassW(&wc);

    std::wstring wtitle = utf8_to_wstring(title);
    g_hWnd = CreateWindowExW(
        0,
        L"PalletVisualizerClass",
        wtitle.c_str(),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (!g_hWnd) return false;

    HINSTANCE hInst = GetModuleHandle(NULL);

    g_hTabControl = CreateWindowW(
        WC_TABCONTROLW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        0, 0, width - 16, 50,
        g_hWnd,
        (HMENU)IDC_TAB_CONTROL,
        hInst,
        NULL
    );

    // вкладки
    TCITEMW tie;
    tie.mask = TCIF_TEXT;
    
    tie.pszText = (LPWSTR)L"Ввод данных";
    TabCtrl_InsertItem(g_hTabControl, 0, &tie);

    TabCtrl_SetCurSel(g_hTabControl, 0);

    int yPos = 60;
    int xLabel = 20;
    int xEdit = 200;
    int labelWidth = 170;
    int editWidth = 200;
    int lineHeight = 30;

    // Параметры паллеты
    g_hPalletLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Параметры паллеты (мм):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, 300, 20, g_hWnd, NULL, hInst, NULL);
    yPos += 25;

    g_hPWidthLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Ширина (X):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20, g_hWnd, NULL, hInst, NULL);
    g_hPWidthEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        g_hWnd, (HMENU)IDC_P_WIDTH_EDIT, hInst, NULL);
    SetDlgItemInt(g_hWnd, IDC_P_WIDTH_EDIT, PALLET_X, FALSE);
    yPos += lineHeight;

    g_hPHeightLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Высота (Y):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20, g_hWnd, NULL, hInst, NULL);
    g_hPHeightEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        g_hWnd, (HMENU)IDC_P_HEIGHT_EDIT, hInst, NULL);
    SetDlgItemInt(g_hWnd, IDC_P_HEIGHT_EDIT, PALLET_Y, FALSE);
    yPos += lineHeight;

    g_hPDepthLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Глубина (Z):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20, g_hWnd, NULL, hInst, NULL);
    g_hPDepthEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        g_hWnd, (HMENU)IDC_P_DEPTH_EDIT, hInst, NULL);
    SetDlgItemInt(g_hWnd, IDC_P_DEPTH_EDIT, PALLET_Z, FALSE);
    yPos += lineHeight;

    g_hPMaxMassLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Макс. вес (кг):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20, g_hWnd, NULL, hInst, NULL);
    g_hPMaxMassEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        g_hWnd, (HMENU)IDC_P_MAX_MASS_EDIT, hInst, NULL);
    SetDlgItemInt(g_hWnd, IDC_P_MAX_MASS_EDIT, PALLET_MAX_MASS, FALSE);
    yPos += lineHeight + 20;

    // Добавление коробок
    g_hBoxesLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Добавление коробок:").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, 300, 20, g_hWnd, NULL, hInst, NULL);
    yPos += 25;

    g_hWidthLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Ширина (мм):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20, g_hWnd, NULL, hInst, NULL);
    g_hWidthEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"300",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        g_hWnd, (HMENU)IDC_WIDTH_EDIT, hInst, NULL);
    yPos += lineHeight;

    g_hHeightLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Высота (мм):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20, g_hWnd, NULL, hInst, NULL);
    g_hHeightEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"300",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        g_hWnd, (HMENU)IDC_HEIGHT_EDIT, hInst, NULL);
    yPos += lineHeight;

    g_hDepthLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Глубина (мм):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20, g_hWnd, NULL, hInst, NULL);
    g_hDepthEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"300",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        g_hWnd, (HMENU)IDC_DEPTH_EDIT, hInst, NULL);
    yPos += lineHeight;

    g_hQuantityLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Количество:").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20, g_hWnd, NULL, hInst, NULL);
    g_hQuantityEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"10",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        g_hWnd, (HMENU)IDC_QUANTITY_EDIT, hInst, NULL);
    yPos += lineHeight;

    g_hWeightLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Вес (кг):").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLabel, yPos, labelWidth, 20, g_hWnd, NULL, hInst, NULL);
    g_hWeightEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"20",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        xEdit, yPos - 3, editWidth, 25,
        g_hWnd, (HMENU)IDC_WEIGHT_EDIT, hInst, NULL);
    yPos += lineHeight;

    std::wstring fullRotateText = utf8_to_wstring("Полный поворот коробок?");
    g_hFullRotateCheck = CreateWindowW(L"BUTTON", fullRotateText.c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        xEdit, yPos, 200, 20,
        g_hWnd, (HMENU)IDC_FULL_ROTATE_CHECK, hInst, NULL);
    yPos += lineHeight + 10;

    std::wstring addButtonText = utf8_to_wstring("Добавить коробки");
    g_hAddButton = CreateWindowW(L"BUTTON", addButtonText.c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        xLabel, yPos, 180, 30,
        g_hWnd, (HMENU)IDC_ADD_BUTTON, hInst, NULL);

    std::wstring clearButtonText = utf8_to_wstring("Очистить список");
    g_hClearButton = CreateWindowW(L"BUTTON", clearButtonText.c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        xLabel + 190, yPos, 180, 30,
        g_hWnd, (HMENU)IDC_CLEAR_BUTTON, hInst, NULL);

    yPos += 40;

    int y2Pos = 60;
	int x2Label = xLabel + 440;

    g_hBoxListLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Добавленные типы коробок:").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x2Label, y2Pos, 300, 20, g_hWnd, NULL, hInst, NULL);
    y2Pos += 25;

    g_hBoxList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        x2Label, y2Pos, 520, 310,
        g_hWnd, (HMENU)IDC_BOX_LIST, hInst, NULL);
    y2Pos += 315;

    g_hMethodLabel = CreateWindowW(L"STATIC", utf8_to_wstring("Метод укладки:").c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x2Label, y2Pos, 300, 20, g_hWnd, NULL, hInst, NULL);
    y2Pos += 25;

    std::wstring centerMassText = utf8_to_wstring("По центру масс");
    g_hCenterMassCheck = CreateWindowW(L"BUTTON", centerMassText.c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x2Label, y2Pos, 200, 20,
        g_hWnd, (HMENU)IDC_SET_CENTER_MASS, hInst, NULL);

    std::wstring maxVolumeText = utf8_to_wstring("По максимальному объёму");
    g_hMaxVolumeCheck = CreateWindowW(L"BUTTON", maxVolumeText.c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x2Label + 220, y2Pos, 250, 20,
        g_hWnd, (HMENU)IDC_SET_MAX_VOLUME, hInst, NULL);

    // Кнопка расчёта/новой укладки
    g_hCalcButton = CreateWindowW(
        L"BUTTON",
        L"Рассчитать укладку",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, height - 150, width - 40, 40,
        g_hWnd,
        (HMENU)IDC_CALC_BUTTON,
        hInst,
        NULL
    );
    // Кнопка "Удалить" 
    g_hDeleteButton = CreateWindowW(
        L"BUTTON",
        L"X",
        WS_CHILD | BS_PUSHBUTTON,
        0, 0, 0, 0,                 // позицию/размер выставляем в update_delete_button_position()
        g_hWnd,
        (HMENU)IDC_DELETE_BUTTON,
        hInst,
        NULL
    );
    ShowWindow(g_hDeleteButton, SW_HIDE);

    HFONT hFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    SendMessage(g_hCalcButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(g_hTabControl, WM_SETFONT, (WPARAM)hFont, TRUE);

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    g_hDC = GetDC(g_hWnd);

    return true;
}

void cleanup_graphics() {
    if (g_hDC) {
        ReleaseDC(g_hWnd, g_hDC);
        g_hDC = NULL;
    }
    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
    }
}

bool should_close_window() {
    return !g_running;
}

void process_input() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void run_graphics_mode() {
    cout << "Starting graphics mode...\n";

    if (!init_graphics(g_windowWidth, g_windowHeight, "Визуализация укладки коробок на паллет")) {
        cout << "Graphics initialization error!\n";
        return;
    }

    cout << "Окно открыто. Добавьте коробки на вкладке 'Ввод данных' и нажмите 'Рассчитать укладку'.\n";

    while (!should_close_window()) {
        process_input();
        Sleep(16); // ~60 FPS
    }

    reset_state_for_new_packing();
    cleanup_graphics();
}