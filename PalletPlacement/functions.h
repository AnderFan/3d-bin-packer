#pragma once
#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include "data.h"

void height_map_init(pallet* pal_ptr); // инициализация карты высот паллета
// Возможно ли вовсе разместить хоть что то на этом паллете
bool is_placement_possible(pallet* pallet_pointer);

// выбрать зону для размщенения коробок в ней 
zone* select_zone(pallet* pallet_pointer);

/* разделить зону(после тогу как поместили короб в неё)
* @param Ну сюда мы ложим нашу паллету, указатель на зону которую надо разделить, и указатель на коробку которую только что сюда положили
* @return Ничаво, напрямую изменяет зоны в паллете
*/
void split_zone(pallet* pallet_pointer, zone* zone_to_split_pointer, box* box_that_just_was_placed);

//найти и разместить коробку для данной зоны, возворащает коробку которую разместило, или же НУЛЛ если хуйня история
box* box_placement_handle(pallet* pallet_pointer, zone* zone_to_handle);

// ну за такое можно и выпить
void celebrate(void);

CenterMassResult simulate_center_mass(const pallet* pal_ptr, int box_mass, double cx_box, double cy_box, double cz_box); // симуляция центра масс паллета после добавления коробки

void center_mass_calculate(pallet* pal_ptr, box* box_ptr); // расчет центра масс паллета после добавления коробки

// УТОЧНИ У БЕЗЗЫММЯННИКА ПРО ДЕТАЛИ, эта хуйомба убивает мусорную зону -> ту зону на которой нельзя разместить коробку
void kill_zone(pallet* pallet_pointer, zone* zone_to_kill);

/* @brief ВОТ ОНА, ОНА, эта функия считает кол-во очков для данной коробки И ЛОЖИТ ЭТИ ОЧКИ НАЗАД В КОРОБКУ
* @param ну там типо и так понятно
* @return нихуя ОЧКИ ХРАНЯТСЯ В САМОЙ КОРОБКЕ
* */
array<int, SCORES_NUM> assess_box_in_zone(zone* zone_ptr, int bx, int by, int bz, int bw, int bh, int bd, int mass, pallet* pal_ptr);

/*
@brief после того как мы посчитали очки для каждой коробки, мы этой функцией выбираем лучшую
@return возвразащает указатель лучшей коробки

*/
box* select_best_box();

/* ну типо размещаем данную коробку в данную зону
	@param pal_pointer нужен шоб добавить в сам паллет коробку которую разместили
*/
void place_box(pallet* pallet_pointer, zone* zone_pointer, box* box_pointer);


// aux functions

// функция которая проверяет можно ли физически разместить данную коробку в данной зоне
bool can_place_box_in_zone(zone* zone, int w, int h, int d);

// функция которая проверяет можно ли разместить коробку в зоне с учетом карты высот паллета
bool can_place_box_height_map(pallet* pal_ptr, zone* zone, int bw, int bd, int& out_x, int& out_z);

void zone_cleanup(pallet* pal_ptr); // чистим зоны от мусора. 

void check_meb(pallet* pal_ptr);

bool fits_without_collision(int bx, int by, int bz, int w, int h, int d, const vector<box*>& placed);

/*
хуйлуша которая создает файл с отрисовкой паллета по слоям
*/
void create_ascii_layers(pallet* pallet_ptr);

/*
бля даже не спрашивай
*/
void assign_index_to_boxes(pallet* pallet_ptr);

bool merge_any_pair_XYZ(pallet* pal); // попытка чё-нибудь слить, что бы не расстраиваться

void record_pallet_details(pallet* pallet_ptr, vector<box*> fucking_die_already);

void create_ascii_layers_zones(pallet* given_pal_ptr);

void create_ascii_layers_m(pallet* given_pal_ptr);

void replace_zones_with_meb(pallet* pal); // Удаляем все зоны и строим новые на основании MEB-боксов

void record_pallet_details(pallet* pallet_ptr, vector<box*> fucking_die_already);

bool try_merge_once(std::vector<zone*>& zs); // пытаемся слить хоть одну пару зон, если получилось возвращаем тру
std::vector<zone*> build_meb_zones(pallet* pal); // Строим новый список зон (zone*) из MEB-боксов на основании УЖЕ уложенных коробок.

#endif