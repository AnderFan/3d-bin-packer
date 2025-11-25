//#include "data.h"
#include "functions.h"
#include <iostream>
#include <array>
#include <algorithm>
#include <fstream>
#include <string>
#include <format>


using namespace std;

ofstream pallet_details;
ofstream ascii_view;
pallet* pal_ptr;

string line;

char layer_ascii[PALLET_Z + 1][PALLET_X + 1];


void preppend_header(void);

void render_layer(int layer_num);

void print_boxes(vector<box> boxes);
void render_layer_zones(int layer_num);
void render_layer_boxes_m(int layer_num);

void assign_index_to_zones(pallet* pal_ptr) {
	//box pl_box;
	for (int i = 0; i < pal_ptr->zone_vector.size(); i++) {
		pal_ptr->zone_vector.at(i)->index = i + 1;

	}
}

void record_pallet_details(pallet* given_pal_ptr, vector<box*> total_boxes) {
	pallet_details.open("pallet_details.txt");
	box box;
	for (int i = 0; i < given_pal_ptr->placed_boxes.size(); i++) {
		line.clear();
		box = *given_pal_ptr->placed_boxes.at(i);

		line.append("Box index: ");
		line.append(to_string(box.index));
		line.append("\n");

		line.append("Its start X ");
		line.append(to_string(box.xyz[0]));
		line.append(", and its X end ");
		line.append(to_string(box.xyz[0] + box.xyz_size[box.rotate][0]));
		line.append("\n");

		line.append("Its start Y ");
		line.append(to_string(box.xyz[1]));
		line.append(", and its Y end ");
		line.append(to_string(box.xyz[1] + box.xyz_size[box.rotate][1]));
		line.append("\n");


		line.append("Its start Z ");
		line.append(to_string(box.xyz[2]));
		line.append(", and its Z end ");
		line.append(to_string(box.xyz[2] + box.xyz_size[box.rotate][2]));
		line.append("\n");

		pallet_details << line;
	}

	pallet_details << given_pal_ptr->placed_boxes.size() << " boxes placed out of" << total_boxes.size() << endl;
	pallet_details << endl << endl << "ZONES" << endl;

	assign_index_to_zones(given_pal_ptr);
	zone zone;
	for (int i = 0; i < given_pal_ptr->zone_vector.size(); i++) {
		line.clear();
		zone = *given_pal_ptr->zone_vector.at(i);

		line.append("Zone index: ");
		line.append(to_string(zone.index));
		line.append("\n");

		line.append("Its start X ");
		line.append(to_string(zone.xyz[0]));
		line.append(", and its X end ");
		line.append(to_string(zone.xyz[0] + zone.xyz_size[0]));
		line.append("\n");

		line.append("Its start Y ");
		line.append(to_string(zone.xyz[1]));
		line.append(", and its Y end ");
		line.append(to_string(zone.xyz[1] + zone.xyz_size[1]));
		line.append("\n");


		line.append("Its start Z ");
		line.append(to_string(zone.xyz[2]));
		line.append(", and its Z end ");
		line.append(to_string(zone.xyz[2] + zone.xyz_size[2]));
		line.append("\n");

		pallet_details << line;
	}


	line.clear();
	pallet_details.close();
}


void flush_ascii_layer() {

	for (int i = 0; i < PALLET_Z; i++) {
		for (int j = 0; j < PALLET_X; j++) {
			layer_ascii[i][j] = ' ';
		}

	}

}


void assign_index_to_boxes(pallet* pal_ptr) {
	//box pl_box;
	for (int i = 0; i < pal_ptr->placed_boxes.size(); i++) {
		pal_ptr->placed_boxes.at(i)->index = i;

	}
}

void create_ascii_layers_zones(pallet* given_pal_ptr) {
	ascii_view.open("ascii_layers_zones.txt");


	pal_ptr = given_pal_ptr;
	//	preppend_header();

	assign_index_to_zones(pal_ptr);
	assign_index_to_boxes(pal_ptr);

	//assign_index_to_boxes();

	for (int i = 0; i < pal_ptr->xyz_size[1]; i++) {
		render_layer_zones(i);
	}

	ascii_view.close();


}

void create_ascii_layers(pallet* given_pal_ptr) {

	pal_ptr = given_pal_ptr;
	ascii_view.open("ascii_layers.txt");
	preppend_header();
	assign_index_to_boxes(pal_ptr);



	for (int i = 0; i < pal_ptr->xyz_size[1]; i++) {
		render_layer(i);
	}

	ascii_view.close();


}

void create_ascii_layers_m(pallet* given_pal_ptr) {

	pal_ptr = given_pal_ptr;
	ascii_view.open("ascii_layers_boxes_m.txt");
	preppend_header();
	assign_index_to_boxes(pal_ptr);



	for (int i = 0; i < pal_ptr->xyz_size[1]; i++) {
		render_layer_boxes_m(i);
	}

	ascii_view.close();


}







void print_hor_border(void) {

	// принтим верхний ободок
	//ascii_view << "##";
	line.clear();
	line.append("##");

	for (int i = 0; i < pal_ptr->xyz_size[0]; i++) {
		if (i < 10) {
			line.append("x");
			line.append(to_string(i));
		}
		else
		{
			line.append(to_string(i));
		}
	}
	line.append("##");

	ascii_view << line;
	ascii_view << endl;

	line.clear();
	line.append("##");
	for (int i = 0; i < pal_ptr->xyz_size[0]; i++) {
		line.append("x#");
	}
	line.append("##");

	ascii_view << line;
	ascii_view << endl;
	line.clear();
}


void render_layer_zones(int layer_num) {
	ascii_view << endl << "Layer num " << layer_num << endl;

	box pl_box;
	vector<box> boxes_on_this_layer;
	vector < zone > zones_on_this_layer;
	zone cur_zone;
	bool no_box;

	vector<box> boxes_on_this_line;
	vector<zone> zones_on_this_line;
	for (int i = 0; i < pal_ptr->placed_boxes.size(); i++) {
		pl_box = *(pal_ptr->placed_boxes.at(i));
		//cur_zone = *(pal_ptr->zone_vector.at(i));
		if (pl_box.xyz[1] <= layer_num && pl_box.xyz[1] + pl_box.xyz_size[pl_box.rotate][1] > layer_num) {
			boxes_on_this_layer.push_back(pl_box);
		}



	}


	for (int i = 0; i < pal_ptr->zone_vector.size(); i++) {

		cur_zone = *(pal_ptr->zone_vector.at(i));

		cout << "Zone id " << cur_zone.index << " assssede";
		if (cur_zone.xyz[1] <= layer_num && cur_zone.xyz[1] + cur_zone.xyz_size[1] > layer_num) {
			cout << "And its on this layer";
			zones_on_this_layer.push_back(cur_zone);
		}
		cout << endl;

	}
	//cout << "zonmes on layer " << zones_on_this_layer.size() << endl;
	//print_boxes(boxes_on_this_layer);

	print_hor_border();

	// вглубь (если смотреть сверху то снизу вверх)
	for (int i = 0; i < pal_ptr->xyz_size[2]; i++)
	{
		//cout << endl;
		line.append("##");
		boxes_on_this_line.clear();
		//zones_on_this_layer.clear();

		zones_on_this_line.clear();
		// добавляем коробки которые находятся на этой линии 
		for (int j = 0; j < boxes_on_this_layer.size(); j++) {
			if (boxes_on_this_layer.at(j).xyz[2] <= i && boxes_on_this_layer.at(j).xyz[2] + boxes_on_this_layer.at(j).xyz_size[boxes_on_this_layer.at(j).rotate][2] > i) {
				//cout << "RREURUEURERUEURURUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU" << endl;
				//cout << "box z start " << boxes_on_this_layer.at(j).xyz[2] << ", and with z end " << boxes_on_this_layer.at(j).xyz[2] + boxes_on_this_layer.at(j).xyz_size[boxes_on_this_layer.at(j).rotate][2] << endl;
				boxes_on_this_line.push_back(boxes_on_this_layer.at(j));
			}
		}

		for (int j = 0; j < zones_on_this_layer.size(); j++) {

			cur_zone = zones_on_this_layer.at(j);

			if (cur_zone.xyz[2] <= i && cur_zone.xyz[2] + cur_zone.xyz_size[2] > i) {
				zones_on_this_line.push_back(cur_zone);
				if (cur_zone.index == 1)
					cout << "zone 1 is on this line!!!!" << endl;
			}

		}

		cout << "zones on layer " << layer_num << ", line " << i << " zones num " << zones_on_this_layer.size() << endl;
		//print_boxes(boxes_on_this_line);

		// прохоидмся по Х, слева направо
		//cout << endl;
		for (int j = 0; j < pal_ptr->xyz_size[0]; j++) {
			no_box = true;

			for (int k = 0; k < zones_on_this_line.size(); k++) {

				cur_zone = zones_on_this_line.at(k);

				if (zones_on_this_line.at(k).xyz[0] <= j && zones_on_this_line.at(k).xyz[0] + zones_on_this_line.at(k).xyz_size[0] > j) {

					//cout << "box index " << boxes_on_this_line.at(k).index <<", x start " << boxes_on_this_line.at(k).xyz[0] << ", and with x end " << boxes_on_this_line.at(k).xyz[0] + boxes_on_this_layer.at(k).xyz_size[boxes_on_this_line.at(k).rotate][0] << ", on position " << j << endl;
					///cout << "cur zone index " << cur_zone.index << endl;
					if (zones_on_this_line.at(k).index < 10) { // числа меньше 10 по разверу как один чар, а надо как два (9 -> 09)
						line.append("z");
					}

					line.append(to_string(cur_zone.index));


					//line.append(zones_on_this_line.at(k));

					no_box = false;
				}


			}


			for (int k = 0; k < boxes_on_this_line.size(); k++) {
				if (boxes_on_this_line.at(k).xyz[0] <= j && boxes_on_this_line.at(k).xyz[0] + boxes_on_this_line.at(k).xyz_size[boxes_on_this_line.at(k).rotate][0] > j) {

					//cout << "box index " << boxes_on_this_line.at(k).index <<", x start " << boxes_on_this_line.at(k).xyz[0] << ", and with x end " << boxes_on_this_line.at(k).xyz[0] + boxes_on_this_layer.at(k).xyz_size[boxes_on_this_line.at(k).rotate][0] << ", on position " << j << endl;

					//if (boxes_on_this_line.at(k).index < 10) { // числа меньше 10 по разверу как один чар, а надо как два (9 -> 09)
					line.append("bb");
					//}

					//line.append(to_string(boxes_on_this_line.at(k).index));

					no_box = false;
				}
			}

			if (no_box) {
				line.append("  ");
			}

		}

		line.append("##");
		ascii_view << line;
		ascii_view << endl;
		//ascii_view << line;
		//ascii_view << endl;
		line.clear();
	}
	print_hor_border();

	cout << "layer " << layer_num << " finished!" << endl;
}

void render_layer_boxes_m(int layer_num) {
	ascii_view << endl << "Layer num " << layer_num << endl;

	box pl_box;
	vector<box> boxes_on_this_layer;
	bool no_box;
	flush_ascii_layer(); // очищаем слой

	//vector<box> boxes_on_this_line;
	for (int i = 0; i < pal_ptr->placed_boxes.size(); i++) {
		pl_box = *(pal_ptr->placed_boxes.at(i)); 

		if (pl_box.xyz[1] <= layer_num && pl_box.xyz[1] + pl_box.xyz_size[pl_box.rotate][1] > layer_num) {
			boxes_on_this_layer.push_back(pl_box);
		}


	}

	for (int i = 0; i < PALLET_X + 2; i++) {
		ascii_view << '#';
	}

	for (box box : boxes_on_this_layer) {

		for (int i = box.xyz[0] + 1; i < box.xyz[0] + box.xyz_size[box.rotate][0]; i++) {
			layer_ascii[box.xyz[2]][i] = '-';
			layer_ascii[box.xyz[2] + box.xyz_size[box.rotate][2]][i] = '-';
		}


		for (int i = box.xyz[2]; i < box.xyz[2] + box.xyz_size[box.rotate][2]; i++) {
			layer_ascii[i][box.xyz[0]] = '|';
			layer_ascii[i][box.xyz[0] + box.xyz_size[box.rotate][0]] = '|';
		}


		layer_ascii[box.xyz[2]][box.xyz[0]] = 'x'; // left top
		layer_ascii[box.xyz[2]][box.xyz[0] + box.xyz_size[box.rotate][0]] = 'x'; // right top
		layer_ascii[box.xyz[2] + box.xyz_size[box.rotate][2]][box.xyz[0]] = 'x'; // left bottom
		layer_ascii[box.xyz[2] + box.xyz_size[box.rotate][2]][box.xyz[0] + box.xyz_size[box.rotate][0]] = 'x';


		if (box.index >= 10) {
			layer_ascii[box.xyz[2] + 1][box.xyz[0] + 1] = to_string(box.index).at(0);  // left top
			layer_ascii[box.xyz[2] + 1][box.xyz[0] + 2] = to_string(box.index).at(1); // left top
		}
		else {
			layer_ascii[box.xyz[2] + 1][box.xyz[0] + 1] = to_string(box.index).at(0);  // left top
			//layer_ascii[box.xyz[2] + 1][box.xyz[0] + 2] = 'x'; // left top
		}


	}

	for (int i = 0; i < PALLET_Z; i++) {
		ascii_view << endl;
		ascii_view << "#";
		for (int j = 0; j < PALLET_X; j++) {
			ascii_view << layer_ascii[i][j];
		}
		ascii_view << "#";

	}

	ascii_view << endl;
	for (int i = 0; i < PALLET_X + 2; i++) {
		ascii_view << '#';
	}

	ascii_view << endl;
	cout << "layer " << layer_num << " finished!" << endl;
}


void render_layer(int layer_num) {
	ascii_view << endl << "Layer num " << layer_num << endl;

	box pl_box;
	vector<box> boxes_on_this_layer;
	bool no_box;

	vector<box> boxes_on_this_line;
	for (int i = 0; i < pal_ptr->placed_boxes.size(); i++) {
		pl_box = *(pal_ptr->placed_boxes.at(i));

		if (pl_box.xyz[1] <= layer_num && pl_box.xyz[1] + pl_box.xyz_size[pl_box.rotate][1] > layer_num) {
			boxes_on_this_layer.push_back(pl_box);
		}


	}
	//cout << "boxes on layer " << layer_num << endl;
	//print_boxes(boxes_on_this_layer);

	print_hor_border();

	// вглубь (если смотреть сверху то снизу вверх)
	for (int i = 0; i < pal_ptr->xyz_size[2]; i++)
	{
		//cout << endl;
		line.append("##");
		boxes_on_this_line.clear();
		// добавляем коробки которые находятся на этой линии 
		for (int j = 0; j < boxes_on_this_layer.size(); j++) {
			if (boxes_on_this_layer.at(j).xyz[2] <= i && boxes_on_this_layer.at(j).xyz[2] + boxes_on_this_layer.at(j).xyz_size[boxes_on_this_layer.at(j).rotate][2] > i) {
				//cout << "RREURUEURERUEURURUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU" << endl;
				//cout << "box z start " << boxes_on_this_layer.at(j).xyz[2] << ", and with z end " << boxes_on_this_layer.at(j).xyz[2] + boxes_on_this_layer.at(j).xyz_size[boxes_on_this_layer.at(j).rotate][2] << endl;
				boxes_on_this_line.push_back(boxes_on_this_layer.at(j));
			}
		}

		//cout << "boxes on layer " << layer_num << ", line " << i << endl;
		//print_boxes(boxes_on_this_line);

		// прохоидмся по Х, слева направо
		//cout << endl;
		for (int j = 0; j < pal_ptr->xyz_size[0]; j++) {
			no_box = true;

			for (int k = 0; k < boxes_on_this_line.size(); k++) {
				if (boxes_on_this_line.at(k).xyz[0] <= j && boxes_on_this_line.at(k).xyz[0] + boxes_on_this_line.at(k).xyz_size[boxes_on_this_line.at(k).rotate][0] > j) {

					//cout << "box index " << boxes_on_this_line.at(k).index <<", x start " << boxes_on_this_line.at(k).xyz[0] << ", and with x end " << boxes_on_this_line.at(k).xyz[0] + boxes_on_this_layer.at(k).xyz_size[boxes_on_this_line.at(k).rotate][0] << ", on position " << j << endl;

					if (boxes_on_this_line.at(k).index < 10) { // числа меньше 10 по разверу как один чар, а надо как два (9 -> 09)
						line.append("x");
					}

					line.append(to_string(boxes_on_this_line.at(k).index));

					no_box = false;
				}
			}

			if (no_box) {
				line.append("  ");
			}

		}

		line.append("##");
		ascii_view << line;
		ascii_view << endl;
		//ascii_view << line;
		//ascii_view << endl;
		line.clear();
	}
	print_hor_border();

	cout << "layer " << layer_num << " finished!" << endl;
}

void print_boxes(vector<box> boxes) {
	string print_line_boxes;
	for (int i = 0; i < boxes.size(); i++) {
		print_line_boxes.append("Box with index ");
		print_line_boxes.append(to_string(boxes.at(i).index));
		print_line_boxes.append(";");
	}
	cout << print_line_boxes;
	cout << endl;
}

void preppend_header(void) {
	ascii_view << "# This file generates layer by layer view of given pallet!" << endl;
}