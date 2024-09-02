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
// #define ENEMY_SPEED 20

#define BULLET_SCREEN_PASS_COND(head) ((head) && (head->entity.x > SCREEN_WIDTH))
#define ENEMY_SCREEN_PASS_COND(head) ((head) && (head->entity.x + head->entity.w < 0))

struct App
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    int termination;
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

struct Obj_Node
{
    struct Entity entity;
    struct Obj_Node *next;
    struct Obj_Node *prev;
};

struct Obj_Stage
{
    struct Obj_Node *head;
    struct Obj_Node *tail;
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

int get_spawn_coordinate(int height)
{
    return rand() % (SCREEN_HEIGHT - height);
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

void initializeBullet(struct Obj_Node *bullet)
{
    bullet->entity.x = 0;
    bullet->entity.y = 0;
    bullet->entity.w = BULLET_WIDTH;
    bullet->entity.h = BULLET_HEIGHT;
    bullet->prev = NULL;
    bullet->next = NULL;
    bullet->entity.texture = IMG_LoadTexture(app.renderer, "bullet.png");
}

void initializeEnemy(struct Obj_Node *enemy)
{
    enemy->prev = NULL;
    enemy->next = NULL;
    enemy->entity.texture = IMG_LoadTexture(app.renderer, "enemy.png");
    enemy->entity.w = PLAYER_WIDTH;
    enemy->entity.h = PLAYER_HEIGHT;
    enemy->entity.x = SCREEN_WIDTH - enemy->entity.w;
    enemy->entity.y = get_spawn_coordinate(enemy->entity.h);
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

void placeBulletStage(struct Obj_Stage *bullet_stage)
{
    for (struct Obj_Node *bullet = bullet_stage->head; bullet != NULL; bullet = bullet->next) {
	placeEntity(&bullet->entity);
    }
}

void placeEnemyStage(struct Obj_Stage *enemy_stage)
{
    for (struct Obj_Node *enemy = enemy_stage->head; enemy; enemy = enemy->next) {
	placeEntity(&enemy->entity);
    }
}

/*
 * Collisions
 */
void removeCollidedObj(struct Obj_Stage *obj_stage, struct Obj_Node *collided)
{
    struct Obj_Node *head = obj_stage->head;
    struct Obj_Node *tail = obj_stage->tail;
    if (collided == head) {
	if (collided == obj_stage->tail) {
	    obj_stage->tail = NULL;
	}
	obj_stage->head = head->next;
    } else if (collided == tail) {
	tail->prev->next = NULL;
	obj_stage->tail = collided->prev;
    } else {
	collided->prev->next = collided->next;
    }
    free(collided);
}

/*
 * Checks if two objects collided with each other
 */
int checkForCollision(struct Entity *e1, struct Entity *e2)
{
    // e1.x+e1.w > e2.x
    // e2.x+e2.w > e1.x
    // e1.y+e1.h > e2.y
    // e2.y+e2.h > e1.y

    return e1->x + e1->w > e2->x &&
	e2->x + e2->w > e1->x &&
	e1->y + e1->h > e2->y &&
	e2->y + e2->h > e1->y;
}

/*
 * e.x
 */
int removeCollisions(struct Obj_Node *bullet, struct Obj_Stage *enemy_stage)
{
    for (struct Obj_Node *enemy = enemy_stage->head; enemy; enemy = enemy->next) {
	if (checkForCollision(&bullet->entity, &enemy->entity)) {
	    removeCollidedObj(enemy_stage, enemy);
	    return 1;
	}
    }

    return 0;
}

void addNodeToObjStage(struct Obj_Stage *obj_stage, struct Obj_Node *obj_node)
{
    if (!obj_stage->tail) {
	obj_stage->head = obj_node;
	obj_stage->tail = obj_node;
    } else {
	obj_stage->tail->next = obj_node;
	obj_node->prev = obj_stage->tail;
	obj_stage->tail = obj_node;
    }
}

/*
 * Bullets
 */
void handleBulletFiring(struct Player *player, struct Obj_Stage *bullet_stage)
{
    struct Obj_Node *bullet = malloc(sizeof(struct Obj_Node));

    initializeBullet(bullet);
    struct Entity e = player->entity;
    updateEntityPosition(&bullet->entity, e.x + e.w, e.y + e.h / 2.5);
    addNodeToObjStage(bullet_stage, bullet);
}

void removeScreenPassedObj(struct Obj_Stage *obj_stage, int pass_width_cond)
{
    struct Obj_Node *head = obj_stage->head;
    if (pass_width_cond) {
	obj_stage->head = head->next;
	if (!obj_stage->head) {
	    obj_stage->tail = NULL;
	}
	free(head);
    }
}

void moveBullets(struct Obj_Stage *bullet_stage, struct Obj_Stage *enemy_stage)
{
    removeScreenPassedObj(bullet_stage, BULLET_SCREEN_PASS_COND(bullet_stage->head));
    for (struct Obj_Node *bullet = bullet_stage->head; bullet; bullet = bullet->next) {
	bullet->entity.x += BULLET_SPEED;
	int collision = removeCollisions(bullet, enemy_stage);
	if (collision) {
	    removeCollidedObj(bullet_stage, bullet);
	}
    }
}

void moveEnemies(struct Obj_Stage *enemy_stage)
{
    removeScreenPassedObj(enemy_stage, ENEMY_SCREEN_PASS_COND(enemy_stage->head));
    for (struct Obj_Node *enemy = enemy_stage->head; enemy; enemy = enemy->next) {
	enemy->entity.x -= ENEMY_SPEED;
    }
}

/*
 * Enemies
 */
void spawnEnemies(struct Obj_Stage *enemy_stage)
{
    if (!spawn()) return;
    struct Obj_Node *enemy = malloc(sizeof(struct Obj_Node));
    initializeEnemy(enemy);

    addNodeToObjStage(enemy_stage, enemy);
}

void onKeyListener(struct Player *player, struct Obj_Stage *bullet_stage, SDL_KeyboardEvent *event)
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

void gameListener(struct Player *player, struct Obj_Stage *bullet_stage)
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
	    app.termination = 1;
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

    app.termination = 0;
    app.window = initialize_window();

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    app.renderer = initialize_renderer(app.window);

    if (!IMG_Init(IMG_INIT_PNG)) {
	printf("Couldn't initialize image: %s\n", SDL_GetError());
	exit(1);
    }

    struct Player player;
    initializePlayer(&player);

    struct Obj_Stage bullet_stage;
    memset(&bullet_stage, 0, sizeof(struct Obj_Stage));

    struct Obj_Stage enemy_stage;
    memset(&enemy_stage, 0, sizeof(struct Obj_Stage));

    SDL_Event event;
    while (!app.termination) {
	prepareScene();
	gameListener(&player, &bullet_stage);
	updateEntityPosition(&player.entity, player.dx, player.dy);
	moveBullets(&bullet_stage, &enemy_stage);
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

// TODO: Resolve Segmentation fault in collisions
// TODO: Resolve collisions of bullets and aircrafts
// TODO: Add enemy aircrafts bullets
// TODO: Add audio
// TODO: Add score table
// TODO: manage memory leaks
