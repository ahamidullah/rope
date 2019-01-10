GLuint debug_vao;
GLuint debug_vbo;
GLuint text_vao;
GLuint text_vbo;
GLuint debug_shader;
GLuint debug_text_shader;

u8 debug_draw = true;
u8 draw_grid = false;

enum Editor_Mode {
	LEVEL_EDITOR,
	COLLISION_EDITOR,
	NUM_EDITOR_MODES
};

Color random_colors[256]; // @TEMP

Editor_Mode current_editor_mode = LEVEL_EDITOR;

u8 show_editor    = false;
u8 show_colliders = false;

struct Debug_Render_Command {
	u32         num_verts        = 0;
	u32         gl_mode          = GL_TRIANGLES;
	u8          screen_space     = true;
	const char *texture_name     = NULL;
	f32         line_thickness   = 1.0f;
	f32         alpha            = 1.0f;
};

constexpr GLuint DEBUG_MAX_RENDER_COMMANDS = 512;
constexpr GLuint DEBUG_MAX_RENDER_VERTS = DEBUG_MAX_RENDER_COMMANDS*4;

struct Debug_Render_Commands {
	Debug_Render_Command data[DEBUG_MAX_RENDER_COMMANDS];
	size_t count;
	GLuint num_total_verts;
} debug_render_commands;

const char *debug_vertex_shader_source = R"(
	uniform	mat4  zoomed_orthographic_projection;
	uniform	mat4  orthographic_projection;
	uniform vec2  view_vector;
	uniform bool screen_space;

	layout (location = 0) in vec2 vert_position;
	layout (location = 1) in vec4 vert_color;
	layout (location = 2) in vec2 vert_uv;

	out vec4 frag_color;
	out vec2 frag_uv;

	void main()
	{
		vec2 p = vert_position;
		if (!screen_space) {
			p += view_vector;
			gl_Position = zoomed_orthographic_projection * vec4(p, 0.0f, 1.0f);
		} else {
			gl_Position = orthographic_projection * vec4(p, 0.0f, 1.0f);
		}
		frag_color = vert_color;
		frag_uv = vert_uv;
	}
)";

const char *debug_fragment_shader_source = R"(
	in vec4 frag_color;
	in vec2 frag_uv;

	out vec4 color;

	uniform bool has_texture;
	uniform sampler2D utexture;
	uniform float alpha;

	void main()
	{
		if (has_texture) {
			color = texture(utexture, frag_uv);
		} else {
			color = frag_color;
		}

		color.w = alpha;
	}
)";

struct Debug_Vertex {
	V2    position;
	Color color;
	V2    uv;
};

struct Glyph {
	GLuint texture_id; // ID handle of the glyph texture
	V2u    size;       // Size of glyph
	V2s    bearing;    // Offset from baseline to left/top of glyph
	FT_Pos advance;    // Offset to advance to next glyph
};

Glyph characters[128];

const char *text_vertex_shader_source = R"(
	layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
	out vec2 TexCoords;

	uniform mat4 projection;

	void main()
	{
	    gl_Position = vec4(vertex.xy, 0.0, 1.0);
	    TexCoords = vertex.zw;
	}
)";

const char *text_fragment_shader_source = R"(
	in vec2 TexCoords;
	out vec4 color;

	uniform sampler2D text;
	uniform vec3 textColor;

	void main()
	{    
	    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
	    //color = vec4(textColor, 1.0) * sampled;
	    color = vec4(0.0, 0.0, 0.0, 1.0);
	}  
)";

V3
make_v3(V2 v, f32 z)
{
	return { v.x, v.y, z };
}

void
debug_init()
{
	for (u32 i = 0; i < 256; ++i)
		random_colors[i] = random_color();

	glGenVertexArrays(1, &debug_vao);
	glBindVertexArray(debug_vao);
	glGenBuffers(1, &debug_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
	glBufferData(GL_ARRAY_BUFFER, DEBUG_MAX_RENDER_VERTS * sizeof(Debug_Vertex), NULL, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Debug_Vertex), (GLvoid *)offsetof(Debug_Vertex, position));
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Debug_Vertex), (GLvoid *)offsetof(Debug_Vertex, color));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Debug_Vertex), (GLvoid *)offsetof(Debug_Vertex, uv));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	debug_shader = make_shader(String{debug_vertex_shader_source, 0}, String{debug_fragment_shader_source, 0});
	glUseProgram(debug_shader);
	glUniformMatrix4fv(glGetUniformLocation(debug_shader, "orthographic_projection"), 1, false, (GLfloat *)&orthographic_projection);
	glUniformMatrix4fv(glGetUniformLocation(debug_shader, "zoomed_orthographic_projection"), 1, false, (GLfloat *)&orthographic_projection);

	debug_text_shader = make_shader(String{text_vertex_shader_source, 0}, String{text_fragment_shader_source, 0});
	glGenVertexArrays(1, &text_vao);
	glGenBuffers(1, &text_vbo);
	glBindVertexArray(text_vao);
	glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(debug_text_shader);
	M4 m = make_orthographic_projection(0, window_pixel_width, 0, window_pixel_height);
	glUniformMatrix4fv(glGetUniformLocation(debug_text_shader, "projection"), 1, false, (GLfloat *)&m);
	glBindVertexArray(0);
}

s64
push_debug_vertices_and_command(Debug_Vertex *verts, u32 num_verts, u32 gl_mode, bool screen_space, Debug_Render_Command rc)
{
	glUseProgram(debug_shader);
	glBindVertexArray(debug_vao);
	glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, debug_render_commands.num_total_verts * sizeof(Debug_Vertex), num_verts * sizeof(Debug_Vertex), verts);

	u32 start_offset = debug_render_commands.num_total_verts * sizeof(Debug_Vertex);
	debug_render_commands.num_total_verts += num_verts;
	assert(debug_render_commands.num_total_verts < DEBUG_MAX_RENDER_VERTS);
	assert(debug_render_commands.count < DEBUG_MAX_RENDER_COMMANDS);

	rc.num_verts    = num_verts;
	rc.gl_mode      = gl_mode;
	rc.screen_space = screen_space;

	debug_render_commands.data[debug_render_commands.count] = rc;
	++debug_render_commands.count;

	return start_offset;
}

Debug_Vertex
make_debug_vertex(V2 position, Color c)
{
	return (Debug_Vertex){ position, c, { 0.0f, 0.0f } };
}

Debug_Vertex
make_debug_vertex(f32 position_x, f32 position_y, Color c)
{
	V2 p = { position_x, position_y };
	return (Debug_Vertex){ p, c, { 0.f, 0.f } };
}

Debug_Vertex
make_debug_vertex(f32 position_x, f32 position_y, V2 uv)
{
	V2 p = { position_x, position_y };
	return (Debug_Vertex){ p, black, uv };
}

void
debug_draw_triangle(V2 v1, V2 v2, V2 v3, Color c, bool screen_space, V2 view_vector)
{
	v1 = round_to_nearest_pixel(v1) + view_vector;
	v2 = round_to_nearest_pixel(v2) + view_vector;
	v3 = round_to_nearest_pixel(v3) + view_vector;

	Debug_Vertex tri[3] = { make_debug_vertex(v1, c),
	                        make_debug_vertex(v2, c),
	                        make_debug_vertex(v3, c) };

	Debug_Render_Command rc;

	push_debug_vertices_and_command(tri, 3, GL_TRIANGLES, screen_space, rc);
}

void
debug_draw_line(V2 p1, V2 p2, Color c, bool screen_space = false, f32 thickness = 1.0f)
{
	p1 = round_to_nearest_pixel(p1);
	p2 = round_to_nearest_pixel(p2);

	Debug_Vertex line[2] = { make_debug_vertex(p1, c),
	                         make_debug_vertex(p2, c) };

	Debug_Render_Command rc;
	rc.line_thickness = thickness;

	push_debug_vertices_and_command(line, 2, GL_LINES, screen_space, rc);
}

void
debug_draw_rectangle(Rectangle r, Color c, bool screen_space = false, f32 alpha = 1.0f)
{
	V2 rp = round_to_nearest_pixel({ r.x, r.y });

	if (!screen_space) {
		//rp += view_vector;
	}

	Debug_Vertex v[] = { make_debug_vertex(rp.x, rp.y,             c),
	                     make_debug_vertex(rp.x, rp.y + r.h,       c),
	                     make_debug_vertex(rp.x + r.w, rp.y,       c),
	                     make_debug_vertex(rp.x, rp.y + r.h,       c),
	                     make_debug_vertex(rp.x + r.w, rp.y + r.h, c),
	                     make_debug_vertex(rp.x + r.w, rp.y,       c) };

	Debug_Render_Command rc;
	rc.alpha = alpha;

	push_debug_vertices_and_command(v, ARRAY_COUNT(v), GL_TRIANGLES, screen_space, rc);
}

void
debug_draw_panel(Rectangle panel, Color c, V2 view_vector)
{
	debug_draw_rectangle(panel, c, true);
}

void
debug_draw_button(Rectangle button, Color c, V2 view_vector)
{
	debug_draw_rectangle(button, c, true);
}

void
debug_draw_textured_rectangle(Rectangle r, const char *sprite_name, bool screen_space = false, f32 alpha = 1.0f)
{
	Sprite_Asset *ls = get_sprite(sprite_name);
	if (!ls) {
		return;
	}

	V2 rp = round_to_nearest_pixel({ r.x, r.y });

	f32 tex_x = ls->frames[0].texture_scissor.x;
	f32 tex_w = ls->frames[0].texture_scissor.w;
	f32 tex_y = ls->frames[0].texture_scissor.y;
	f32 tex_h = ls->frames[0].texture_scissor.h;

	Debug_Vertex v[] = { make_debug_vertex(rp.x, rp.y,             { tex_x, tex_y + tex_h }),
	                     make_debug_vertex(rp.x, rp.y + r.h,       { tex_x, tex_y }),
	                     make_debug_vertex(rp.x + r.w, rp.y,       { tex_x + tex_w, tex_y + tex_h }),
	                     make_debug_vertex(rp.x, rp.y + r.h,       { tex_x, tex_y }),
	                     make_debug_vertex(rp.x + r.w, rp.y + r.h, { tex_x + tex_w, tex_y }),
	                     make_debug_vertex(rp.x + r.w, rp.y,       { tex_x + tex_w, tex_y + tex_h }) };

	Debug_Render_Command rc;
	rc.alpha = alpha;
	rc.texture_name = ls->texture_name;
	
	push_debug_vertices_and_command(v, ARRAY_COUNT(v), GL_TRIANGLES, screen_space, rc);
}

void
debug_draw_textured_button(Rectangle button, const char *sprite_name, V2 view_vector)
{
	debug_draw_textured_rectangle(button, sprite_name, true);
}

void
debug_draw_quad_outline(Rectangle r, Color c, bool screen_space = false, f32 thickness = 1.0f)
{
	V2 p1 = { r.x, r.y };
	V2 p2 = { r.x + r.w, r.y };
	V2 p3 = { r.x + r.w, r.y + r.h};
	V2 p4 = { r.x, r.y + r.h};

	V2 alignment = { thickness * scaled_meters_per_pixel / 2.0f, 0.0f };

	debug_draw_line(p1 - alignment, p2, c, screen_space, thickness);
	debug_draw_line(p2 - alignment, p3 - alignment, c, screen_space, thickness);
	debug_draw_line(p3, p4 - alignment, c, screen_space, thickness);
	debug_draw_line(p4, p1, c, screen_space, thickness);
}

// @TODO: Make the text rendering use the same projection as everybody else.
// @TODO: Make the text push vertices and draw all text in one call.
void
debug_draw_text(V2 position, Color c, const char *text, f32 scale = 1.0f)
{
	glUseProgram(debug_text_shader);
	glUniform3f(glGetUniformLocation(debug_text_shader, "textColor"), c.r, c.g, c.b);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(text_vao);

	// Iterate through all characters
	for (; *text; ++text) {
		u8 uc = (u8)*text;
		Glyph g = characters[uc];

		f32 xpos = position.x + g.bearing.x * scale;
		f32 ypos = position.y - (g.size.y - g.bearing.y) * scale;

		f32 w = g.size.x * scale;
		f32 h = g.size.y * scale;
		// Update VBO for each character
		f32 vertices[6][4] = {
			{ 0,     1,   0.0, 0.0 },            
			{ 0,     0,       0.0, 1.0 },
			{ 1, 0,       1.0, 1.0 },

			{ 0,     1,   0.0, 0.0 },
			{ 1, 0,       1.0, 1.0 },
			{ 1, 1,   1.0, 0.0 }           
			/*
			{ xpos,     ypos + h,   0.0, 0.0 },            
			{ xpos,     ypos,       0.0, 1.0 },
			{ xpos + w, ypos,       1.0, 1.0 },

			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos + w, ypos,       1.0, 1.0 },
			{ xpos + w, ypos + h,   1.0, 0.0 }           
*/
		};
		// Render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, g.texture_id);
		// Update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		position.x += (g.advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Loaded_Texture *get_texture(Asset_Id);

void
debug_render()
{
	//glEnable(GL_DEPTH_TEST);

	if (debug_draw) {
		glUseProgram(debug_shader);

		glBindVertexArray(debug_vao);

		GLuint num_verts_drawn = 0;
		glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
		for (u32 i = 0; i < debug_render_commands.count; ++i) {
			Debug_Render_Command cmd = debug_render_commands.data[i];

			if (cmd.texture_name != NULL) {
				Texture_Asset *texture = get_texture(cmd.texture_name);
				if (!texture) {
					continue;
				}

				glUniform1i(glGetUniformLocation(debug_shader, "has_texture"), 1);
				glBindTexture(GL_TEXTURE_2D, texture->gpu_handle);
			} else {
				glUniform1i(glGetUniformLocation(debug_shader, "has_texture"), 0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			glUniform1i(glGetUniformLocation(debug_shader, "screen_space"), cmd.screen_space);

			glUniform1f(glGetUniformLocation(debug_shader, "alpha"), cmd.alpha);

			glLineWidth(cmd.line_thickness);

			glDrawArrays(cmd.gl_mode, num_verts_drawn, cmd.num_verts);
			num_verts_drawn += cmd.num_verts;
		}
	}

	debug_render_commands.count = debug_render_commands.num_total_verts = 0;

	//glDisable(GL_DEPTH_TEST);
}

bool
key_pressed(const Keyboard &kb, Key_Symbol ks)
{
	unsigned sc = platform_keysym_to_scancode(ks);
	return kb.keys[sc].pressed;
}

bool
key_down(const Keyboard &kb, Key_Symbol ks)
{
	unsigned sc = platform_keysym_to_scancode(ks);
	return kb.keys[sc].down;
}

bool
key_released(const Keyboard &kb, Key_Symbol ks)
{
	unsigned sc = platform_keysym_to_scancode(ks);
	return kb.keys[sc].released;
}

bool
mouse_button_pressed(const Mouse &m, Mouse_Button b)
{
	return m.buttons[b].pressed;
}

bool
mouse_button_down(const Mouse &m, Mouse_Button b)
{
	return m.buttons[b].down;
}

bool
mouse_button_released(const Mouse &m, Mouse_Button b)
{
	return m.buttons[b].released;
}

bool
active(Rectangle button, f32 mouse_x, f32 mouse_y)
{
	return mouse_x >= button.x
	    && mouse_x <= button.x + button.w
	    && mouse_y >= button.y
	    && mouse_y <= button.y + button.h;
}

bool
point_inside_rectangle(V2 p, V2 center, f32 side_length)
{
	return p.x >= center.x - side_length
	    && p.x <= center.x + side_length
	    && p.y >= center.y - side_length
	    && p.y <= center.y + side_length;
}

u32 gui_id_counter = 0;

struct Button {
	Button() { gui_id = gui_id_counter++; }
	Rectangle rect;
	u32 gui_id;
};

//s32 tile_preview = GRAPHICS_TILES_START;
Asset_Id active_tile_asset_id = ASSET_DOES_NOT_EXIST;
const char *active_tile_name = NULL;

Collider_Id add_collider(f32, f32, f32, f32, Array<Rectangle> *);

V2
world_to_screen(V2 wp, const Camera &c)
{
	wp += c.view_vector;
	V4 v = orthographic_projection * V4{ wp.x, wp.y, 0.0f, 1.0f };
	v *= v.w;
	V4 sv = inverse(make_orthographic_projection(0, window_scaled_meter_width, 0, window_scaled_meter_height)) * v;
	sv /= sv.w;
	V2 result = { sv.x, sv.y };
	//result -= c.view_vector;


	return result;
}

V2
screen_to_world(f32 screen_x, f32 screen_y, const Camera &camera)
{
	V4 v = inverse(orthographic_projection) * V4{ screen_x, screen_y, 0.0f, 1.0f };
	v /= v.w;
	V2 result = { v.x, v.y };
	result -= camera.view_vector;;
	return result;
}

Rectangle
rectangle_at_point(V2 p, f32 diameter)
{
	f32 half_diameter = diameter / 2.0f;

	return { p.x - half_diameter,
	         p.y - half_diameter,
	         diameter,
	         diameter };
}

Sweep sweep_copy[256]; // @TEMP
u32 num_sweep_copies = 0;

void
debug_draw_sweeps(Sweep *sweeps, u32 num_sweeps)
{
	if (debug_mode_bend_test) {
		memcpy(&sweep_copy[num_sweep_copies], sweeps, sizeof(Sweep) * num_sweeps);
		num_sweep_copies += num_sweeps;
	}
}

void
load_level(Array<Tile> *tiles, Array<Rectangle> *colliders)
{
	array_reset(tiles);
	array_reset(colliders);

	FILE *fh = fopen("../data/level.txt", "r");

	u32 num_tiles = 0;
	fscanf(fh, "%u", &num_tiles);

	char name_buf[256];
	for (u32 i = 0; i < num_tiles; ++i) {
		s32 tile_x = 0, tile_y = 0;
		fscanf(fh, "%s %d %d", name_buf, &tile_x, &tile_y);

		add_tile(name_buf, (V2){ tile_x * TILE_SIDE_IN_METERS, tile_y * TILE_SIDE_IN_METERS }, tiles, colliders);
	}
#if 0
	auto handle = platform_open_file("../data/level.ahh", O_RDONLY);
	assert(handle != FILE_HANDLE_ERROR);

	u32 num_tiles = 0;
	assert(platform_read_file(handle, sizeof(num_tiles), &num_tiles));

	char asset_name[256]; // @TEMP
	for (u32 i = 0; i < num_tiles; ++i) {
		u32 asset_name_length = 0;
		V2 tile_world_position = {};

		assert(platform_read_file(handle, sizeof(asset_name_length), &asset_name_length));
		assert(platform_read_file(handle, asset_name_length, &asset_name));
		asset_name[asset_name_length] = '\0';
		assert(platform_read_file(handle, sizeof(tile_world_position), &tile_world_position));

		add_tile("tiles_girder", tile_world_position, tiles, colliders);
	}
#endif
}

enum Edit_Mode {
	EDIT_MODE_ADD,
	EDIT_MODE_REMOVE,
	EDIT_MODE_MOVE,
	EDIT_MODE_SELECT,
};

Edit_Mode edit_mode = EDIT_MODE_SELECT;

u32 remove_tile(Array<Tile> *tiles, Array<Rectangle> *colliders, u32 tile_index);

struct Level_Change {
	u32       tile_indices[32];
	u32       num_tile_indices = 0;
	V2        meter_delta_position;
	Rectangle selection_rect_before_move;

	Edit_Mode operation;
};

#define MAX_LEVEL_CHANGES 256
Level_Change level_changes[MAX_LEVEL_CHANGES];
u32 num_level_changes = 0;
u32 level_change_head = 0;

void
add_level_change(const Level_Change &ele)
{
	memcpy(&level_changes[level_change_head], &ele, sizeof(ele));
	++level_change_head;
	num_level_changes = level_change_head;
}

u8
undo(Level_Change *ele)
{
	if (level_change_head == 0) {
		return false;
	}

	*ele = level_changes[--level_change_head];

	return true;
}

u8
redo(Level_Change *ele)
{
	if (level_change_head == num_level_changes) {
		return false;
	}

	*ele = level_changes[level_change_head++];

	return true;
}

V2s
world_position_to_tile_position(V2 wp)
{
	return { (s32)floor(wp.x / TILE_SIDE_IN_METERS), (s32)floor(wp.y / TILE_SIDE_IN_METERS) };
}

V2
clamp_point_to_tile_origin(V2 p)
{
	V2 result;

	result.x = p.x - (fmod(p.x, TILE_SIDE_IN_METERS) + TILE_SIDE_IN_METERS * (p.x < 0.0f));
	result.y = p.y - (fmod(p.y, TILE_SIDE_IN_METERS) + TILE_SIDE_IN_METERS * (p.y < 0.0f));

	return result;
}

Rectangle
expand_rect_to_tile_boundaries(Rectangle r)
{
	Rectangle result;

	result.x = r.x - (fmod(r.x, TILE_SIDE_IN_METERS) + TILE_SIDE_IN_METERS * (r.x < 0.0f));
	result.y = r.y - (fmod(r.y, TILE_SIDE_IN_METERS) + TILE_SIDE_IN_METERS * (r.y < 0.0f));

	result.w = ceil(fabs(result.x - (r.x + r.w)) / TILE_SIDE_IN_METERS) * TILE_SIDE_IN_METERS;
	result.h = ceil(fabs(result.y - (r.y + r.h)) / TILE_SIDE_IN_METERS) * TILE_SIDE_IN_METERS;

	return result;
}

bool
do_button(const Button &b, const Mouse &m)
{
	debug_draw_rectangle(b.rect, blue, true);
	
	if (intersect_point_rectangle(m.position, b.rect) && mouse_button_released(m, MOUSE_BUTTON_LEFT)) {
		return true;
	}

	return false;
}

#define ZOOM_STEP 1.2f

struct Tile_Rectangle {
	s32 x, y, w, h;
};

Program_State
debug_update(const Input &input, Game_State *game_state)
{
	static f32 zoom_factor = 1.0f;
	static f32 proj_w = window_scaled_meter_width, proj_h = window_scaled_meter_height, proj_x = 0, proj_y = 0;

	Program_State return_state = PROGRAM_STATE_RUNNING;

	Rope *  r = &game_state->rope;
	Player *p = &game_state->player;

	if (key_pressed(input.keyboard, F_KEY))
		debug_draw = !debug_draw;
	if (key_pressed(input.keyboard, P_KEY)) {
		zoom_factor = 1.0f;
		proj_w = window_scaled_meter_width;
		proj_h = window_scaled_meter_height;
		proj_x = 0;
		proj_y = 0;

		orthographic_projection = make_orthographic_projection(proj_x, proj_w, proj_y, proj_h);
		glUseProgram(shader);
		glUniformMatrix4fv(glGetUniformLocation(shader, "projection_matrix"), 1, false, (GLfloat *)&orthographic_projection);
		glUseProgram(debug_shader);
		glUniformMatrix4fv(glGetUniformLocation(debug_shader, "zoomed_orthographic_projection"), 1, false, (GLfloat *)&orthographic_projection);
		glUseProgram(0);

		show_editor = !show_editor;
	}

	if (key_pressed(input.keyboard, C_KEY))
		show_colliders = !show_colliders;

	static Rectangle selection_rect;
	debug_draw_text({0, 0}, black, "hello", 10.0f);

	if (key_down(input.keyboard, LCTRL_KEY) && key_pressed(input.keyboard, S_KEY)) {
		log_print(STANDARD_LOG, "Saving the level: %u.", game_state->tiles.size);

		FILE *fh = fopen("../data/level.txt", "w");
		if (!fh)  _abort("Could not open level file for writing.");
		DEFER(fclose(fh));

		u32 num_tiles = game_state->tiles.size;
		fprintf(fh, "%u\n", num_tiles);

		for (auto t : game_state->tiles) {
			size_t name_len = strlen(t.sprite.name);

			V2s tp = world_position_to_tile_position(t.world_position);
			fprintf(fh, "%s %d %d\n", t.sprite.name, tp.x, tp.y);
		}

#if 0
		auto handle = platform_open_file("../data/level.txt", O_WRONLY | O_CREAT);
		assert(handle != FILE_HANDLE_ERROR);

		u32 num_tiles = game_state->tiles.size;
		assert(platform_write_file(handle, sizeof(num_tiles), &num_tiles));

		for (auto t : game_state->tiles) {
			u32 string_len = 0;
			char name_buffer[256]; // @TEMP
			assert(0);
			//string_len = get_asset_id_string(t.sprite.sprite_asset_id, name_buffer);
			assert(string_len != 0);
			name_buffer[string_len] = '\0';
			printf("NAME %u %s\n", string_len, name_buffer);
			assert(platform_write_file(handle, sizeof(string_len), &string_len));
			assert(platform_write_file(handle, string_len, &name_buffer));
			assert(platform_write_file(handle, sizeof(t.world_position), &t.world_position));
		}

		platform_close_file(handle);
#endif
	}

	if (key_down(input.keyboard, LCTRL_KEY) && (key_pressed(input.keyboard, Z_KEY) || key_pressed(input.keyboard, Y_KEY))) {
		bool undoing = false, redoing = false;

		if (key_pressed(input.keyboard, Z_KEY)) {
			undoing = true;
		} else {
			redoing = true;
		}

		Level_Change ele;
		u8 success = false;

		if (undoing) {
			success = undo(&ele);
		} else {
			success = redo(&ele);
		}

		if (success) {
			for (u32 i = 0; i < ele.num_tile_indices; ++i) {
				u32 tile_index = ele.tile_indices[i];

				assert(game_state->tiles.size > tile_index);

				switch (ele.operation) {
				case EDIT_MODE_MOVE: {
					if (undoing) {
						move_tile(-ele.meter_delta_position, tile_index, game_state);
						selection_rect = ele.selection_rect_before_move;
					} else {
						move_tile(ele.meter_delta_position, tile_index, game_state);
						selection_rect = ele.selection_rect_before_move + ele.meter_delta_position;
					}
				} break;

				case EDIT_MODE_REMOVE: {
					if (undoing) {
						unhide_tile(&game_state->tiles, tile_index);
					} else {
						hide_tile(&game_state->tiles, tile_index);
					}
				} break;

				case EDIT_MODE_ADD: {
					if (undoing) {
						hide_tile(&game_state->tiles, tile_index);
					} else {
						unhide_tile(&game_state->tiles, tile_index);
					}
				} break;

				default: {
				} break;
				}
			}
		}
	}

	if (show_editor) {
		return_state = PROGRAM_STATE_PAUSED;

		static s32 step = 0;
		if (mouse_button_pressed(input.mouse, MOUSE_BUTTON_WHEEL_DOWN) || mouse_button_pressed(input.mouse, MOUSE_BUTTON_WHEEL_UP)) {
			if (mouse_button_pressed(input.mouse, MOUSE_BUTTON_WHEEL_DOWN)) {
					proj_w += ZOOM_STEP;
					proj_h += ZOOM_STEP * (window_scaled_meter_height / window_scaled_meter_width);
					proj_x -= ZOOM_STEP;
					proj_y -= ZOOM_STEP * (window_scaled_meter_height / window_scaled_meter_width);

					step += 1;
			}
			if (mouse_button_pressed(input.mouse, MOUSE_BUTTON_WHEEL_UP)) {
					proj_w -= ZOOM_STEP;
					proj_h -= ZOOM_STEP * (window_scaled_meter_height / window_scaled_meter_width);
					proj_x += ZOOM_STEP;
					proj_y += ZOOM_STEP * (window_scaled_meter_height / window_scaled_meter_width);

					step -= 1;
			}

			zoom_factor = window_scaled_meter_width / (proj_w + ZOOM_STEP * step);

			orthographic_projection = make_orthographic_projection(proj_x, proj_w, proj_y, proj_h);
			glUseProgram(shader);
			glUniformMatrix4fv(glGetUniformLocation(shader, "projection_matrix"), 1, false, (GLfloat *)&orthographic_projection);
			glUseProgram(debug_shader);
			glUniformMatrix4fv(glGetUniformLocation(debug_shader, "zoomed_orthographic_projection"), 1, false, (GLfloat *)&orthographic_projection);
			glUseProgram(0);
		}

		if (key_pressed(input.keyboard, R_KEY)) {
			edit_mode = EDIT_MODE_REMOVE;
		}
		if (key_pressed(input.keyboard, G_KEY)) {
			draw_grid = !draw_grid;
		}

		static Array<Asset_Id> selected_tiles = make_array<Asset_Id>(100, 0);

		static Color grid_color              = white;

		if (mouse_button_down(input.mouse, MOUSE_BUTTON_MIDDLE)) {
			move_camera(&game_state->camera, -input.mouse.delta_position);
		}

		if (draw_grid) {
			f32 camera_l = game_state->camera.position.x - proj_w; 
			f32 camera_r = game_state->camera.position.x + proj_w; 
			f32 camera_b = game_state->camera.position.y - proj_h; 
			f32 camera_t = game_state->camera.position.y + proj_h; 

			f32 grid_x_offset = -fmod(camera_l, TILE_SIDE_IN_METERS);
			f32 grid_y_offset = -fmod(camera_b, TILE_SIDE_IN_METERS);

			for (f32 w = camera_l; w < camera_r; w += TILE_SIDE_IN_METERS) {
				debug_draw_line({ w + grid_x_offset, camera_b }, { w + grid_x_offset, camera_t }, grid_color);
			}

			for (f32 h = camera_b; h < proj_h; h += TILE_SIDE_IN_METERS) {
				debug_draw_line({ camera_l, h + grid_y_offset }, { camera_r, h + grid_y_offset }, grid_color);
			}
		}

		V2 mouse_world_position = screen_to_world(2.0f * input.mouse.position.x / window_scaled_meter_width - 1.0f, /* Convert to OpenGL screen coordinates (-1, 1). */
		                                          2.0f * input.mouse.position.y / window_scaled_meter_height - 1.0f,
		                                          game_state->camera);

		if (key_pressed(input.keyboard, ESCAPE_KEY)) {
			active_tile_name = NULL;
			edit_mode = EDIT_MODE_SELECT;
			selected_tiles.resize(0);
		}

		
		f32 button_width  = 16 * meters_per_pixel;
		f32 button_height = 16 * meters_per_pixel;
		Rectangle panel;
		panel.w = button_width * 2;
		panel.h = button_height * 4;
		panel.x = 0;
		panel.y = window_scaled_meter_height - panel.h * 2;
		static Color pc = random_color();

		bool clicked_on_ui = false;

		if (active(panel, input.mouse.position.x, input.mouse.position.y)) {
			if (mouse_button_pressed(input.mouse, MOUSE_BUTTON_LEFT) || mouse_button_released(input.mouse, MOUSE_BUTTON_LEFT)) {
				clicked_on_ui = true;
			}
		}

		if (!clicked_on_ui) {
			static u8 defining_edit_rect        = false;
			static V2 edit_rect_initial_corner  = { 0.0f, 0.0f };

			f32 tile_w = TILE_SIDE_IN_METERS;
			f32 tile_h = TILE_SIDE_IN_METERS;

			V2 tile_world_position = mouse_world_position;

			tile_world_position.x -= fmod(tile_world_position.x, tile_w) + tile_w * (tile_world_position.x < 0.0f);
			tile_world_position.y -= fmod(tile_world_position.y, tile_h) + tile_h * (tile_world_position.y < 0.0f);

			if (mouse_button_pressed(input.mouse, MOUSE_BUTTON_LEFT)) {
				defining_edit_rect = true;

				edit_rect_initial_corner.x = mouse_world_position.x;
				edit_rect_initial_corner.y = mouse_world_position.y;
			}

			Rectangle edit_rect = NO_RECTANGLE;

			if (defining_edit_rect) {
				edit_rect.x = fmin(edit_rect_initial_corner.x, mouse_world_position.x);
				edit_rect.y = fmin(edit_rect_initial_corner.y, mouse_world_position.y);
				edit_rect.w = fabs(edit_rect_initial_corner.x - mouse_world_position.x);
				edit_rect.h = fabs(edit_rect_initial_corner.y - mouse_world_position.y);

				edit_rect = expand_rect_to_tile_boundaries(edit_rect);
			}

			if (edit_mode == EDIT_MODE_ADD) {
				if (defining_edit_rect) {
					f32 preview_rect_origin_x = edit_rect.x;
					f32 preview_rect_origin_y = edit_rect.y;

					u32 preview_tile_width  = round(edit_rect.w / tile_w);
					u32 preview_tile_height = round(edit_rect.h / tile_h);

					u8 do_add = false;

					Level_Change ec;
					if (mouse_button_released(input.mouse, MOUSE_BUTTON_LEFT)) {
						ec.operation        = EDIT_MODE_ADD;

						do_add = true;
					}

					for (u32 x = 0; x < preview_tile_width; ++x) {
						for (u32 y = 0; y < preview_tile_height; ++y) {
							if (do_add) {
								add_tile(active_tile_name, { preview_rect_origin_x + (x * tile_w), preview_rect_origin_y + (y * tile_h) }, &game_state->tiles, &game_state->colliders);

								ec.tile_indices[ec.num_tile_indices++] = (u32)(game_state->tiles.size - 1);
							} else {
								debug_draw_textured_rectangle({ preview_rect_origin_x + (x * tile_w), preview_rect_origin_y + (y * tile_h), tile_w, tile_h }, active_tile_name, false, 0.5f);
							}
						}
					}

					if (do_add) {
						add_level_change(ec);
					}
				} else {
					debug_draw_textured_rectangle({ tile_world_position.x, tile_world_position.y, tile_w, tile_h }, active_tile_name, false, 0.5f);
				}
			}
			
			static V2s move_origin;
			static bool moving = false;

			if (edit_mode == EDIT_MODE_REMOVE && defining_edit_rect && mouse_button_released(input.mouse, MOUSE_BUTTON_LEFT)) {
					Level_Change lc;
					lc.operation = EDIT_MODE_REMOVE;

					for (u32 i = 0; i < game_state->tiles.size; ++i) {
						Rectangle tile_rect = { game_state->tiles[i].world_position.x, game_state->tiles[i].world_position.y, TILE_SIDE_IN_METERS, TILE_SIDE_IN_METERS };

						if (intersect_rectangle_rectangle(edit_rect, tile_rect)) {
							lc.tile_indices[lc.num_tile_indices++] = i;

							hide_tile(&game_state->tiles, i);
						}
					}

					if (lc.num_tile_indices > 0)  add_level_change(lc);
			}

			if (edit_mode == EDIT_MODE_SELECT && defining_edit_rect && mouse_button_released(input.mouse, MOUSE_BUTTON_LEFT)) {
				for (u32 i = 0; i < game_state->tiles.size; ++i) {
					Rectangle tile_rect = { game_state->tiles[i].world_position.x, game_state->tiles[i].world_position.y, TILE_SIDE_IN_METERS, TILE_SIDE_IN_METERS };

					if (intersect_rectangle_rectangle(edit_rect, tile_rect)) {
						selected_tiles.push(i);
					}
				}

				edit_mode = EDIT_MODE_MOVE;
				selection_rect = edit_rect;
			}

			if (mouse_button_released(input.mouse, MOUSE_BUTTON_LEFT)) {
				defining_edit_rect = false;
			}

			Color c;

			if      (edit_mode == EDIT_MODE_ADD)     c = green;
			else if (edit_mode == EDIT_MODE_REMOVE)  c = red;
			else                                     c = black;

			static bool pressed_outside_selection_rect = false;

			if (edit_mode == EDIT_MODE_MOVE) {
				if (mouse_button_pressed(input.mouse, MOUSE_BUTTON_LEFT)) {
					if (intersect_point_rectangle(mouse_world_position, selection_rect)) {
						moving = true;
						move_origin = world_position_to_tile_position(mouse_world_position);

						for (auto i : selected_tiles) {
							hide_tile(&game_state->tiles, i);
						}
					} else {
						pressed_outside_selection_rect = true;;
					}
				}

				if (pressed_outside_selection_rect && mouse_button_released(input.mouse, MOUSE_BUTTON_LEFT)) {
					if (!intersect_point_rectangle(mouse_world_position, selection_rect)) {
						edit_mode = EDIT_MODE_SELECT;
						selected_tiles.resize(0);
						edit_rect = NO_RECTANGLE;
					}

					pressed_outside_selection_rect = false;
				}

				V2s tile_move = world_position_to_tile_position(mouse_world_position) - move_origin;

				V2 meter_move = TILE_SIDE_IN_METERS * tile_move;

				if (moving && mouse_button_released(input.mouse, MOUSE_BUTTON_LEFT)) {
					Level_Change ec;
					ec.meter_delta_position       = meter_move;
					ec.selection_rect_before_move = selection_rect;
					ec.operation                  = EDIT_MODE_MOVE;

					for (auto i : selected_tiles) {
						unhide_tile(&game_state->tiles, i);

						move_tile(meter_move, i, game_state);

						ec.tile_indices[ec.num_tile_indices++] = i;
					}

					add_level_change(ec);

					selection_rect = selection_rect + meter_move;

					moving = false;
				}

				if (moving) {
					for (auto i : selected_tiles) {
						Rectangle move_preview = { game_state->tiles[i].world_position.x + meter_move.x,
						                           game_state->tiles[i].world_position.y + meter_move.y,
						                           TILE_SIDE_IN_METERS,
						                           TILE_SIDE_IN_METERS };

						debug_draw_textured_rectangle(move_preview, game_state->tiles[i].sprite.name, false, 0.5f);
					}
				} else {
					debug_draw_quad_outline(selection_rect, black, false, 4.0f);
				}

			}

			if (defining_edit_rect) {
				if (edit_mode != EDIT_MODE_MOVE)  debug_draw_quad_outline(edit_rect, c, false, 4.0f);
			} else if (edit_mode != EDIT_MODE_MOVE) {
				V2 hover_p = world_to_screen(clamp_point_to_tile_origin(mouse_world_position), game_state->camera);
				Rectangle hover_tile = { hover_p.x, hover_p.y, TILE_SIDE_IN_METERS * zoom_factor, TILE_SIDE_IN_METERS * zoom_factor };

				f32 marker_length    = (tile_w / 4.0f);
				f32 marker_thickness = 0.05f;

				debug_draw_rectangle({ hover_tile.x, hover_tile.y, marker_length, marker_thickness }, c, true);
				debug_draw_rectangle({ hover_tile.x, hover_tile.y, marker_thickness, marker_length }, c, true);

				debug_draw_rectangle({ hover_tile.x + hover_tile.w - marker_length, hover_tile.y, marker_length, marker_thickness }, c, true);
				debug_draw_rectangle({ hover_tile.x + hover_tile.w - marker_thickness, hover_tile.y, marker_thickness, marker_length }, c, true);

				debug_draw_rectangle({ hover_tile.x, hover_tile.y + hover_tile.h - marker_thickness, marker_length, marker_thickness }, c, true);
				debug_draw_rectangle({ hover_tile.x, hover_tile.y + hover_tile.h - marker_length, 0.05f, marker_length }, c, true);

				debug_draw_rectangle({ hover_tile.x + hover_tile.w - marker_length, hover_tile.y + hover_tile.h - marker_thickness, marker_length, marker_thickness }, c, true);
				debug_draw_rectangle({ hover_tile.x + hover_tile.w - marker_thickness, hover_tile.y + hover_tile.h - marker_length, marker_thickness, marker_length }, c, true);
			}
		}

		debug_draw_panel(panel, pc, game_state->camera.view_vector);

		u32 num_buttons = 0;
		for (size_t i = 0; i < sprite_catalog.tags.size; ++i) {
			if (sprite_catalog.tags[i] & SPRITE_TILE_TAG) {
				s32 row = num_buttons / (panel.w / button_width);
				s32 col = fmod(num_buttons, (panel.w / button_width));

				Button button;
				button.rect.x = panel.x + (col * button_width);
				button.rect.y = panel.y + panel.h - button_height - (row * button_height);
				button.rect.w = button_width;
				button.rect.h = button_height;

				debug_draw_textured_button(button.rect, sprite_catalog.names[i], game_state->camera.view_vector);

				if (active(button.rect, input.mouse.position.x, input.mouse.position.y)) {
					if (mouse_button_released(input.mouse, MOUSE_BUTTON_LEFT)) {
						active_tile_name = sprite_catalog.names[i];
						pc = random_color();
						edit_mode = EDIT_MODE_ADD;
					} else if (mouse_button_pressed(input.mouse, MOUSE_BUTTON_LEFT)) {

					} else {
						// Hovering.
						debug_draw_rectangle(button.rect, white, true, 0.15f);
					}
				}

				++num_buttons;
			}
		}

		Button make_room_button;
		make_room_button.rect.x = 0;
		make_room_button.rect.y = 0;
		make_room_button.rect.w = 1;
		make_room_button.rect.h = 1;

		if (do_button(make_room_button, input.mouse)) {
			// Do the button.
		}
	}

	if (show_colliders) {
		static Color collider_color{199, 21, 133, 100};
		for (u32 i = 0; i < game_state->colliders.size; ++i) {
			debug_draw_rectangle(game_state->colliders[i], collider_color, false, 0.45f);
		}
	}

	if (game_state->rope.out) {
		for (u32 i = 0; i <= r->end; ++i) {
			debug_draw_rectangle(rectangle_at_point(r->control_points[i], 0.09), green);
		}
		// @TODO: Draw stretch and line in two different colors.
		for (u32 i = 0; i < r->end; ++i) {
			debug_draw_line(r->control_points[i], r->control_points[i + 1], green);
		}
	}

	debug_draw_rectangle(rectangle_at_point(round_to_nearest_pixel(p->world_position) + V2{ game_state->player.collider.w / 2.0f, 0.0f }, 0.09), game_state->player.grounded ? green : red);

	// Dot in center of screen for camera tracking.
	debug_draw_rectangle(rectangle_at_point({ window_scaled_meter_width / 2.0f, window_scaled_meter_height / 2.0f }, 0.04), cyan, true);

#if 0
	if (debug_mode_bend_test) {
		if (input.keyboard.keys[platform_keysym_to_scancode(J_KEY)].down)
			p->velocity.y += JUMP_ACCELERATION;
		else if (input.keyboard.keys[platform_keysym_to_scancode(S_KEY)].down)
			p->velocity.y += -JUMP_ACCELERATION;
		else
			p->velocity.y = 0.0f;
		if (r->out) {
			static bool started_test = false;
			static V2 test_start_pos, test_end_pos, a, epb;

			if (input.keyboard.keys[platform_keysym_to_scancode(K_KEY)].released) {
				num_sweep_copies = 0;
			}
			if (input.keyboard.keys[platform_keysym_to_scancode(I_KEY)].pressed) {
				if (!started_test) {
					test_start_pos = r->control_points[0];
					test_end_pos = r->control_points[r->end];
					num_sweep_copies = 0;
					a = { 0.0f, 0.0f };
					epb = { 0.0f, 0.0f };
					started_test = true;
					printf("Bend test start.\n");
				} else {
					add_and_remove_bends(game_state->colliders, test_start_pos, test_end_pos, p->physical_position, p->collider_id, r);
					started_test = false;
					printf("Bend test end.\n");
				}
			}

			for (u32 i = 0; i < num_sweep_copies; ++i) {
				debug_draw_triangle(sweep_copy[i].anchor, sweep_copy[i].start, sweep_copy[i].end, Color(random_colors[i].r, random_colors[i].g, random_colors[i].b, 0.2f), game_state->camera.view_vector);
			}
		}
		if (r->out && r->was_attached) {
			V2 active_end                   = r->control_points[1];
			r->unstretched_length = length(active_end - r->control_points[0]);
		}
	}

#endif

	glUseProgram(debug_shader);
	glUniform2f(glGetUniformLocation(debug_shader, "view_vector"), game_state->camera.view_vector.x, game_state->camera.view_vector.y);

	return return_state;
}

void
debug_set_mode_bend_test(Game_State *game_state)
{
	debug_mode_bend_test = true;

	GROUND_RUN_ACCELERATION  = 0.5f;
	AIR_RUN_ACCELERATION     = 0.5f;
	GRAVITY                  = 0.0f;
	JUMP_ACCELERATION        = 0.8f;
	GROUND_FRICTION          = INFINITY;
	AIR_FRICTION             = INFINITY;
	ROPE_EXTEND_RATE         = 1.0f;
	ROPE_RETRACT_RATE        = 10.0f;
	ROPE_MAX_EXTENT          = 5.0f;
	ROPE_STRETCH_MODIFIER    = 0.0f;

}

void
debug_set_game_speed(f32 modifier)
{
	delta_time *= modifier;
	GRAVITY *= modifier;
	GROUND_RUN_ACCELERATION *= modifier;
}

