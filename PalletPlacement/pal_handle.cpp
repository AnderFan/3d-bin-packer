#include "functions.h"
#include <iostream>
#include <algorithm>
using namespace std;

box* clone_box(const box* src) {
    if (!src) return nullptr;

    box* b = new box(*src);   // копирует размеры, массу, full_rotateble и т.д.
    b->placed = false;
    b->rotate = 0;
    b->xyz[0] = b->xyz[1] = b->xyz[2] = -1;
    b->temp_xz[0] = b->temp_xz[1] = -1;
    b->scores = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX };
    return b;
}

void sort_boxes(vector<box*> total_boxes) {
    std::sort(total_boxes.begin(), total_boxes.end(), [](const box* a, const box* b) {
        int vol_a = a->xyz_size[0][0] * a->xyz_size[0][1] * a->xyz_size[0][2];
        int vol_b = b->xyz_size[0][0] * b->xyz_size[0][1] * b->xyz_size[0][2];

        if (vol_a != vol_b) {
            return vol_a > vol_b;  // Сначала большие по объёму
        }

        // При равном объёме — сначала более тяжёлые (для лучшего центра масс)
        return a->mass > b->mass;
        });
}

void centering_box (pallet* pal_ptr) {
    int max_x = 0, max_z = 0;
	for (const auto& box_ptr : pal_ptr->placed_boxes) { // находим макс координаты по x и z среди размещенных коробок
        int r = box_ptr->rotate;
        int box_x_end = box_ptr->xyz[0] + box_ptr->xyz_size[r][0];
        int box_z_end = box_ptr->xyz[2] + box_ptr->xyz_size[r][2];
        max_x = max(max_x, box_x_end);
        max_z = max(max_z, box_z_end);
	}
    
    int indent_x = 0, indent_z = 0;
    if (max_x < pal_ptr->xyz_size[0]) {
		indent_x = (pal_ptr->xyz_size[0] - max_x) / 2;
	}
    if (max_z < pal_ptr->xyz_size[2]) {
        indent_z = (pal_ptr->xyz_size[2] - max_z) / 2;
    }

	for (auto& box_ptr : pal_ptr->placed_boxes) { // смещаем все коробки на indent_x и indent_z, чтобы центрировать по x и z
		box_ptr->xyz[0] += indent_x;
		box_ptr->xyz[2] += indent_z;
    }

    pal_ptr->xyz_mass_centre[0] += indent_x;
    pal_ptr->xyz_mass_centre[2] += indent_z;
}

void pallet_handle(pallet* pal_ptr, vector<box*> total_boxes) {
    sort_boxes(total_boxes);

    // Если выбран режим Макс  докидывать коробки одного типа бесконечно
    const box* unlimited_template = nullptr;
    if (pal_ptr->hMaxQtyCheck) {
        unlimited_template = total_boxes.front(); // один тип: берем первый как шаблон
    }

    cout << "Всего коробок: " << total_boxes.size() << endl;
	cout << "Кол-во ЗОН " << pal_ptr->zone_vector.size() << endl;
    zone* pal_zone_ptr;
    box* placed_box_ptr = nullptr;
    height_map_init(pal_ptr);
    cout << "Начинаем размещение коробок на паллете\n";

    int max_iterations = total_boxes.size() * 10; // Лимит итераций
    int iterations = 0;
    int failed_iterations = 0;
    const int MAX_FAILED = 100; // Если 100 итераций подряд не получилось разместить - выходим

    while (is_placement_possible(pal_ptr, total_boxes)) {
        iterations++;

        if (pal_ptr->hMaxQtyCheck && unlimited_template) {
            // если коробок не осталось
            if (total_boxes.empty()) {
                total_boxes.push_back(clone_box(unlimited_template));
            }
        }

        check_meb(pal_ptr, total_boxes);

        if (pal_ptr->zone_vector.empty()) {
            cout << "Нет доступных зон для размещения\n";
            break;
        }

        pal_zone_ptr = select_zone(pal_ptr);

        if (!pal_zone_ptr || !pal_zone_ptr->usable) {
            cout << "Выбранная зона мертва, удаляем её\n";
            if (pal_zone_ptr) kill_zone(pal_ptr, pal_zone_ptr);
            failed_iterations++;
            if (failed_iterations > MAX_FAILED) {
                cout << "Слишком много неудачных итераций, прекращаем укладку\n";
                break;
            }
            continue;
        }

        placed_box_ptr = box_placement_handle(pal_ptr, pal_zone_ptr, total_boxes);

        if (placed_box_ptr) {
            celebrate();
            split_zone(pal_ptr, pal_zone_ptr, placed_box_ptr);
            zone_cleanup(pal_ptr);
            cout << "Зон щас:" << pal_ptr->zone_vector.size() << endl;
            cout << "Всего размещено коробок: " << pal_ptr->placed_boxes.size() << endl;
            failed_iterations = 0; 
        }
        else {
            // Размещение не удалось
            if (merge_any_pair_XYZ(pal_ptr)) {
                zone_cleanup(pal_ptr);
                pal_ptr->failed_in_row++;
                failed_iterations++;
                continue;
            }

            cout << "Убираем зону из доступных\n";
            kill_zone(pal_ptr, pal_zone_ptr);
            zone_cleanup(pal_ptr);
            failed_iterations++;

            if (failed_iterations > MAX_FAILED) {
                cout << "Слишком много неудачных итераций, прекращаем укладку\n";
                break;
            }

            if (pal_ptr->zone_vector.size() == 0) {
                cout << "Протокол ПОСЛЕДНИЙ ШАНС" << endl;
                
                meb_gen(pal_ptr);
            }
        }
    }

    if (pal_ptr->lim_lay) {
        if (pal_ptr->placed_boxes.empty()) {
            // нечего удалять
        }
        else {
            int w_p = pal_ptr->xyz_size[0];
            int d_p = pal_ptr->xyz_size[2];

            box* b0 = pal_ptr->placed_boxes.front();
            int r = b0->rotate;
            int w_b = b0->xyz_size[r][0];
            int d_b = b0->xyz_size[r][2];

            // защита от деления на 0
            if (w_b > 0 && d_b > 0) {
                int qbox_lay = (w_p * d_p) / (w_b * d_b); // сколько коробок в ПОЛНОМ слое (идеально)
                if (qbox_lay > 0) {
                    int placed = (int)pal_ptr->placed_boxes.size();
                    int full_layers = placed / qbox_lay;
                    int need_box = full_layers * qbox_lay;              // оставить только целые слои
                    int del_box = placed - need_box;                    // удалить только неполный хвост

                    for (int i = 0; i < del_box; ++i) {
                        pal_ptr->placed_boxes.pop_back();
                    }
                }
            }
        }
    }

	centering_box(pal_ptr);
 
    cout << "\n========== Размещение завершено ==========\n";
    cout << "Всего размещено коробок: " << pal_ptr->placed_boxes.size() << endl;
    cout << "Максимальная высота: " << pal_ptr->max_height_box << " см\n";
    cout << "Вес паллеты: " << pal_ptr->total_mass << " кг из " << pal_ptr->max_mass << " кг\n";

    int total_volume = pal_ptr->xyz_size[0] * pal_ptr->xyz_size[1] * pal_ptr->xyz_size[2];
    int used_volume = 0;

    for (const auto& box_ptr : pal_ptr->placed_boxes) {
        int r = box_ptr->rotate;
        used_volume += box_ptr->xyz_size[r][0] * box_ptr->xyz_size[r][1] * box_ptr->xyz_size[r][2];
    }

    cout << "Заполнено объема: " << used_volume << " из " << total_volume
        << " (" << (used_volume * 100.0) / total_volume << "%)\n";
    cout << "Осталось неразмещенных коробок: " << total_boxes.size() << endl;

    if (!total_boxes.empty()) {
        cout << "\nНеразмещенные коробки:\n";
        for (auto& box_ptr : total_boxes) {
            if (box_ptr->placed == false) {
                cout << "  Размер: (" << box_ptr->xyz_size[0][0] << ", "
                    << box_ptr->xyz_size[0][1] << ", " << box_ptr->xyz_size[0][2]
                    << ") см, вес: " << box_ptr->mass << " кг\n";
            }
        }
    }

    if (iterations >= max_iterations) {
        cout << "WARNING: Достигнут лимит итераций (" << max_iterations << ")\n";
    }
}

