#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define SCREEN_WIDTH 1400
#define SCREEN_HEIGHT 900

struct App
{
    SDL_Window *window;
    SDL_Renderer *renderer;
} app;

struct Entity
{
    int x;
    int y;
    int w;
    int h;
    SDL_Texture *texture;
};

struct Bullet
{
    struct Entity entity;
    struct Bullet *next;
};

struct Bullet_Stage
{
    struct Bullet *head;
    struct Bullet *tail;
};

void updatePlayer(struct Entity *player, int x, int y, int w, int h)
{
    player->x = x;
    player->y = y;
    player->w = w;
    player->h = h;
}

void handleBulletFiring(struct Entity *player, struct Bullet_Stage *stage)
{
    struct Bullet *bullet = malloc(sizeof(struct Bullet));
    bullet->next = NULL;
    bullet->entity.texture = IMG_LoadTexture(app.renderer, "bullet.png");
    updatePlayer(&bullet->entity, player->x + player->w, player->y + player->h / 2.5, 25, 25);

    if (!stage->tail) {
	stage->head = bullet;
	stage->tail = bullet;
    } else {
	stage->tail->next = bullet;
	stage->tail = bullet;
    }
}

void updateCoordinates(struct Entity *player, struct Bullet_Stage *stage, SDL_KeyboardEvent *event)
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
	case SDL_SCANCODE_LCTRL:
	    handleBulletFiring(player, stage);
	    break;
	default:
	    break;
	}
	if (player->y < 0) player->y = 0;
	if (player->x < 0) player->x = 0;
    }
}

void prepareScene()
{
    SDL_SetRenderDrawColor(app.renderer, 20, 20, 20, 20);
    SDL_RenderClear(app.renderer);
}

void presentScene()
{
    SDL_RenderPresent(app.renderer);
}

void playerListener(struct Entity *player, struct Bullet_Stage *stage)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
	switch (event.type) {
	case SDL_KEYUP:
	    updateCoordinates(player, stage, &event.key);
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

void placeEntity(struct Entity *entity)
{
    SDL_Rect rect;
    rect.x = entity->x;
    rect.y = entity->y;
    SDL_QueryTexture(entity->texture, NULL, NULL, &rect.w, &rect.h);
    rect.w = entity->w;
    rect.h = entity->h;
    SDL_RenderCopy(app.renderer, entity->texture, NULL, &rect);
}

void placeBulletStage(struct Bullet_Stage *stage)
{
    for (struct Bullet *bullet = stage->head; bullet != NULL; bullet = bullet->next) {
	placeEntity(&bullet->entity);
    }
}

void moveBullets(struct Bullet_Stage *stage)
{
    for (struct Bullet *bullet = stage->head; bullet != NULL; bullet = bullet->next) {
	bullet->entity.x += 10;
	// TODO: handle screen
    }
}

int main(void)
{
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
    updatePlayer(&player, 0, 0, 75, 75);

    struct Bullet_Stage stage;
    memset(&stage, 0, sizeof(struct Bullet_Stage));

    SDL_Event event;
    while (1) {
	prepareScene();
	playerListener(&player, &stage);
	moveBullets(&stage);
	placeEntity(&player);
	placeBulletStage(&stage);
	// TODO: clear them as they closer to WIDTH
	presentScene();
	SDL_Delay(16);
    }

    return 0;
}
