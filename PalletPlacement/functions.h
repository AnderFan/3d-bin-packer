#pragma once
#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include "data.h"


void pallet_handle(pallet* pal_ptr, vector<box*> total_boxes); 


// Зоновые функции #####################

// выбрать зону для размщенения коробок в ней 
zone* select_zone(pallet* pallet_pointer);

/* разделить зону(после тогу как поместили короб в неё)
* @param Ну сюда мы ложим нашу паллету, указатель на зону которую надо разделить, и указатель на коробку которую только что сюда положили
* @return Ничаво, напрямую изменяет зоны в паллете
*/
void split_zone(pallet* pallet_pointer, zone* zone_to_split_pointer, box* box_that_just_was_placed);

// Убивает мусорную зону -> ту зону на которой нельзя разместить коробку
void kill_zone(pallet* pallet_pointer, zone* zone_to_kill);

void zone_cleanup(pallet* pal_ptr); // чистим зоны от мусора. 



// MEB-регенерация зон #####################

void replace_zones_with_meb(pallet* pal); // Удаляем все зоны и строим новые на основании MEB-боксов

bool try_merge_once(std::vector<zone*>& zs); // пытаемся слить хоть одну пару зон, если получилось возвращаем тру

std::vector<zone*> build_meb_zones(pallet* pal); // Строим новый список зон (zone*) из MEB-боксов на основании УЖЕ уложенных коробок.

void check_meb(pallet* pal_ptr, vector<box*> total_boxes); // проверяем нужно ли делать дефрагментацию зон

void meb_gen(pallet* pal_ptr); 

// Функции взаимодействия с коробокой #####################
void sort_boxes_decreasing();
void height_map_init(pallet* pal_ptr); // инициализация карты высот паллета

// Возможно ли вовсе разместить хоть что то на этом паллете
bool is_placement_possible(pallet* pallet_pointer, vector<box*> total_boxes);

//найти и разместить коробку для данной зоны, возворащает коробку которую разместило, или же НУЛЛ если хуйня история
box* box_placement_handle(pallet* pallet_pointer, zone* zone_to_handle, vector<box*> total_boxes);

// проверка на коллизию коробки с уже размещенными коробками
bool fits_without_collision(int bx, int by, int bz, int w, int h, int d, const vector<box*>& placed);

// симуляция центра масс паллета после добавления коробки
CenterMassResult simulate_center_mass(const pallet* pal_ptr, int box_mass, double cx_box, double cy_box, double cz_box); 

void center_mass_calculate(pallet* pal_ptr, box* box_ptr); // расчет центра масс паллета после добавления коробки

// эта функия считает кол-во очков для данной коробки И ЛОЖИТ ЭТИ ОЧКИ НАЗАД В КОРОБКУ
array<int, SCORES_NUM> assess_box_in_zone(zone* zone_ptr, int bx, int by, int bz, int bw, int bh, int bd,
										int mass, double ratio, pallet* pal_ptr, int index, int rotate, vector<box*> total_boxes);

/*
@brief после того как мы посчитали очки для каждой коробки, мы этой функцией выбираем лучшую
@return возвразащает указатель лучшей коробки
*/
box* select_best_box(vector<box*> total_boxes);

/* размещаем данную коробку в данную зону
	@param pal_pointer нужен шоб добавить в сам паллет коробку которую разместили
*/
void place_box(pallet* pallet_pointer, zone* zone_pointer, box* box_pointer, vector<box*> total_boxes);

// функция которая проверяет можно ли физически разместить данную коробку в данной зоне
bool can_place_box_in_zone(zone* zone, int w, int h, int d);

// функция которая проверяет можно ли разместить коробку в зоне с учетом карты высот паллета
bool can_place_box_height(pallet* pal_ptr, zone* zone, int bw, int bd, int& out_x, int& out_z, double& ratio);

void celebrate(void);



// Аски рендер паллета #####################

void create_ascii_layers(pallet* pallet_ptr); // Аски рендер слоев паллета

void assign_index_to_boxes(pallet* pallet_ptr);// присваиваем индекс коробкам для аски рендера

bool merge_any_pair_XYZ(pallet* pal); // попытка чё-nибудь слить, что бы не расстраиваться

void record_pallet_details(pallet* pallet_ptr, vector<box*> fucking_die_already); // записываем детали паллета в лог файл

void create_ascii_layers_zones(pallet* given_pal_ptr); // аски рендер зон паллета

void create_ascii_layers_m(pallet* given_pal_ptr); // аски рендер MEB-боксов паллета

void record_pallet_details(pallet* pallet_ptr, vector<box*> fucking_die_already); // записываем детали паллета в лог файл

#endif