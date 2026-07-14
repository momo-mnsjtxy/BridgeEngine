#include "BridgeEngine.h"
#include <stdio.h>

#define MY_PI 3.14159265358979323846

static float my_sinf(float x)
{
	x = x - (int)(x / (2 * MY_PI)) * 2 * MY_PI;
	if (x < 0) x += 2 * MY_PI;
	float term = x;
	float result = x;
	for (int i = 1; i <= 7; i++) {
		term *= -x * x / ((2 * i) * (2 * i + 1));
		result += term;
	}
	return result;
}

static float my_cosf(float x)
{
	return my_sinf(x + MY_PI / 2);
}

static float my_fabsf(float x)
{
	return x < 0 ? -x : x;
}

static float ball_x = 400.0f;
static float ball_y = 300.0f;
static float ball_vx = 3.0f;
static float ball_vy = 2.5f;
static const float ball_radius = 20.0f;

static float polygon_angle = 0.0f;

static float pulse_phase = 0.0f;

static float wave_phase = 0.0f;

#define MAX_PARTICLES 60

typedef struct {
	float x, y;
	float vx, vy;
	float life;
	float max_life;
	bapi_color_t color;
} particle_t;

static particle_t particles[MAX_PARTICLES];
static int particle_count = 0;
static float particle_timer = 0.0f;

static void spawn_particle(float x, float y)
{
	if (particle_count >= MAX_PARTICLES)
		return;
	particle_t *p = &particles[particle_count++];
	p->x = x;
	p->y = y;
	p->vx = (my_sinf(x * 0.1f + wave_phase) * 2.0f);
	p->vy = -1.0f - (float)(particle_count % 3);
	p->max_life = 40.0f + (float)(particle_count % 20);
	p->life = p->max_life;
	uint8_t r = 200 + (uint8_t)(particle_count * 7 % 55);
	uint8_t g = 100 + (uint8_t)(particle_count * 13 % 100);
	uint8_t b = 30 + (uint8_t)(particle_count * 3 % 60);
	p->color = bapi_color(r, g, b, 255);
}

static void update_particles(void)
{
	for (int i = 0; i < particle_count; i++) {
		particle_t *p = &particles[i];
		p->x += p->vx;
		p->y += p->vy;
		p->vy += 0.05f;
		p->life -= 1.0f;
		if (p->life <= 0) {
			particles[i] = particles[particle_count - 1];
			particle_count--;
			i--;
		}
	}
}

static void draw_particles(void)
{
	for (int i = 0; i < particle_count; i++) {
		particle_t *p = &particles[i];
		float alpha = p->life / p->max_life;
		uint8_t a = (uint8_t)(alpha * 255);
		bapi_color_t c = bapi_color(p->color.r, p->color.g, p->color.b, a);
		float size = 3.0f + alpha * 5.0f;
		bapi_fill_circle(p->x, p->y, size, c);
	}
}

static void draw_rotated_polygon(float cx, float cy, float radius, int sides,
				 float angle, bapi_color_t color)
{
	for (int i = 0; i < sides; i++) {
		float a1 = angle + (2 * MY_PI * i) / sides;
		float a2 = angle + (2 * MY_PI * (i + 1)) / sides;
		float x1 = cx + radius * my_cosf(a1);
		float y1 = cy + radius * my_sinf(a1);
		float x2 = cx + radius * my_cosf(a2);
		float y2 = cy + radius * my_sinf(a2);
		bapi_draw_line(x1, y1, x2, y2, color);
	}
}

int main(int argc, char **argv, char **envp)
{
	(void)argc;
	(void)argv;
	(void)envp;

	if (bapi_engine_init("BridgeEngine XJ380 Demo", 800, 600) != 0) {
		return 1;
	}

	bapi_text_init();

	int running = 1;
	uint32_t frame_count = 0;
	uint32_t last_fps_time = bapi_get_ticks();
	int fps = 0;
	uint32_t tick = 0;

	while (running) {
		tick++;

		bapi_event_t event;
		while (bapi_poll_event(&event)) {
			int type = bapi_event_get_type(&event);
			if (type == BAPI_EVENT_QUIT) {
				running = 0;
			} else if (type == BAPI_EVENT_KEY_DOWN) {
				uint8_t key = bapi_event_get_key_code(&event);

			}
		}

		ball_x += ball_vx;
		ball_y += ball_vy;
		if (ball_x - ball_radius < 0 || ball_x + ball_radius > 800)
			ball_vx = -ball_vx;
		if (ball_y - ball_radius < 0 || ball_y + ball_radius > 600)
			ball_vy = -ball_vy;
		if (ball_x < ball_radius) ball_x = ball_radius;
		if (ball_x > 800 - ball_radius) ball_x = 800 - ball_radius;
		if (ball_y < ball_radius) ball_y = ball_radius;
		if (ball_y > 600 - ball_radius) ball_y = 600 - ball_radius;

		polygon_angle += 0.03f;
		pulse_phase += 0.05f;
		wave_phase += 0.08f;

		particle_timer += 1.0f;
		if (particle_timer >= 3.0f) {
			particle_timer = 0;
			spawn_particle(ball_x, ball_y);
		}
		update_particles();

		{
			uint8_t bg_b = 55 + (uint8_t)(my_sinf(pulse_phase * 0.3f) * 8);
			bapi_set_render_color(bapi_color(15, 20, bg_b, 255));
			bapi_render_clear();
		}

		{
			const char *title = "BridgeEngine XJ380 Demo";
			int len = 0;
			while (title[len]) len++;
			float start_x = 200.0f;
			for (int i = 0; i < len; i++) {
				float y_off = my_sinf(wave_phase + i * 0.4f) * 8.0f;
				uint8_t r = (uint8_t)(128 + 127 * my_sinf(wave_phase * 0.5f + i * 0.3f));
				uint8_t g = (uint8_t)(128 + 127 * my_sinf(wave_phase * 0.5f + i * 0.3f + 2.0f));
				uint8_t b = (uint8_t)(128 + 127 * my_sinf(wave_phase * 0.5f + i * 0.3f + 4.0f));
				char ch[2] = { title[i], '\0' };
				bapi_draw_text(ch, start_x + i * 16, 30 + y_off, 24,
					       bapi_color(r, g, b, 255));
			}
		}

		{
			struct {
				float x, y, w, h;
				uint8_t r, g, b;
			} rects[] = {
				{ 50, 80, 150, 100, 220, 50, 50 },
				{ 230, 80, 150, 100, 50, 180, 50 },
				{ 410, 80, 150, 100, 50, 100, 220 },
				{ 590, 80, 150, 100, 220, 180, 50 },
			};
			for (int i = 0; i < 4; i++) {
				float p = my_sinf(pulse_phase + i * 0.8f) * 5.0f;
				float rx = rects[i].x - p / 2;
				float ry = rects[i].y - p / 2;
				float rw = rects[i].w + p;
				float rh = rects[i].h + p;
				bapi_fill_rect(rx, ry, rw, rh,
					       bapi_color(rects[i].r, rects[i].g, rects[i].b, 255));
			}
		}

		bapi_draw_text("Red", 95, 115, 16, bapi_color(255, 255, 255, 255));
		bapi_draw_text("Green", 270, 115, 16, bapi_color(255, 255, 255, 255));
		bapi_draw_text("Blue", 455, 115, 16, bapi_color(255, 255, 255, 255));
		bapi_draw_text("Yellow", 630, 115, 16, bapi_color(0, 0, 0, 255));

		{
			for (int x = 50; x < 750; x += 4) {
				float t = (float)(x - 50) / 700.0f;
				uint8_t r = (uint8_t)(255 * my_fabsf(my_sinf(wave_phase + t * 3)));
				uint8_t g = (uint8_t)(255 * my_fabsf(my_sinf(wave_phase + t * 3 + 2)));
				uint8_t b = (uint8_t)(255 * my_fabsf(my_sinf(wave_phase + t * 3 + 4)));
				bapi_draw_line((float)x, 220, (float)(x + 4), 220,
					       bapi_color(r, g, b, 255));
			}
		}

		draw_rotated_polygon(200, 320, 60, 6, polygon_angle,
				     bapi_color(100, 200, 255, 255));
		draw_rotated_polygon(200, 320, 35, 3, -polygon_angle * 1.5f,
				     bapi_color(150, 230, 255, 200));

		{
			float r = 60 + my_sinf(pulse_phase * 1.2f) * 10;
			bapi_fill_circle(400, 320, r, bapi_color(200, 100, 255, 255));
			bapi_draw_circle(400, 320, r + 8 + my_sinf(pulse_phase) * 4,
					 bapi_color(230, 150, 255, 180));
		}

		{
			float star_angle = -polygon_angle * 0.7f;
			for (int i = 0; i < 10; i++) {
				float a1 = star_angle + MY_PI * i / 5;
				float a2 = star_angle + MY_PI * (i + 1) / 5;
				float r1 = (i % 2 == 0) ? 60 : 25;
				float r2 = ((i + 1) % 2 == 0) ? 60 : 25;
				float x1 = 600 + r1 * my_cosf(a1);
				float y1 = 320 + r1 * my_sinf(a1);
				float x2 = 600 + r2 * my_cosf(a2);
				float y2 = 320 + r2 * my_sinf(a2);
				bapi_draw_line(x1, y1, x2, y2,
					       bapi_color(255, 200, 100, 255));
			}
		}

		bapi_draw_text("Hexagon + Triangle", 110, 400, 14,
			       bapi_color(180, 180, 180, 255));
		bapi_draw_text("Pulse Circle", 345, 400, 14,
			       bapi_color(180, 180, 180, 255));
		bapi_draw_text("Rotating Star", 555, 400, 14,
			       bapi_color(180, 180, 180, 255));

		{
			for (int i = 3; i >= 1; i--) {
				float tx = ball_x - ball_vx * i * 2;
				float ty = ball_y - ball_vy * i * 2;
				uint8_t a = (uint8_t)(80 - i * 20);
				float tr = ball_radius - i * 3;
				if (tr > 0) {
					bapi_fill_circle(tx, ty, tr,
							 bapi_color(255, 180, 50, a));
				}
			}
			bapi_fill_circle(ball_x, ball_y, ball_radius,
					 bapi_color(255, 200, 50, 255));
			bapi_fill_circle(ball_x - 5, ball_y - 5, 6,
					 bapi_color(255, 255, 200, 200));
		}

		draw_particles();

		{
			float progress = (my_sinf(pulse_phase * 0.5f) + 1.0f) / 2.0f;
			float bar_w = 300.0f * progress;
			bapi_fill_rect(250, 460, 300, 12,
				       bapi_color(40, 40, 60, 255));
			for (int x = 0; x < (int)bar_w; x += 3) {
				float t = (float)x / 300.0f;
				uint8_t r = (uint8_t)(50 + 200 * t);
				uint8_t g = (uint8_t)(200 - 100 * t);
				uint8_t b = 50;
				bapi_fill_rect(250 + x, 460, 3, 12,
					       bapi_color(r, g, b, 255));
			}
		}

		bapi_draw_text("Hello BridgeEngine!", 300, 500, 16,
			       bapi_color(150, 150, 150, 255));

		{
			char fps_text[64];
			snprintf(fps_text, sizeof(fps_text), "FPS: %d  Tick: %u",
				 fps, tick);
			bapi_draw_text(fps_text, 600, 560, 14,
				       bapi_color(100, 100, 100, 255));
		}

		bapi_render_present();

		frame_count++;
		uint32_t now = bapi_get_ticks();
		if (now - last_fps_time >= 1000) {
			fps = (int)frame_count;
			frame_count = 0;
			last_fps_time = now;
		}

		bapi_delay(16);
	}

	bapi_text_cleanup();
	bapi_engine_quit();

	return 0;
}
