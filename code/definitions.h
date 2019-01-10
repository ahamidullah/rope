#ifndef __CORE_H__
#define __CORE_H__

//#include "asset_packer/asset_packer.h"
//#include "file_offset.h"
//#include <alloca.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef float    f32;
typedef double   f64;

enum Program_State {
	PROGRAM_STATE_RUNNING,
	PROGRAM_STATE_PAUSED,
	PROGRAM_STATE_EXITING,
	NUM_PROGRAM_STATES
};

//
// Library.
//
enum Log_Type {
	STANDARD_LOG,
	MINOR_ERROR_LOG,
	MAJOR_ERROR_LOG,
	CRITICAL_ERROR_LOG
};

#define INVALID_CODE_PATH assert(!"Invalid code path.");

void debug_print(const char *fmt, ...);
void debug_print(const char *fmt, va_list args);

#define _abort(fmt, ...) _abort_actual(__FILE__, __LINE__, __func__, fmt, ## __VA_ARGS__)
#define log_print(log_type, fmt, ...) log_print_actual(log_type, __FILE__, __LINE__, __func__, fmt, ## __VA_ARGS__)

#define ARRAY_COUNT(a) (sizeof(a)/sizeof(a[0]))

#define KILOBYTE(b) ((size_t)b*1024)
#define MEGABYTE(b) (KILOBYTE(b)*1024)
#define GIGABYTE(b) (MEGABYTE(b)*1024)

template <typename F>
struct Scope_Exit {
	Scope_Exit(F _f) : f(_f) {}
	~Scope_Exit() { f(); }
	F f;
};

template <typename F>
Scope_Exit<F>
make_scope_exit(F f)
{
	return Scope_Exit<F>(f);
}

#define DO_STRING_JOIN(arg1, arg2) arg1 ## arg2
#define STRING_JOIN(arg1, arg2) DO_STRING_JOIN(arg1, arg2)
#define DEFER(code) auto STRING_JOIN(scope_exit_, __LINE__) = make_scope_exit([=](){code;})

void log_print_actual(Log_Type log_type, const char *file, int line, const char *func, const char *fmt, ...);
void _abort_actual(const char *file, int line, const char *func, const char *fmt, ...);

#if 0
#define TIMED_BLOCK(name) Block_Timer __block_timer__##__LINE__(#name)
#define MAX_TIMER_NAME_LEN 256

struct Block_Timer {
	Block_Timer(const char *n)
	{
		size_t name_len = strlen(n);
		assert(name_len < MAX_TIMER_NAME_LEN);
		strcpy(name, n);
		start = platform_get_time();
	}
	~Block_Timer()
	{
		Time_Spec end = platform_get_time();
		unsigned ns_res=1, us_res=1000, ms_res=1000000;
		long ns = platform_time_diff(start, end, ns_res);
		long ms = platform_time_diff(start, end, ms_res);
		long us = platform_time_diff(start, end, us_res);
		debug_print("%s - %dns %dus %dms\n", name, ns, us, ms);
	}
	Time_Spec start;
	char name[MAX_TIMER_NAME_LEN];
};
#endif

template <typename T>
struct Array {
	T &
	operator[](size_t i)
	{
		assert(i < size);
		return data[i];
	}
	void
	resize(size_t new_size)
	{
		size = new_size;
		if (size > capacity) {
			capacity = size * 2;
			data = (T *)realloc(data, sizeof(T) * capacity);
		}
		assert(size <= capacity);
	}
	void
	push(T it)
	{
		data[size++] = it;
		assert(size <= capacity);
	}
	void
	insert(T new_element, u32 index)
	{
		assert(size + 1 < capacity);
		assert(index >= 0 && index <= size);

		T previous = data[index];
		for (u32 i = index + 1; i < size; ++i) {
			T copy = data[i];
			data[i] = previous;
			previous = copy;
		}

		size += 1;
		data[size - 1] = previous;
		data[index] = new_element;
	}
	void
	remove(u32 index)
	{
		assert(index >= 0 && index < size);

		for (u32 i = index + 1; i < size; ++i) {
			data[i - 1] = data[i];
		}

		size -= 1;
	}
	T &
	last()
	{
		assert(size != 0);
		return data[size - 1];
	}
	void
	clear()
	{
		size = 0;
	}
	T *
	begin()
	{
		return &data[0];
	}
	T *
	end()
	{
		return &data[size];
	}
	T *data;
	size_t capacity;
	size_t size;
};

template <typename T>
struct Static_Array {
	T &
	operator[](size_t i)
	{
		assert(i < size);
		return data[i];
	}
	T *
	begin()
	{
		return &data[0];
	}
	T *
	end()
	{
		return &data[size];
	}
	T *data;
	size_t capacity;
	size_t size;
};

struct String {
	const char *data;
	size_t length;

	String() {
		data = NULL;
		length = 0;
	}
	String(const char *t) {
		data = t;
		length = strlen(t);
	}
	String(const char *t, size_t l) {
		data = t;
		length = l;
	}
	bool
	operator==(const String &other)
	{
		if (other.length == (size_t)-1)
			return this->length == (size_t)-1;
		if (other.length != this->length)
			return false;
		for (size_t i = 0; i < other.length; ++i) {
			if (other.data[i] != this->data[i])
				return false;
		}
		return true;
	}
	bool
	operator!=(const String &other)
	{
		return !(*this == other);
	}
	const char *
	begin()
	{
		return &data[0];
	}
	const char *
	end()
	{
		return &data[length];
	}
};

#define STRING_ERROR String{NULL, (size_t)-1}

struct Rectangle {
	f32 x, y, w, h;
	//const Rectangle &operator=(Rectangle);
};

#define NO_RECTANGLE ((Rectangle){ NAN, NAN, NAN, NAN })

struct Rectangle_s32 {
	s32 x, y, w, h;
	const Rectangle &operator=(Rectangle);
};

inline bool
operator==(Rectangle a, Rectangle b)
{
	return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

inline bool
operator!=(Rectangle a, Rectangle b)
{
	return !(a==b);
}

struct Color {
	Color() : r{0}, g{0}, b{0}, a{1.0f} {}
	Color(f32 r, f32 g, f32 b) : r{r}, g{g}, b{b}, a{1.0f} {}
	Color(f32 r, f32 g, f32 b, f32 a) : r{r}, g{g}, b{b}, a{a} {}
	Color(s32 r, s32 g, s32 b, s32 a) : r{r/255.0f}, g{g/255.0f}, b{b/255.0f}, a{a/255.0f} {}
	f32 r, g, b, a;
};

struct Timer {
	u32 wait_time  = 0;
	u32 start_time = 0;
};

struct Memory_Arena;

String read_entire_file(const char *path, Memory_Arena *ma);

//
// Math.
//

template <typename T>
struct Extent2D {
	T width, height;
};

struct V2s {
	s32 x, y;
};

inline V2s
operator-(V2s a, V2s b)
{
	return {a.x - b.x, a.y - b.y};
}

struct V2u {
	u32 x, y;
};

struct V2 {
	f32 x, y;
	inline V2 &operator*=(float a);
	inline V2 &operator+=(V2 v);
	inline V2 &operator-=(V2 v);
	inline float operator[](int i) const;
	inline float &operator[](int i);
};

inline Rectangle
operator+(Rectangle r, V2 v)
{
	return (Rectangle) { r.x + v.x, r.y + v.y, r.w, r.h };
}

inline V2
operator*(f32 s, V2s v)
{
	return {s * v.x, s * v.y };
}

inline f32 V2::
operator[](int i) const {
	assert(i >= 0 && i <= 1);
	if (i == 0) return x;
	return y;
}

inline float &V2::
operator[](int i) {
	assert(i >= 0 && i <= 1);
	if (i == 0) return x;
	return y;
}

#define print_v2(v) print_v2_actual(v, #v)
#define print_rectangle(v) print_rectangle_actual(v, #v)

void
print_v2_actual(V2 v, const char *name)
{
	printf("%s: %.9g %.9g\n", name, v.x, v.y);
}

void
print_rectangle_actual(Rectangle r, const char *name)
{
	printf("%s: %.9g %.9g %.9g %.9g\n", name, r.x, r.y, r.w, r.h);
}

inline V2
operator*(float s, V2 v)
{
	return {v.x * s, v.y * s};
}

inline V2
operator+(V2 a, V2 b)
{
	return {a.x + b.x, a.y + b.y};
}

inline V2
operator-(V2 a, V2 b)
{
	return {a.x - b.x, a.y - b.y};
}

inline V2
operator-(V2 a)
{
	return {-a.x, -a.y};
}

inline bool
operator==(V2 a, V2 b)
{
	return a.x == b.x && a.y == b.y;
}

inline bool
operator!=(V2 a, V2 b)
{
	return !(a == b);
}

inline V2& V2::
operator*=(float a)
{
	*this = a * *this;
	return *this;
}

inline V2& V2::
operator+=(V2 v)
{
	*this = v + *this;
	return *this;
}

inline V2& V2::
operator-=(V2 v)
{
	*this = *this - v;
	return *this;
}

struct V3 {
	float x, y, z;
	//V3() : x{0}, y{0}, z{0} {}
	//V3(float x, float y, float z) : x{x}, y{y}, z{z} {}
	inline V3 &operator+=(V3 v);
	inline V3 &operator-=(V3 v);
	inline V3 &operator*=(f32 s);
	inline float operator[](int i) const;
	inline float &operator[](int i);
};

inline float V3::
operator[](int i) const {
	assert(i >= 0 && i <= 2);
	if (i == 0) return x;
	if (i == 1) return y;
	return z;
}

inline float &V3::
operator[](int i) {
	assert(i >= 0 && i <= 2);
	if (i == 0) return x;
	if (i == 1) return y;
	return z;
}

inline V3
operator*(float s, V3 v)
{
	return { v.x * s, v.y * s, v.z * s };
}

inline V3
operator*(V3 v, float s)
{
	return { v.x * s, v.y * s, v.z * s };
}

inline V3
operator/(V3 v, float divisor)
{
	assert(divisor != 0);
	return { v.x / divisor, v.y / divisor, v.z / divisor };
}

inline V3
operator-(V3 v)
{
	return -1.0f * v;
}

inline V3
operator-(V3 a, V3 b)
{
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline V3
operator+(V3 a, V3 b)
{
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline V3& V3::
operator-=(V3 v)
{
	*this = *this - v;
	return *this;
}

inline V3& V3::
operator*=(f32 s)
{
	*this = s * *this;
	return *this;
}

inline V3& V3::
operator+=(V3 v)
{
	*this = *this + v;
	return *this;
}

struct V4 {
	float x, y, z, w;
	inline V4 &operator/=(float v);
	inline V4 &operator*=(float v);
	inline float operator[](int i) const;
	inline float &operator[](int i);
};

inline float V4::
operator[](int i) const {
	assert(i >= 0 && i <= 3);
	if (i == 0) return x;
	if (i == 1) return y;
	if (i == 2) return z;
	return w;
}

inline float &V4::
operator[](int i) {
	assert(i >= 0 && i <= 3);
	if (i == 0) return x;
	if (i == 1) return y;
	if (i == 2) return z;
	return w;
}

inline V4
operator/(V4 v, float s)
{
	return {v.x / s, v.y / s, v.z / s, v.w / s};
}

inline V4
operator*(V4 v, float s)
{
	return {v.x * s, v.y * s, v.z * s, v.w * s};
}

inline V4
operator+(V4 a, V4 b)
{
	return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

inline V4 &V4::
operator/=(float v)
{
	*this = *this / v;
	return *this;
}

inline V4 &V4::
operator*=(float v)
{
	*this = *this * v;
	return *this;
}

inline V4
operator*(float s, V4 v)
{
	return { v.x * s, v.y * s, v.z * s, v.w * s };
}


struct M4 {
	M4() : M4(1.0f, 0.0f, 0.0f, 0.0f,
	          0.0f, 1.0f, 0.0f, 0.0f,
	          0.0f, 0.0f, 1.0f, 0.0f,
	          0.0f, 0.0f, 0.0f, 1.0f) {}
	M4(float n[4][4])
	{
		memcpy(m, n, sizeof(m));
	}
	M4(float t00, float t01, float t02, float t03,
	   float t10, float t11, float t12, float t13,
	   float t20, float t21, float t22, float t23,
	   float t30, float t31, float t32, float t33)
	{
		m[0][0] = t00;
		m[0][1] = t01;
		m[0][2] = t02;
		m[0][3] = t03;
		m[1][0] = t10;
		m[1][1] = t11;
		m[1][2] = t12;
		m[1][3] = t13;
		m[2][0] = t20;
		m[2][1] = t21;
		m[2][2] = t22;
		m[2][3] = t23;
		m[3][0] = t30;
		m[3][1] = t31;
		m[3][2] = t32;
		m[3][3] = t33;
	}
	V4 &
	operator[](int i)
	{
		return *(V4 *)m[i];
	}
	float &operator()(int row, int col) { return m[col-1][row-1]; }

	float m[4][4];
};

M4
operator*(M4 a, M4 b)
{
	M4 r;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			r.m[i][j] = a.m[0][j] * b.m[i][0]
			          + a.m[1][j] * b.m[i][1]
			          + a.m[2][j] * b.m[i][2]
			          + a.m[3][j] * b.m[i][3];
		}
	}
	return r;
/*
	M4 res;
	for (int i = 1; i <= 4; ++i) {
		float a1 = a(i,1), a2 = a(i,2), a3 = a(i,3), a4 = a(i,4);
		res(i,1) = a1*b(1,1) + a2*b(2,1) + a3*b(3,1) + a4*b(4,1);
		res(i,2) = a1*b(1,2) + a2*b(2,2) + a3*b(3,2) + a4*b(4,2);
		res(i,3) = a1*b(1,3) + a2*b(2,3) + a3*b(3,3) + a4*b(4,3);
		res(i,4) = a1*b(1,4) + a2*b(2,4) + a3*b(3,4) + a4*b(4,4);
	}
	return res;
	*/
}

M4
operator*(float s, M4 m)
{
	return M4{s * m.m[0][0], s * m.m[1][0], s * m.m[2][0], s * m.m[3][0],
	          s * m.m[0][1], s * m.m[1][1], s * m.m[2][1], s * m.m[3][1],
	          s * m.m[0][2], s * m.m[1][2], s * m.m[2][2], s * m.m[3][2],
	          s * m.m[0][3], s * m.m[1][3], s * m.m[2][3], s * m.m[3][3]};
}

inline V4
operator*(M4 m, V4 v)
{
	V4 result;
	result.x = v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0]  + v.w*m[3][0];
	result.y = v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1]  + v.w*m[3][1];
	result.z = v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2]  + v.w*m[3][2];
	result.w = v.x*m[0][3] + v.y*m[1][3] + v.z*m[2][3]  + v.w*m[3][3];
	return result;
}

M4 make_view_matrix(V2 position);

//
// Input.
//
struct IO_Button {
	s8 down;
	u32 pressed;
	u32 released;
};

struct Mouse {
	V2 position       = { 0.0f, 0.0f };
	V2 delta_position = { 0.0f, 0.0f };
	IO_Button buttons[5];
};

#define MAX_KEYS_PRESSED_IN_ONE_UPDATE 1024
#define MAX_SCANCODES 256
enum Key_Symbol : int;

// @TODO: Clean up input!
// Get rid of the text editor stuff.
// Stop clearing so much.
// Get rid of the num presses stuff.
struct Keyboard {
	IO_Button keys[MAX_SCANCODES];
	Key_Symbol keys_pressed_since_last_pull[MAX_KEYS_PRESSED_IN_ONE_UPDATE];
	size_t num_keys_pressed_since_last_pull;
};

void _memset(void *destination, int set_to, size_t count);

struct Input {
	Input() {
		memset(&mouse, 0, sizeof(mouse));
		memset(&keyboard, 0, sizeof(keyboard));
	};
	Mouse mouse;
	Keyboard keyboard;
};

enum Mouse_Button : int;

void input_key_down(Keyboard *kb, unsigned scancode);
void input_key_up(Keyboard *kb, unsigned scancode);
void input_mbutton_down(Mouse_Button button, Mouse *m);
void input_mbutton_up(Mouse_Button button, Mouse *m);

//
// Memory.
//
struct Chunk_Footer {
	char *base;
	char *block_frontier;
	Chunk_Footer *next;
};

struct Block_Footer {
	size_t capacity;
	size_t nbytes_used;
	Block_Footer *next;
	Block_Footer *prev;
};

struct Arena_Address {
	char *byte_addr;
	Block_Footer *block_footer;
};

struct Free_Entry {
	size_t size;
	Free_Entry *next;
};

// Points to the start of the next and prev entry data.
struct Entry_Header {
	size_t size;
	char *next;
	char *prev;
};

struct Memory_Arena {
	Free_Entry *entry_free_head;
	char *last_entry;
	Block_Footer *base;
	Block_Footer *active_block;
};

#define mem_alloc(type, arena) (type *)mem_push(sizeof(type), arena)
#define mem_alloc_array(type, count, arena) (type *)mem_push_contiguous(sizeof(type)*count, arena)

void *mem_push_contiguous(size_t size, Memory_Arena *ma);
Memory_Arena mem_make_arena();
void mem_destroy_arena(const Memory_Arena *ma);

#define MAX_JOBS 256
#define NUM_JOB_THREADS 4

//
// Game.
//
struct Camera {
	V2 view_vector;
	V2 position;
};

enum Asset_Load_Status {
	ASSET_UNLOADED = 0,
	ASSET_LOAD_IN_PROGRESS,
	ASSET_LOADED
};

struct Sprite_Frame {
	Rectangle texture_scissor;
	V2        meter_offset;
	f32       meter_width;
	f32       meter_height;
	s32       duration;
};

#define TEXTURE_DOES_NOT_EXIST 0

typedef u32 Gpu_Texture_Handle;

struct Loaded_Texture {
	Gpu_Texture_Handle gpu_handle;
};

struct Loaded_Sound {
};

struct Loaded_Font {
};

struct Loaded_Sprite {
	u32                 num_frames;
	char *              texture_name;
	Array<Sprite_Frame> frames;
	Rectangle           collider;
};

struct Texture_Asset {
	//u32 pixel_width;
	//u32 pixel_height;

	Gpu_Texture_Handle gpu_handle;
};

struct Sound_Asset {
};

struct Font_Asset {
};

struct Sprite_Asset {
	Rectangle           collider;
	char *              texture_name;
	Array<Sprite_Frame> frames;
};

#define SPRITE_INSTANCE_NAME_BUFFER_LENGTH 32

struct Sprite_Instance {
	u32      frame_start_time = 0;
	s32      current_frame    = -1;
	Asset_Id sprite_asset_id  = ASSET_DOES_NOT_EXIST;
	char name[SPRITE_INSTANCE_NAME_BUFFER_LENGTH];
};

struct Asset_Data {
	union {
		Loaded_Texture texture;
		Loaded_Sprite  sprite;
		Loaded_Sound   sound;
		Loaded_Font    font;
	};

	Asset_Load_Status load_status;
	File_Offset       file_offset;
	Asset_Id          dependency;
};

struct Associated_Asset_Data {
	union {
		Rectangle sprite_collider;
	};
};

struct Assets {
	u32 count;

	Asset_Tags *tags;

	Asset_Data *lookup;

	Associated_Asset_Data *associated_data;
};

struct Gpu_Make_Texture_Job {
	Gpu_Texture_Handle *output_gpu_texture_handle;
	Asset_Load_Status *asset_load_status;
	u32 gl_tex_unit;
	s32 texture_format;
	s32 pixel_format;
	s32 pixel_width;
	s32 pixel_height;
	u8 *pixels;
};

enum Load_Asset_Job_Type {
	LOAD_ASE,
	LOAD_SPRITE,
	LOAD_TEXTURE,
};

struct Load_Asset_Job {
	union {
		struct {
			const char *path;
		} ase;
		struct {
			char path[128];
			char base_name[128];
		} texture;
		struct {
			char sprite_path[128];
			char collider_path[128];
			char base_name[128];
		} sprite;
	};
	Load_Asset_Job_Type type;
};

typedef void *(*Thread_Procedure)(void *);

typedef u32 Collider_Id;
#define NO_COLLIDER_ID ((u32)-1)
#define NO_COLLIDER    ((Rectangle){ NAN, NAN, NAN, NAN })

struct Rope {
	V2        cast_direction;
	V2        cast_point; // @TODO: Get rid of this.
	f32       unstretched_length;
	bool      out;
	bool      extending;
	bool      retracting;
	bool      was_attached;
	bool      was_released;
	bool      end_moving;
	Array<V2> control_points;
	u32       end;
};

struct Player {
	Sprite_Instance      sprite;
	Rectangle   collider;
	Collider_Id collider_id;
	s32         facing;
	V2          world_position;
	V2          velocity;
	s8          grounded;
	s8          grabbing_ledge;
	Timer       jump_grace_timer;
};

#define HIDE_TILE_FLAG 0x1

struct Tile {
	Sprite_Instance sprite;
	V2              world_position;
	Collider_Id     collider_id;
	u32             flags;
};

struct Game_State {
	Array<Rectangle> colliders;
	Array<Tile>      tiles;
	Player           player;
	Rope             rope;
	Camera           camera;
};

struct Check_Grounded_Result {
	bool left = false;
	bool right = false;
};

enum Collision_Direction {
	LEFT_COLLISION,
	BOTTOM_COLLISION,
	RIGHT_COLLISION,
	TOP_COLLISION,
	NO_COLLISION
};

struct Axis_Collision_Check_Result {
	f32 penetration;
	Collision_Direction direction;
};

struct Collision_Report {
	Rectangle collider;
	f32 penetration;
};

struct Collision_Check_Result {
	V2 delta_position;
	Collision_Direction x_collision_direction;
	Collision_Direction y_collision_direction;
	Array<Collision_Report> x_collisions;
	Array<Collision_Report> y_collisions;
};

struct Sweep {
	V2 anchor;
	V2 start;
	V2 end;
};
 
#define COLLISION_GAP .0001f

#endif
