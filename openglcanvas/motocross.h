
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


typedef struct {
	Vec3   pos;
    Vec3   vel;
    Vec3   acc;
    float  acceleration;
    float  velocity;
    float  rot;
	float  speed;
	float  dig_value;
    float  height;
    float  wheelie_value;
    float  wheelie_acc;
    u8     wheelie_state;
    u8     grounded;
    u8     dig_dirt;
    float height_last[5];
    int   height_i;
} MotorCycle;

typedef struct {
	int dummy;
	int map_scale;
	MotorCycle player;
    float gravity;
    u8 rerender;
} MotoCross;

static MotoCross motocross = {
    .dummy = 1453,
    .map_scale = 120,
    .player = {
        .pos = {
            .y = 50.f
        },
        .dig_value = 0.5f,
        .speed = 0.f,
        .acceleration = 0.045f,
        .height = 8.5f,
        .grounded = 0,
        .wheelie_value = 0.f,
	},
    .gravity = 0.061f,
    .rerender = 1,
};

#define JUMP_FACTOR 2.0f
#define JUMP_STR    3.5f
#define WHEELIE_ACC 1.f
#define MAX_SPEED   2.f
#define AIR_ROT_CONTROL 0.5f

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

#include <float.h>
static void motocross_physics(float dt) {
    MotorCycle* player = &motocross.player;

// 
    // player->vel.x *= 0.999 * dt;
    player->vel.y -= motocross.gravity * dt;
    // player->vel.z *= 0.999 ;

#if 1

    Vec2 unit = { cos(player->rot), sin(player->rot) };

    // player->vel.x += player->acc.x * player->speed;
    // player->vel.z += player->acc.z * player->speed;
    // player->vel.x = CLAMP(player->vel.x, -1.f, 1.f);
    // player->vel.z = CLAMP(player->vel.z, -1.f, 1.f);

    Vec3 last_pos = player->pos;

    //Vec2 offset = vec2_mul_scl(unit, -speed * dt);

    if (player->grounded) {
        player->vel.x += player->acc.x * dt * player->acceleration;
    }

    float spd = player->vel.x;
    spd = CLAMP(spd, -MAX_SPEED, MAX_SPEED);
    Vec2 offset;
    offset.x = unit.y * spd * dt;
    offset.y = unit.x * spd * dt;

    player->pos.x += offset.x;
    player->pos.y += player->vel.y;
    player->pos.z += offset.y;

    // printf("%f %f %f %f\n", player->vel.x, player->vel.z, player->acc.x, player->acc.y);

    // player->pos.x += unit.x * player->vel.x * dt;
    // player->pos.y += player->vel.y;
    // player->pos.z += unit.z * player->vel.z * dt;

    float h = height_map_from_pos(player->pos.x, player->pos.z);

    if (player->pos.y - player->height < h) { // under ground
        player->pos.y = h + player->height;
        player->vel.y = h - (last_pos.y - player->height);
    }


    if (player->pos.y <= h + player->height + 4.f ) {
        player->grounded = 1;
    }
    else {
        player->grounded = 0;
    }
    // printf("on ground %i %f %f\n", player->grounded, player->pos.y, h + player->height);

    if (player->pos.y > 10000.f) {
        player->vel.y = 0.f;
        player->pos.y = 10000.f;
        // printf("wtf\n");
    }


    return;
#else



    float saved_y = player->pos.y;

    player->vel.y -= motocross.gravity * dt;
    player->pos.y += player->vel.y;


    printf("%f \n", player->vel.y);

    if (!player->grounded) {
#if 0
        player->vel.y -= motocross.gravity * dt;
        if (player->vel.y > 0.f)
            player->vel.y *= 0.98f * dt;
#endif
    }
    else {
    }


    float h = height_map_from_pos(player->pos.x, player->pos.z);
    
    player->height_last[player->height_i++ % 5] = player->pos.y;
    if (player->grounded) {
        float min = 10000.f;
        float max = 0.f;

        // u8 descending = 0;

        for (int i = 0; i < 5; i++) {
            // avg += player->height_last[i];
            max = MAX(max, player->height_last[i]);
            min = MIN(min, player->height_last[i]);
            
        }

        if (player->pos.y < max) {
          //  descending = 1;
        }
        else {
			printf("avg climb %f\n", max - min);
			float avg = max - min;
			if (avg >= 8.f) {

				for (int i = 1; i < 5; i++)
					player->height_last[i] = player->pos.y;

                float jump_factor = (max - (min - player->height)) * 0.05f;
				printf("Jack pot: %f , dt: %f, bignum: %f\n", jump_factor, dt,
                    JUMP_STR * jump_factor * dt);

			    player->vel.y = JUMP_STR * jump_factor * dt; // 
            }
        }
    }

    if (player->pos.y - player->height + 2.f > h) {
        // air ?
        player->grounded = 0;
    }

    if (player->pos.y - player->height < h) { // under ground

        float jump_factor = h - (player->pos.y - player->height);
        player->pos.y = h + player->height;

        // while (player->pos.y < h) {
        // }
        // printf("collide jump factor %f\n", jump_factor);

        if (player->dig_dirt) {
            jump_factor *= 0.1f;
        }

        if (jump_factor > JUMP_FACTOR) {
            player->vel.y += JUMP_STR * jump_factor * dt;
            player->grounded = 0;
        }
        else {
            player->grounded = 1;
            player->vel.y *= 0.5f * dt;
        }
    }
    else {
        // printf("no collide\n");
    }
    // printf("player %f : %f", player->pos.y, h);

   #endif
}

static void motocross_update(float dt) {

    MotorCycle* player = &motocross.player;
    float dir = 0.f;
    float rot = 0.f;
    float y = 0.f;
    float speed = player->speed;

    if (isKeyDown(Key_W)) {
        dir -= 1.0f;
    }
    if (isKeyDown(Key_S)) {
        dir += 1.0f;
    }

    if (isKeyDown(Key_A)) {
        rot += 0.025f;
    }
    if (isKeyDown(Key_D)) {
        rot -= 0.025f;
    }

    // debug
#if 0
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
#endif
    /////// debug

    // if (dir != 0.f || rot != 0.f) {

    if (player->grounded) {
        player->rot += rot * dt;
    }
    else {
        player->rot += rot * dt * AIR_ROT_CONTROL;
    }

        Vec2 unit = { cos(player->rot), sin(player->rot) };
        Vec2 offset = vec2_mul_scl(unit, dir * dt);

        player->acc.x = dir;
        if (dir == 0.f) {
            player->vel.x *= 0.99f;
        }

        // player->pos.x += offset.y;
        // player->pos.z += offset.x;

        // player->acc.x = unit.y * dir;
        // player->acc.z = unit.x * dir;
       // player->pos.y += y;
    // }

    motocross_physics(dt);

    if (isKeyDown(Key_Space)) {
        player->wheelie_acc += WHEELIE_ACC;
    }
    else {
        player->wheelie_acc -= WHEELIE_ACC;
    }

    player->wheelie_acc = CLAMP(player->wheelie_acc, -10.f, 10.f);
    player->wheelie_value += player->wheelie_acc * dt;
    player->wheelie_value = CLAMP(player->wheelie_value, 0.f, 500.f);

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

   

    static float last_rot     = 0.f;
    static float last_wheelie = 0.f;


    if (fabs(motocross.player.vel.x) > 0.0001f || fabs(motocross.player.vel.y) > 0.0001f
        || player->rot != last_rot || player->wheelie_value != last_wheelie) {
        motocross.rerender = 1;
    }
    else {
        motocross.rerender = 0;
    }
    last_rot = player->rot;
    last_wheelie = player->wheelie_value;
    if (motocross.rerender) {
        render_rot(p, player->rot, player->pos.y, 220 + player->wheelie_value, 
		    motocross.map_scale, 500, SCREEN_W, SCREEN_H);
    }

}

