#define DEBUG

#include <assert.h>
#include <stdarg.h>
#include <stddef.h> // size_t
#include <stdint.h>
#include <limits.h>
#include <stdio.h>

#include "file_offset.h"               // @TEMP
#include "asset_packer/asset_packer.h" // @TEMP

#define STB_IMAGE_IMPLEMENTATION
#include "image.cpp"

#include "definitions.h"

#include "unicode_table.cpp"

void application_entry();

#ifdef __linux__ 
#include <GL/gl.h>
#include <GL/glx.h>
#include "linux.cpp"

#elif _WIN32
#error "Windows version incomplete."

#elif __APPLE__
#error "Apple version incomplete."

#else
#error "Unsupported operating system."
#endif

// @TODO: Move this!
//
// Threads.
//
typedef void (*Do_Job_Callback)(void *);

struct Thread_Job {
	void *job_data = NULL;
	Do_Job_Callback do_job_callback;
};

Semaphore_Handle platform_make_semaphore(u32 initial_value);

struct Job_Queue {
	Thread_Job       jobs[MAX_JOBS];
	Semaphore_Handle semaphore          = platform_make_semaphore(0);
	volatile u32     read_head          = 0;
	volatile u32     write_head         = 0;
	volatile u32     num_jobs_completed = 0;
};

#include "math.cpp"
#include "memory.cpp"
#include "library.cpp"
#include "sound.cpp"

#include "assets.cpp"
#include "threads.cpp"

#include "opengl.cpp"

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

Color black{ 0.0f, 0.0f, 0.0f };
Color white{ 1.0f, 1.0f, 1.0f };
Color white_transparent{ 1.0f, 1.0f, 1.0f, 0.15f };
Color red  { 1.0f, 0.0f, 0.0f };
Color green{ 0.0f, 1.0f, 0.0f };
Color blue { 0.0f, 0.0f, 1.0f };
Color cyan { 0.0f, 1.0f, 1.0f };

f32 MAX_X_VELOCITY                  = 08.0f;
f32 GROUND_RUN_ACCELERATION         = 2.0f;
f32 AIR_RUN_ACCELERATION            = GROUND_RUN_ACCELERATION;
f32 GROUND_FRICTION                 = 5.0f;
f32 AIR_FRICTION                    = GROUND_FRICTION;
u32 PLAYER_JUMP_GRACE_MS            = 60;

f32 MAX_Y_VELOCITY                  = 15.0f;
f32 JUMP_ACCELERATION               = 15.0f;
f32 GRAVITY                         = 1.00f;
f32 LEDGE_GRAB_JUMP_ACCELERATION    = 25.0f;
f32 MAX_LEDGE_GRAB_DISTANCE         = 0.5f;
//f32 AIR_RUN_ACCELERATION            = GROUND_RUN_ACCELERATION * 0.65f;
//f32 GROUND_FRICTION                 = GROUND_RUN_ACCELERATION;
//f32 AIR_FRICTION                    = GROUND_RUN_ACCELERATION * 0.03125f;

f32 ROPE_EXTEND_RATE                     = 20.0f;
f32 ROPE_RETRACT_RATE                    = 26.0f;
f32 ROPE_MAX_EXTENT                      = 6.0f;
f32 ROPE_LENGTH_CHANGE_RATE              = 3.0f;
f32 ROPE_STRETCH_MODIFIER                = 1.0f;
f32 ROPE_CAST_POINT_DISTANCE_FROM_PLAYER = 0.2f;

f32 CAMERA_SPEED       = 14.0f;
f32 CAMERA_SMOOTH_TIME = 0.20f;

f32 TILE_SIDE_IN_METERS = 0;

bool debug_mode_bend_test = false;

f32 delta_time;

void
try_next_sprite_frame(Sprite_Instance *si)
{
	u32 current_time = platform_get_time_ms();

	if (si->current_frame == -1) {
		si->current_frame = 0;
		si->frame_start_time = current_time;
		return;
	}

	Sprite_Asset *ls = get_sprite(si->name);
	if (!ls) {
		return;
	}

	if (current_time >= si->frame_start_time + ls->frames[si->current_frame].duration || current_time < si->frame_start_time) {
		si->frame_start_time = si->frame_start_time + ls->frames[si->current_frame].duration;
		si->current_frame = (si->current_frame + 1) % ls->frames.size;
	}
}

void debug_draw_sweeps(Sweep *sweeps, u32 num_sweeps);
//void debug_draw_line(V2 p1, V2 p2, Color c = red, bool is_screen_space = false);

inline u32
time_spec_to_milliseconds(Time_Spec ts)
{
	return (ts.tv_sec * 1000) + round(ts.tv_nsec / 1.0e6);
}

Collider_Id
add_collider(f32 x, f32 y, f32 w, f32 h, Array<Rectangle> *colliders)
{
	Collider_Id id = colliders->size;
	printf("%f %f %f %f\n", x, y, w, h);
	colliders->push({x, y, w, h});
	return id;
}

Collider_Id
add_collider(Rectangle c, Array<Rectangle> *colliders)
{
	print_rectangle(c);
	return add_collider(c.x, c.y, c.w, c.h, colliders);
}

Collider_Id
add_sprite_collider(const char *sprite_name, Array<Rectangle> *colliders)
{
	Sprite_Asset *a = get_sprite(sprite_name);
	if (!a) {
		assert(0);
	}

	return add_collider(a->collider, colliders);
}

f32
intersect_line_segments(V2 oa, V2 ea, V2 ob, V2 eb)
{
	V2 r = ea - oa;
	V2 s = eb - ob;

	f32 numerator = cross((ob - oa), r);
	f32 denominator = cross(r, s);

	if (numerator == 0 && denominator == 0) {
		// The lines are co-linear.
		printf("The lines are co-linear.\n");
		return INFINITY;
	}
	if (denominator == 0) {
		// The lines are parallel.
		printf("The lines are parallel.\n");
		return INFINITY;
	}

	f32 u = numerator / denominator;
	f32 t = cross((ob - oa), s) / denominator;
	printf("t:%f u:%f\n", u, t);
	if ((t >= 0) && (t <= 1) && (u >= 0) && (u <= 1))
		return t;

	printf("The lines just didn't intersect.\n");
	return INFINITY;
}

Axis_Collision_Check_Result
check_collision_x(Rectangle minkowski_sum, f32 dx)
{
	// Test if if the move will put the origin inside the minkowski aabb.
	f32 minkowski_l = minkowski_sum.x;
	f32 minkowski_r = minkowski_sum.x + minkowski_sum.w;
	f32 minkowski_b = minkowski_sum.y;
	f32 minkowski_t = minkowski_sum.y + minkowski_sum.h;

	if (minkowski_l <= 0.0f && minkowski_r >= 0.0f && minkowski_b <= 0.0f && minkowski_t >= 0.0f) {
		return dx > 0.0f ? (Axis_Collision_Check_Result){ minkowski_l - COLLISION_GAP, RIGHT_COLLISION }
		                 : (Axis_Collision_Check_Result){ minkowski_r + COLLISION_GAP, LEFT_COLLISION };
	}

	return { 0.0f, NO_COLLISION };
}

Axis_Collision_Check_Result
check_collision_y(Rectangle minkowski_sum, f32 dy)
{
	// Test if if the move will put the origin inside the minkowski aabb.
	f32 minkowski_l = minkowski_sum.x;
	f32 minkowski_r = minkowski_sum.x + minkowski_sum.w;
	f32 minkowski_b = minkowski_sum.y;
	f32 minkowski_t = minkowski_sum.y + minkowski_sum.h;

	if (minkowski_l <= 0.0f && minkowski_r >= 0.0f && minkowski_b <= 0.0f && minkowski_t >= 0.0f) {
		return dy > 0.0f ? (Axis_Collision_Check_Result){ minkowski_b - COLLISION_GAP, TOP_COLLISION }
		                 : (Axis_Collision_Check_Result){ minkowski_t + COLLISION_GAP, BOTTOM_COLLISION };
	}

	return { 0.0f, NO_COLLISION };
}

bool
check_collision(Rectangle us, Array<Rectangle> colliders, Collider_Id exclude = NO_COLLIDER_ID)
{
	for (u32 i = 0; i < colliders.size; ++i) {
		if (exclude != NO_COLLIDER_ID && i == exclude)
			continue;

		Rectangle them = colliders[i];

		Rectangle minkowski_sum = { them.x - us.x - us.w,
		                            them.y - us.y - us.h,
		                            us.w + them.w,
		                            us.h + them.h };
		
		if (intersect_point_rectangle({ 0.0f, 0.0f }, minkowski_sum)) {
			return true;
		}
	}

	return false;
}

bool
check_collision(V2 point, Array<Rectangle> colliders, Collider_Id exclude = NO_COLLIDER_ID)
{
	for (u32 i = 0; i < colliders.size; ++i) {
		if (exclude != NO_COLLIDER_ID && i == exclude)
			continue;

		if (intersect_point_rectangle(point, colliders[i])) {
			return true;
		}
	}

	return false;
}

// @NOTE: This continually registers collisions even if the colliders are just touching without moving.
// Not sure if that is what we want.
Collision_Check_Result
check_collision(Collider_Id id, V2 delta_position, Array<Rectangle> colliders)
{
	V2 dp = delta_position;

	Collision_Check_Result result;
	result.delta_position        = dp;
	result.x_collision_direction = NO_COLLISION;
	result.y_collision_direction = NO_COLLISION;
	result.x_collisions          = make_array<Collision_Report>(256, 0); // @TEMP
	result.y_collisions          = make_array<Collision_Report>(256, 0); // @TEMP

	Rectangle us = colliders[id];

	for (u32 i = 0; i < colliders.size; ++i) {
		if (i == id)
			continue;
		Rectangle them = colliders[i];
		Rectangle minkowski_sum = { them.x - (us.x + dp.x) - us.w,
		                            them.y - us.y - us.h,
		                            us.w + them.w,
		                            us.h + them.h };
		auto x = check_collision_x(minkowski_sum, dp.x);
		if (x.direction != NO_COLLISION) {
			if (result.x_collision_direction == NO_COLLISION) {
				result.x_collision_direction = x.direction;
				result.delta_position.x += x.penetration;
			}
			result.x_collisions.push((Collision_Report){ them, x.penetration });
		}
	}
	for (u32 i = 0; i < colliders.size; ++i) {
		if (i == id)
			continue;
		Rectangle them = colliders[i];
		Rectangle minkowski_sum = { them.x - (us.x + result.delta_position.x) - us.w,
		                           them.y - (us.y + dp.y) - us.h,
		                           us.w + them.w,
		                           us.h + them.h };
		auto y = check_collision_y(minkowski_sum, dp.y);
		if (y.direction != NO_COLLISION) {
			if (result.y_collision_direction == NO_COLLISION) {
				result.y_collision_direction = y.direction;
				result.delta_position.y += y.penetration;
			}
			result.y_collisions.push((Collision_Report){ them, y.penetration });
		}
	}

	return result;
}

void
add_control_point(V2 new_control_point, u32 index, Rope *r)
{
	r->control_points.insert(new_control_point, index);
	++r->end;
	return;
}

void
remove_control_point(u32 index, Rope *r)
{
	r->control_points.remove(index);
	--r->end;
	return;
}

f32
rope_length(Rope *r)
{
	f32 len = 0;
	V2 prev_cp = r->control_points[0];

	for (u32 i = 1; i < r->control_points.size; ++i) {
		len += length(r->control_points[i] - prev_cp);
		prev_cp = r->control_points[i];
	}

	return len;
	
}

Check_Grounded_Result
check_grounded(V2 player_position, f32 player_width, Array<Rectangle> &colliders)
{
	Check_Grounded_Result result;
	f32 ray_length = 0.1f;
	V2 left_origin = { player_position.x, player_position.y }, left_end = { left_origin.x, left_origin.y - ray_length };
	V2 right_origin = { player_position.x + player_width, player_position.y }, right_end = { right_origin.x, right_origin.y - ray_length };
	for (const auto &c : colliders) {
		V2 top_origin = { c.x, c.y + c.h }, top_end = { c.x + c.w, c.y + c.h };
		bool l = intersect_line_segments(left_origin, left_end, top_origin, top_end) != INFINITY;
		bool r = intersect_line_segments(right_origin, right_end, top_origin, top_end) != INFINITY;
		if (l)
			result.left = true;
		if (r)
			result.right = true;
		if (l && r)
			return result;
	}
	return result;
}

V2
player_center(Player *p)
{
	return (V2){ p->collider.x + (p->collider.w / 2.0f), p->collider.y + (p->collider.h / 2.0f) };
}

f32
smoothstep(f32 edge0, f32 edge1, f32 x)
{
	f32 t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	f32 smoothed_t = ((t) * (t) * (t) * ((t) * ((t) * 6 - 15) + 10));
	return edge0 + (smoothed_t * (edge1 - edge0));
}

f32
smoothdamp(f32 previous, f32 target, f32 smooth_time, f32 *smooth_speed)
{
	f32 t1 = 0.36f * smooth_time;
	f32 t2 = 0.64f * smooth_time;
	f32 x = previous - target;
	f32 new_smooth_speed = (*smooth_speed) + delta_time * (-1.0f / (t1 * t2) * x - (t1 + t2) / (t1 * t2) * (*smooth_speed));
	f32 target_offset = x + delta_time * (*smooth_speed);
	*smooth_speed = new_smooth_speed;
	return target + target_offset;
}

V2
smoothdamp(V2 previous, V2 target, f32 smooth_time, f32 *x_smooth_speed, f32 *y_smooth_speed)
{
	f32 new_x = smoothdamp(previous.x, target.x, smooth_time, x_smooth_speed);
	f32 new_y = smoothdamp(previous.y, target.y, smooth_time, y_smooth_speed);

	return { new_x, new_y };
}

void
move_camera_towards(Camera *c, V2 target)
{
	//V2 target = (player_center(p));
	//V2 old_position = c->position;
	//V2 target = (p->physical_position + (V2) { 2.0f, 0.0f });
	//V2 target = player_center(p);

	//static f32 x_smooth_speed = 0.0f, y_smooth_speed = 0.0f;
	//c->position = smoothdamp(c->position, target, CAMERA_SMOOTH_TIME, &x_smooth_speed, &y_smooth_speed);

	c->position += 0.1f * (target - c->position);
	c->position = round_to_nearest_pixel(c->position);

	//c->position = player_pixel_center(p);

	c->view_vector = make_2d_view_vector(c->position);
}

void
move_camera_towards_player(Camera *c, Player *p)
{
	move_camera_towards(c, player_center(p));
}

void
move_camera(Camera *c, V2 dp)
{
	c->position += dp;
	c->view_vector = make_2d_view_vector(c->position);
}

void
set_camera_position(Camera *c, V2 p)
{
	c->position = p;
	c->view_vector = make_2d_view_vector(c->position);
}

void
player_set_world_position(V2 position, Array<Rectangle> colliders, Player *p)
{
	p->world_position = position;

	colliders[p->collider_id].x = position.x + get_sprite(p->sprite.name)->collider.x;
	colliders[p->collider_id].y = position.y + get_sprite(p->sprite.name)->collider.y;

	p->collider = colliders[p->collider_id];
}

void
player_set_world_position_and_update_camera(V2 position, Array<Rectangle> colliders, Camera *c, Player *p)
{
	player_set_world_position(position, colliders, p);

	move_camera_towards_player(c, p);
}

void
player_set_world_position_resolve_collisions_and_update_camera(V2 position, Array<Rectangle> colliders, Camera *c, Player *p)
{
	player_set_world_position(position, colliders, p);

	auto collision_check_result = check_collision(p->collider_id, { 0.0f, 0.0f }, colliders);

	V2 new_player_position = p->world_position + collision_check_result.delta_position;

	player_set_world_position_and_update_camera(new_player_position, colliders, c, p);
}

V2
player_clamp_acceleration(V2 accel, Player *p)
{
	if (p->velocity.x + accel.x > MAX_X_VELOCITY || p->velocity.x + accel.x < -MAX_X_VELOCITY) {
		accel.x = sign(p->velocity.x + accel.x) * MAX_X_VELOCITY - p->velocity.x;
	}

	if (p->velocity.y + accel.y > MAX_Y_VELOCITY || p->velocity.y + accel.y < -MAX_Y_VELOCITY) {
		accel.y = sign(p->velocity.y + accel.y) * MAX_Y_VELOCITY - p->velocity.y;
	}

	assert(p->velocity.x + accel.x <= MAX_X_VELOCITY && p->velocity.x + accel.x >= -MAX_X_VELOCITY);
	assert(p->velocity.y + accel.y <= MAX_Y_VELOCITY && p->velocity.y + accel.y >= -MAX_Y_VELOCITY);

	return accel;
}

//void debug_draw_rectangle(Rectangle r, Color c, f32 zlevel, bool is_screen_space, bool asdf = false);

Collision_Check_Result
player_move(V2 delta_position, s32 input_x, s32 input_y, Array<Rectangle> colliders, Player *p)
{
	auto collision_check_result = check_collision(p->collider_id, delta_position, colliders);

	V2 new_player_position = p->world_position + collision_check_result.delta_position;
	V2 old_player_position = p->world_position;

	bool was_grounded = p->grounded;
	p->grounded = false;

	if (collision_check_result.y_collision_direction == BOTTOM_COLLISION) {
		p->grounded = true;
	}

	if (was_grounded && !p->grounded) {
		timer_set(PLAYER_JUMP_GRACE_MS, &p->jump_grace_timer);
	}

	if (p->velocity.y < 0.0f && input_y != -1) {
		// @TEMP
		//Rectangle ledge_grab_check_rect = { new_player_position.x + ((p->facing == 1) ? p->sprite.data.frames[p->sprite.current_frame].w : -MAX_LEDGE_GRAB_DISTANCE),
		                                    //new_player_position.y + p->sprite.data.frames[p->sprite.current_frame].h,
		                                    //MAX_LEDGE_GRAB_DISTANCE,
		                                    //old_player_position.y - new_player_position.y };
		Rectangle ledge_grab_check_rect = { 0, 0, 0, 0 };

		//debug_draw_rectangle(ledge_grab_check_rect, green, 0.1f, false);

		// @TODO: Make this part of the regular collision checking system?
		for (u32 i = 0; i < colliders.size; ++i) {
			if (i == p->collider_id) {
				continue;
			}

			V2 ledge_corner = (p->facing == 1) ? (V2){ colliders[i].x, colliders[i].y + colliders[i].h }
			                                   : (V2){ colliders[i].x + colliders[i].w, colliders[i].y + colliders[i].h };

			// Check slightly above the ledge we want to grab onto to make sure it isn't blocked by another collider.
			// And check to make sure the corner actually falls inside of our ledge grab check rectangle.
			if (!check_collision((V2){ ledge_corner.x, ledge_corner.y + COLLISION_GAP }, colliders, p->collider_id)
		          && intersect_point_rectangle(ledge_corner, ledge_grab_check_rect)) {
				if (p->facing == 1) {
					new_player_position = { colliders[i].x - p->collider.w - COLLISION_GAP, colliders[i].y + colliders[i].h - p->collider.h };
				} else {
					new_player_position = { colliders[i].x + colliders[i].w + COLLISION_GAP, colliders[i].y + colliders[i].h - p->collider.h };
				}

				p->grabbing_ledge = true;
				p->grounded       = true;
				p->velocity.y     = 0.0f;

				break;
			}
		}
	}

	if (collision_check_result.x_collision_direction != NO_COLLISION) {
		p->velocity.x = 0.0f;
	}

	if (collision_check_result.y_collision_direction != NO_COLLISION) {
		p->velocity.y = 0.0f;
	}

	player_set_world_position(new_player_position, colliders, p);

	return collision_check_result;
}

void
player_clamp_and_apply_rope_acceleration(V2 accel, s32 input_x, s32 input_y, Array<Rectangle> colliders, Rope *r, Player *p)
{
	accel = player_clamp_acceleration(accel, p);

	p->velocity += accel;

	auto collision_check_result = player_move(delta_time * accel, input_x, input_y, colliders, p);

	auto attached_to_this_collider = [](V2 attach_point, Rectangle collider)
	{
		if ((attach_point.x >= collider.x - COLLISION_GAP)
		 && (attach_point.x <= collider.x + collider.w + COLLISION_GAP)
		 && (attach_point.y >= collider.y - COLLISION_GAP)
		 && (attach_point.y <= collider.y + collider.h + COLLISION_GAP)) {
		  	return true;
		}

		return false;
	};

	auto slide_player_perpendicular_to_collision = [&](f32 collider_x, f32 collider_penetration, u32 dimension_index)
	{
			s32 slide_dir = r->end == 1 ? sign(r->control_points[0][dimension_index] - collider_x) : sign(r->control_points[1][dimension_index] - collider_x);

			if (sign(accel[dimension_index]) != slide_dir) {
				p->velocity[dimension_index] -= accel[dimension_index];
			}

			V2 slide_delta_position = (sign(collider_penetration) == slide_dir) ? (V2){ collider_penetration, 0.0f } : (V2){ -collider_penetration, 0.0f };

			if (sign(collision_check_result.delta_position[dimension_index]) != slide_dir) {
				slide_delta_position[dimension_index] -= collision_check_result.delta_position[dimension_index];
			}

			p->velocity += player_clamp_acceleration({ 0.0f, GRAVITY }, p);
			slide_delta_position.y += delta_time * accel.y;

			//player_move(slide_delta_position, input_x, input_y, colliders, p);
	};

	if (input_y == -1) {
		if (collision_check_result.y_collision_direction != NO_COLLISION && fabs(accel.y) > fabs(accel.x)) {
			f32 cx = collision_check_result.y_collisions[0].collider.x;
			f32 cp = collision_check_result.y_collisions[0].penetration;

			slide_player_perpendicular_to_collision(cx, cp, 0);
		}

		if (collision_check_result.x_collision_direction != NO_COLLISION && fabs(accel.x) > fabs(accel.y)) {
			f32 cy = collision_check_result.x_collisions[0].collider.y;
			f32 cp = collision_check_result.x_collisions[0].penetration;

			slide_player_perpendicular_to_collision(cy, cp, 1);
		}
	}
}

void
player_clamp_and_apply_velocity(V2 accel, s32 input_x, s32 input_y, Array<Rectangle> colliders, Player *p)
{
	accel = player_clamp_acceleration(accel, p);

	p->velocity += accel;

	player_move(delta_time * p->velocity, input_x, input_y, colliders, p);
}

bool
sweep_rope(V2 sweep_anchor, V2 sweep_start, V2 sweep_end, V2 player_position, Collider_Id player_cid, Array<Rectangle> colliders, bool s, Rope *r)
{
	bool sweep_hit = false;

	V2  hit_corners[4];
	V2  first_hit_corner;
	u32 first_hit_collider_num_hit_corners = 0;
	f32 first_hit_distance                 = INFINITY;

	Sweep sweeps[256]; // @TEMP
	u32   num_sweeps = 0;

	V2 sweep_segment_start = sweep_start;

	if (r->end != 1) {
		// Remove bends.
		auto sign = [](V2 p, V2 l0, V2 l1)
		{
			return (p.x - l1.x) * (l0.y - l1.y) - (l0.x - l1.x) * (p.y - l1.y);
		};

		bool removed_bend;
		do {
			V2 sweep_control_point;
			V2 acp;
			V2 aacp;
			u32 acp_index;

			if (s) {
				sweep_control_point = r->control_points[0];
				acp                 = r->control_points[1];
				acp_index           = 1;
				aacp                = r->control_points[2];
			} else {
				sweep_control_point = r->control_points[r->end];
				acp                 = r->control_points[r->end - 1];
				acp_index           = r->end - 1;
				aacp                = r->control_points[r->end - 2];
			}

			removed_bend = false;

			f32 this_frame_half_plane = sign(sweep_control_point, acp, aacp);
			f32 last_frame_half_plane = sign(sweep_start, acp, aacp);

			bool bt = this_frame_half_plane < 0.0f;
			bool bl = last_frame_half_plane < 0.0f;

			if (this_frame_half_plane == 0.0f || bt != bl) {
				printf("Removing control point %u, this_frame_half_plane: %f last_frame_half_plane: %f\n", acp_index, this_frame_half_plane, last_frame_half_plane);
				V2 sweep_segment_anchor = acp;
				V2 b = acp - aacp;
				f32 i = intersect_ray_line_dangerous(b, aacp, sweep_segment_start, sweep_control_point);
				V2 sweep_segment_end = sweep_segment_start + i * (sweep_control_point - sweep_segment_start);
				sweeps[num_sweeps++] = { sweep_anchor, sweep_segment_start, sweep_segment_end };
				sweep_segment_start = sweep_segment_end;

				// Removal of the control point should happen after we calculate the sweep.
				remove_control_point(acp_index, r);
				removed_bend = true;
			}
		} while (removed_bend && r->end != 1);
	}

	sweeps[num_sweeps++] = { sweep_anchor, sweep_segment_start, sweep_end };

	debug_draw_sweeps(sweeps, num_sweeps);

	do {
		first_hit_collider_num_hit_corners = 0;
		first_hit_distance = INFINITY;

		for (u32 sweep_i = 0; sweep_i < num_sweeps; ++sweep_i) { 
			V2 v0 = sweeps[sweep_i].anchor, v1 = sweeps[sweep_i].start, v2 = sweeps[sweep_i].end;

			for (u32 col_i = 0; col_i < colliders.size; ++col_i) {
				if (col_i == player_cid)
					continue;

				auto c = colliders[col_i];

				// We only care about corner tiles. (Nothing above and below or left and right.)
				//if (check_collision(colliders, c.x - TILE_SIDE / 2.0f) || check_collision(colliders, c.x + c.w + TILE_SIDE / 2.0f)
				//&& check_collision(colliders, c.y - TILE_SIDE / 2.0f) || check_collision(colliders, c.y + c.h + TILE_SIDE / 2.0f))
				//	continue;

				u32 num_hit_corners = 0;
				V2 col_left_start = { c.x, c.y }, col_left_end = { col_left_start.x, col_left_start.y + c.h };
				V2 col_right_start = { c.x + c.w, c.y }, col_right_end = { col_right_start.x, col_right_start.y + c.h };

				if (intersect_point_triangle_ccw(col_left_start, v0, v1, v2)) {
					print_v2(col_left_start);
					printf("%f %f %f %f\n", c.x, c.y, c.w, c.h);
					hit_corners[num_hit_corners++] = col_left_start + (V2){ -COLLISION_GAP, -COLLISION_GAP };
				}
				if (intersect_point_triangle_ccw(col_left_end, v0, v1, v2)) {
					print_v2(col_left_end);
					printf("%f %f %f %f\n", c.x, c.y, c.w, c.h);
					hit_corners[num_hit_corners++] = col_left_end + (V2){ -COLLISION_GAP, COLLISION_GAP };
				}
				if (intersect_point_triangle_ccw(col_right_start, v0, v1, v2)) {
					print_v2(col_right_start);
					hit_corners[num_hit_corners++] = col_right_start + (V2){ COLLISION_GAP, -COLLISION_GAP };
				}
				if (intersect_point_triangle_ccw(col_right_end, v0, v1, v2)) {
					print_v2(col_right_end);
					hit_corners[num_hit_corners++] = col_right_end + (V2){ COLLISION_GAP, COLLISION_GAP };
				}

				assert(num_hit_corners >= 0 && num_hit_corners <= 4);

				for (u32 j = 0; j < num_hit_corners; ++j) {
					f32 t = intersect_ray_line_dangerous(hit_corners[j] - v0, v0, v1, v2);

					printf("Checking corner: index: %u dist: %.9g min_dist: %.9g x: %.9g y: %.9g\n", j, t, first_hit_distance, hit_corners[j].x, hit_corners[j].y);
					if (fabs(t - first_hit_distance) < 0.000001f) {
						f32 len1 = length2(player_position - first_hit_corner);
						f32 len2 = length2(player_position - hit_corners[j]);

						printf("LENGTHS %f %f\n", len1, len2);

						if (len2 < len1) {
							first_hit_corner   = hit_corners[j];
							first_hit_collider_num_hit_corners = num_hit_corners;
						}
					} else if (t < first_hit_distance) {
						first_hit_corner   = hit_corners[j];
						first_hit_distance = t;
						first_hit_collider_num_hit_corners = num_hit_corners;
					}
				}
			}

			if (first_hit_distance != INFINITY) {
				printf("Added:\n\t");
				print_v2(first_hit_corner);
				if (s) {
					add_control_point(first_hit_corner, 1, r);
				} else {
					add_control_point(first_hit_corner, r->end, r);
				}

				// Get rid of anchor??????
				sweeps[sweep_i].anchor = first_hit_corner;
				sweep_hit = true;
			}
		}
	} while(first_hit_collider_num_hit_corners > 1);

	return sweep_hit;
}

// @TODO: Get rid of end_moving?
void
add_and_remove_bends(Array<Rectangle> &colliders, V2 prev_rope_start_pos, V2 prev_rope_end_pos, V2 player_position, Collider_Id player_cid, Rope *r)
{
	if (r->extending) {
		// If the end is moving, we also want to sweep the triangle formed by its movement.
		// No point in sweeping the retraction because it's never going to collide with anything due to the levels being static.
		bool no_bends         = (r->end == 1);
		V2   end_sweep_anchor = no_bends ? prev_rope_start_pos : r->control_points[r->end - 1];
		V2   end_sweep_0      = prev_rope_end_pos;
		V2   end_sweep_1      = r->control_points[r->end];

		bool sweep_hit = sweep_rope(end_sweep_anchor, end_sweep_0, end_sweep_1, player_position, player_cid, colliders, false, r);
		if (sweep_hit && no_bends) {
			return;
		}
	}

	V2 start_sweep_anchor = r->control_points[1];
	V2 start_sweep_0      = prev_rope_start_pos;
	V2 start_sweep_1      = r->control_points[0];

	sweep_rope(start_sweep_anchor, start_sweep_0, start_sweep_1, player_position, player_cid, colliders, true, r);
}

void
rope_attach(V2 attach_point, Rope *r)
{
	r->control_points[r->end] = attach_point;
	r->was_attached           = true;
	r->extending              = false;
	r->retracting             = false;
	r->end_moving             = false;
	r->unstretched_length     = rope_length(r);
}

void
rope_update_start_position(Player *p, Rope *r)
{
	r->control_points[0] = player_center(p) + (ROPE_CAST_POINT_DISTANCE_FROM_PLAYER / length(r->control_points[1] - player_center(p))) * (r->control_points[1] - player_center(p));
}

// @TODO @BUG: Start jittering when trying but unable to move.

V2 player_start_position;

void
update(const Input &i, Game_State *game_state)
{
	s8 walk = 0;
	Player *p = &game_state->player;
	Rope   *r = &game_state->rope;

	if (i.keyboard.keys[platform_keysym_to_scancode(R_KEY)].pressed) {
		player_set_world_position_resolve_collisions_and_update_camera(player_start_position, game_state->colliders, &game_state->camera, p);
		p->velocity = { 0.0f, 0.0f };
	}

	auto calc_x_accel_due_to_running = [](const Player *p, Rope *r, s32 input_dir)
	{
		f32 accel;

		if (p->grounded) {
			if (p->velocity.x == 0.0f || sign(p->velocity.x) != input_dir) {
				// We are moving from a standstill or input direction is the opposite of the direction we are currently travelling.
				// We want to juice the acceleration in order to make things feel a bit more snappy.
				accel = 2.0f * GROUND_RUN_ACCELERATION;
			} else {
				// Input direction is the same as the direction we are currently travelling in.
				// Accelerate as normal.
				accel = GROUND_RUN_ACCELERATION;
			}
		} else {
			if (r->out && r->was_attached && !r->was_released) {
				accel = 0.65f;
			} else {
				accel = AIR_RUN_ACCELERATION;
			}
		}

		return accel;
	};

	s32 input_x             = 0;
	s32 input_y             = 0;
	s32 input_jump_down     = 0;
	s32 input_jump_pressed  = 0;
	s32 input_jump_released = 0;

	if (i.keyboard.keys[platform_keysym_to_scancode(J_KEY)].down) {
		input_jump_down = 1;
	}
	if (i.keyboard.keys[platform_keysym_to_scancode(J_KEY)].pressed) {
		input_jump_pressed = 1;
	}
	if (i.keyboard.keys[platform_keysym_to_scancode(J_KEY)].released) {
		input_jump_released = 1;
	}

	if (i.keyboard.keys[platform_keysym_to_scancode(W_KEY)].down) {
		input_y = 1;
	} else if (i.keyboard.keys[platform_keysym_to_scancode(S_KEY)].down) {
		input_y = -1;
	}

	if (i.keyboard.keys[platform_keysym_to_scancode(D_KEY)].down) {
		input_x = 1;
	} else if (i.keyboard.keys[platform_keysym_to_scancode(A_KEY)].down) {
		input_x = -1;
	}

	V2 accel = { 0.0f, 0.0f };

	if (p->grabbing_ledge) {
		if (input_y == -1) {
			p->grabbing_ledge = false;
		} else if (input_jump_pressed) {
			p->grabbing_ledge = false;
			p->velocity.y += LEDGE_GRAB_JUMP_ACCELERATION;
		}
	} else {
		if (input_x) {
			accel.x = calc_x_accel_due_to_running(p, r, input_x) * input_x;
			p->facing = input_x;
		} else {
			// Apply friction.
			if (r->out && r->was_attached && !r->was_released) {
				accel.x = fmin(fabs(p->velocity.x), p->grounded ? GROUND_FRICTION : 0.8f * 0.03125f) * -sign(p->velocity.x);
			} else {
				accel.x = fmin(fabs(p->velocity.x), p->grounded ? GROUND_FRICTION : AIR_FRICTION) * -sign(p->velocity.x);
			}
		}

		walk = input_x;

		if ((p->grounded || timer_check_one_shot(&p->jump_grace_timer) == false) && input_jump_pressed) {
			accel.y += JUMP_ACCELERATION;
		}
		accel.y -= GRAVITY;
	}

	player_clamp_and_apply_velocity(accel, input_x, input_y, game_state->colliders, p);

	// @TODO: What order do we apply the rope/movement operations in? Use last frame's position as the start position of the
	// rope, which prevents us from starting the rope in the wall, but it's kind of stale. Use next frame's position in rope force
	// calculations. Should the first frame with the rope out start with it at length zero and then extend? Or start with some length?

	if (i.keyboard.keys[platform_keysym_to_scancode(K_KEY)].pressed) {
		if (!r->out) {
			if (input_y) {
				if (input_x) {
					r->cast_direction = (V2){ 1.0f * input_x, 1.0f };
					r->cast_point     = (V2){ (input_x == 1) ? p->collider.w : 0.0f, (input_y == 1) ? p->collider.h : 0.0f };
				} else {
					r->cast_direction = (V2){ 0.0f, 1.0f * input_y };
					r->cast_point     = (V2){ p->collider.w / 2.0f, (input_y == 1) ? p->collider.h : 0.0f };
				}
			} else {
				r->cast_direction = (V2){ 1.0f * p->facing, 0.0f };
				r->cast_point     = (V2){ (p->facing == 1) ? p->collider.w : 0.0f, p->collider.h / 2.0f };
			}

			r->cast_point += (V2) { ROPE_CAST_POINT_DISTANCE_FROM_PLAYER * input_x, ROPE_CAST_POINT_DISTANCE_FROM_PLAYER * input_y };

			r->out          = true;
			r->extending    = true;
			r->retracting   = false;
			r->was_attached = false;
			r->was_released = false;
			r->end_moving   = true;

			V2 start_position = (V2){ p->collider.x + r->cast_point.x, p->collider.y + r->cast_point.y };
			r->control_points.resize(0);
			r->control_points.push(start_position); // Start.
			r->control_points.push(start_position); // End.
			r->end = 1;
		}
	} else if (i.keyboard.keys[platform_keysym_to_scancode(K_KEY)].released) {
		r->end_moving   = true;
		r->retracting   = true;
		r->extending    = false;
		r->was_released = true;
	}

	// @TODO: Change cast point based on bend.
	// @TODO: Fix grounding when attached above or below.

	V2 old_rope_start_position;
	V2 old_rope_end_position;

	if (r->out) {
		// Apply extension and retraction.
		old_rope_start_position = r->control_points[0];
		old_rope_end_position   = r->control_points[r->end];

		rope_update_start_position(p, r);

		if (r->extending) {
			V2  new_end    = r->control_points[r->end] + delta_time * ROPE_EXTEND_RATE * r->cast_direction;
			f32 new_length = rope_length(r);
			if (new_length >= ROPE_MAX_EXTENT) {
				//r->end = ROPE_MAX_EXTENT; @TODO
				r->extending  = false;
				r->retracting = true;
			} else {
				r->control_points[r->end] = new_end;
			}
		} else if (r->retracting) {
			r->control_points[r->end] -= delta_time * ROPE_RETRACT_RATE * normalize(r->control_points[r->end] - r->control_points[r->end - 1]);
			// Check if we're fully retracted.
			if (intersect_point_rectangle(r->control_points[r->end], p->world_position.x, p->world_position.y, p->collider.w, p->collider.h)) {
				// Fully retracted.
				r->out = false;
			} else {
				// Reached another control point while retracting.
				f32 t = intersect_point_line(r->control_points[r->end - 1], old_rope_end_position, r->control_points[r->end]);
				if (t != INFINITY) {
					if (r->end - 1 == 0) {
						// Control point was the start, so we are fully retracted and finished for now.
						r->out = false;
					} else {
						if (!r->was_released && !r->was_attached) {
							// If we haven't hit anything yet and we hit a control point, we want to attach the end.
							V2 attach_point = r->control_points[r->end - 1];
							rope_attach(attach_point, r);
						} else {
							// Adjust the end's position so it goes around the bend.
							r->control_points[r->end] = r->control_points[r->end - 1] - ((1.0f - t) * delta_time * ROPE_RETRACT_RATE * normalize(r->control_points[r->end] - r->control_points[r->end - 2]));
						}
						remove_control_point(r->end - 1, r);
					}
				}
			}
		}

		// Should happen after the attach test?
		if (!debug_mode_bend_test)
			add_and_remove_bends(game_state->colliders, old_rope_start_position, old_rope_end_position, p->world_position, p->collider_id, r);

		//if (changed_bends) // Update the cast point.
			//r->start->data.position = player_center(p) + CAST_POINT_DISTANCE_FROM_PLAYER * normalize(r->start->next->data.position - player_center(p));
	}

	// Recheck to make sure we haven't fully retracted.
	if (r->out) {
		// Check for end hits.
		if (!r->was_attached && !r->was_released)
		{
			if (r->cast_direction.x != 0.0f) {
				print_v2(r->control_points[r->end]);
				for (u32 i = 0; i < game_state->colliders.size; ++i) {
					if (i == p->collider_id)
						continue;
					auto c  = game_state -> colliders[i];
					V2   ls = r->cast_direction.x > 0.0f ? (V2){ c.x, c.y } : (V2){ c.x + c.w, c.y };
					V2   le = { ls.x, ls.y + c.h };
					f32  t  = intersect_line_segments(old_rope_end_position, r->control_points[r->end], ls, le);
					printf("%f\n", t);
					if (t != INFINITY) {
						// Offset the attach point a bit to create a gap and prevent the rope from creating spurious bends.
						V2 gap          = r->cast_direction.x > 0.0f ? (V2){ -COLLISION_GAP, 0 } : (V2){ COLLISION_GAP, 0 };
						V2 attach_point = old_rope_end_position + t * (r->control_points[r->end] - old_rope_end_position) + gap;
						print_v2(attach_point);
						rope_attach(attach_point, r);
					}
				}
			}
			if (r->cast_direction.y != 0.0f) {
				for (u32 i = 0; i < game_state->colliders.size; ++i) {
					if (i == p->collider_id)
						continue;
					auto c  = game_state -> colliders[i];
					V2   ls = r->cast_direction.y > 0.0f ? (V2){ c.x, c.y } : (V2){ c.x, c.y + c.h };
					V2   le = { ls.x + c.w, ls.y };
					f32  t  = intersect_line_segments(old_rope_end_position, r->control_points[r->end], ls, le);
					if (t != INFINITY) {
						// Offset the attach point a bit to create a gap and prevent the rope from creating spurious bends.
						V2 gap          = r->cast_direction.y > 0.0f ? (V2){0, -COLLISION_GAP} : (V2){0, COLLISION_GAP};
						V2 attach_point = old_rope_end_position + t * (r->control_points[r->end] - old_rope_end_position) + gap;
						print_v2(attach_point);
						rope_attach(attach_point, r);
					}
				}
			}
		}

		// @TODO: Simulate player walking against stretch.

		bool currently_attached = r->was_attached && !r->retracting;

		// Apply rope forces.
		if (currently_attached) {
			if (input_y) {
				r->unstretched_length += delta_time * ROPE_LENGTH_CHANGE_RATE * input_y;
			}

			f32 stretched_length = rope_length(r);
			f32 stretch = (stretched_length - r->unstretched_length) * ROPE_STRETCH_MODIFIER;

			if (stretch > 0.0f) {
				V2 accel_due_to_stretch = { 0.0f, 0.0f };
				V2 first = r->control_points[1] - r->control_points[0];

				accel_due_to_stretch.x = cos(first, { first.x, 0.0f }) * stretch;
				accel_due_to_stretch.y = sin(first, { first.x, 0.0f }) * stretch;

				if (p->velocity.y > 0.0f) {
					//p->velocity.y += stretch * 0.2f; // ???
				}

				if (p->velocity.x < 0.0f) {
					//accel_due_to_rope.x -= (stretched_rope_length - r.length) * 0.7f; // ???
				}

				//accel_due_to_rope.x = clamp(accel_due_to_rope.x, 6.0f);

/*
				if (walk && p->grounded && sign(p->velocity.x + accel_due_to_rope.x) != sign(p->velocity.x)) {
					p->velocity.x = 0.0f;
				} else {
					p->velocity += accel_due_to_rope;
				}
*/
				player_clamp_and_apply_rope_acceleration(accel_due_to_stretch, input_x, input_y, game_state->colliders, r, p);
			}

		}
	}

	move_camera_towards_player(&game_state->camera, p);
}

void add_tile(Asset_Id sprite, V2 p, Array<Tile> *tiles, Array<Rectangle> *colliders);

V2
set_level_collision_test(Array<Tile> *tiles, Array<Rectangle> *colliders)
{
	f32 tile_side = 36.0f * meters_per_pixel; // @TEMP: Need a better way to figure this out.

	s32 level_width = 30, level_height = 10;
	for (s32 i = 0; i < level_width + 1; ++i) {
		//add_tile(STONE_SPRITE, {i * tile_side, 0}, tiles, colliders);
		//add_tile(STONE_SPRITE, {i * tile_side, level_height * tile_side}, tiles, colliders);
	}
	for (s32 i = 1; i < level_height; ++i) {
		//add_tile(STONE_SPRITE, {0, i * tile_side}, tiles, colliders);
		//add_tile(STONE_SPRITE, {level_width * tile_side, i * tile_side}, tiles, colliders);
	}

	return { 1.0f, 2.0f };
}

V2
set_level_rope_test_one(Array<Tile> *tiles, Array<Rectangle> *colliders)
{
	f32 horizontal_gap_width = TILE_SIDE_IN_METERS * 5;
	f32 vertical_gap_width = TILE_SIDE_IN_METERS * 3;

	f32 x = -TILE_SIDE_IN_METERS * 20, y = 0;
	for (s32 i = 0; i < 20; ++i) {
		//add_tile(3, { x, 0 }, tiles, colliders);
		x += TILE_SIDE_IN_METERS;
	}

	for (s32 i = 0; i < 5; ++i) {
		//add_tile(3, { x, 0 }, tiles, colliders);
		x += TILE_SIDE_IN_METERS;
	}

	//add_tile(STONE_SPRITE, { TILE_SIDE_IN_METERS, TILE_SIDE_IN_METERS }, tiles, colliders);
	//add_tile(STONE_SPRITE, { TILE_SIDE_IN_METERS * 5, TILE_SIDE_IN_METERS }, tiles, colliders);

	x += horizontal_gap_width;
	y += vertical_gap_width;

	for (s32 i = 0; i < 5; ++i) {
		//add_tile(3, { x, y }, tiles, colliders);
		y += TILE_SIDE_IN_METERS;
	}

	for (s32 i = 0; i < 5; ++i) {
		//add_tile(3, { x, 0 }, tiles, colliders);
		x += TILE_SIDE_IN_METERS;
	}
	
	return { 000.0f, 0.0f };
}

f32
align_to_tile_grid(f32 world_position_dim)
{
	f32 result = world_position_dim - (fmod(world_position_dim, TILE_SIDE_IN_METERS));

	return result;
}

V2
align_to_tile_grid(V2 world_position)
{
	world_position.x = align_to_tile_grid(world_position.x);
	world_position.y = align_to_tile_grid(world_position.y);

	return world_position;
}

void
move_tile(V2 dp, u32 tile_index, Game_State *gs)
{
	Tile *     t = &gs->tiles[tile_index];
	Rectangle *c = &gs->colliders[t->collider_id];

	t->world_position.x = (t->world_position.x + dp.x);
	t->world_position.y = (t->world_position.y + dp.y);

	c->x = t->world_position.x + get_sprite(t->sprite.name)->collider.x;
	c->y = t->world_position.y + get_sprite(t->sprite.name)->collider.y;
}

void
add_tile(const char *sprite_name, V2 p, Array<Tile> *tiles, Array<Rectangle> *colliders)
{
	tiles->push((Tile){ make_sprite_instance(sprite_name, false), p, add_collider(p.x, p.y, TILE_SIDE_IN_METERS, TILE_SIDE_IN_METERS, colliders) });
	//Rectangle collider = assets.associated_data[id].sprite_collider;
	//if (collider != NO_COLLIDER)
		//add_collider(tile.x + collider.x, tile.y + collider.y, collider.w, collider.h, colliders);
	//(*tiles)[tiles->size - 1].collider_id ;
}

// @TODO: The memory for the tiles and the tile colliders should probably live in the same place. Also move the player out of the colliders.
u32
remove_tile(Array<Tile> *tiles, Array<Rectangle> *colliders, u32 index)
{
	array_remove(colliders, (*tiles)[index].collider_id);
	array_remove(tiles, index);

	// @TODO: Figure out a proper data structure for the tiles that will allow us to remove without having to mess with the index!
	return index - 1;
}

void
hide_tile(Array<Tile> *tiles, u32 index)
{
	(*tiles)[index].flags |= HIDE_TILE_FLAG;
}

void
unhide_tile(Array<Tile> *tiles, u32 index)
{
	(*tiles)[index].flags &= ~HIDE_TILE_FLAG;
}

#include "editor.cpp"

void
application_entry()
{
	assert(ROPE_EXTEND_RATE  > MAX_X_VELOCITY && ROPE_EXTEND_RATE  > MAX_Y_VELOCITY);
	assert(ROPE_RETRACT_RATE > MAX_X_VELOCITY && ROPE_RETRACT_RATE > MAX_Y_VELOCITY);

	TILE_SIDE_IN_METERS = 16.0f * meters_per_pixel; // @TEMP: Need a better way to figure this out.

	// Load font.
	{
		FT_Library ft;
		if (FT_Init_FreeType(&ft))
			_abort("FREETYPE: Could not init FreeType Library.");

		FT_Face face;
		if (FT_New_Face(ft, "../data/fonts/arial.ttf", 0, &face))
			_abort("FREETYPE: Failed to load font.");

		FT_Set_Pixel_Sizes(face, 0, 30);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

		for (GLubyte c = 0; c < 128; c++)
		{
			// Load character glyph 
			if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
				log_print(MAJOR_ERROR_LOG, "FREETYTPE: Failed to load glyph '%c'.", c);
				continue;
			}
			// Generate texture
			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_RED,
					face->glyph->bitmap.width,
					face->glyph->bitmap.rows,
					0,
					GL_RED,
					GL_UNSIGNED_BYTE,
					face->glyph->bitmap.buffer
				    );
			// Set texture options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			// Now store character for later use
			Glyph character = {
				texture, 
				{ face->glyph->bitmap.width, face->glyph->bitmap.rows },
				{ face->glyph->bitmap_left, face->glyph->bitmap_top },
				face->glyph->advance.x
			};
			characters[c] = character;
		}
	}

	Game_State game_state;
	{
		game_state.colliders = make_array<Rectangle>(2056, 0); // @TEMP
		game_state.tiles     = make_array<Tile>(2056, 0);      // @TEMP

		game_state.player.sprite         = make_sprite_instance("player_run");
		game_state.player.world_position = { 0.0f, 0.0f };
		game_state.player.facing         = 1;
		game_state.player.velocity       = { 0.0f, 0.0f };
		game_state.player.collider_id    = add_sprite_collider("player_run", &game_state.colliders);
		game_state.player.collider       = game_state.colliders[game_state.player.collider_id];
		game_state.player.grounded       = false;
		game_state.player.grabbing_ledge = false;

		game_state.camera.view_vector = { 0.0f, 0.0f };
		game_state.camera.position    = { 0.0f, 0.0f };

		game_state.rope.unstretched_length = 0.0f;
		game_state.rope.extending          = false;
		game_state.rope.retracting         = false;
		game_state.rope.out                = false;
		game_state.rope.was_attached       = false;
		game_state.rope.end_moving         = false;
		game_state.rope.control_points     = make_array<V2>(256, 0); // @TEMP
	}

	//player_start_position = set_level_rope_test_one(&game_state.tiles, &game_state.colliders);
	load_level(&game_state.tiles, &game_state.colliders);
	player_set_world_position_resolve_collisions_and_update_camera(player_start_position, game_state.colliders, &game_state.camera, &game_state.player);

	set_camera_position(&game_state.camera, player_center(&game_state.player));

	// @TODO: Use real monitor refresh rate.
	f32 game_update_hz = 60.0f;
	f32 target_seconds_per_frame = 1.0f / game_update_hz;

	Input input;
	platform_update_mouse_position(&input.mouse);
	Program_State state = PROGRAM_STATE_RUNNING;
	delta_time = target_seconds_per_frame;

	//debug_set_game_speed(0.02f);

	// @TODO: Figure out what to do if vsync is not enabled!

	while(state != PROGRAM_STATE_EXITING) {
		state = platform_handle_events(&input, state);

		if (state == PROGRAM_STATE_EXITING) {
			break;
		}

		if (key_pressed(input.keyboard, P_KEY)) {
			static u32 temp = 0;
			if (state == PROGRAM_STATE_RUNNING) {
				state = PROGRAM_STATE_PAUSED;
				temp = platform_get_time_ms() - game_state.player.sprite.frame_start_time;
				printf("%u\n", temp);
			} else {
				state = PROGRAM_STATE_RUNNING;
				game_state.player.sprite.frame_start_time = platform_get_time_ms() - temp;
			}
		}

		if (state == PROGRAM_STATE_RUNNING) {
			update(input, &game_state);

			try_next_sprite_frame(&game_state.player.sprite);
		}

		state = debug_update(input, &game_state);

		add_sprite_render_commands(game_state.player.sprite, game_state.player.world_position, game_state.camera.view_vector);

		for (auto t : game_state.tiles) {
			if (!(t.flags & HIDE_TILE_FLAG)) {
				add_sprite_render_commands(t.sprite, t.world_position, game_state.camera.view_vector);
			}
		}

		submit_render_commands_and_swap_backbuffer();
	}

	platform_exit(EXIT_SUCCESS);
}

