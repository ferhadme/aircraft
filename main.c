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

#define ENEMY_SPAWN_PROBABILITY 10
#define ENEMY_BULLET_SPAWN_PROBABILITY 3

#define BULLET_SCREEN_PASS_COND(head) \
    ((head) && (head->entity.x > SCREEN_WIDTH))
#define ENEMY_SCREEN_PASS_COND(head) \
    ((head) && (head->entity.x + head->entity.w < 0))

/*
 * Structs and Enums
 */
enum BULLET_SIDE
{
    PLAYER,
    ENEMY
};

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
    enum BULLET_SIDE bullet_side;
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
    SDL_Window *window = SDL_CreateWindow("Shooter",
					  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					  SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags);

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
    bullet->node.prev = NULL;
    bullet->node.next = NULL;
    bullet->entity.texture = IMG_LoadTexture(app.renderer, "bullet.png");
}

void initializeBulletStage(struct Bullet_Stage *bullet_stage, enum BULLET_SIDE bullet_side)
{
    memset(bullet_stage, 0, sizeof(struct Bullet_Stage));
    bullet_stage->bullet_side = bullet_side;
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
struct Bullet *removeCollidedObjNode(struct Obj_Node **head, struct Obj_Node **tail,
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
    free(collided);
}


int checkForCollision(struct Entity *e1, struct Entity *e2)
{
    return e1->x + e1->w > e2->x &&
	e2->x + e2->w > e1->x &&
	e1->y + e1->h > e2->y &&
	e2->y + e2->h > e1->y;
}

int removeCollisions(struct Bullet *bullet, struct Enemy_Stage *enemy_stage)
{
    for (struct Enemy *enemy = enemy_stage->head; enemy; enemy = getNextEnemy(enemy)) {
	if (checkForCollision(&bullet->entity, &enemy->entity)) {
	    removeCollidedObjNode((struct Obj_Node **) &enemy_stage->head,
				  (struct Obj_Node **) &enemy_stage->tail,
				  &enemy->node);
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

void removeScreenPassedObj(struct Obj_Node **head, struct Obj_Node **tail,
			   int pass_width_cond)
{
    if (pass_width_cond) {
	struct Obj_Node *tmp = *head;
	*head = (*head)->next;
	if (!(*head)) {
	    *tail = NULL;
	}
	free((struct Bullet *) (tmp));
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
    addNodeToObjStage((struct Obj_Node **) &bullet_stage->head,
		      (struct Obj_Node **) &bullet_stage->tail,
		      &bullet->node);
}

void moveBullets(struct Bullet_Stage *bullet_stage, struct Enemy_Stage *enemy_stage)
{
    removeScreenPassedObj((struct Obj_Node **) &bullet_stage->head,
			  (struct Obj_Node **) &bullet_stage->tail,
			  BULLET_SCREEN_PASS_COND(bullet_stage->head));
    struct Bullet *bullet = bullet_stage->head;

    while (bullet) {
	bullet->entity.x += BULLET_SPEED;
	int collision = removeCollisions(bullet, enemy_stage);
	if (collision) {
	    struct Bullet *next = getNextBullet(bullet);
	    removeCollidedObjNode((struct Obj_Node **) &bullet_stage->head,
				  (struct Obj_Node **) &bullet_stage->tail,
				  &bullet->node);
	    bullet = next;
	} else {
	    bullet = getNextBullet(bullet);
	}
    }
}

void spawnEnemyBullets(struct Enemy_Stage *enemy_stage,
		       struct Bullet_Stage *enemy_bullet_stage)
{
    // TODO: Implement
    return;
}

void moveEnemyBullets(struct Bullet_Stage *enemy_bullet_stage,
		      struct Player *player)
{
    // TODO: Implement
    return;
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

void moveEnemies(struct Enemy_Stage *enemy_stage)
{
    removeScreenPassedObj((struct Obj_Node **) &enemy_stage->head,
			  (struct Obj_Node **) &enemy_stage->tail,
			  ENEMY_SCREEN_PASS_COND(enemy_stage->head));

    for (struct Enemy *enemy = enemy_stage->head; enemy; enemy = getNextEnemy(enemy)) {
	enemy->entity.x -= ENEMY_SPEED;
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

    struct Bullet_Stage player_bullet_stage;
    initializeBulletStage(&player_bullet_stage, PLAYER);

    struct Bullet_Stage enemy_bullet_stage;
    initializeBulletStage(&enemy_bullet_stage, ENEMY);

    struct Enemy_Stage enemy_stage;
    memset(&enemy_stage, 0, sizeof(struct Enemy_Stage));

    SDL_Event event;
    while (!app.termination) {
	prepareScene();
	gameListener(&player, &player_bullet_stage);
	updateEntityPosition(&player.entity, player.dx, player.dy);
	spawnEnemies(&enemy_stage);
	spawnEnemyBullets(&enemy_stage, &enemy_bullet_stage);

	moveBullets(&player_bullet_stage, &enemy_stage);
	moveEnemyBullets(&enemy_bullet_stage, &player);
	moveEnemies(&enemy_stage);

	placeEntity(&player.entity);
	placeBulletStage(&player_bullet_stage);
	placeBulletStage(&enemy_bullet_stage);
	placeEnemyStage(&enemy_stage);
	presentScene();
	SDL_Delay(16);
    }

    return 0;
}

// TODO: Add enemy aircrafts bullets
// TODO: Player protects wall from enemy (will affect to score of player)
// TODO: Make window dynamically resizable
// TODO: Add audio
// TODO: Add score table
// TODO: Add cli args for changing game settings (text file config better)
// TODO: Replace readme image with gif
// TODO: manage memory leaks
