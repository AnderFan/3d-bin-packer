#ifndef DATA_H_INCLUDED
#define DATA_H_INCLUDED
#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <map>

#define SCORES_NUM 6 // сколько у нас всего параметров оценки у коробки - РАЗМЕР МАССИВА С ОЧКАМИ
#define SCORE_HEIGHT_IND 0 // на каком индексе у нас очки по высоте
#define SCORE_WASTE_AREA_IND 1 // на каком индексе очки по вытесняемой площади
#define SCORE_LONG_SIDE_IND 2 
#define SCORE_SHORT_SIDE_IND 3

#define PALLET_X 1200
#define PALLET_Y 1555
#define PALLET_Z 800
#define PALLET_MAX_MASS 1050  // Максимальный вес паллеты по умолчанию (кг)


#define MIN_SUPPORT 0.8 // минимальная поддержка коробки снизу в процентах от площади дна коробки

using namespace std;

 struct box {
	 array<int, SCORES_NUM> scores = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX };  // оценка коробки, типо там по высоте, по  насколько она хорошо помещатеся и т.д. - это для типо функции по подсчету оценки да 
	 int index; // для рендера на аскии
	int xyz[3] = {-1,-1, -1}; // коориднаты верхней левой точки
	int xyz_size[3][3]; // ширина[x][0], высота[x][1], глубота[x][2], [0][x] без поворота, [1][x] с 90 по x, [2][x] с 90 по y, 

	int temp_xz[2]; // временные координаты для оценки коробки в зоне

	int mass = 0; // масса коробки
	
	int rotate; // если 1 то кабы она повернута да. Указывает на массив xyz_size какой из двух использовать
	
	bool full_rotateble = false; // полный поворот
	bool placed = false; // РАЗМЩЕНА ЛИ ЭТА КОРОБКА В ПАЛЛЕТЕ - 
	
} ;

struct zone {
	int xyz[3]; 
	int xyz_size[3]; // ширина[0], высота[1], глубота[2]
	//int index = 0;	 
	bool usable = true; // жива ли зона - если фалс то зона метрва да


};

 struct pallet {
	//int xyz_size[3] = { 1200, 1555, 800 }; // ширина[0], высота[1], глубота[2]
	int xyz_size[3] = { PALLET_X, PALLET_Y, PALLET_Z }; // ширина[0], высота[1], глубота[2]
	double xyz_mass_centre[3] = { 0, 0, 0 }; // центр массы паллета

	double ideal_cx; // идеальный центр 
	double ideal_cy;
	double ideal_cz;


	int total_mass = 0; // суммарный вес 
	int max_mass = PALLET_MAX_MASS; // макс вес паллета (теперь изменяемый)

	vector<box*> placed_boxes; // размещенные коробоки
	//vector<zone*> zone_vector = { new zone{ {0, 0, 0}, {xyz_size[0], xyz_size[1], xyz_size[2]}, true} }; // вектор с активными зонами
	vector<zone*> zone_vector;
	//unique_ptr<zone> zone_vector = make_unique<zone>(zone{ {0,0,0},{10,10,10}, true });
	vector<zone*> zone_dead_vector; // мертвые зоны - зоны на которых невозможно размеситть коробок

	int max_height_box = INT_MAX; // высота самой высокой коробки размещенной на паллете

	int placed_since_meb = 0;  // сколько коробок уложено с последней дефрагации
	int failed_in_row = 0;  // сколько подряд неудач найти место

	bool was_defrag = false; // была ли дефрагментация
	//std::map<pair<int, int>, pair<int, int>> mru_positions;
	vector<vector<int>> height_map; // Вектор который хранит y в точке  [x][z]

	int center_mass_or_max_volume = 0; // 0 - центр масс, 1 - объем

	bool hMaxQtyCheck = false; // Если true, то кол-во коробок не ограничено.
	bool lim_lay = false; // Запретить неполные слои. Только если hMaxQtyCheck = true.	

    // Конструктор для инициализации размеров
    pallet(int x = PALLET_X, int y = PALLET_Y, int z = PALLET_Z, int maxMass = PALLET_MAX_MASS, int centerMassOrMaxVolume = 0, bool hMaxQtyCheck = false, bool lim_lay = false)
        : xyz_size{ x, y, z }, max_mass(maxMass), center_mass_or_max_volume(centerMassOrMaxVolume), hMaxQtyCheck(hMaxQtyCheck), lim_lay(lim_lay) {
         ideal_cx = x / 2.0;
 		ideal_cy = 0.0;
         ideal_cz = z / 2.0;
         zone_vector.push_back(new zone{ {0, 0, 0}, {x, y, z}, true });
     }



	//// ✅ ИСПРАВЛЕННЫЙ конструктор - с ФИГУРНЫМИ СКОБКАМИ в конце!
	//pallet(int x = PALLET_X, int y = PALLET_Y, int z = PALLET_Z,
	//	int maxMass = PALLET_MAX_MASS, int centerMassOrMaxVolume = 0)
	//	: xyz_size{ x, y, z },
	//	max_mass(maxMass),
	//	center_mass_or_max_volume(centerMassOrMaxVolume),
	//	ideal_cx(x / 2.0),
	//	ideal_cz(z / 2.0)
	//{  // ✅ ФИГУРНАЯ СКОБКА, НЕ ТОЧКА С ЗАПЯТОЙ!
	//	zone_vector.push_back(new zone{ {0, 0, 0}, {x, y, z}, 0, true });
	//}
};

struct box_property {
	int Quantity; //кол-во коробок

	int width;
	int height; // высота, глубина, ширина
	int depth;

	int weight; // вес коробки

	bool full_rotateble = false; // полный поворот
};

struct CenterMassResult {
	double cx;
	double cy;
	double cz;
	int total_mass; 
};


 //extern vector<box*> total_boxes; // Все коробки которые возомжно разместить

 extern std::ofstream debug_log;


#endif