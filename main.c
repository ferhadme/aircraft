#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

/*
 * Game settings
 */
#define SCREEN_WIDTH 1400
#define SCREEN_HEIGHT 900
#define SCREEN_Y_START_POS 50

#define PLAYER_START_POS_X 0
#define PLAYER_START_POS_Y SCREEN_Y_START_POS

#define PLAYER_WIDTH 75
#define PLAYER_HEIGHT 75
#define BULLET_WIDTH 25
#define BULLET_HEIGHT 25

#define PLAYER_HEALTH 100
#define HOME_HEALTH 300
#define ENEMY_HEALTH 20
#define PLAYER_BULLET_DAMAGE 10
#define ENEMY_BULLET_DAMAGE 10

#define PLAYER_SPEED 5
#define BULLET_SPEED 8
#define ENEMY_BULLET_SPEED 3
#define ENEMY_SPEED 1

#define ENEMY_SPAWN_PROBABILITY 10
#define ENEMY_BULLET_SPAWN_PROBABILITY 3

#define END_OF_SCREEN_PASS_COND(obj)		\
    ((obj) && (obj->entity.x > SCREEN_WIDTH))

#define BEGINNING_OF_SCREEN_PASS_COND(obj)	\
    ((obj) && (obj->entity.x < 0))

#define SDL_INIT_ERR_TEMPLATE "Couldn't initialize %s: %s\n"

/*
 * Structs
 */
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
    struct Obj_Node *next;
    struct Obj_Node *prev;
};

struct Enemy
{
    struct Obj_Node node;
    struct Entity entity;
};

struct Bullet
{
    struct Obj_Node node;
    struct Entity entity;
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
void SDL_SafeInitialize(int initializer, const char *component)
{
    if (!initializer) {
	fprintf(stderr, SDL_INIT_ERR_TEMPLATE, component, SDL_GetError());
	exit(1);
    }
}

SDL_Window *initialize_window(void)
{
    int windowFlags = 0;
    SDL_Window *window = SDL_CreateWindow("2D Aircraft Game",
					  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					  SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags);

    if (!window) {
	fprintf(stderr, SDL_INIT_ERR_TEMPLATE, "window", SDL_GetError());
	exit(1);
    }

    return window;
}

SDL_Renderer *initialize_renderer(SDL_Window *window)
{
    int rendererFlags = SDL_RENDERER_ACCELERATED;
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, rendererFlags);

    if (!renderer) {
	fprintf(stderr, SDL_INIT_ERR_TEMPLATE, "renderer", SDL_GetError());
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
int spawn(int probability)
{
    int n = rand() % 1001;
    return n <= probability;
}

int get_spawn_coordinate(int height)
{
    return rand() % (SCREEN_HEIGHT - height);
}

struct Bullet *getNextBullet(struct Bullet *bullet)
{
    return (struct Bullet *) bullet->node.next;
}

struct Enemy *getNextEnemy(struct Enemy *enemy)
{
    return (struct Enemy *) enemy->node.next;
}

void free_enemy(struct Enemy *enemy)
{
    SDL_DestroyTexture(enemy->entity.texture);
    free(enemy);
}

void free_bullet(struct Bullet *bullet)
{
    SDL_DestroyTexture(bullet->entity.texture);
    free(bullet);
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
    bullet->entity.y = SCREEN_Y_START_POS;
    bullet->entity.w = BULLET_WIDTH;
    bullet->entity.h = BULLET_HEIGHT;
    bullet->node.prev = NULL;
    bullet->node.next = NULL;
    bullet->entity.texture = IMG_LoadTexture(app.renderer, "bullet.png");
}

void initializeBulletStage(struct Bullet_Stage *bullet_stage)
{
    memset(bullet_stage, 0, sizeof(struct Bullet_Stage));
}

void initializeEnemy(struct Enemy *enemy)
{
    enemy->node.prev = NULL;
    enemy->node.next = NULL;
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
}

void updatePlayerPosition(struct Player *player)
{
    struct Entity *e = &player->entity;
    updateEntityPosition(e, player->dx, player->dy);

    if (e->x < 0)
	e->x = 0;
    else if (e->x > SCREEN_WIDTH - e->w)
	e->x = SCREEN_WIDTH - e->w;

    if (e->y < SCREEN_Y_START_POS)
	e->y = SCREEN_Y_START_POS;
    else if (e->y > SCREEN_HEIGHT - e->h)
	e->y = SCREEN_HEIGHT - e->h;
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
    for (struct Bullet *bullet = bullet_stage->head; bullet; bullet = getNextBullet(bullet)) {
	placeEntity(&bullet->entity);
    }
}

void placeEnemyStage(struct Enemy_Stage *enemy_stage)
{
    for (struct Enemy *enemy = enemy_stage->head; enemy; enemy = getNextEnemy(enemy)) {
	placeEntity(&enemy->entity);
    }
}

/*
 * Object collisions
 */
void unlinkCollidedObjNode(struct Obj_Node **head, struct Obj_Node **tail,
			   struct Obj_Node *collided)
{
    if (collided == *head) {
	if (collided == *tail) {
	    *tail = NULL;
	}
	*head = (*head)->next;
    } else if (collided == *tail) {
	(*tail)->prev->next = NULL;
	*tail = (*tail)->prev;
    } else {
	collided->next->prev = collided->prev;
	collided->prev->next = collided->next;
    }
}


int checkForCollision(struct Entity *e1, struct Entity *e2)
{
    return e1->x + e1->w > e2->x &&
	e2->x + e2->w > e1->x &&
	e1->y + e1->h > e2->y &&
	e2->y + e2->h > e1->y;
}

int unlinkCollisions(struct Bullet *bullet, struct Enemy_Stage *enemy_stage)
{
    for (struct Enemy *enemy = enemy_stage->head; enemy; enemy = getNextEnemy(enemy)) {
	if (checkForCollision(&bullet->entity, &enemy->entity)) {
	    unlinkCollidedObjNode((struct Obj_Node **) &enemy_stage->head,
				  (struct Obj_Node **) &enemy_stage->tail,
				  &enemy->node);
	    free_enemy(enemy);
	    return 1;
	}
    }
    return 0;
}

void addNodeToObjStage(struct Obj_Node **head, struct Obj_Node **tail,
		       struct Obj_Node *obj_node)
{
    if (!(*tail)) {
	*head = obj_node;
	*tail = obj_node;
    } else {
	(*tail)->next = obj_node;
	obj_node->prev = *tail;
	*tail = obj_node;
    }
}

void unlinkScreenPassedObj(struct Obj_Node **head, struct Obj_Node **tail)
{
    *head = (*head)->next;
    if (!(*head)) {
	*tail = NULL;
    }
}

/*
 * Bullets
 */
void handleBulletFiring(struct Entity *from, struct Bullet_Stage *bullet_stage)
{
    struct Bullet *bullet = malloc(sizeof(struct Bullet));

    initializeBullet(bullet);

    updateEntityPosition(&bullet->entity, from->x + from->w, from->y + from->h / 2.5);
    addNodeToObjStage((struct Obj_Node **) &bullet_stage->head,
		      (struct Obj_Node **) &bullet_stage->tail,
		      &bullet->node);
}

void moveBullets(struct Bullet_Stage *bullet_stage, struct Enemy_Stage *enemy_stage)
{
    if (END_OF_SCREEN_PASS_COND(bullet_stage->head)) {
	struct Bullet *tmp = bullet_stage->head;
	unlinkScreenPassedObj((struct Obj_Node **) &bullet_stage->head,
			      (struct Obj_Node **) &bullet_stage->tail);
	free_bullet(tmp);
    }

    struct Bullet *bullet = bullet_stage->head;

    while (bullet) {
	updateEntityPosition(&bullet->entity, BULLET_SPEED, 0);

	int collision = unlinkCollisions(bullet, enemy_stage);

	if (collision) {
	    struct Bullet *next = getNextBullet(bullet);
	    unlinkCollidedObjNode((struct Obj_Node **) &bullet_stage->head,
				  (struct Obj_Node **) &bullet_stage->tail,
				  &bullet->node);

	    free_bullet(bullet);
	    bullet = next;
	} else {
	    bullet = getNextBullet(bullet);
	}
    }
}

void spawnEnemyBullets(struct Enemy_Stage *enemy_stage,
		       struct Bullet_Stage *enemy_bullet_stage)
{
    for (struct Enemy *enemy = enemy_stage->head; enemy; enemy = getNextEnemy(enemy)) {
	if (spawn(ENEMY_BULLET_SPAWN_PROBABILITY)) {
	    handleBulletFiring(&enemy->entity, enemy_bullet_stage);
	}
    }
}

void moveEnemyBullets(struct Bullet_Stage *enemy_bullet_stage,
		      struct Player *player)
{
    if (BEGINNING_OF_SCREEN_PASS_COND(enemy_bullet_stage->head)) {
	struct Bullet *tmp = enemy_bullet_stage->head;
	unlinkScreenPassedObj((struct Obj_Node **) &enemy_bullet_stage->head,
			      (struct Obj_Node **) &enemy_bullet_stage->tail);
	free_bullet(tmp);
    }

    struct Bullet *bullet = enemy_bullet_stage->head;
    while (bullet) {
	updateEntityPosition(&bullet->entity, -(BULLET_SPEED), 0);

	int collisionWithPlayer = checkForCollision(&bullet->entity, &player->entity);
	int collisionWithHome = BEGINNING_OF_SCREEN_PASS_COND(bullet);

	if (collisionWithPlayer || collisionWithHome) {
	    struct Bullet *next = getNextBullet(bullet);
	    unlinkCollidedObjNode((struct Obj_Node **) &enemy_bullet_stage->head,
				  (struct Obj_Node **) &enemy_bullet_stage->tail,
				  &bullet->node);
	    free_bullet(bullet);
	    bullet = next;

	    if (collisionWithPlayer) {
		player->self_health -= ENEMY_BULLET_DAMAGE;
		printf("Player health: %d\n", player->self_health);
	    } else {
		player->home_health -= ENEMY_BULLET_DAMAGE;
		printf("Home health: %d\n", player->home_health);
	    }
	} else {
	    bullet = getNextBullet(bullet);
	}
    }
}

/*
 * Enemies
 */
void spawnEnemies(struct Enemy_Stage *enemy_stage)
{
    if (!spawn(ENEMY_SPAWN_PROBABILITY)) return;
    struct Enemy *enemy = malloc(sizeof(struct Enemy));
    initializeEnemy(enemy);

    addNodeToObjStage((struct Obj_Node **) &enemy_stage->head,
		      (struct Obj_Node **) &enemy_stage->tail,
		      &enemy->node);
}

void moveEnemies(struct Enemy_Stage *enemy_stage, struct Player *player)
{
    if (BEGINNING_OF_SCREEN_PASS_COND(enemy_stage->head)) {
	struct Enemy *tmp = enemy_stage->head;
	unlinkScreenPassedObj((struct Obj_Node **) &enemy_stage->head,
			      (struct Obj_Node **) &enemy_stage->tail);
	free_enemy(tmp);
    }

    for (struct Enemy *enemy = enemy_stage->head; enemy; enemy = getNextEnemy(enemy)) {
	updateEntityPosition(&enemy->entity, -(ENEMY_SPEED), 0);
	if (checkForCollision(&enemy->entity, &player->entity)) {
	    unlinkCollidedObjNode((struct Obj_Node **) &enemy_stage->head,
				  (struct Obj_Node **) &enemy_stage->tail,
				  &enemy->node);
	    player->self_health = 0;
	    exit(1); // Find better way
	}
    }
}

/*
 * Event loop listeners
 */
void onKeyListener(struct Player *player, struct Bullet_Stage *bullet_stage,
		   SDL_KeyboardEvent *event)
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
	    handleBulletFiring(&player->entity, bullet_stage);
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
	    app.termination = 1;
	    break;
	default:
	    break;
	}
    }
}

int main(void)
{
    SDL_SafeInitialize(SDL_Init(SDL_INIT_VIDEO) >= 0, "SDL System");

    app.termination = 0;
    app.window = initialize_window();

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    app.renderer = initialize_renderer(app.window);

    SDL_SafeInitialize(IMG_Init(IMG_INIT_PNG), "image");

    struct Player player;
    initializePlayer(&player);

    struct Bullet_Stage player_bullet_stage;
    initializeBulletStage(&player_bullet_stage);

    struct Bullet_Stage enemy_bullet_stage;
    initializeBulletStage(&enemy_bullet_stage);

    struct Enemy_Stage enemy_stage;
    memset(&enemy_stage, 0, sizeof(struct Enemy_Stage));

    SDL_Event event;
    while (!app.termination) {
	prepareScene();

	gameListener(&player, &player_bullet_stage);
	updatePlayerPosition(&player);

	spawnEnemies(&enemy_stage);
	spawnEnemyBullets(&enemy_stage, &enemy_bullet_stage);

	moveBullets(&player_bullet_stage, &enemy_stage);
	moveEnemyBullets(&enemy_bullet_stage, &player);
	moveEnemies(&enemy_stage, &player);

	placeEntity(&player.entity);
	placeBulletStage(&player_bullet_stage);
	placeBulletStage(&enemy_bullet_stage);
	placeEnemyStage(&enemy_stage);

	presentScene();
	SDL_Delay(16);
    }

    return 0;
}

// TODO: Handle better spawn strategy for enemy aircrafts
// TODO: Make window dynamically resizable
// TODO: Add audio
// TODO: Add score table
// TODO: Add cli args for changing game settings (text file config better)
// TODO: Manage memory leaks
// TODO: Refactor repetitive codes
// TODO: Make better UI
// TODO: Update readme
