#ifndef DATA_H_INCLUDED
#define DATA_H_INCLUDED
#include <iostream>
#include <vector>
#include <array>
#include <fstream>


#define SCORES_NUM 6 // сколько у нас всего параметров оценки у коробки - РАЗМЕР МАССИВА С ОЧКАМИ
#define SCORE_HEIGHT_IND 0 // на каком индексе у нас очки по высоте
#define SCORE_WASTE_AREA_IND 1 // на каком индексе очки по вытесняемой площади
#define SCORE_LONG_SIDE_IND 2 // не ебу что это за очки 
#define SCORE_SHORT_SIDE_IND 3


#define PALLET_X 15
#define PALLET_Y 15
#define PALLET_Z 12


#define MIN_SUPPORT 0.8 // минимальная поддержка коробки снизу в процентах от площади дна коробки

using namespace std;

 struct box {
	int xyz[3] = {-1,-1, -1}; // коориднаты верхней левой точки
	int xyz_size[2][3]; // ширина[x][0], высота[x][1], глубота[x][2], [0][x] без поворота, [1][x] с поворотом

	int temp_xz[2]; // временные координаты для оценки коробки в зоне

	int mass = 0; // масса коробки
	bool placed = false; // РАЗМЩЕНА ЛИ ЭТА КОРОБКА В ПАЛЛЕТЕ - 
	int rotate; // если 1 то кабы она повернута да. Указывает на массив xyz_size какой из двух использовать
	
	array<int, SCORES_NUM> scores = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX };  // оценка коробки, типо там по высоте, по  насколько она хорошо помещатеся и т.д. - это для типо функции по подсчету оценки да 
	int index; // для рендера на аскии
} ;

struct zone {
	int xyz[3]; 
	int xyz_size[3]; // ширина[0], высота[1], глубота[2]
	bool usable = true; // жива ли зона - если фалс то зона метрва да

	int index = 0;
};

 struct pallet {
	//int xyz_size[3] = { 1200, 1555, 800 }; // ширина[0], высота[1], глубота[2]
	const int xyz_size[3] = { PALLET_X, PALLET_Y, PALLET_Z }; // ширина[0], высота[1], глубота[2]
	int xyz_mass_centre[3] = { 0, 0, 0 }; // центр массы паллета

	double ideal_cx = xyz_size[0] / 2.0; // идеальный центр масс по X
	double ideal_cz = xyz_size[2] / 2.0; // идеальный центр масс по Z

	int total_mass = 0; // суммарный вес 
	const int max_mass = 1050; // макс вес паллета

	vector<box*> placed_boxes; // размещенные коробоки
	vector<zone*> zone_vector = { new zone{ {0, 0, 0}, {xyz_size[0], xyz_size[1], xyz_size[2]}, true}}; // вектор с активными зонами
	//unique_ptr<zone> zone_vector = make_unique<zone>(zone{ {0,0,0},{10,10,10}, true });
	vector<zone*> zone_dead_vector; // мертвые зоны - зоны на которых невозможно размеситть коробок

	int max_height_box = INT_MAX; // высота самой высокой коробки размещенной на паллете

	int placed_since_meb = 0;  // сколько коробок уложено с последней дефрагации
	int failed_in_row = 0;  // сколько подряд неудач найти место

	bool was_defrag = false; // была ли дефрагментация

	vector<vector<int>> height_map; // Вектор который хранит y в точке  [x][z]
};

struct box_property {
	int Quantity; //кол-во коробок

	int width;
	int height; // высота, глубина, ширина
	int depth;

	int weight; // вес коробки
};

struct CenterMassResult {
	double cx;
	double cy;
	double cz;
	int total_mass; 
};


 extern vector<box*> total_boxes; // Все коробки которые возомжно разместить

 extern std::ofstream debug_log;


#endif