#ifndef GAME_CONTROL_AND_LOGIC_H
#define GAME_CONTROL_AND_LOGIC_H

#include <IND_Camera2d.h>
#include "characters.h"
#include "level_manager.h"
#include "interface.h"

namespace game_control {
	enum game_state {show_menu, play_game, change_level, quit, pause_game, _game_over};

	game_state gstate =										show_menu;
	unsigned char current_level =							0;
	string active_message =									"";

	bool show_bareas =										false;
	bool free_control =										false;

	vector2d direction_v;

	//-------------------------Functions-------------------------

	void update_camera(IND_Camera2d &cam, IND_Point pos, IND_Point tl_point, IND_Point br_point, int width, int height);
	void update_ingame_interface(int tl_x, int tl_y, main_hero &hero, info &hero_info);
	void update_ingame_interface(int tl_x, int tl_y, ProgressBar progressBar);
	void main_page_interface(main_page &MainPage);
	void pause_screen (pause &Pause);

	void play(level_manager &lm, chmanager &chm, IND_Camera2d &camera);
	void complete_level(level_manager &lm);
	void load_level(level_manager &lm, chmanager &chm);
	void game_over();
}

namespace game_logic {
	enum spawn_type {random, one_room, every_room};

	//------------------------Constants-----------------------

	const int level_border =										5;
	const float fall_speed =										200.0f;
	const float speed_over_fear_reduce =							2;

	//------------------------Exceptions-------------------------

	class spawn_error {};
	class load_error {};

	//--------------------Update functions--------------------

	void update_characters(chmanager &chm, level_manager &lm, float df);

	//--------------------Check functions---------------------

	void check_npc_collision(chmanager &chm);
	void check_floor_collision(level_manager &lm, chinfo &char_info, float df);
	void check_movement(level_manager &lm, chmanager &chm, chinfo &char_info);

	void hero_movement(level_manager &lm, main_hero &hero, chinfo &char_info);
	void go_through_door(level_manager &lm, chinfo &char_info, float df);

	void check_items(level_manager &lm, main_hero &hero);

	//---------------------Logic functions--------------------

	void put_character(chinfo &char_info, room &dest_room);
	void spawn_mobs(level_manager &lm, chmanager &chm, spawn_type spawn, unsigned char amount);

	//------------------------Mob's AI------------------------

	void mob_ai(level_manager &lm, mob &Mob, chinfo &mob_info, main_hero &hero, chinfo &hero_info, float df);
}

//-------------------------------------------------------GAME CONTROL-------------------------------------------------------

namespace game_control {
	//---------------------Update camera----------------------

	void update_camera(IND_Camera2d &cam, IND_Point pos, IND_Point tl_point, IND_Point br_point, int width, int height) {
		static int pos_x =					cam.GetPosX();
		static int pos_y =					cam.GetPosY();

		// Moving camera
		if (pos.x - width / 2 > tl_point.x + game_logic::level_border && pos.x + width / 2 < br_point.x - game_logic::level_border)
			pos_x =							(int) pos.x;

		if (pos.y - height / 2 > tl_point.y + game_logic::level_border && pos.y + height / 2 < br_point.y - game_logic::level_border)
			pos_y =							(int) pos.y;

		game_state tmp = gstate;

		if (gstate == change_level) {
			pos_x =							pos.x;
			pos_y =							pos.y;
			
			if (pos.x < tl_point.x + CIndieLib::Instance()->Window->GetWidth() / 2)
				pos_x =						tl_point.x + CIndieLib::Instance()->Window->GetWidth() / 2;
			else if (pos.x > br_point.x - CIndieLib::Instance()->Window->GetWidth() / 2)
				pos_x =						br_point.x - CIndieLib::Instance()->Window->GetWidth() / 2;

			if (pos.y < tl_point.y + CIndieLib::Instance()->Window->GetHeight() / 2)
				pos_y =						tl_point.y + CIndieLib::Instance()->Window->GetHeight() / 2;
			else if (pos.y > br_point.y - CIndieLib::Instance()->Window->GetHeight() / 2)
				pos_y =						br_point.y - CIndieLib::Instance()->Window->GetHeight() / 2;
		}

		cam.SetPosition(pos_x, pos_y);
	}

	//-----------------Update ingame interface----------------

	void update_ingame_interface(int tl_x, int tl_y, main_hero &hero, info &hero_info) {
		// Updating info
		hero_info.set_text("Fear: ");
		hero_info.add_text((int) hero.get_fear());
		hero_info.set_position(tl_x + game_logic::level_border, tl_y + game_logic::level_border);
	}

	void update_ingame_interface(int tl_x, int tl_y, ProgressBar progressBar) {
		progressBar.SetPosition((float) (tl_x +20), (float) (tl_y+20));
	}

	//-----------------------Play game------------------------

	void play(level_manager &lm, chmanager &chm, IND_Camera2d &camera) {
		// Show or hide bounding areas
		if (CIndieLib::Instance()->Input->OnKeyPress(IND_TAB))
			show_bareas =												!show_bareas;

		// Switch free control
		if (CIndieLib::Instance()->Input->OnKeyPress(IND_F))
			free_control =												!free_control;

		else game_logic::update_characters(chm, lm, CIndieLib::Instance()->Render->GetFrameTime() / 1000.0f);

		update_camera(camera, IND_Point((int) chm.get_hero().get_position().x, (int) chm.get_hero().get_position().y),
			lm.get_tl_point(), lm.get_br_point(), CIndieLib::Instance()->Window->GetWidth(), CIndieLib::Instance()->Window->GetHeight());

		game_logic::check_items(lm, chm.get_hero());
	}

	//---------------------Complete level---------------------

	void complete_level(level_manager &lm) {
		if (current_level == lm.get_levels_count())
			gstate =													quit;
	}

	//-----------------------Load level-----------------------

	void load_level(level_manager &lm, chmanager &chm) {
		if (current_level < lm.get_levels_count()) {
			try {
				if (!lm.load_level(current_level))
					gstate =											quit;
				chm.clear();
				chm.get_hero().reset_stats();
				
				game_logic::put_character(chm.get_info((unsigned short) 0), lm.get_start_room());
				spawn_mobs(lm, chm, game_logic::every_room, 2);
				++current_level;
			}
			catch (game_logic::spawn_error) {
				gstate =												quit;
			}
		}
	}

	//--------------------main page control-------------------------------

	void main_page_interface (main_page &MainPage) {
		if (CIndieLib::Instance()->Input->OnKeyPress(IND_KEYUP))
			MainPage.SetActivePlay();

		if (CIndieLib::Instance()->Input->OnKeyPress(IND_KEYDOWN))
			MainPage.SetActiveQuit();

		if (MainPage.PlayActive() && CIndieLib::Instance()->Input->OnKeyPress(IND_RETURN)){
			gstate =													change_level;
			MainPage.Show(false);
		}

		if (!MainPage.PlayActive() && CIndieLib::Instance()->Input->OnKeyPress(IND_RETURN))
			gstate =													quit;
			
	}

	void pause_screen (pause &Pause) {
		if (CIndieLib::Instance()->Input->OnKeyPress(IND_KEYUP))
			Pause.SetActivePlay();

		if (CIndieLib::Instance()->Input->OnKeyPress(IND_KEYDOWN))
			Pause.SetActiveQuit();

		if (Pause.PlayActive() && CIndieLib::Instance()->Input->OnKeyPress(IND_RETURN)){
			gstate =													play_game;
			Pause.Show(false);
		}

		if (!Pause.PlayActive() && CIndieLib::Instance()->Input->OnKeyPress(IND_RETURN))
			gstate =													quit;
	}

	//-------------------Game over--------------------

	void game_over() {
		gstate =														_game_over;
		current_level--;
	}
}

//-------------------------------------------------------GAME LOGIC-------------------------------------------------------

namespace game_logic {
	//-------------------Update characters--------------------

	void update_characters(chmanager &chm, level_manager &lm, float df) {
		// Checking movement for all characters
		for (unsigned short i = 0; i < chm.get_info_count(); i++) {
			chinfo &char_info =				chm.get_info(i);

			if (char_info._char->get_state() == on_stairs) continue;

			check_movement(lm, chm, char_info);
		}

		// Checking main hero
		hero_movement(lm, chm.get_hero(), chm.get_info((unsigned short) 0));
		if (chm.get_hero().get_state() == on_stairs)
			go_through_door(lm, chm.get_info((unsigned short) 0), df);
		
		// Calm down
		chm.get_hero().calmdown();
		
		// Scream
		if (!chm.get_hero().is_screaming()) {
			if (CIndieLib::Instance()->Input->OnKeyPress(IND_SPACE)) {
				chm.get_hero().scream();
				if (chm.get_hero().is_screaming()) {
					chm.get_hero().GetScreamSound()	->	play	();
					chm.get_hero().GetFootstepSound()->	stop	();
				}
			}
		} else {
			for (unsigned short i = 0; i < chm.get_mob_count(); i++)
				if (chm.get_info((unsigned short) 0).current_room == chm.get_info(&chm.get_mob(i)).current_room)
					chm.get_mob(i).set_state(shocked);
		}

		// Checking fear level
		if (chm.get_hero().get_fear() == chm.get_hero().get_max_fear())
			game_control::game_over();

		// Checking mobs
		for (unsigned short i = 0; i < chm.get_mob_count(); i++) {
			mob_ai(lm, chm.get_mob(i), chm.get_info(&chm.get_mob(i)), chm.get_hero(), chm.get_info((unsigned short) 0), df);
		}
	}

	//--------------------Check functions---------------------

	// Collision with NPCs

	void check_npc_collision(chmanager &chm) {
		for (unsigned short i = 0; i < chm.get_npc_count(); i++) {
			if (CIndieLib::Instance()->Entity2dManager->IsCollision(&chm.get_hero().get_entity(), "act_radius", &chm.get_npc(i).get_entity(), "act_radius")) {
				if (!chm.get_npc(i).get_text_entity().IsShow())
					chm.get_npc(i).start_talking();
			} else chm.get_npc(i).get_text_entity().SetShow(false);
		}
	}

	// Collision with level floor

	void check_floor_collision(level_manager &lm, chinfo &char_info, float df) {

		for (unsigned short i = 0; i < lm.get_rooms_count(); i++) {
			room *current_room =									&lm.get_room(i);

			for (unsigned short j = 0; j < current_room->floor.size(); j++) {
				if (CIndieLib::Instance()->Entity2dManager->IsCollision(&char_info._char->get_entity(), "feet", current_room->floor[j]->entity, "*")) {
					char_info.current_room =						current_room->ID;
					char_info.current_floor =						current_room->floor[j];
					char_info._char->set_state(stay);

					break;
				}
			}

			if (char_info.current_floor) break;
			else if (i == lm.get_rooms_count() - 1) {
				vector2d speed_v, direction_v;

				char_info.current_room =							0;
				char_info.current_floor =							0;

				speed_v.x =											0;
				speed_v.y =											fall_speed;

				direction_v.x =										0;
				direction_v.y =										1;

				// Make character fall down
				char_info._char->move(speed_v * direction_v * df);
			}
		}
	}

	// Movement

	void check_movement(level_manager &lm, chmanager &chm, chinfo &char_info) {

		// Checking wall collision
		if (char_info.direction.x)
			for (unsigned short i = 0; i < lm.get_room(char_info.current_room - 1).walls.size(); i++) {
				if (char_info.direction.x > 0)
					char_info.rcollision =							CIndieLib::Instance()->Entity2dManager->IsCollision(&char_info._char->get_entity(), "rside",
					lm.get_room(char_info.current_room - 1).walls[i], "*");

				else  
					char_info.lcollision =							CIndieLib::Instance()->Entity2dManager->IsCollision(&char_info._char->get_entity(), "rside",
					lm.get_room(char_info.current_room - 1).walls[i], "*");


				if (char_info.lcollision || char_info.rcollision) break;
			}

		char_info.possible_floor =									0;

		// Searching for possible floor to go
		for (unsigned short i = 0; i < char_info.current_floor->near_floor.size(); i++) {
			if (CIndieLib::Instance()->Entity2dManager->IsCollision(&char_info._char->get_entity(), "feet", char_info.current_floor->near_floor[i]->entity, "*")) {
				char_info.possible_floor =							char_info.current_floor->near_floor[i];
				break;
			}
		}

		// Go to another possible floor
		if (char_info.possible_floor) {

			if (char_info.current_floor->entity->GetPosY() > char_info.possible_floor->entity->GetPosY()) {
				char_info.current_floor =							char_info.possible_floor;
				char_info.possible_floor =							0;
			}
		}
	}

	// Main hero movement

	void hero_movement(level_manager &lm, main_hero &hero, chinfo &char_info) {
		float df =													CIndieLib::Instance()->Render->GetFrameTime() / 1000.0f;

		if (CIndieLib::Instance()->Input->IsKeyPressed(IND_KEYUP))
			hero.GetScreamSound()->stop();

		if (CIndieLib::Instance()->Input->IsKeyPressed(IND_KEYLEFT) && char_info.current_floor && !char_info.lcollision &&
			hero.get_position().x > lm.get_tl_point().x + level_border) {

				if (CIndieLib::Instance()->Input->OnKeyPress(IND_KEYLEFT)){
					hero.set_aMoveLeft_Right_Stay	(hero.get_aMoveLeft_Right());
					hero.get_entity().SetMirrorX	(true);
					hero.get_entity().SetHotSpot	(0.5f,0.5f);
					hero.GetFootstepSound()->play	();
				}

				hero.set_state(go);
				char_info.rcollision =								false;

				char_info.direction.x =								-1;
				if (char_info.current_floor->get_coord_quarter() == 1 || char_info.current_floor->get_coord_quarter() == 3)
					char_info.direction.y =							-1;
				else char_info.direction.y =						1;

				hero.move(char_info.current_floor->get_speed_vector(char_info._char->get_speed()) * char_info.direction * df *
					(1.0f - hero.get_fear() / hero.get_max_fear() / speed_over_fear_reduce));

				hero.dRoute = 1;
		}

		if (CIndieLib::Instance()->Input->IsKeyPressed(IND_KEYRIGHT) && char_info.current_floor && !char_info.rcollision &&
			hero.get_position().x < lm.get_br_point().x - level_border) {

				if (CIndieLib::Instance()->Input->OnKeyPress(IND_KEYRIGHT)){
					hero.set_aMoveLeft_Right_Stay	(hero.get_aMoveLeft_Right());
					hero.get_entity().SetHotSpot	(0.5f,0.5f);
					hero.GetFootstepSound()->play	();
				}

				hero.set_state(go);
				char_info.lcollision =								false;

				char_info.direction.x =								1;
				if (char_info.current_floor->get_coord_quarter() == 1 || char_info.current_floor->get_coord_quarter() == 3)
					char_info.direction.y =							1;
				else char_info.direction.y =						-1;

				hero.move(char_info.current_floor->get_speed_vector(char_info._char->get_speed()) * char_info.direction * df *
					(1.0f - hero.get_fear() / hero.get_max_fear() / speed_over_fear_reduce));

				hero.dRoute = 1;
		}

		// Go through doors

		if (CIndieLib::Instance()->Input->OnKeyPress(IND_KEYUP) && char_info.current_room)
			go_through_door(lm, char_info, df);

		// Staying

		if (!CIndieLib::Instance()->Input->IsKeyPressed(IND_KEYRIGHT) && !CIndieLib::Instance()->Input->IsKeyPressed(IND_KEYLEFT)
			&& hero.dRoute != stay && hero.get_state() != on_stairs) {
				float x = hero.get_entity().	GetPosX							();
				float y = hero.get_entity().	GetPosY							();
				int z = hero.get_entity().		GetPosZ							();
				hero.							set_aMoveLeft_Right_Stay		(hero.get_aStay());
				hero.get_entity().				SetPosition						(x,y,z);
				hero.get_entity().				SetHotSpot						(0.5f,0.5f);
				hero.set_state													(stay);
				hero.dRoute =					stay;
				hero.GetFootstepSound()->		stop							();
		}

	}

	// On stairs

	void go_through_door(level_manager &lm, chinfo &char_info, float df) {
		static door_conf *current_door =							0;
		static vector2d distance;
		static vector2d speed;
		static vector2d direction;
		static unsigned char trys =									0;

		if (char_info._char->get_state() == on_stairs) {
			if (distance.x <= 0 && distance.y <= 0) {
				if (char_info.current_floor) {
					char_info._char->set_state(stay);
					char_info._char->dRoute = 1;
				} else {
					if (trys > 2) char_info._char->move(vector2d(0, 0.5f));

					char_info.current_room =							current_door->paired->ID;
					char_info._char->get_entity().SetShow(true);
					trys++;
					for (unsigned short i = 0; i < lm.get_room(char_info.current_room - 1).floor.size(); i++) {
						if (CIndieLib::Instance()->Entity2dManager->IsCollision(&char_info._char->get_entity(), "feet", lm.get_room(char_info.current_room - 1).floor[i]->entity, "*")) {
							char_info.current_floor =					lm.get_room(char_info.current_room - 1).floor[i];
							trys =										0;
							break;
						}
					}
				}

			} else {
				distance.x -=										speed.x * df;
				distance.y -=										speed.y * df;

				if (distance.x < speed.x * df)
					char_info._char->move(vector2d(distance.x, speed.y) * direction * df);
				else if (distance.y < speed.y * df)
					char_info._char->move(vector2d(speed.x, distance.y) * direction * df);
				else char_info._char->move(speed * direction * df);
			}

		} else {

			room *current_room =									&lm.get_room(char_info.current_room - 1);

			for (unsigned short i = 0; i < current_room->doors.size(); i++) {
				if (CIndieLib::Instance()->Entity2dManager->IsCollision(&char_info._char->get_entity(), "act_radius", current_room->doors[i]->entity, "*")) {
					current_door =									current_room->doors[i];

					switch (current_door->ID) {
					case -1:
						break;

					case -2:
						if (!lm.is_key_needed())
							game_control::gstate =					game_control::change_level;
						break;

					default:
						if (!current_door) break;
						char_info._char->set_state(on_stairs);
						distance.x =								current_door->paired->entity->GetPosX() - char_info._char->get_entity().GetPosX();
						distance.y =								current_door->paired->entity->GetPosY() - current_door->entity->GetPosY();

						direction.x =								(distance.x > 0) ? (1.0f) : (-1.0f);
						direction.y =								(distance.y > 0) ? (1.0f) : (-1.0f);

						distance =									distance * direction;

						speed.x =									distance.x * 2;
						speed.y =									distance.y * 2;

						char_info._char->get_entity().SetShow(false);
						char_info.current_floor =					0;
						char_info.current_room =					0;
					}
				}
			}
		}
	}

	//---------Put character to the destination room----------

	void put_character(chinfo &char_info, room &dest_room) {
		char_info.current_room =									dest_room.ID;
		while (!char_info.current_floor) {
			if (dynamic_cast<main_hero *> (char_info._char)) {
				for (unsigned short i = 0; i < dest_room.doors.size(); i++) {
					if (dest_room.doors[i]->paired) continue;
					for (unsigned short j = 0; j < dest_room.floor.size(); j++) {
						if (CIndieLib::Instance()->Entity2dManager->IsCollision(dest_room.doors[i]->entity, "*", dest_room.floor[j]->entity, "*")) {
							char_info.current_floor =				dest_room.floor[j];
							break;
						}
					}
					break;
				}
			} else
				char_info.current_floor =							dest_room.floor[(unsigned short) CIndieLib::Instance()->Math->Rand(0, dest_room.floor.size() - 1)];
			
			for (unsigned short i = 0; i < dest_room.walls.size(); i++)
				if (CIndieLib::Instance()->Entity2dManager->IsCollision(char_info.current_floor->entity, "*", dest_room.walls[i], "*") && char_info.current_floor->near_floor.size() != 1) {
					char_info.current_floor =						0;
					break;
				}
		}
		float pos_x =												char_info.current_floor->entity->GetPosX();
		int height = 0;
		if (char_info._char->get_entity().GetAnimation())
			height = char_info._char->get_entity().GetAnimation()->GetSurface(1)->GetHeight();
		else
			height = char_info._char->get_entity().GetSurface()->GetHeight();
		
		float pos_y =												char_info.current_floor->entity->GetPosY() - height / 2 - 5;
		char_info._char->set_position(vector2d(pos_x, pos_y));
	}

	// Spawn exact amount of mobs due to specified type

	void spawn_mobs(level_manager &lm, chmanager &chm, spawn_type spawn, unsigned char amount) {
		try {
			switch (spawn) {
			case random:
				{
					for ( ; amount; amount--) {
						chm.add_mob();
						chinfo &mob_info =							chm.get_info(chm.get_info_count() - 1);

						room *spawn_room =							0;
						while (!spawn_room) {
							spawn_room =							&lm.get_room((unsigned short) CIndieLib::Instance()->Math->Rand(0, lm.get_rooms_count() - 1));
							if (!spawn_room) throw spawn_error();		// TERMINATE THE SPAWN IF THERE IS NO ROOMS ON THE LEVEL
							if (spawn_room->doors.size() == 1 || spawn_room == &lm.get_start_room() || spawn_room == &lm.get_finish_room())
								spawn_room =							0;
						}

						put_character(mob_info, *spawn_room);
						if (!mob_info.direction.x){
							mob_info.direction.x =					(CIndieLib::Instance()->Math->Rand(1, 10) % 2) ? (1.0f) : (-1.0f);
						if (mob_info.direction.x > 0) chm.get_mob(chm.get_mob_count()-1).get_entity().SetMirrorX							(false);
						if (mob_info.direction.x < 0) chm.get_mob(chm.get_mob_count()-1).get_entity().SetMirrorX							(true);
						chm.get_mob(chm.get_mob_count()-1).get_entity().SetHotSpot(0.5f,0.5f);
						}
					}
				}
				break;

			case one_room:
				{
					room *spawn_room =								0;
					while (!spawn_room) {
						spawn_room =								&lm.get_room((unsigned short) CIndieLib::Instance()->Math->Rand(0, lm.get_rooms_count() - 1));
						if (!spawn_room) throw spawn_error();			// TERMINATE THE SPAWN IF THERE IS NO ROOMS ON THE LEVEL
						if (spawn_room->doors.size() == 1 || spawn_room == &lm.get_start_room() || spawn_room == &lm.get_finish_room())
							spawn_room =							0;
					}

					for ( ; amount; amount--) {
						chm.add_mob();
						chinfo &mob_info =							chm.get_info(chm.get_info_count() - 1);

						put_character(mob_info, *spawn_room);
						if (!mob_info.direction.x){
							mob_info.direction.x =					(CIndieLib::Instance()->Math->Rand(1, 10) % 2) ? (1.0f) : (-1.0f);
							if (mob_info.direction.x > 0) chm.get_mob(chm.get_mob_count()-1).get_entity().SetMirrorX							(false);
							if (mob_info.direction.x < 0) chm.get_mob(chm.get_mob_count()-1).get_entity().SetMirrorX							(true);
							chm.get_mob(chm.get_mob_count()-1).get_entity().SetHotSpot(0.5f,0.5f);
						}

					}
				}
				break;

			case every_room:
				{
					for (unsigned short i = 0; i < lm.get_rooms_count(); i++) {
						if (lm.get_room(i).doors.size() < 2 || &lm.get_room(i) == &lm.get_start_room() || &lm.get_room(i) == &lm.get_finish_room()) continue;
						for (unsigned short k = 0; k < amount; k++) {
							chm.add_mob();
							chinfo &mob_info =						chm.get_info(chm.get_info_count() - 1);

							put_character(mob_info, lm.get_room(i));
							if (!mob_info.direction.x){
								mob_info.direction.x =				(CIndieLib::Instance()->Math->Rand(1, 10) % 2) ? (1.0f) : (-1.0f);
								if (mob_info.direction.x > 0) chm.get_mob(chm.get_mob_count()-1).get_entity().SetMirrorX							(false);
								if (mob_info.direction.x < 0) chm.get_mob(chm.get_mob_count()-1).get_entity().SetMirrorX							(true);
								chm.get_mob(chm.get_mob_count()-1).get_entity().SetHotSpot(0.5f,0.5f);
							}
						}
					}
				}
				break;
			}
		}
		catch (spawn_error) {
			for (unsigned short i = 0; i < chm.get_mob_count(); i++)
				chm.del_mob(i);
			throw;
		}
	}

	// Mob's AI

	void mob_ai(level_manager &lm, mob &Mob, chinfo &mob_info, main_hero &hero, chinfo &hero_info, float df) {
		switch (mob_info._char->get_state()) {
		case stay:
			mob_info._char->set_state(go);
			break;

		case shocked:
			if (hero.is_screaming())
				return;
			else mob_info._char->set_state(go);
			break;

		case atack:
			mob_info.direction.x =									(mob_info._char->get_position().x < hero_info._char->get_position().x) ? (1.0f) : (-1.0f);
			if (mob_info.direction.x > 0) Mob.get_entity().SetMirrorX							(false);
			if (mob_info.direction.x < 0) Mob.get_entity().SetMirrorX							(true);
			Mob.get_entity().SetHotSpot(0.5f,0.5f);
			Mob.scare(hero);

			if ((int) mob_info._char->get_position().x == (int) hero_info._char->get_position().x) break;

			if (hero_info.current_room != mob_info.current_room) {
				mob_info._char->set_state(go);
				mob_info.direction.x =								(CIndieLib::Instance()->Math->Rand(1, 10) % 2) ? (1.0f) : (-1.0f);
				if (mob_info.direction.x > 0) Mob.get_entity().SetMirrorX							(false);
				if (mob_info.direction.x < 0) Mob.get_entity().SetMirrorX							(true);
				Mob.get_entity().SetHotSpot(0.5f,0.5f);
			}
			

		case go:
			if (!mob_info.current_floor) break;

			if (hero_info.current_room == mob_info.current_room)
				mob_info._char->set_state(atack);

			if (mob_info.current_floor->get_coord_quarter() == 1 || mob_info.current_floor->get_coord_quarter() == 3)
				mob_info.direction.y =								-1;
			else mob_info.direction.y =								1;
			if (mob_info.lcollision || mob_info.rcollision) {
				mob_info.direction.x *=								-1;
				mob_info.lcollision =								false;
				mob_info.rcollision =								false;
				if (mob_info.direction.x > 0) Mob.get_entity().SetMirrorX						(false);
				if (mob_info.direction.x < 0) Mob.get_entity().SetMirrorX						(true);
				Mob.get_entity().SetHotSpot(0.5f,0.5f);
			}

			mob_info._char->move(mob_info.current_floor->get_speed_vector(mob_info._char->get_speed()) * mob_info.direction * df);

			break;

		}
	}

	//-------------------------Check items-------------------------

	void check_items(level_manager &lm, main_hero &hero) {
		for (unsigned short i = 0; i < lm.get_items_count(); i++) {
			if (lm.get_item(i) && CIndieLib::Instance()->Entity2dManager->IsCollision(&hero.get_entity(), "act_radius", &lm.get_item(i)->get_entity(), "*")) {
				game_control::active_message =						lm.get_item(i)->get_name();
				
				if (lm.get_item(i)->get_name() == item_key)
					lm.set_key_need(false);

				else if (lm.get_item(i)->get_name() == item_valeriana) {
					hero.set_max_calmdown(hero.get_max_calmdown() - (int) valeriana_fear_reduce);
					hero.change_fear(valeriana_fear_reduce);
				}

				if (lm.get_item(i)->get_name() != item_candle)
					lm.delete_item(i);
				else {
					if (lm.get_item(i)->get_entity().GetTransparency() < 254)
						lm.get_item(i)->get_entity().SetTransparency(lm.get_item(i)->get_entity().GetTransparency() + 2);
				}
			} else if (lm.get_item(i)){
				if (lm.get_item(i)->get_name() == item_candle && lm.get_item(i)->get_entity().GetTransparency() > 0)
					lm.get_item(i)->get_entity().SetTransparency(lm.get_item(i)->get_entity().GetTransparency() - 2);
			}
		}
	}
}

#endif