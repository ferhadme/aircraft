/*
 * Copyright (c) 2024, Farhad Mehdizada (@ferhadme)
 */

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

typedef struct
{
    int x;
    int y;
    int w;
    int h;
    SDL_Texture *texture;
} Entity;

typedef struct
{
    Entity entity;
    int dx;
    int dy;
    int self_health;
    int home_health;
} Player;

typedef struct Obj_Node
{
    struct Obj_Node *next;
    struct Obj_Node *prev;
} Obj_Node;

typedef struct
{
    Obj_Node node;
    Entity entity;
} Enemy;

typedef struct
{
    Obj_Node node;
    Entity entity;
} Bullet;

typedef struct
{
    Bullet *head;
    Bullet *tail;
} Bullet_Stage;

typedef struct
{
    Enemy *head;
    Enemy *tail;
    int *spawn_slots;
} Enemy_Stage;

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
void prepare_scene()
{
    SDL_SetRenderDrawColor(app.renderer, 20, 20, 20, 20);
    SDL_RenderClear(app.renderer);
}

void present_scene()
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

int get_spawn_coordinate()
{
    int min = SCREEN_Y_START_POS;
    int max = SCREEN_HEIGHT - PLAYER_HEIGHT;
    return (rand() % (max - min + 1)) + min;
    // int offset = (rand() % (max - min + 1)) + min;

    // if (*(spawn_slots + offset) == 0) {
    // 	*(spawn_slots + offset) == 1;
    // 	return offset;
    // }

    // return getSpawnCoordinate(spawn_slots);
}

Bullet *get_next_bullet(Bullet *bullet)
{
    return (Bullet *) bullet->node.next;
}

Enemy *get_next_enemy(Enemy *enemy)
{
    return (Enemy *) enemy->node.next;
}

void free_enemy(Enemy *enemy)
{
    SDL_DestroyTexture(enemy->entity.texture);
    free(enemy);
}

void free_bullet(Bullet *bullet)
{
    SDL_DestroyTexture(bullet->entity.texture);
    free(bullet);
}

/*
 * Game objects initializers
 */
void initialize_player(Player *player)
{
    player->entity.texture = IMG_LoadTexture(app.renderer, "res/aircraft.png");
    player->entity.x = PLAYER_START_POS_X;
    player->entity.y = PLAYER_START_POS_Y;
    player->entity.w = PLAYER_WIDTH;
    player->entity.h = PLAYER_HEIGHT;
    player->dx = 0;
    player->dy = 0;
    player->self_health = PLAYER_HEALTH;
    player->home_health = HOME_HEALTH;
}

void initialize_bullet(Bullet *bullet)
{
    bullet->entity.x = 0;
    bullet->entity.y = 0;
    bullet->entity.w = BULLET_WIDTH;
    bullet->entity.h = BULLET_HEIGHT;
    bullet->node.prev = NULL;
    bullet->node.next = NULL;
    bullet->entity.texture = IMG_LoadTexture(app.renderer, "res/bullet.png");
}

void initialize_enemy_stage(Enemy_Stage *enemy_stage)
{
    memset(enemy_stage, 0, sizeof(Enemy_Stage));
    // int possible_slot_len = (SCREEN_HEIGHT - height) - SCREEN_Y_START_POS + 1;
    // enemy_stage->spawn_slots = malloc(possible_slot_len * sizeof(int));
    // memset(enemy_stage->spawn_slots, 0, possible_slot_len * sizeof(int));
}

void initialize_bullet_stage(Bullet_Stage *bullet_stage)
{
    memset(bullet_stage, 0, sizeof(Bullet_Stage));
}

void initialize_enemy(Enemy *enemy)
{
    enemy->node.prev = NULL;
    enemy->node.next = NULL;
    enemy->entity.texture = IMG_LoadTexture(app.renderer, "res/enemy.png");
    enemy->entity.w = PLAYER_WIDTH;
    enemy->entity.h = PLAYER_HEIGHT;
    enemy->entity.x = SCREEN_WIDTH - enemy->entity.w;
    enemy->entity.y = get_spawn_coordinate();
}

/*
 * Game Entity position manipulation
 */
void update_entity_position(Entity *entity, int dx, int dy)
{
    entity->x += dx;
    entity->y += dy;
}

void update_player_position(Player *player)
{
    Entity *e = &player->entity;
    update_entity_position(e, player->dx, player->dy);

    if (e->x < 0)
	e->x = 0;
    else if (e->x > SCREEN_WIDTH - e->w)
	e->x = SCREEN_WIDTH - e->w;

    if (e->y < SCREEN_Y_START_POS)
	e->y = SCREEN_Y_START_POS;
    else if (e->y > SCREEN_HEIGHT - e->h)
	e->y = SCREEN_HEIGHT - e->h;
}

void place_entity(Entity *entity)
{
    SDL_Rect rect;
    rect.x = entity->x;
    rect.y = entity->y;
    SDL_QueryTexture(entity->texture, NULL, NULL, &rect.w, &rect.h);
    rect.w = entity->w;
    rect.h = entity->h;
    SDL_RenderCopy(app.renderer, entity->texture, NULL, &rect);
}

void place_bullet_stage(Bullet_Stage *bullet_stage)
{
    for (Bullet *bullet = bullet_stage->head; bullet; bullet = get_next_bullet(bullet)) {
	place_entity(&bullet->entity);
    }
}

void place_enemy_stage(Enemy_Stage *enemy_stage)
{
    for (Enemy *enemy = enemy_stage->head; enemy; enemy = get_next_enemy(enemy)) {
	place_entity(&enemy->entity);
    }
}

/*
 * Object collisions
 */
void unlink_collided_obj_node(Obj_Node **head, Obj_Node **tail,
			      Obj_Node *collided)
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


int check_for_collision(Entity *e1, Entity *e2)
{
    return e1->x + e1->w > e2->x &&
	e2->x + e2->w > e1->x &&
	e1->y + e1->h > e2->y &&
	e2->y + e2->h > e1->y;
}

int unlink_collisions(Bullet *bullet, Enemy_Stage *enemy_stage)
{
    for (Enemy *enemy = enemy_stage->head; enemy; enemy = get_next_enemy(enemy)) {
	if (check_for_collision(&bullet->entity, &enemy->entity)) {
	    unlink_collided_obj_node((Obj_Node **) &enemy_stage->head,
				     (Obj_Node **) &enemy_stage->tail,
				     &enemy->node);
	    free_enemy(enemy);
	    return 1;
	}
    }
    return 0;
}

void add_node_to_obj_stage(Obj_Node **head, Obj_Node **tail,
			   Obj_Node *obj_node)
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

void unlink_screen_passed_obj(Obj_Node **head, Obj_Node **tail)
{
    *head = (*head)->next;
    if (!(*head)) {
	*tail = NULL;
    }
}

/*
 * Bullets
 */
void handle_bullet_firing(Entity *from, Bullet_Stage *bullet_stage)
{
    Bullet *bullet = malloc(sizeof(Bullet));

    initialize_bullet(bullet);

    update_entity_position(&bullet->entity, from->x + from->w, from->y + from->h / 2.5);
    add_node_to_obj_stage((Obj_Node **) &bullet_stage->head,
			  (Obj_Node **) &bullet_stage->tail,
			  &bullet->node);
}

void move_bullets(Bullet_Stage *bullet_stage, Enemy_Stage *enemy_stage)
{
    if (END_OF_SCREEN_PASS_COND(bullet_stage->head)) {
	Bullet *tmp = bullet_stage->head;
	unlink_screen_passed_obj((Obj_Node **) &bullet_stage->head,
				 (Obj_Node **) &bullet_stage->tail);
	free_bullet(tmp);
    }

    Bullet *bullet = bullet_stage->head;

    while (bullet) {
	update_entity_position(&bullet->entity, BULLET_SPEED, 0);

	int collision = unlink_collisions(bullet, enemy_stage);

	if (collision) {
	    Bullet *next = get_next_bullet(bullet);
	    unlink_collided_obj_node((Obj_Node **) &bullet_stage->head,
				     (Obj_Node **) &bullet_stage->tail,
				     &bullet->node);

	    free_bullet(bullet);
	    bullet = next;
	} else {
	    bullet = get_next_bullet(bullet);
	}
    }
}

void spawn_enemy_bullets(Enemy_Stage *enemy_stage,
			 Bullet_Stage *enemy_bullet_stage)
{
    for (Enemy *enemy = enemy_stage->head; enemy; enemy = get_next_enemy(enemy)) {
	if (spawn(ENEMY_BULLET_SPAWN_PROBABILITY)) {
	    handle_bullet_firing(&enemy->entity, enemy_bullet_stage);
	}
    }
}

void move_enemy_bullets(Bullet_Stage *enemy_bullet_stage,
		      Player *player)
{
    if (BEGINNING_OF_SCREEN_PASS_COND(enemy_bullet_stage->head)) {
	Bullet *tmp = enemy_bullet_stage->head;
	unlink_screen_passed_obj((Obj_Node **) &enemy_bullet_stage->head,
				 (Obj_Node **) &enemy_bullet_stage->tail);
	free_bullet(tmp);
    }

    Bullet *bullet = enemy_bullet_stage->head;
    while (bullet) {
	update_entity_position(&bullet->entity, -(BULLET_SPEED), 0);

	int collision_with_player = check_for_collision(&bullet->entity, &player->entity);
	int collision_with_home = BEGINNING_OF_SCREEN_PASS_COND(bullet);

	if (collision_with_player || collision_with_home) {
	    Bullet *next = get_next_bullet(bullet);
	    unlink_collided_obj_node((Obj_Node **) &enemy_bullet_stage->head,
				     (Obj_Node **) &enemy_bullet_stage->tail,
				     &bullet->node);
	    free_bullet(bullet);
	    bullet = next;

	    if (collision_with_player) {
		player->self_health -= ENEMY_BULLET_DAMAGE;
		printf("Player health: %d\n", player->self_health);
	    } else {
		player->home_health -= ENEMY_BULLET_DAMAGE;
		printf("Home health: %d\n", player->home_health);
	    }
	} else {
	    bullet = get_next_bullet(bullet);
	}
    }
}

/*
 * Enemies
 */
void spawn_enemies(Enemy_Stage *enemy_stage)
{
    if (!spawn(ENEMY_SPAWN_PROBABILITY)) return;
    Enemy *enemy = malloc(sizeof(Enemy));
    initialize_enemy(enemy);

    add_node_to_obj_stage((Obj_Node **) &enemy_stage->head,
		      (Obj_Node **) &enemy_stage->tail,
		      &enemy->node);
}

void move_enemies(Enemy_Stage *enemy_stage, Player *player)
{
    if (BEGINNING_OF_SCREEN_PASS_COND(enemy_stage->head)) {
	Enemy *tmp = enemy_stage->head;
	unlink_screen_passed_obj((Obj_Node **) &enemy_stage->head,
				 (Obj_Node **) &enemy_stage->tail);
	free_enemy(tmp);
    }

    for (Enemy *enemy = enemy_stage->head; enemy; enemy = get_next_enemy(enemy)) {
	update_entity_position(&enemy->entity, -(ENEMY_SPEED), 0);
	if (check_for_collision(&enemy->entity, &player->entity)) {
	    unlink_collided_obj_node((Obj_Node **) &enemy_stage->head,
				     (Obj_Node **) &enemy_stage->tail,
				     &enemy->node);
	    player->self_health = 0;
	    exit(1); // Find better way
	}
    }
}

/*
 * Event loop listeners
 */
void on_key_listener(Player *player, Bullet_Stage *bullet_stage,
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
	    handle_bullet_firing(&player->entity, bullet_stage);
	    break;
	default:
	    break;
	}

	player->dx = dx;
	player->dy = dy;
    }
}

void game_listener(Player *player, Bullet_Stage *bullet_stage)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
	switch (event.type) {
	case SDL_KEYDOWN:
	    on_key_listener(player, bullet_stage, &event.key);
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

    Player player;
    initialize_player(&player);

    Bullet_Stage player_bullet_stage;
    initialize_bullet_stage(&player_bullet_stage);

    Bullet_Stage enemy_bullet_stage;
    initialize_bullet_stage(&enemy_bullet_stage);

    Enemy_Stage enemy_stage;
    memset(&enemy_stage, 0, sizeof(Enemy_Stage));

    SDL_Event event;
    while (!app.termination) {
	prepare_scene();

	game_listener(&player, &player_bullet_stage);
	update_player_position(&player);

	spawn_enemies(&enemy_stage);
	spawn_enemy_bullets(&enemy_stage, &enemy_bullet_stage);

	move_bullets(&player_bullet_stage, &enemy_stage);
	move_enemy_bullets(&enemy_bullet_stage, &player);
	move_enemies(&enemy_stage, &player);

	place_entity(&player.entity);
	place_bullet_stage(&player_bullet_stage);
	place_bullet_stage(&enemy_bullet_stage);
	place_enemy_stage(&enemy_stage);

	present_scene();
	SDL_Delay(16);
    }

    // free(enemy_stage.spawn_slots);
    SDL_Quit();

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
// TODO: Make game harder as it progresses
// TODO: Update readme
