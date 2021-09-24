#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform* player = nullptr;
	Scene::Transform* enemy = nullptr;

	float player_speed = 20.0f;
	float enemy_speed = 7.0f;

	glm::vec3 enemy_init_pos;
	
	//music sample pointers
	std::shared_ptr< Sound::PlayingSample > begin_sfx; // game begin

	std::shared_ptr< Sound::PlayingSample > over_sfx;	// game over
	std::shared_ptr< Sound::PlayingSample > bleed_sfx;
	std::shared_ptr< Sound::PlayingSample > scream_sfx;

	std::shared_ptr< Sound::PlayingSample > block_sfx;	// block attack
	
	std::shared_ptr< Sound::PlayingSample > sword_sfx_loop; // sword

	std::shared_ptr< Sound::PlayingSample > train_sfx_loop; //train
	std::shared_ptr< Sound::PlayingSample > track_sfx_loop;

	uint8_t attack_type = 0; //0:sword, 1:train
	uint8_t attack_from = 0; //0:front, 1:left, 2:right
	float timer = 3.0f;
	uint8_t game_over = 0;
	uint8_t block = 0;
	uint8_t game_stat = 0; //0: counting down; 1: enemy acting; 2:enemy finish acting; 3: game over
	uint8_t attack_play = 0;

	//camera:
	Scene::Camera *camera = nullptr;


};
