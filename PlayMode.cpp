#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <time.h>       /* time */
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("arena.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > arena_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("arena.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > begin_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("begin.wav"));
	});

Load< Sound::Sample > over_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("over.wav"));
	});

Load< Sound::Sample > bleed_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("bleed.wav"));
	});

Load< Sound::Sample > scream_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("scream.wav"));
	});

Load< Sound::Sample > block_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("block.wav"));
	});

Load< Sound::Sample > sword_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("sword.wav"));
	});


Load< Sound::Sample > train_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("train.wav"));
	});

Load< Sound::Sample > track_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("track.wav"));
	});

PlayMode::PlayMode() : scene(*arena_scene) {

	for (auto& transform : scene.transforms) {
		if (transform.name == "enemy") enemy = &transform;
		else if (transform.name == "player") player = &transform;
	}
	if (player == nullptr) throw std::runtime_error("player not found.");
	if (enemy == nullptr) throw std::runtime_error("enemy not found.");

	enemy_init_pos = enemy->position;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//begin sfx
	Sound::play(*begin_sample, 5.0f, 0.1f);


}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_r) {
			block = true;
			return true;
		}
	}
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_r) {
			block = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (game_over == 1) {
		return;
	}
	static std::mt19937 mt;

	if (game_stat == 0) {
		timer -= elapsed;
		if (timer < 0) {
			// set attack type and direction, set enemy position, set game stat
			game_stat = 1;
			attack_type = mt() % 2;
			attack_from = mt() % 3;
			float offset = (mt() / float(mt.max())) * 20.0f + 0.3f;
			enemy_speed = (mt() / float(mt.max())) * 20.0f + 16.3f;
			if (attack_from == 0) {
				enemy->position = glm::vec3(0.0f, 0.0f, -10.0f - offset);
			}
			else if (attack_from == 1) {
				enemy->position = glm::vec3(-20.0f - offset, 0.0f, 10.0f);
			}
			else if (attack_from == 2) {
				enemy->position = glm::vec3(20.0f + offset, 0.0f, 10.0f);
			}
		}
	}
	else if (game_stat == 1) {
		// enemy move
		if (attack_type == 0) {
			if (attack_play == 0) {
				sword_sfx_loop = Sound::loop_3D(*sword_sample, 7.0f, enemy->position, 0.1f);
				attack_play = 1;
			}
			//check hit
			{
				auto diff = enemy->position - player->position;
				if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z <= 1.0f) {
					if (block == 0) {
						sword_sfx_loop->stop();
						game_stat = 3;
						return;
					}
					else {
						sword_sfx_loop->stop();
						block_sfx = Sound::play(*block_sample, 2.0f, 0.1f);
						game_stat = 2;
						return;
					}
					
				}
			}
			if (attack_from == 0 && enemy->position.z < 10.0f) {
				enemy->position.z += enemy_speed * elapsed ;
			}
			else if (attack_from == 1 && enemy->position.x < 0.0f) {
				enemy->position.x += enemy_speed * elapsed ;
			}
			else if (attack_from == 2 && enemy->position.x > 0.0f) {
				enemy->position.x -= enemy_speed * elapsed ;
			}
			else {
				sword_sfx_loop->stop();
				game_stat = 2;
				return;
			}
			sword_sfx_loop->set_position(enemy->position, 1.0f / 60.0f);
		}
		else if (attack_type == 1) {
			if (attack_play == 0) {
				train_sfx_loop = Sound::loop_3D(*train_sample, 7.0f, enemy->position, 0.1f);
				track_sfx_loop = Sound::loop_3D(*track_sample, 7.0f, enemy->position, 0.1f);
				attack_play = 1;
			}
			//check hit
			{
				auto diff = enemy->position - player->position;
				if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z <= 1.0f) {
					train_sfx_loop->stop();
					track_sfx_loop->stop();
					game_stat = 3;
					return;

				}
			}
			if (attack_from == 0 && enemy->position.z < 40.0f) {
				enemy->position.z += enemy_speed * elapsed ;
			}
			else if (attack_from == 1 && enemy->position.x < 40.0f) {
				enemy->position.x += enemy_speed * elapsed ;
			}
			else if (attack_from == 2 && enemy->position.x > -40.0f) {
				enemy->position.x -= enemy_speed * elapsed ;
			}
			else {
				train_sfx_loop->stop();
				track_sfx_loop->stop();
				game_stat = 2;
				return;
			}
			train_sfx_loop->set_position(enemy->position, 1.0f / 60.0f);
			track_sfx_loop->set_position(enemy->position, 1.0f / 60.0f);
		}
	}
	else if (game_stat == 2) {
		//reset enemy to init pos, reset timer
		timer = (mt() / float(mt.max())) * 3.0f + 0.3f;
		enemy->position = enemy_init_pos;
		attack_play = 0;
		game_stat = 0;
	}
	else if (game_stat == 3) {
		game_over = 1;
		
		bleed_sfx = Sound::play(*bleed_sample, 3.0f, 0.1f);
		scream_sfx = Sound::play(*scream_sample, 4.0f, 0.1f);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		over_sfx = Sound::play(*over_sample, 3.0f, 0.1f);

		return;
	}


	// player movement
	if (up.pressed == 1 ) {
		if(player->position.z > 5.0f)
			player->position.z -= 2 * player_speed * elapsed;
	}
	else if (down.pressed == 1 ) {
		if(player->position.z < 15.0f)
			player->position.z += 2 * player_speed * elapsed;
	} 
	else if (left.pressed == 1) {
		if(player->position.x > -6.0f)
			player->position.x -= 2 * player_speed * elapsed;
	} 
	else if (right.pressed == 1 ) {
		if(player->position.x < 6.0f)
			player->position.x += 2 * player_speed * elapsed;
	} 
	else {
		if (player->position.z > 10.1f) player->position.z -= player_speed * elapsed * glm::abs(player->position.z - 10.0f);
		else if(player->position.z < 9.9f) player->position.z += player_speed * elapsed * glm::abs(player->position.z - 10.0f);
		if(player->position.x > 0.1f) player->position.x -= player_speed * elapsed * glm::abs(player->position.x + 0.5f);
		else if (player->position.x < -0.1f) player->position.x += player_speed * elapsed * glm::abs(player->position.x + 0.5f);
	}
		
	

	{ //update listener to camera position:
		glm::mat4x3 frame = player->make_local_to_parent();
		glm::vec3 right = frame[0];
		glm::vec3 at = frame[3];
		Sound::listener.set_position_right(at, right, 1.0f / 60.0f);
	}


	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		if (!game_over)
		{
			constexpr float H = 0.09f;
			std::string s = "WASD moves player; R to block, Q to quit the game ";
			lines.draw_text(s,
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text(s,
				glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
		else
		{
			constexpr float H = 0.29f;
			std::string s = "You died";

			lines.draw_text(s,
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text(s,
				glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0x00, 0x00, 0x00));
		}
	}
	GL_ERRORS();
}


//glm::vec3 PlayMode::get_leg_tip_position() {
//	//the vertex position here was read from the model in blender:
//	//return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
//}
