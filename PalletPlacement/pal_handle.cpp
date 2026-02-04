#include "functions.h"
#include <iostream>
#include <algorithm>
using namespace std;

void sort_boxes() {
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
void pallet_handle(pallet* pal_ptr) {
    sort_boxes();

    cout << "Всего коробок: " << total_boxes.size() << endl;
    zone* pal_zone_ptr;
    box* placed_box_ptr = nullptr;
    height_map_init(pal_ptr);
    cout << "Начинаем размещение коробок на паллете\n";

    int max_iterations = total_boxes.size() * 10; // Лимит итераций
    int iterations = 0;
    int failed_iterations = 0;
    const int MAX_FAILED = 100; // Если 100 итераций подряд не получилось разместить - выходим

    while (is_placement_possible(pal_ptr)) {
        iterations++;

        check_meb(pal_ptr);

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

        placed_box_ptr = box_placement_handle(pal_ptr, pal_zone_ptr);

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
//void pallet_handle(pallet* pal_ptr) {  // обработчик паллета
//    cout << "Всего коробок: "<< total_boxes.size() << endl;
//    zone* pal_zone_ptr;
//    box* placed_box_ptr = nullptr;
//    height_map_init(pal_ptr);
//    cout << "Начинаем размещение коробок на паллете\n";
//    while (is_placement_possible(pal_ptr)) {
//        check_meb(pal_ptr);
//        pal_zone_ptr = select_zone(pal_ptr);
//        placed_box_ptr = box_placement_handle(pal_ptr, pal_zone_ptr);
//        if (placed_box_ptr) {
//            celebrate();
//            split_zone(pal_ptr, pal_zone_ptr, placed_box_ptr);
//            zone_cleanup(pal_ptr);
//            cout << "Зон щас:" << pal_ptr->zone_vector.size() << endl;
//            cout << "Всего размещено коробок: " << pal_ptr->placed_boxes.size() << endl;
//        }
//        else
//        {
//            if (merge_any_pair_XYZ(pal_ptr)) {
//                zone_cleanup(pal_ptr);
//                pal_ptr->failed_in_row++;
//                continue;
//            }
//            cout << "Убираем зону из доступных\n";
//            kill_zone(pal_ptr, pal_zone_ptr);
//        }
//    }
//    cout << "Размещение завершено\n";
//    cout << "Всего размещено коробок: " << pal_ptr->placed_boxes.size() << endl;
//    cout << "Максимальная высота " << pal_ptr->max_height_box << endl;
//    cout << "Вес паллеты " << pal_ptr->total_mass << " | " << pal_ptr->max_mass << endl;
//
//    int total_volume = pal_ptr->xyz_size[0] * pal_ptr->xyz_size[1] * pal_ptr->xyz_size[2];
//    int used_volume = 0;
//    for (const auto& box_ptr : pal_ptr->placed_boxes) {
//        int r = box_ptr->rotate;
//        used_volume += box_ptr->xyz_size[r][0] * box_ptr->xyz_size[r][1] * box_ptr->xyz_size[r][2];
//    }
//    cout << "Заполнено объема: " << used_volume << " из " << total_volume << " (" << (used_volume * 100.0) / total_volume << "%)" << endl;
//
//    cout << "Осталось неразмещенных коробок: " << total_boxes.size() << endl;
//    for (auto& box_ptr : total_boxes) {
//        if (box_ptr->placed == false) {
//            cout << "Коробка размером (" << box_ptr->xyz_size[0][0] << ", " << box_ptr->xyz_size[0][1] << ", " << box_ptr->xyz_size[0][2] << ") не была размещена\n";
//        }
//    }
//}
//void pallet_handle(pallet* pal_ptr) {
//    cout << "Starting pallet placement algorithm..." << endl;
//    
//    height_map_init(pal_ptr); // Initialize height map
//    
//    zone* initial_zone = new zone;
//    initial_zone->xyz[0] = 0;
//    initial_zone->xyz[1] = 0;
//    initial_zone->xyz[2] = 0;
//    initial_zone->xyz_size[0] = pal_ptr->xyz_size[0];
//    initial_zone->xyz_size[1] = pal_ptr->xyz_size[1];
//    initial_zone->xyz_size[2] = pal_ptr->xyz_size[2];
//    
//    pal_ptr->zone_vector.push_back(initial_zone);
//    
//    int iteration = 0;
//    const int MAX_ITERATIONS = 10000;
//    
//    while (is_placement_possible(pal_ptr) && iteration < MAX_ITERATIONS) {
//        iteration++;
//        
//        cout << "\n=== Iteration " << iteration << " ===" << endl;
//        cout << "Available zones: " << pal_ptr->zone_vector.size() << endl;
//        cout << "Remaining boxes: " << total_boxes.size() << endl;
//        
//        zone* selected_zone = select_zone(pal_ptr);
//        
//        if (!selected_zone) {
//            cout << "No suitable zone found, ending placement" << endl;
//            break;
//        }
//        
//        box* placed_box = box_placement_handle(pal_ptr, selected_zone);
//        
//        if (!placed_box) {
//            cout << "Failed to place box in selected zone" << endl;
//            kill_zone(pal_ptr, selected_zone);
//            zone_cleanup(pal_ptr);
//            
//            if (pal_ptr->failed_in_row > 5) {
//                check_meb(pal_ptr);
//                pal_ptr->failed_in_row = 0;
//            }
//    int iteration = 0;
//    const int MAX_ITERATIONS = 10000;
//    
//    while (is_placement_possible(pal_ptr) && iteration < MAX_ITERATIONS) {
//        iteration++;
//        
//        cout << "\n=== Iteration " << iteration << " ===" << endl;
//        cout << "Available zones: " << pal_ptr->zone_vector.size() << endl;
//        cout << "Remaining boxes: " << total_boxes.size() << endl;
//        
//        zone* selected_zone = select_zone(pal_ptr);
//        
//        if (!selected_zone) {
//            cout << "No suitable zone found, ending placement" << endl;
//            break;
//        }
//        
//        box* placed_box = box_placement_handle(pal_ptr, selected_zone);
//        
//        if (!placed_box) {
//            cout << "Failed to place box in selected zone" << endl;
//            kill_zone(pal_ptr, selected_zone);
//            zone_cleanup(pal_ptr);
//            
//            if (pal_ptr->failed_in_row > 5) {
//                check_meb(pal_ptr);
//                pal_ptr->failed_in_row = 0;
//            }
//            continue;
//        }
//        
//        celebrate();
//        split_zone(pal_ptr, selected_zone, placed_box);
//        zone_cleanup(pal_ptr);
//        check_meb(pal_ptr);
//    }
//    
//    cout << "\nPlacement complete!" << endl;
//    cout << "Total boxes placed: " << pal_ptr->placed_boxes.size() << endl;
//    cout << "Total weight: " << pal_ptr->total_mass << " kg" << endl;
//    cout << "Max height: " << pal_ptr->max_height_box << " cm" << endl;
//}
