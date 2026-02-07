//#include "data.h"
#include "functions.h"
#include <iostream>
#include <array>
#include <algorithm>
#include <cmath>
#include <utility>      
#include <omp.h>
#include <set>

//#define DEBUG_PALLET

#ifdef DEBUG_PALLET
    #define DEBUG_LOG(x) cout << x
    #define DEBUG_LOG_ENDL(x) cout << x << endl
#else
    #define DEBUG_LOG(x)
    #define DEBUG_LOG_ENDL(x)
#endif

void height_map_init(pallet* pal_ptr) {
	int W = pal_ptr->xyz_size[0];
	int D = pal_ptr->xyz_size[2];
	pal_ptr->height_map.assign(W, vector<int>(D, 0)); // инициализируем карту высот нулями
}


bool is_placement_possible(pallet* pallet_ptr, vector<box*> total_boxes) {
	DEBUG_LOG_ENDL("\n=== ПРОВЕРКА ВОЗМОЖНОСТИ РАЗМЕЩЕНИЯ ===");
	DEBUG_LOG_ENDL("Коробок в total_boxes: " << total_boxes.size());
	DEBUG_LOG_ENDL("Зон доступно: " << pallet_ptr->zone_vector.size());
	
	bool flag = false;
	for (int i = 0; i < total_boxes.size(); i++) { // проходимся по коробкам
		DEBUG_LOG_ENDL("  Box[" << i << "] placed=" << (bool)total_boxes[i]->placed);
		if (total_boxes[i]->placed == false) { // если есть хоть одна коробка которая не размещена
			DEBUG_LOG_ENDL(">>> Найдена неразмещённая коробка, продолжаем");
			return true;
		}
	}


	if (flag == false) {
		DEBUG_LOG_ENDL(">>> Все коробки уже размещены");
		return false; // все коробки уже размещены
	}

	if (pallet_ptr->zone_vector.size() > 0) { // если есть хоть одна живая зона
		DEBUG_LOG_ENDL(">>> Есть доступные зоны для размещения коробок");
		return true;
	}

	DEBUG_LOG_ENDL(">>> Нет доступных зон!");
	return false;
}



//void split_zone() {
//	return;
//}

void celebrate() {
	DEBUG_LOG_ENDL("КОРОБКА УЛОЖЕНА!");
}


box* select_best_box(vector<box*> total_boxes) {
	box* best_box_ptr = nullptr;
	array<int, SCORES_NUM> best = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX }; // тут храним лучшую коробку
	for (int i = 0; i < total_boxes.size(); i++) { // проходимся по коробкам 
		if (total_boxes.at(i)->scores < best) {
			best = total_boxes.at(i)->scores;
			best_box_ptr = total_boxes.at(i);
		}
	}
	if (best_box_ptr == nullptr || best == array<int, SCORES_NUM>{INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX}) { // если не нашли ни одной коробки
		DEBUG_LOG_ENDL("Не удалось найти подходящую коробку для размещенияEEE");
		return nullptr;
	}

	DEBUG_LOG_ENDL("Лучшая коробка получила оценки: " << best[0] << " " << best[1] << " " << best[2] << " " << best[3]);
	return best_box_ptr;
}

void place_box(pallet* pal_ptr, zone* zone_ptr, box* box_ptr, vector<box*> total_boxes) {
	DEBUG_LOG_ENDL("\n--- РАЗМЕЩЕНИЕ КОРОБКИ ---");
	DEBUG_LOG_ENDL("Позиция: (" << box_ptr->temp_xz[0] << ", " << zone_ptr->xyz[1] << ", " << box_ptr->temp_xz[1] << ")");
	
	box_ptr->xyz[0] = box_ptr->temp_xz[0];
	box_ptr->xyz[1] = zone_ptr->xyz[1];
	box_ptr->xyz[2] = box_ptr->temp_xz[1];

	pal_ptr->placed_boxes.push_back(box_ptr);

	if (pal_ptr->max_height_box == INT_MAX) {
		pal_ptr->max_height_box = pal_ptr->placed_boxes[0]->xyz_size[pal_ptr->placed_boxes[0]->rotate][1];
	}
	pal_ptr->max_height_box = std::max(pal_ptr->max_height_box,
		box_ptr->xyz[1] + box_ptr->xyz_size[box_ptr->rotate][1]);

	DEBUG_LOG_ENDL("Удаляем коробку из total_boxes (было: " << total_boxes.size() << ")");
	total_boxes.erase(
		remove(total_boxes.begin(), total_boxes.end(), box_ptr),
		total_boxes.end()
	);
	DEBUG_LOG_ENDL("После удаления: " << total_boxes.size());

	box_ptr->placed = true;
	pal_ptr->placed_since_meb++;
	pal_ptr->was_defrag = false;

	int x_start = box_ptr->xyz[0];
	int x_end = x_start + box_ptr->xyz_size[box_ptr->rotate][0];
	int z_start = box_ptr->xyz[2];
	int z_end = z_start + box_ptr->xyz_size[box_ptr->rotate][2];
	int new_height = box_ptr->xyz[1] + box_ptr->xyz_size[box_ptr->rotate][1];

	DEBUG_LOG_ENDL("Обновляем height_map: x=[" << x_start << ".." << x_end << "), z=[" << z_start << ".." << z_end << "), new_height=" << new_height);

	for (int x = x_start; x < x_end; ++x) {
		for (int z = z_start; z < z_end; ++z) {
			pal_ptr->height_map[x][z] = std::max(pal_ptr->height_map[x][z], new_height);
		}
	}

	center_mass_calculate(pal_ptr, box_ptr);
	
}


CenterMassResult simulate_center_mass(const pallet* pal_ptr, int box_mass, double cx_box, double cy_box, double cz_box) {
	CenterMassResult res{};

	int prev_mass = pal_ptr->total_mass;
	int new_mass = prev_mass + box_mass;
	res.total_mass = new_mass;

	double new_cx, new_cy, new_cz;

	if (prev_mass == 0) {
		// Если на паллете ничего не было – центр масс = центр этой коробки
		new_cx = cx_box;
		new_cy = cy_box;
		new_cz = cz_box;
	}
	else {
		new_cx = (pal_ptr->xyz_mass_centre[0] * (double)prev_mass + cx_box * box_mass)
			/ (double)new_mass;
		new_cy = (pal_ptr->xyz_mass_centre[1] * (double)prev_mass + cy_box * box_mass)
			/ (double)new_mass;
		new_cz = (pal_ptr->xyz_mass_centre[2] * (double)prev_mass + cz_box * box_mass)
			/ (double)new_mass;
	}

	res.cx = new_cx;
	res.cy = new_cy;
	res.cz = new_cz;

	return res;
}

void center_mass_calculate(pallet* pal_ptr, box* box_ptr) {
	int r = box_ptr->rotate;

	double bx = box_ptr->xyz[0];
	double by = box_ptr->xyz[1];
	double bz = box_ptr->xyz[2];

	double bw = box_ptr->xyz_size[r][0];
	double bh = box_ptr->xyz_size[r][1];
	double bd = box_ptr->xyz_size[r][2];

	// геометрический центр коробки
	double cx_box = bx + bw / 2.0;
	double cy_box = by + bh / 2.0;
	double cz_box = bz + bd / 2.0;

	CenterMassResult cm = simulate_center_mass(pal_ptr, box_ptr->mass, cx_box, cy_box, cz_box);

	pal_ptr->total_mass = cm.total_mass;
	pal_ptr->xyz_mass_centre[0] = (int)std::round(cm.cx);
	pal_ptr->xyz_mass_centre[1] = (int)std::round(cm.cy);
	pal_ptr->xyz_mass_centre[2] = (int)std::round(cm.cz);

	DEBUG_LOG_ENDL("Новый центр масс паллета: (" << pal_ptr->xyz_mass_centre[0] << ", "
		<< pal_ptr->xyz_mass_centre[1] << ", "
		<< pal_ptr->xyz_mass_centre[2] << ") с массой " << pal_ptr->total_mass);
}

box* box_placement_handle(pallet* pal_ptr, zone* zone_ptr, vector<box*> total_boxes) {
	DEBUG_LOG_ENDL("\n========== BOX_PLACEMENT_HANDLE ==========");
	DEBUG_LOG_ENDL("Целевая зона: pos=(" << zone_ptr->xyz[0] << "," << zone_ptr->xyz[1] << "," << zone_ptr->xyz[2] 
		 << ") size=(" << zone_ptr->xyz_size[0] << "x" << zone_ptr->xyz_size[1] << "x" << zone_ptr->xyz_size[2] << ")");
	DEBUG_LOG_ENDL("Коробок для проверки: " << total_boxes.size());
	DEBUG_LOG_ENDL("Текущая масса паллета: " << pal_ptr->total_mass << " / " << pal_ptr->max_mass);
	
	box* cur_box_ptr = nullptr;
	int best_idx = -1;
	array<int, SCORES_NUM> best = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX };

	int debug_skipped_placed = 0;
	int debug_skipped_mass = 0;
	int debug_skipped_size = 0;
	int debug_skipped_height = 0;
	int debug_skipped_collision = 0;
	int debug_fit_count = 0;

	//  ПАРАЛЛЕЛЬНЫЙ ЦИКЛ
#pragma omp parallel for schedule(dynamic, 4) \
        shared(best, best_idx, cur_box_ptr, pal_ptr, zone_ptr, debug_skipped_placed, debug_skipped_mass, debug_skipped_size, debug_skipped_height, debug_skipped_collision, debug_fit_count) \
        firstprivate(pal_ptr, zone_ptr)
	for (int i = 0; i < (int)total_boxes.size(); ++i) {
		box* box_ptr = total_boxes[i];

		if (box_ptr->placed) {
			#pragma omp atomic
			debug_skipped_placed++;
			continue;
		}
		if (box_ptr->mass + pal_ptr->total_mass > pal_ptr->max_mass) {
			#pragma omp atomic
			debug_skipped_mass++;
#ifdef DEBUG_PALLET
			#pragma omp critical(debug_print)
			{
				cout << "  [Box " << i << "] SKIP: масса " << box_ptr->mass << " + " << pal_ptr->total_mass << " > " << pal_ptr->max_mass << endl;
			}
#endif
			continue;
		}

		array<int, SCORES_NUM> local_best = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX };
		int local_best_rot = 0;
		int local_best_x = -1, local_best_z = -1;
		bool local_any_fit = false;

		int rotate_i = box_ptr->full_rotateble ? 3 : 2;

#ifdef DEBUG_PALLET
		#pragma omp critical(debug_print)
		{
			cout << "\n  [Box " << i << "] Размер: (" << box_ptr->xyz_size[0][0] << "x" << box_ptr->xyz_size[0][1] << "x" << box_ptr->xyz_size[0][2] << "), масса=" << box_ptr->mass << ", поворотов=" << rotate_i << endl;
		}
#endif

		for (int j = 0; j < rotate_i; ++j) {
			int w = box_ptr->xyz_size[j][0];
			int h = box_ptr->xyz_size[j][1];
			int d = box_ptr->xyz_size[j][2];

			if (w > zone_ptr->xyz_size[0] || h > zone_ptr->xyz_size[1] || d > zone_ptr->xyz_size[2]) {
#ifdef DEBUG_PALLET
				#pragma omp critical(debug_print)
				{
					cout << "    Rot[" << j << "] (" << w << "x" << h << "x" << d << "): НЕ ВЛАЗИТ в зону";
					if (w > zone_ptr->xyz_size[0]) cout << " [W>" << zone_ptr->xyz_size[0] << "]";
					if (h > zone_ptr->xyz_size[1]) cout << " [H>" << zone_ptr->xyz_size[1] << "]";
					if (d > zone_ptr->xyz_size[2]) cout << " [D>" << zone_ptr->xyz_size[2] << "]";
					cout << endl;
				}
#endif
				#pragma omp atomic
				debug_skipped_size++;
				continue;
			}

			int out_x, out_z;
			double ratio;

			if (!can_place_box_height(pal_ptr, zone_ptr, w, d, out_x, out_z, ratio)) {
#ifdef DEBUG_PALLET
				#pragma omp critical(debug_print)
				{
					cout << "    Rot[" << j << "] (" << w << "x" << h << "x" << d << "): НЕТ ОПОРЫ (height_map)" << endl;
				}
#endif
				#pragma omp atomic
				debug_skipped_height++;
				continue;
			}

			if (!fits_without_collision(out_x, zone_ptr->xyz[1], out_z, w, h, d, pal_ptr->placed_boxes)) {
#ifdef DEBUG_PALLET
				#pragma omp critical(debug_print)
				{
					cout << "    Rot[" << j << "] (" << w << "x" << h << "x" << d << "): КОЛЛИЗИЯ на pos=(" << out_x << "," << zone_ptr->xyz[1] << "," << out_z << ")" << endl;
				}
#endif
				#pragma omp atomic
				debug_skipped_collision++;
				continue;
			}

			auto res = assess_box_in_zone(zone_ptr, out_x, zone_ptr->xyz[1], out_z, w, h, d,
				box_ptr->mass, ratio, pal_ptr, i, j, total_boxes);

#ifdef DEBUG_PALLET
			#pragma omp critical(debug_print)
			{
				cout << "    Rot[" << j << "] (" << w << "x" << h << "x" << d << "): OK! pos=(" << out_x << "," << zone_ptr->xyz[1] << "," << out_z << ") ratio=" << ratio << " scores=[" << res[0] << "," << res[1] << "," << res[2] << "," << res[3] << "]" << endl;
			}
#endif

			if (res < local_best) {
				local_best = res;
				local_best_rot = j;
				local_best_x = out_x;
				local_best_z = out_z;
				local_any_fit = true;
			}
		}

	
		if (local_any_fit) {
			#pragma omp atomic
			debug_fit_count++;
#pragma omp critical(update_best)
			{
				if (local_best < best) {
					best = local_best;
					best_idx = i;
					cur_box_ptr = box_ptr;
					box_ptr->rotate = local_best_rot;
					box_ptr->temp_xz[0] = local_best_x;
					box_ptr->temp_xz[1] = local_best_z;
					DEBUG_LOG_ENDL("  >>> Новый лучший кандидат: Box " << i << " rot=" << local_best_rot << " scores=[" << local_best[0] << "," << local_best[1] << "," << local_best[2] << "]");
				}
			}
		}
	}

	DEBUG_LOG_ENDL("\n--- СТАТИСТИКА ПОИСКА ---");
	DEBUG_LOG_ENDL("Пропущено (уже placed): " << debug_skipped_placed);
	DEBUG_LOG_ENDL("Пропущено (превышение массы): " << debug_skipped_mass);
	DEBUG_LOG_ENDL("Пропущено (не влезает в зону): " << debug_skipped_size);
	DEBUG_LOG_ENDL("Пропущено (нет опоры/height_map): " << debug_skipped_height);
	DEBUG_LOG_ENDL("Пропущено (коллизия): " << debug_skipped_collision);
	DEBUG_LOG_ENDL("Подошли хотя бы в одном повороте: " << debug_fit_count);

	if (best_idx == -1) {
		DEBUG_LOG_ENDL(">>> РЕЗУЛЬТАТ: НЕ НАЙДЕНО подходящей коробки для этой зоны!");
		pal_ptr->failed_in_row++;
		return nullptr;
	}

	DEBUG_LOG_ENDL(">>> РЕЗУЛЬТАТ: Выбрана Box " << best_idx << " размер=(" 
		 << cur_box_ptr->xyz_size[cur_box_ptr->rotate][0] << "x" 
		 << cur_box_ptr->xyz_size[cur_box_ptr->rotate][1] << "x" 
		 << cur_box_ptr->xyz_size[cur_box_ptr->rotate][2] << ") rot=" << cur_box_ptr->rotate);

	place_box(pal_ptr, zone_ptr, cur_box_ptr, total_boxes);
	return cur_box_ptr;
}

int get_max_remaining_box_height(vector<box*> total_boxes) {
	int max_h = 0;
	for (auto* b : total_boxes) {
		if (b->placed) continue;
		// обе ориентации, на всякий случай
		max_h = std::max(max_h, b->xyz_size[0][1]);
		max_h = std::max(max_h, b->xyz_size[1][1]);
	}
	return max_h;
}
array<int, SCORES_NUM> assess_box_in_zone(zone* zone_ptr, int bx, int by, int bz, int bw, int bh, int bd, int mass, double ratio, pallet* pal_ptr, int index, int rotate, vector<box*> total_boxes) {

	int com_y_score, com_center_score;
	if (pal_ptr->center_mass_or_max_volume == 0) { // Если укладка по центру масс
		double cx_box = bx + bw / 2.0;
		double cy_box = by + bh / 2.0;
		double cz_box = bz + bd / 2.0;

		CenterMassResult cm = simulate_center_mass(pal_ptr, mass, cx_box, cy_box, cz_box);
		// идеальный центр масс паллета

		// отклонение от центра по XZ
		double dx = cm.cx - pal_ptr->ideal_cx;
		double dz = cm.cz - pal_ptr->ideal_cz;
		double dist2_xz = dx * dx + dz * dz;

		com_y_score = (int)std::round(cm.cy * 100.0);      // ниже центр масс по Y
		com_center_score = (int)std::round(dist2_xz * 100.0);   // ближе к центру по XZ
	}


	int w_zone = zone_ptr->xyz_size[0] - (bx - zone_ptr->xyz[0]);
	int y_zone = zone_ptr->xyz_size[1] - (by - zone_ptr->xyz[1]);
	int d_zone = zone_ptr->xyz_size[2] - (bz - zone_ptr->xyz[2]);

	int height_diff = zone_ptr->xyz[1] + bh; 
	//if ((pal_ptr->xyz_size[0] - bw) > (pal_ptr->xyz_size[0] - bd)) height_diff += 10000; // длинная сторона коробки смотри на длинную сторону паллета 
	//float orient_one = ((float)pal_ptr->xyz_size[0] / (float)bw) * ((float)pal_ptr->xyz_size[2] / (float)bd);
	//float orient_two = ((float)pal_ptr->xyz_size[0] / (float)bd) * ((float)pal_ptr->xyz_size[2] / (float)bw);
	//cout << "orient_one=" << ((float)pal_ptr->xyz_size[0] / (float)bw) << " * " << ((float)pal_ptr->xyz_size[2] / (float)bd) << " = " << orient_one
	//	<< ", orient_two=" << ((float)pal_ptr->xyz_size[0] / (float)bd) << " * " << ((float)pal_ptr->xyz_size[2] / (float)bw) << " = " << orient_two
	//	<< endl;
	//if (orient_one < orient_two) {
	//	cout << "ЛУЧШЕ ПОВЕРНУТЬ КОРОБКУ" << endl;
	//	height_diff += 10000; // если коробка лучше укладывается в другую ориентацию, штрафуем
	//}

	if ((pal_ptr->xyz_size[0] / bw) * (pal_ptr->xyz_size[2] / bd) <
		(pal_ptr->xyz_size[0] / bd) * (pal_ptr->xyz_size[2] / bw)) {
		cout << "ЛУЧШЕ ПОВЕРНУТЬ КОРОБКУ" << endl;
		height_diff += 500; // если коробка лучше укладывается в другую ориентацию, штрафуем
	}

	if (!pal_ptr->placed_boxes.empty()) {
		auto prev_box = pal_ptr->placed_boxes[pal_ptr->placed_boxes.size() - 1];
		if (prev_box->xyz_size[rotate][0] == bw && prev_box->xyz_size[rotate][1] == bh && prev_box->xyz_size[rotate][2] == bd) {
			if (prev_box->rotate != rotate) {
				height_diff += 500; // если предыдущая коробка была такого же размера, но в другой ориентации, штрафуем
			}
		}
	}


	int waste = w_zone * d_zone - bw * bd;
	int long_side = (std::max)(w_zone - bw, d_zone - bd);
	int short_side = (std::min)(w_zone - bw, d_zone - bd);

	int max_remaining_box_height = get_max_remaining_box_height(total_boxes);
	//if (max_remaining_box_height > bh && (pal_ptr->xyz_size[1] - (by + bh)) < max_remaining_box_height) {
	//	// если после этой укладки не останется места для самой высокой оставшейся коробки
	//	height_diff += 10000; // штрафуем сильно
	//}
	if (ratio < 0.8) height_diff += 500; // если ratfio меньше 0.8, штрафуем
	if (bh > bw && bh > bd) height_diff += 200; // преиущественно коробки должны ложиться плашмя
	height_diff += (index * 10); // Коробки отсортированы по убыванию. Чем больше индекс у коробки тем она меньше

	if (pal_ptr->center_mass_or_max_volume == 0) { // Если укладка по центру масс
		return array<int, SCORES_NUM>{ height_diff, com_center_score, com_y_score, waste, long_side, short_side };
	}
	else if (pal_ptr->center_mass_or_max_volume == 1) { // Если укладка по максимальному объему
		return array<int, SCORES_NUM>{ height_diff, waste, long_side, short_side, INT_MAX, INT_MAX };
	}
}

bool can_place_box_height(
	pallet* pal_ptr,
	zone* zone,
	int bw, int bd,
	int& out_x, int& out_z,
	double& ratio
)
{
	int y0 = zone->xyz[1];
	if (y0 == 0) {
		out_x = zone->xyz[0];
		out_z = zone->xyz[2];
		ratio = 1.0;
		return true;
	}

	int x0 = zone->xyz[0];
	int z0 = zone->xyz[2];
	int wz = zone->xyz_size[0];
	int dz = zone->xyz_size[2];
	int palW = pal_ptr->xyz_size[0];
	int palD = pal_ptr->xyz_size[2];

	if (x0 + wz > palW || z0 + dz > palD || wz < bw || dz < bd) {
		return false;
	}

	const vector<vector<int>>& height_map = pal_ptr->height_map;
	int total_area = bw * bd;
	int min_support_pixels = (int)(MIN_SUPPORT * total_area);

	int max_start_x = std::min(x0 + 3, x0 + wz - bw); 
	int max_start_z = std::min(z0 + 3, z0 + dz - bd); 

	for (int px = x0; px <= max_start_x; ++px) {
		for (int pz = z0; pz <= max_start_z; ++pz) {

			int px_end = px + bw;
			int pz_end = pz + bd;

			if (px_end > palW || pz_end > palD) {
				continue;
			}

			int supported_area = 0;

			for (int x = px; x < px_end; ++x) {
				const vector<int>& row = height_map[x];

				for (int z = pz; z < pz_end; ++z) {
					if (row[z] == y0) {
						++supported_area;
					}
				}
			}

			if (supported_area >= min_support_pixels) {
				out_x = px;
				out_z = pz;
				ratio = static_cast<double>(supported_area) / total_area;
				return true;
			}
		}
	}

	return false;
}



bool can_place_box_in_zone(zone* zone, int w, int h, int d) {
	DEBUG_LOG_ENDL("Проверяю можно ли поместить коробку размером (" << w << ", " << h << ", " << d << ") в зону размером (" << zone->xyz_size[0] << ", " << zone->xyz_size[1] << ", " << zone->xyz_size[2] << ")");
	DEBUG_LOG_ENDL("Координаты зоны (" << zone->xyz[0] << ", " << zone->xyz[1] << ", " << zone->xyz[2] << ")");
	if (w <= zone->xyz_size[0] &&
		h <= zone->xyz_size[1] &&
		d <= zone->xyz_size[2]) {
		DEBUG_LOG_ENDL("Проверил, что коробка помещается");
		return true;
	}
	DEBUG_LOG_ENDL("Проверил, что коробка НЕ помещается");
	
	return false;

}
