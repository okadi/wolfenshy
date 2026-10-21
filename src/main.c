#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <SDL3/SDL.h>

typedef struct { float x, y; } Vec2;

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Color pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
	bool isRunning;
} State;

State state;

#define PLAYER_WALK_SPEED 1
#define PLAYER_TURN_SPEED 2

typedef struct {
	Vec2 position, direction, plane;
} Player;

Player player;

#define MAP_WIDTH 8
#define MAP_HEIGHT 8
static int map[MAP_WIDTH][MAP_HEIGHT] = {
	{ 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 0, 0, 0, 0, 0, 0, 1 },
	{ 1, 0, 2, 0, 0, 0, 0, 1 },
	{ 1, 0, 0, 0, 0, 0, 0, 1 },
	{ 1, 0, 0, 0, 0, 0, 0, 1 },
	{ 1, 0, 0, 0, 0, 3, 0, 1 },
	{ 1, 0, 0, 0, 0, 0, 0, 1 },
	{ 1, 1, 1, 1, 1, 1, 1, 1 },
};

// rotate a vec2 and return it as a new vector
static Vec2 vec2_rotate(Vec2 vector, float radians) {
	Vec2 rotated;
	rotated.x = vector.x * cosf(radians) - vector.y * sinf(radians);
	rotated.y = vector.x * sinf(radians) + vector.y * cosf(radians);
	return rotated;
}

static void ver_line(int x, int y0, int y1, SDL_Color color) {
	for (int y = y0; y < y1; y++) {
		state.pixels[(y * SCREEN_WIDTH) + x] = color;
	}
}

int main(void) {
	if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		SDL_Log("Couldn't start video: %s", SDL_GetError());
		return 1;
	}

	state.window = SDL_CreateWindow("hi", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	state.renderer = SDL_CreateRenderer(state.window, NULL);
	state.texture = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	state.isRunning = true;

	player.position = (Vec2){ 4, 4 };
	// face up
	player.direction.y = -1;
	player.plane = (Vec2){ 0.66f, 0 };

	uint64_t last_ticks = 0;
	uint64_t now_ticks = SDL_GetTicksNS();

	while (state.isRunning) {
		last_ticks = now_ticks;
		now_ticks = SDL_GetTicksNS();
		float delta_time = (double)(now_ticks - last_ticks) / 1e9; // ns -> s

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT:
					state.isRunning = false;
					break;
			}
		}

		const bool *keyboard_state = SDL_GetKeyboardState(NULL);

		if (keyboard_state[SDL_SCANCODE_UP]) {
			player.position.x += player.direction.x * PLAYER_WALK_SPEED * delta_time;
			player.position.y += player.direction.y * PLAYER_WALK_SPEED * delta_time;
		}

		if (keyboard_state[SDL_SCANCODE_DOWN]) {
			player.position.x -= player.direction.x * PLAYER_WALK_SPEED * delta_time;
			player.position.y -= player.direction.y * PLAYER_WALK_SPEED * delta_time;
		}

		if (keyboard_state[SDL_SCANCODE_RIGHT]) {
			player.direction = vec2_rotate(player.direction, PLAYER_TURN_SPEED * delta_time);
			player.plane = vec2_rotate(player.plane, PLAYER_TURN_SPEED * delta_time);
		}

		if (keyboard_state[SDL_SCANCODE_LEFT]) {
			player.direction = vec2_rotate(player.direction, -PLAYER_TURN_SPEED * delta_time);
			player.plane = vec2_rotate(player.plane, -PLAYER_TURN_SPEED * delta_time);
		}

		memset(state.pixels, 0, sizeof(state.pixels));

		for (int i = 0; i < SCREEN_WIDTH; i++) {
			float camera_x = 2 * i / (float)SCREEN_WIDTH - 1;
			float ray_dir_x = player.direction.x + player.plane.x * camera_x;
			float ray_dir_y = player.direction.y + player.plane.y * camera_x;

			int map_x = (int)player.position.x;
			int map_y = (int)player.position.y;

			float side_dist_x;
			float side_dist_y;

			float delta_dist_x = fabs(1 / ray_dir_x);
			float delta_dist_y = fabs(1 / ray_dir_y);
			float wall_distance;

			int step_x;
			int step_y;

			bool hit = 0;
			int side;

			if (ray_dir_x < 0) {
				step_x = -1;
				side_dist_x = (player.position.x - map_x) * delta_dist_x;
			} else {
				step_x = 1;
				side_dist_x = (map_x + 1.0 - player.position.x) * delta_dist_x;
			}

			if (ray_dir_y < 0) {
				step_y = -1;
				side_dist_y = (player.position.y - map_y) * delta_dist_y;
			} else {
				step_y = 1;
				side_dist_y = (map_y + 1.0 - player.position.y) * delta_dist_y;
			}

			while (!hit) {
				if (side_dist_x < side_dist_y) {
					side_dist_x += delta_dist_x;
					map_x += step_x;
					side = 0;
				} else {
					side_dist_y += delta_dist_y;
					map_y += step_y;
					side = 1;
				}

				if (map[map_x][map_y] > 0) {
					hit = true;
				}
			}

			if (side == 0) {
				wall_distance = (side_dist_x - delta_dist_x);
			} else {
				wall_distance = (side_dist_y - delta_dist_y);
			}

			int line_height = (int)(SCREEN_HEIGHT / wall_distance);

			int draw_start = -line_height / 2 + SCREEN_HEIGHT / 2;
			if (draw_start < 0) draw_start = 0;
			int draw_end = line_height / 2 + SCREEN_HEIGHT / 2;
			if (draw_end > SCREEN_HEIGHT) draw_end = SCREEN_HEIGHT;

			SDL_Color color;
			switch(map[map_x][map_y]) {
				case 1:
					color = (SDL_Color){ 0, 0, 255, 255 };
					break;
				case 2:
					color = (SDL_Color){ 0, 255, 0, 255 };
					break;
				case 3:
					color = (SDL_Color){ 255, 0, 0, 255 };
					break;
				default:
					color = (SDL_Color){ 255, 255, 255, 255 };
					break;
			}

			if (side == 1) {
				color.r /= 2;
				color.g /= 2;
				color.b /= 2;
			}

			ver_line(i, draw_start, draw_end, color);
		}

		SDL_SetRenderDrawColor(state.renderer, 127, 127, 127, 255);
		SDL_RenderClear(state.renderer);
		SDL_UpdateTexture(state.texture, NULL, state.pixels, SCREEN_WIDTH * 4);
		SDL_RenderTexture(state.renderer, state.texture, NULL, NULL);
		SDL_RenderPresent(state.renderer);
	}

	SDL_DestroyTexture(state.texture);
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
	return 0;
}
