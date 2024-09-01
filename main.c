#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define SCREEN_WIDTH 1400
#define SCREEN_HEIGHT 900

#define PLAYER_START_POS_X 0
#define PLAYER_START_POS_Y 0

#define PLAYER_WIDTH 75
#define PLAYER_HEIGHT 75
#define BULLET_WIDTH 25
#define BULLET_HEIGHT 25

#define PLAYER_HEALTH 100
#define HOME_HEALTH 300
#define ENEMY_HEALTH 20
#define PLAYER_BULLET_POW 10
#define ENEMY_BULLET_POW 10

#define PLAYER_SPEED 5
#define BULLET_SPEED 8
#define ENEMY_SPEED 0.1

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

struct Player
{
    struct Entity entity;
    int dx;
    int dy;
    int self_health;
    int home_health;
};

struct Enemy
{
    struct Entity entity;
    int health;
    struct Enemy *next;
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

struct Enemy_Stage
{
    struct Enemy *head;
    struct Enemy *tail;
};

/*
 * SDL Initializers
 */
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

/*
 * SDL Scene
 */
void prepareScene()
{
    SDL_SetRenderDrawColor(app.renderer, 20, 20, 20, 20);
    SDL_RenderClear(app.renderer);
}

void presentScene()
{
    SDL_RenderPresent(app.renderer);
}

/*
 * Helpers
 */
int spawn()
{
    int n = rand() % 1001;
    if (n <= 990) return 0;
    return 1;
}

int get_spawn_coordinate()
{
    return rand() % SCREEN_HEIGHT;
}

/*
 * Game objects initializers
 */
void initializePlayer(struct Player *player)
{
    player->entity.texture = IMG_LoadTexture(app.renderer, "aircraft.png");
    player->entity.x = PLAYER_START_POS_X;
    player->entity.y = PLAYER_START_POS_Y;
    player->entity.w = PLAYER_WIDTH;
    player->entity.h = PLAYER_HEIGHT;
    player->dx = 0;
    player->dy = 0;
    player->self_health = PLAYER_HEALTH;
    player->home_health = HOME_HEALTH;
}

void initializeBullet(struct Bullet *bullet)
{
    bullet->entity.x = 0;
    bullet->entity.y = 0;
    bullet->entity.w = BULLET_WIDTH;
    bullet->entity.h = BULLET_HEIGHT;
    bullet->next = NULL;
    bullet->entity.texture = IMG_LoadTexture(app.renderer, "bullet.png");
}

void initializeEnemy(struct Enemy *enemy)
{
    enemy->health = ENEMY_HEALTH;
    enemy->next = NULL;
    enemy->entity.texture = IMG_LoadTexture(app.renderer, "enemy.png");
    enemy->entity.w = PLAYER_WIDTH;
    enemy->entity.h = PLAYER_HEIGHT;

    enemy->entity.x = SCREEN_WIDTH - enemy->entity.w;
    enemy->entity.y = get_spawn_coordinate();
}

/*
 * Game Entity position manipulation
 */
void updateEntityPosition(struct Entity *entity, int dx, int dy)
{
    entity->x += dx;
    entity->y += dy;

    if (entity->x < 0) entity->x = 0;
    if (entity->y < 0) entity->y = 0;
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

void placeBulletStage(struct Bullet_Stage *bullet_stage)
{
    for (struct Bullet *bullet = bullet_stage->head; bullet != NULL; bullet = bullet->next) {
	placeEntity(&bullet->entity);
    }
}

void placeEnemyStage(struct Enemy_Stage *enemy_stage)
{
    for (struct Enemy *enemy = enemy_stage->head; enemy != NULL; enemy = enemy->next) {
	placeEntity(&enemy->entity);
    }
}

/*
 * Bullets
 */
void handleBulletFiring(struct Player *player, struct Bullet_Stage *bullet_stage)
{
    struct Bullet *bullet = malloc(sizeof(struct Bullet));

    initializeBullet(bullet);
    struct Entity e = player->entity;
    updateEntityPosition(&bullet->entity, e.x + e.w, e.y + e.h / 2.5);

    if (!bullet_stage->tail) {
	bullet_stage->head = bullet;
	bullet_stage->tail = bullet;
    } else {
	bullet_stage->tail->next = bullet;
	bullet_stage->tail = bullet;
    }
}

void moveBullets(struct Bullet_Stage *bullet_stage)
{
    for (struct Bullet *bullet = bullet_stage->head; bullet != NULL; bullet = bullet->next) {
	bullet->entity.x += BULLET_SPEED;
	// TODO: handle screen
    }
}

void moveEnemies(struct Enemy_Stage *enemy_stage)
{
    for (struct Enemy *enemy = enemy_stage->head; enemy != NULL; enemy = enemy->next) {
	enemy->entity.x -= ENEMY_SPEED;
    }
}

/*
 * Enemies
 */
void spawnEnemies(struct Enemy_Stage *enemy_stage)
{
    if (!spawn()) return;
    struct Enemy *enemy = malloc(sizeof(struct Enemy));
    initializeEnemy(enemy);

    if (!enemy_stage->tail) {
	enemy_stage->head = enemy;
	enemy_stage->tail = enemy;
    } else {
	enemy_stage->tail->next = enemy;
	enemy_stage->tail = enemy;
    }
}

void onKeyListener(struct Player *player, struct Bullet_Stage *bullet_stage, SDL_KeyboardEvent *event)
{
    if (event->repeat == 0) {
	int dx = 0;
	int dy = 0;
	switch (event->keysym.scancode) {
	case SDL_SCANCODE_UP:
	    dy = -(PLAYER_SPEED);
	    break;
	case SDL_SCANCODE_DOWN:
	    dy = PLAYER_SPEED;
	    break;
	case SDL_SCANCODE_RIGHT:
	    dx = PLAYER_SPEED;
	    break;
	case SDL_SCANCODE_LEFT:
	    dx = -(PLAYER_SPEED);
	    break;
	case SDL_SCANCODE_LCTRL:
	    handleBulletFiring(player, bullet_stage);
	    break;
	default:
	    break;
	}

	player->dx = dx;
	player->dy = dy;
    }
}

void gameListener(struct Player *player, struct Bullet_Stage *bullet_stage)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
	switch (event.type) {
	case SDL_KEYDOWN:
	    onKeyListener(player, bullet_stage, &event.key);
	    break;
	case SDL_KEYUP:
	    player->dx = 0;
	    player->dy = 0;
	    break;
	case SDL_QUIT:
	    exit(0);
	    break;
	default:
	    break;
	}
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

    struct Player player;
    initializePlayer(&player);

    struct Bullet_Stage bullet_stage;
    memset(&bullet_stage, 0, sizeof(struct Bullet_Stage));

    struct Enemy_Stage enemy_stage;
    memset(&enemy_stage, 0, sizeof(struct Enemy_Stage));

    SDL_Event event;
    while (1) {
	prepareScene();
	gameListener(&player, &bullet_stage);
	updateEntityPosition(&player.entity, player.dx, player.dy);
	moveBullets(&bullet_stage);
	spawnEnemies(&enemy_stage);

	moveEnemies(&enemy_stage);

	placeEntity(&player.entity);
	placeBulletStage(&bullet_stage);
	placeEnemyStage(&enemy_stage);
	presentScene();
	SDL_Delay(16);
    }

    return 0;
}

// TODO: Resolve collisions
// TODO: free bullets as they pass the screen
// TODO: manage memory leaks
