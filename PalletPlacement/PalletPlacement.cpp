#include <climits>
#include "functions.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <windows.h>

vector<box*> total_boxes; 
vector<vector<box*> > grouped_boxes; 

vector<box*> input_values() { // имитация ввода коробок

    std::vector<box_property> input_boxs;

    const box_property small_a_type{ 20,  3, 3, 3,   2 };
    const box_property small_b_type{ 12,  6, 3, 3,   3 };
    const box_property small_c_type{ 8,  3, 6, 3,   3 };
    const box_property small_d_type{ 8,  6, 6, 3,   5 };

    input_boxs.push_back(small_a_type);
    input_boxs.push_back(small_b_type);
    input_boxs.push_back(small_c_type);
    input_boxs.push_back(small_d_type);

    for (const auto& box_type : input_boxs) {
        for (int i = 0; i < box_type.Quantity; ++i) {
            box* new_box = new box();
            new_box->xyz_size[0][0] = box_type.width;
            new_box->xyz_size[0][1] = box_type.height;
            new_box->xyz_size[0][2] = box_type.depth;

            new_box->xyz_size[1][2] = box_type.width;
            new_box->xyz_size[1][1] = box_type.height;
            new_box->xyz_size[1][0] = box_type.depth;

            new_box->mass = box_type.weight;
            total_boxes.push_back(new_box);
        }
	}

    return total_boxes;
}

void pallet_handle(pallet* pal_ptr) {  // обработчик паллета
    zone* pal_zone_ptr;
    box* placed_box_ptr = nullptr;
	height_map_init(pal_ptr);
	cout << "Начинаем размещение коробок на паллете\n";
    while (is_placement_possible(pal_ptr)) {
        check_meb(pal_ptr);
        pal_zone_ptr = select_zone(pal_ptr);
        placed_box_ptr = box_placement_handle(pal_ptr, pal_zone_ptr);
        if (placed_box_ptr) {
            celebrate();
            split_zone(pal_ptr, pal_zone_ptr, placed_box_ptr);
            zone_cleanup(pal_ptr);
			cout << "Зон щас:" << pal_ptr->zone_vector.size() << endl;
        }
        else
        {
            if (merge_any_pair_XYZ(pal_ptr)) {
                zone_cleanup(pal_ptr);
                pal_ptr->failed_in_row++;
                continue;
            }
			cout << "Убираем зону из доступных\n";
            kill_zone(pal_ptr, pal_zone_ptr);
        }
    }
	cout << "Размещение завершено\n";
	cout << "Всего размещено коробок: " << pal_ptr->placed_boxes.size() << endl;
    cout << "Максимальная высота " << pal_ptr->max_height_box << endl;
	cout << "Вес паллеты " << pal_ptr->total_mass << " | " << pal_ptr->max_mass << endl;

	int total_volume = pal_ptr->xyz_size[0] * pal_ptr->xyz_size[1] * pal_ptr->xyz_size[2];
	int used_volume = 0;
    for (const auto& box_ptr : pal_ptr->placed_boxes) {
        int r = box_ptr->rotate;
        used_volume += box_ptr->xyz_size[r][0] * box_ptr->xyz_size[r][1] * box_ptr->xyz_size[r][2];
	}
	cout << "Заполнено объема: " << used_volume << " из " << total_volume << " (" << (used_volume * 100.0) / total_volume << "%)" << endl;

	cout << "Осталось неразмещенных коробок: " << total_boxes.size() << endl;
    for (auto& box_ptr : total_boxes) {
        if (box_ptr->placed == false) {
            cout << "Коробка размером (" << box_ptr->xyz_size[0][0] << ", " << box_ptr->xyz_size[0][1] << ", " << box_ptr->xyz_size[0][2] << ") не была размещена\n";
        }
	}
}
    

int main()
{   
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
   
    input_values();


    //group_boxes();


    pallet pallet1;
    //forcefeed_pallet(&huyushka_pizdushka);
    pallet_handle(&pallet1);

    //record_pallet_details(&huyushka_pizdushka, total_boxes);
    create_ascii_layers(&pallet1);
    //record_pallet_details(&huyushka_pizdushka, total_boxes);
    create_ascii_layers_m(&pallet1);

    
    //std::cout << "Hello World!\n";
}
