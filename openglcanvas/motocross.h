
#define INDEX(x, y) (((y & 1023) << 10) + (x & 1023))

#ifndef MAX 
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#endif

#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)
#define CLAMP(x, min, max) MAX(MIN(x, max), min)

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

typedef struct {
	Vec3   pos;
    Vec3   vel;
    float  rot;
	float  speed;
	float  dig_value;
    float  height;
    float  wheelie_value;
    float  wheelie_acc;
    u8     wheelie_state;
    u8     grounded;
    u8     dig_dirt;
} MotorCycle;

typedef struct {
	int dummy;
	int map_scale;
	MotorCycle player;
    float gravity;
} MotoCross;

static MotoCross motocross = {
    .dummy = 1453,
    .map_scale = 120,
    .player = {
        .pos = {
            .y = 50.f
        },
        .dig_value = 0.5f,
        .speed = 1.f,
        .height = 8.5f,
        .grounded = 0,
        .wheelie_value = 0.f,
	},
    .gravity = 0.51f,
};

#define JUMP_FACTOR 2.5f
#define JUMP_STR    3.f
#define WHEELIE_ACC 2.f

static void modify_height_map_test() {

	int x = 512;
	int y = 512;
	int r = 125;

	float heigth = height_map[INDEX(x, y)].r;
	
	for (int i = 0; i < 1024; i++) {
		for (int j = 0; j < r; j++) {
			height_map[INDEX(x + i, y + j)].r = 0.f;
		}
	}


	for (int i = 0; i < r; i++) {
		for (int j = 0; j < r; j++) {
			height_map[INDEX(x + i, y + j)].r = 200;
		}
	}
}

static float height_map_from_pos(float x, float y) {
    return height_map[INDEX((int)x, (int)y)].r; // (float)motocross.map_scale;
}

static void motocross_dig_dirt(float x, float y) {

    
    height_map[INDEX((int)x, (int)y)].r -= 0.6f;
    Rgb col = color_map[INDEX((int)x, (int)y)];
    Rgb target = { 87, 62, 77 };

    col.r = lerp(col.r, target.r, 0.98);
    col.g = lerp(col.g, target.g, 0.98);
    col.b = lerp(col.b, target.b, 0.98);

    color_map[INDEX((int)x, (int)y)] = col;
}

static void motocross_physics() {
    MotorCycle* player = &motocross.player;
    float saved_y = player->pos.y;
    player->pos.y -= motocross.gravity;

      player->pos.y += player->vel.y;
    if (!player->grounded) {
        player->vel.y -= motocross.gravity;
        player->vel.y *= 0.98f;
    }


    float h = height_map_from_pos(player->pos.x, player->pos.z);
    if (player->pos.y - player->height < h) {

        float jump_factor = h - (player->pos.y - player->height);
        player->pos.y = h + player->height;

        // while (player->pos.y < h) {
        // }
        printf("collide jump factor %f\n", jump_factor);

        if (player->dig_dirt) {
            jump_factor *= 0.1f;
        }

        if (jump_factor > JUMP_FACTOR) {
            player->vel.y += JUMP_STR * jump_factor;
            player->grounded = 0;
        }
        else { 
            player->grounded = 1;
            player->vel.y *= 0.5f;
        }
    }
    else {
        printf("no collide\n");
    }
    printf("player %f : %f", player->pos.y, h);

  }

static void motocross_update() {

    MotorCycle* player = &motocross.player;
    float dir = 0.f;
    float rot = 0.f;
    float y = 0.f;
    float speed = player->speed;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        dir -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        dir += 1.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        rot += 0.025f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        rot -= 0.025f;
    }

    // debug
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        y += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        y -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_R)== GLFW_PRESS) {
        rot += 0.005f;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        rot -= 0.005f;
    }
    static u8 last_frame = 0;
    if ((glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) && last_frame) {
        printf("duff\n");
        // debug_frame++;
        last_frame = 0;
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE)
        last_frame = 1;
    if (glfwGetKey(window, GLFW_KEY_B)) {
        modify_height_map_test();
    }
    /////// debug

    player->rot += rot;
    Vec2 unit = { cos(player->rot), sin(player->rot) };
    Vec2 offset = vec2_mul_scl(unit, speed * dir);

    player->pos.x += offset.y;
    player->pos.z += offset.x;
    player->pos.y += y;

    motocross_physics();

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        player->wheelie_acc += WHEELIE_ACC;
    }
    else {
        player->wheelie_acc -= WHEELIE_ACC;
    }

    player->wheelie_acc = CLAMP(player->wheelie_acc, -10.f, 10.f);
    player->wheelie_value += player->wheelie_acc;
    player->wheelie_value = CLAMP(player->wheelie_value, 0.f, 900.f);

    if (player->wheelie_value > 150.f) {
        motocross_dig_dirt(player->pos.x, player->pos.z);
        player->dig_dirt = 0;
    }
    else {
        player->dig_dirt = 1;
    }

}

static void motocross_render() {
    MotorCycle* player = &motocross.player;
    Vec2 p = { player->pos.x, player->pos.z };
  
    render_rot(p, player->rot, player->pos.y, 220 + player->wheelie_value, 
        motocross.map_scale, 500, SCREEN_W, SCREEN_H, (Vec2){ 0 });
}

