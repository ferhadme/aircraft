#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define SCREEN_WIDTH 1400
#define SCREEN_HEIGHT 900

struct App
{
    SDL_Window *window;
    SDL_Renderer *renderer;
};

struct Entity
{
    int x;
    int y;
    int w;
    int h;
    SDL_Texture *texture;
    int visibility;
};

void updatePlayer(struct Entity *player, int x, int y, int w, int h, int visibility)
{
    player->x = x;
    player->y = y;
    player->w = w;
    player->h = h;
    player->visibility = visibility;
}

void handleBulletFiring(struct Entity *player, struct Entity *bullet)
{
    bullet->visibility = 1;
    bullet->x = player->x + player->w;
    bullet->y = player->y + player->h / 2.5;
}

void handleBulletMove(struct Entity *bullet)
{
    if (bullet->x >= SCREEN_WIDTH) bullet->visibility = 0;
    bullet->x += 10;
}

void updateCoordinates(struct Entity *player, struct Entity *bullet, SDL_KeyboardEvent *event)
{
    if (event->repeat == 0) {
	switch (event->keysym.scancode) {
	case SDL_SCANCODE_UP:
	    player->y -= 10;
	    break;
	case SDL_SCANCODE_DOWN:
	    player->y += 10;
	    break;
	case SDL_SCANCODE_RIGHT:
	    player->x += 10;
	    break;
	case SDL_SCANCODE_LEFT:
	    player->x -= 10;
	    break;
	case SDL_SCANCODE_RETURN:
	    printf("L Ctrl called\n");
	    handleBulletFiring(player, bullet);
	    break;
	default:
	    break;
	}
	if (player->y < 0) player->y = 0;
	if (player->x < 0) player->x = 0;
    }
}

void prepareScene(struct App *app)
{
    SDL_SetRenderDrawColor(app->renderer, 20, 20, 20, 20);
    SDL_RenderClear(app->renderer);
}

void presentScene(struct App *app)
{
    SDL_RenderPresent(app->renderer);
}

void playerListener(struct Entity *player, struct Entity *bullet)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
	switch (event.type) {
	case SDL_KEYUP:
	    updateCoordinates(player, bullet, &event.key);
	    break;
	case SDL_QUIT:
	    exit(0);
	    break;
	default:
	    break;
	}
    }
}

SDL_Window *initialize_window(void)
{
    int windowFlags = 0;
    SDL_Window *window = SDL_CreateWindow("Shooter", SDL_WINDOWPOS_UNDEFINED,
					  SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags);

    if (!window) {
	printf("Couldn't initialize window: %s\n", SDL_GetError());
	exit(1);
    }

    return window;
}

SDL_Renderer *initialize_renderer(SDL_Window *window)
{
    int rendererFlags = SDL_RENDERER_ACCELERATED;
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, rendererFlags);
    if (!renderer) {
	printf("Couldn't initialize renderer: %s\n", SDL_GetError());
	exit(1);
    }

    return renderer;
}

void placeRectangle(struct App *app, struct Entity *entity)
{
    if (!entity->visibility) return;
    SDL_Rect rect;
    rect.x = entity->x;
    rect.y = entity->y;
    SDL_QueryTexture(entity->texture, NULL, NULL, &rect.w, &rect.h);
    rect.w = entity->w;
    rect.h = entity->h;
    SDL_RenderCopy(app->renderer, entity->texture, NULL, &rect);
}

int main(void)
{
    struct App app;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	printf("Couldn't initialize SDL system: %s\n", SDL_GetError());
	exit(1);
    }

    app.window = initialize_window();

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    app.renderer = initialize_renderer(app.window);

    if (!IMG_Init(IMG_INIT_PNG)) {
	printf("Couldn't initialize image: %s\n", SDL_GetError());
	exit(1);
    }

    struct Entity player;
    player.texture = IMG_LoadTexture(app.renderer, "aircraft.png");
    updatePlayer(&player, 0, 0, 75, 75, 1);

    struct Entity bullet;
    bullet.texture = IMG_LoadTexture(app.renderer, "bullet.png");
    updatePlayer(&bullet, 0, 0, 25, 25, 0);

    SDL_Event event;
    while (1) {
	prepareScene(&app);
	playerListener(&player, &bullet);
	handleBulletMove(&bullet);
	placeRectangle(&app, &player);
	placeRectangle(&app, &bullet);
	presentScene(&app);
	SDL_Delay(16);
    }

    return 0;
}
