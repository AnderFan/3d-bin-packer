#pragma execution_character_set("utf-8")
#include "graphics.h"
#include "functions.h"
#include "utils.h"
#include <windows.h>
#include <windowsx.h>
#include <cmath>
#include <string>
#include <sstream>

static HWND g_hWnd = NULL;
static HWND g_hRestartButton = NULL;
static HDC g_hDC = NULL;
static bool g_running = true;
static bool g_needRestart = false;

#define IDC_RESTART_BUTTON 2001

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

void reset_state_for_new_packing() {
    // Очищаем старые коробки из total_boxes
    for (auto* b : total_boxes) {
        delete b;
    }
    total_boxes.clear();

    // Очищаем текущий паллет
    if (g_state.current_pallet) {
        // Удаляем зоны
        for (auto* z : g_state.current_pallet->zone_vector) {
            delete z;
        }
        g_state.current_pallet->zone_vector.clear();

        for (auto* z : g_state.current_pallet->zone_dead_vector) {
            delete z;
        }
        g_state.current_pallet->zone_dead_vector.clear();

        // Коробки уже удалены выше (они были в total_boxes)
        g_state.current_pallet->placed_boxes.clear();

        delete g_state.current_pallet;
        g_state.current_pallet = nullptr;
    }

    // Сброс состояния GUI
    //g_state.box_types.clear();
    g_state.calculation_done = false;
    g_state.show_input_window = true;
    //g_state.pallet_width = PALLET_X;
    //g_state.pallet_height = PALLET_Y;
    //g_state.pallet_depth = PALLET_Z;
    //g_state.pallet_max_mass = PALLET_MAX_MASS;  // Сброс максимального веса
    g_state.use_center_mass = false;
	g_state.use_max_volume = false;
}

void start_new_packing() {
    reset_state_for_new_packing();

    // Скрываем основное окно на время ввода
    ShowWindow(g_hWnd, SW_HIDE);

    // Показываем диалог ввода
    if (!show_input_dialog(&g_state)) {
        cout << "Ввод отменён или произошла ошибка.\n";
        ShowWindow(g_hWnd, SW_SHOW);
        return;
    }

    if (g_state.box_types.empty()) {
        cout << "Коробки не добавлены.\n";
        ShowWindow(g_hWnd, SW_SHOW);
        return;
    }

    // Создаём коробки из введённых типов
    int box_len = 0;
    for (const auto& box_type : g_state.box_types) {
        box_len += box_type.Quantity;
    }
    total_boxes.reserve(box_len);

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

            total_boxes.push_back(new_box);
        }
    }

    if (g_state.use_center_mass) {
        cout << "Метод укладки: Центр масс\n";
		int center_mass_or_max_volume = 0; // 0 - центр масс
        g_state.current_pallet = new pallet(
            g_state.pallet_width,
            g_state.pallet_height,
            g_state.pallet_depth,
            g_state.pallet_max_mass,
            center_mass_or_max_volume
        );
    }
    if (g_state.use_max_volume) {
        cout << "Метод укладки: Максимальный объем\n";
        int center_mass_or_max_volume = 1; // 1 - объем
        g_state.current_pallet = new pallet(
            g_state.pallet_width,
            g_state.pallet_height,
            g_state.pallet_depth,
            g_state.pallet_max_mass,
            center_mass_or_max_volume
        );
	}



    // Обновляем камеру
    g_camera.targetX = g_state.current_pallet->xyz_size[0] / 2.0f;
    g_camera.targetY = g_state.current_pallet->xyz_size[1] / 2.0f;
    g_camera.targetZ = g_state.current_pallet->xyz_size[2] / 2.0f;
    g_camera.distance = g_state.current_pallet->xyz_size[0] * 2.0f;

    // Показываем окно обратно
    ShowWindow(g_hWnd, SW_SHOW);

    // Запускаем расчёт
    cout << "\nЗапуск расчёта укладки...\n";
    cout << "Макс. вес паллеты: " << g_state.current_pallet->max_mass << " кг\n";
    pallet_handle(g_state.current_pallet);

    g_state.calculation_done = true;
    cout << "Расчёт завершён! Размещено коробок: " << g_state.current_pallet->placed_boxes.size() << endl;

    // Перерисовываем окно
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
    
    // Круг XY
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i * 3.14159f * 2.0f / segments;
        float angle2 = (float)(i + 1) * 3.14159f * 2.0f / segments;
        
        float x1 = cx + radius * cos(angle1);
        float y1 = cy + radius * sin(angle1);
        float x2 = cx + radius * cos(angle2);
        float y2 = cy + radius * sin(angle2);
        
        draw_line_3d(hdc, x1, y1, cz, x2, y2, cz, cam, color);
    }   
    // Круг XZ
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i * 3.14159f * 2.0f / segments;
        float angle2 = (float)(i + 1) * 3.14159f * 2.0f / segments;
        
        float x1 = cx + radius * cos(angle1);
        float z1 = cz + radius * sin(angle1);
        float x2 = cx + radius * cos(angle2);
        float z2 = cz + radius * sin(angle2);
        
        draw_line_3d(hdc, x1, cy, z1, x2, cy, z2, cam, color);
    }
    // Круг YZ
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
    
    float line_length = 30.0f;

    // Рисуем вертикальную линию от основания до центра масс (пунктирная)
    draw_line_3d(hdc, com_x, 0, com_z, com_x, com_y, com_z, cam, RGB(255, 100, 100));
    
    // Проекция на плоскость паллета (круг на основании)
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
    
    // Также рисуем идеальный центр масс (полупрозрачный зелёный)
    float ideal_x = (float)pal_ptr->ideal_cx;
    float ideal_z = (float)pal_ptr->ideal_cz;
    
    // Маленький круг на основании для идеального центра
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

    // X - red
    draw_line_3d(hdc, 0, 0, 0, px * 2, 0, 0, cam, RGB(255, 0, 0), 1.5);
    // Y - green
    draw_line_3d(hdc, 0, 0, 0, 0, py * 2, 0, cam, RGB(0, 255, 0), 1.5);
    // Z - blue
    draw_line_3d(hdc, 0, 0, 0, 0, 0, pz * 2, cam, RGB(0, 0, 255), 1.5);
}

// Rendering 3D scene
void render_3d_scene(HDC hdc, pallet* pal_ptr, Camera* cam) {
    if (!pal_ptr) return;

    // Draw elements in order from far to near (simple sorting)
    draw_axes(hdc, cam, pal_ptr);
    draw_grid(hdc, cam, pal_ptr);
    draw_pallet_base(hdc, cam, pal_ptr);

    // Draw all placed boxes
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

void render_input_panel(HDC hdc, GUIState* state, RECT* panelRect) {
    HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
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

    draw_text_line(hdc, xPos, yPos, "Параметры коробок", RGB(0, 0, 128));
    yPos += 40;

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    hOldFont = (HFONT)SelectObject(hdc, hFont);

    draw_text_line(hdc, xPos, yPos, "Управление:");
    yPos += 25;
    draw_text_line(hdc, xPos + 10, yPos, "• Колесико мыши - зум");
    yPos += 22;
    draw_text_line(hdc, xPos + 10, yPos, "• ПКМ + движение - поворот");
    yPos += 35;

    draw_text_line(hdc, xPos, yPos, "Добавленные коробки:", RGB(0, 0, 128));
    yPos += 25;

    if (state->box_types.empty()) {
        draw_text_line(hdc, xPos + 10, yPos, "Коробки не добавлены");
        yPos += 25;
    }
    else {
        for (size_t i = 0; i < state->box_types.size(); i++) {
            auto& bt = state->box_types[i];
            char buffer[256];
            sprintf_s(buffer, "%d. %dx%dx%d см, вес %d кг, кол-во %d",
                (int)i + 1, bt.width, bt.height, bt.depth, bt.weight, bt.Quantity);
            draw_text_line(hdc, xPos + 10, yPos, buffer);
            yPos += 22;
        }
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
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

    sprintf_s(buffer, "X: %.1f (идеал: %.1f)", pal_ptr->xyz_mass_centre[0] / 100.0, pal_ptr->ideal_cx);
    draw_text_line(hdc, xPos + 10, yPos, buffer);
    yPos += 22;

    sprintf_s(buffer, "Z: %.1f (идеал: %.1f)", pal_ptr->xyz_mass_centre[2] / 100.0, pal_ptr->ideal_cz);
    draw_text_line(hdc, xPos + 10, yPos, buffer);

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
        if (LOWORD(wParam) == IDC_RESTART_BUTTON) {
            g_needRestart = true;
            return 0;
        }
        break;

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        g_camera.distance -= delta / 10.0f * 2.0f;
        if (g_camera.distance < 100) g_camera.distance = 100;
        if (g_camera.distance > 5000) g_camera.distance = 5000;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_RBUTTONDOWN:
        isDragging = true;
        lastMousePos.x = GET_X_LPARAM(lParam);
        lastMousePos.y = GET_Y_LPARAM(lParam);
        SetCapture(hwnd);
        return 0;

    case WM_RBUTTONUP:
        isDragging = false;
        ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE:
        if (isDragging) {
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

        if (g_state.current_pallet) {
            render_3d_scene(hdcMem, g_state.current_pallet, &g_camera);
        }

        RECT inputPanel = { 10, 10, 400, 400 };
        render_input_panel(hdcMem, &g_state, &inputPanel);

        if (g_state.current_pallet && g_state.calculation_done) {
            RECT statsPanel = { g_windowWidth - 380, 10, g_windowWidth - 10, 420 };
            render_stats_panel(hdcMem, g_state.current_pallet, &statsPanel);
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

bool init_graphics(int width, int height, const char* title) {
    g_windowWidth = width;
    g_windowHeight = height;

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"PalletVisualizerClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    std::wstring wtitle = utf8_to_wstring(title);
    g_hWnd = CreateWindowExW(
        0,
        L"PalletVisualizerClass",
        wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (!g_hWnd) return false;

    g_hRestartButton = CreateWindowW(
        L"BUTTON",
        L"Новая укладка",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, height - 80, 200, 40,
        g_hWnd,
        (HMENU)IDC_RESTART_BUTTON,
        GetModuleHandle(NULL),
        NULL
    );

    HFONT hFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    SendMessage(g_hRestartButton, WM_SETFONT, (WPARAM)hFont, TRUE);

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

    cout << "Opening box parameters input window...\n";

    if (!show_input_dialog(&g_state)) {
        cout << "Input dialog was canceled or an error occurred.\n";
        return;
    }

    if (g_state.box_types.empty()) {
        cout << "No boxes were added. Exiting graphics mode.\n";
        return;
    }

    cout << "Box types added: " << g_state.box_types.size() << "\n";

    if (!init_graphics(g_windowWidth, g_windowHeight, "Визуализация укладки коробок на паллет")) {
        cout << "Graphics initialization error!\n";
        return;
    }

    cout << "\nStarting packing calculation...\n";

    total_boxes.clear();

    int box_len = 0;
    for (const auto& box_type : g_state.box_types) {
        box_len += box_type.Quantity;
    }
    total_boxes.reserve(box_len);
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
            total_boxes.push_back(new_box);
        }
    }

    g_state.current_pallet = new pallet(
        g_state.pallet_width,
        g_state.pallet_height,
        g_state.pallet_depth,
        g_state.pallet_max_mass  
    );
    g_state.calculation_done = true;

    cout << "Макс. вес паллеты: " << g_state.current_pallet->max_mass << " кг\n";

    g_camera.targetX = g_state.current_pallet->xyz_size[0] / 2.0f;
    g_camera.targetY = g_state.current_pallet->xyz_size[1] / 2.0f;
    g_camera.targetZ = g_state.current_pallet->xyz_size[2] / 2.0f;
    g_camera.distance = g_state.current_pallet->xyz_size[0] * 2.0f;

    pallet_handle(g_state.current_pallet);

    cout << "Calculation completed!\n";
    cout << "Boxes placed: " << g_state.current_pallet->placed_boxes.size() << endl;

    while (!should_close_window()) {
        process_input();
        
        if (g_needRestart) {
            g_needRestart = false;
            start_new_packing();
        }
        
        Sleep(16); // ~60 FPS
    }

    reset_state_for_new_packing();

    cleanup_graphics();
}