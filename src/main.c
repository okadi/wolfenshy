#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <SDL3/SDL.h>

typedef struct { float x, y; } Vec2;
typedef struct { int x, y; } Vec2i;

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
	bool isRunning;
} State;

State state;

// these kinda don't need to be higher than 1 with delta lol
#define PLAYER_WALK_SPEED 1
#define PLAYER_TURN_SPEED 2

typedef struct {
	Vec2 position, direction, plane;
} Player;

Player player;

#define MAP_WIDTH 8
#define MAP_HEIGHT 8
static int map[MAP_HEIGHT][MAP_WIDTH] = {
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

static void draw_line(int x, int top, int bottom, uint32_t color) {
	for (int y = top; y < bottom; y++) {
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

	// center-ish...
	player.position = (Vec2){ 4, 4 };
	// face up
	player.direction.y = -1;
	// plane distance (more or less camera fov)
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

		// clear the buffer before preparing the frame
		memset(state.pixels, 0, sizeof(state.pixels));

		// DDA algorithm
		// https://lodev.org/cgtutor/raycasting.html
		// repeatedly step through to the nearest grid line until a wall is hit
		for (int i = 0; i < SCREEN_WIDTH; i++) {
			// sweep across the screen, -1 to 1 where 0 is the center
			float x_on_plane = 2 * i / (float)SCREEN_WIDTH - 1;

			// point a vector from the player to x_on_plane
			Vec2 ray_dir = {
				player.direction.x + player.plane.x * x_on_plane,
				player.direction.y + player.plane.y * x_on_plane,
			};

			// cell we're currently stepping through
			Vec2i cell = {
				(int)player.position.x,
				(int)player.position.y,
			};

			// the distance the ray has to travel to go from one x grid line
			// to the next x grid line (and y to y) added to the running total
			// each time the loop crosses that kind of line
			Vec2 delta_dist = {
				fabsf(1 / ray_dir.x),
				fabsf(1 / ray_dir.y),
			};

			// initially the distance from the player to the first x and y grid line
			// incremented with delta_dist each step, becoming the distance along
			// the ray from the player to the next line the ray hasn't reached yet
			Vec2 next_dist;

			Vec2i step;

			bool hit = 0;
			float wall_distance;
			int side;

			// set up the initial values of next_dist - the distance from the player
			// to the nearest x and y grid lines along the direction of the ray,
			// and the direction the cell will be moved in each step
			if (ray_dir.x < 0) {
				step.x = -1;
				next_dist.x = (player.position.x - cell.x) * delta_dist.x;
			} else {
				step.x = 1;
				next_dist.x = (cell.x + 1.0 - player.position.x) * delta_dist.x;
			}

			if (ray_dir.y < 0) {
				step.y = -1;
				next_dist.y = (player.position.y - cell.y) * delta_dist.y;
			} else {
				step.y = 1;
				next_dist.y = (cell.y + 1.0 - player.position.y) * delta_dist.y;
			}

			// loop until we find a wall
			while (!hit) {
				// check if the closest grid line is horizontal or vertical
				// and step the ray forward to the next cell accordingly
				if (next_dist.x < next_dist.y) {
					next_dist.x += delta_dist.x;
					cell.x += step.x;
					side = 0;
				} else {
					next_dist.y += delta_dist.y;
					cell.y += step.y;
					side = 1;
				}

				// if we've reached a cell with a value in it, we hit a wall...!
				if (map[cell.y][cell.x] > 0) {
					hit = true;
				}
			}

			// next_dist has already stepped forward to the grid line past the wall
			// step back one delta_dist to find the actual distance to the wall
			if (side == 0) {
				wall_distance = (next_dist.x - delta_dist.x);
			} else {
				wall_distance = (next_dist.y - delta_dist.y);
			}

			int line_height = (int)(SCREEN_HEIGHT / wall_distance);

			int draw_start = -line_height / 2 + SCREEN_HEIGHT / 2;
			if (draw_start < 0) {
				draw_start = 0;
			}

			int draw_end = line_height / 2 + SCREEN_HEIGHT / 2;
			if (draw_end > SCREEN_HEIGHT) {
				draw_end = SCREEN_HEIGHT;
			}

			// color the pixels depending on the number in the cell
			uint32_t color;
			switch(map[cell.y][cell.x]) {
				case 1:
					color = 0xFFFF0000;
					break;
				case 2:
					color = 0xFF00FF00;
					break;
				case 3:
					color = 0xFF0000FF;
					break;
				default:
					color = 0xFFFFFFFF;
					break;
			}

			// dim the y side lines so we can perceive any depth
			if (side == 1) {
				color = ((color >> 1) & 0x007F7F7F) | 0xFF000000;
			}

			draw_line(i, draw_start, draw_end, color);
		}

		// clear to gray, replace the texture with the buffer, and push it to the window
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
