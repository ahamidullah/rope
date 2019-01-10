#define DEFINEPROC
#include "opengl_functions.h"
#undef DEFINEPROC

struct Render_Command {
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	GLuint texture_id;
};

struct Vertex {
	V3 position;
	V2 uv;
};

Array<Render_Command> render_commands = make_array<Render_Command>(1000, 0);
GLuint shader;
M4 orthographic_projection;
bool is_opengl_initialized = false;

#define GL_CHECK_ERROR(obj, ivfn, objparam, infofn, fmt, ...)\
{\
	GLint status;\
	ivfn(obj, objparam, &status);\
	if (status == GL_FALSE) {\
		GLint infolog_len;\
		ivfn(obj, GL_INFO_LOG_LENGTH, &infolog_len);\
		GLchar infolog[2056] = { 0 };\
		infofn(obj, infolog_len, NULL, infolog);\
		log_print(CRITICAL_ERROR_LOG, fmt, ## __VA_ARGS__);\
		_abort(": %s.", infolog);\
	}\
}

GLuint
compile_shader(const GLenum type, String source, const char *defines = "")
{
	const GLuint id = glCreateShader(type);
	
	const char *source_strings[] = { "#version 330\n", defines, source.data };
	glShaderSource(id, ARRAY_COUNT(source_strings), source_strings, NULL);
	glCompileShader(id);

	const char *type_str;
	if (type == GL_VERTEX_SHADER)
		type_str = "vertex";
	else if (type == GL_GEOMETRY_SHADER)
		type_str = "geometry";
	else if (type == GL_FRAGMENT_SHADER)
		type_str = "fragment";
	else
		type_str = "unknown";
	GL_CHECK_ERROR(id, glGetShaderiv, GL_COMPILE_STATUS, glGetShaderInfoLog, "compile failure in %s shader", type_str);
	return id;
}

GLuint
make_shader(String vert_source, String frag_source)
{
	GLuint vert_id = compile_shader(GL_VERTEX_SHADER, vert_source);
	GLuint frag_id = compile_shader(GL_FRAGMENT_SHADER, frag_source);
	GLuint program = glCreateProgram();

	glAttachShader(program, vert_id);
	glAttachShader(program, frag_id);
	glLinkProgram(program);

	GL_CHECK_ERROR(program, glGetProgramiv, GL_LINK_STATUS, glGetProgramInfoLog, "linker failure");

	glDetachShader(program, vert_id);
	glDetachShader(program, frag_id);
	glDeleteShader(vert_id);
	glDeleteShader(frag_id);

	return program;
}

GLuint
gpu_make_texture(u32 gl_tex_unit, s32 texture_format, s32 pixel_format, s32 pixel_width, s32 pixel_height, u8 *pixels)
{
	GLuint tex_id;
	glGenTextures(1, &tex_id);
	glActiveTexture(gl_tex_unit);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, texture_format, pixel_width, pixel_height, 0, pixel_format, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	return tex_id;
}

V2 round_to_nearest_pixel(V2 p);

Loaded_Sprite *get_sprite(Asset_Id id);
Loaded_Texture *get_texture(Asset_Id id);

void
add_quad_render_commands(const char *texture_name, f32 quad_meter_width, f32 quad_meter_height, Rectangle texture_scissor_rect, V2 world_position, V2 view_vector)
{
	// glInvalidateBufferData()?

	//if (!asset_and_dependencies_are_ready(texture_name)) {
		//return;
	//}

	Texture_Asset *texture = get_texture(texture_name);
	if (!texture)  return;

	// @TODO: Worth it to load these up front?
	Render_Command rc;
	rc.texture_id = texture->gpu_handle;
	glGenVertexArrays(1, &rc.vao);
	glGenBuffers(1, &rc.vbo);
	glGenBuffers(1, &rc.ebo);
	glBindVertexArray(rc.vao);
	glBindBuffer(GL_ARRAY_BUFFER, rc.vbo);

#if 0
	static V2 pcp = c->position;
	static V2 dpcp = { 0.0f, 0.0f };
	static V2 pwp = world_position;

	dpcp = c->position - pcp;

	//s32 dpx = (dpcp.x + (sign(dpcp.x) * (scaled_meters_per_pixel / 2.0f))) / scaled_meters_per_pixel;
	//s32 dpy = (dpcp.y + (sign(dpcp.y) * (scaled_meters_per_pixel / 2.0f))) / scaled_meters_per_pixel;

	s32 dpx = 0;
	s32 dpy = 0;

	if (fabs((c->position.x - pcp.x) / scaled_meters_per_pixel) >= 1.0f) {
		dpx -= (c->position.x - pcp.x) / scaled_meters_per_pixel;
		//pcp.x = trunc(c->position.x / scaled_meters_per_pixel) * scaled_meters_per_pixel;
	}

	printf("pcpx pcpy %.9g %.9g\n", pcp.x, pcp.y);

	if (fabs((c->position.y - pcp.y) / scaled_meters_per_pixel) >= 1.0f) {
		dpy -= (c->position.y - pcp.y) / scaled_meters_per_pixel;
		//pcp.y = trunc(c->position.y / scaled_meters_per_pixel) * scaled_meters_per_pixel;
		//pcp.y += trunc(c->position.y / scaled_meters_per_pixel) * scaled_meters_per_pixel;
	}
	printf("dpx dpy %d %d\n", dpx, dpy);

	extern V2 xyxy;

		dpx += round_to_nearest_pixel((world_position.x -xyxy.x)- pwp.x) / scaled_meters_per_pixel;
		dpy += round_to_nearest_pixel((world_position.y-xyxy.y) - pwp.y) / scaled_meters_per_pixel;

	V2 center = { ((window_pixel_width / 2) + 1) * scaled_meters_per_pixel, ((window_pixel_height / 2) + 1) * scaled_meters_per_pixel };
	V2 xxx = (world_position - c->position);
	s32 xx = (xxx.x + (sign(xxx.x) * (scaled_meters_per_pixel / 2.0f))) / scaled_meters_per_pixel;
	s32 yy = (xxx.y + (sign(xxx.y) * (scaled_meters_per_pixel / 2.0f))) / scaled_meters_per_pixel;

	//printf("%d %d\n", xx, yy);
	//printf("%f %f\n", center.x, center.y);
	//print_v2(world_position);
	//V2 x = (world_position);
	//V2 v = (c->view_vector);
	//x.x = floor(x.x / scaled_meters_per_pixel) * scaled_meters_per_pixel;
	//x.y = floor(x.y / scaled_meters_per_pixel) * scaled_meters_per_pixel;
	//rounded_viewport_position += round_to_nearest_pixel(v) - v;
	//round_to_nearest_pixel(rounded_viewport_position);

	static V2 pp = { center.x + (xx * scaled_meters_per_pixel), center.y + (yy * scaled_meters_per_pixel) };

	world_position = pp + (V2){ dpx * scaled_meters_per_pixel, dpy * scaled_meters_per_pixel };
	//print_v2(rounded_viewport_position);
	//V2 rounded_viewport_position = pp;
#endif

	//world_position = round_to_nearest_pixel(world_position);

	//world_position += round_to_nearest_pixelx(c->view_vector);

	//static V2 pp = round_to_nearest_pixel(world_position) + c->view_vector;
	//static V2 old_cd = round_to_nearest_pixel(world_position - c->position);

	//if (round_to_nearest_pixel(world_position - c->position) == old_cd) {
		
	//}
	//world_position.x += c->view_vector - round_to_lowest(c->view_vector, scaled_meters_per_pixel);
	//world_position = round_to_lowest(world_position, scaled_meters_per_pixel);

	V2 camera_space_position = round_to_nearest_pixel(world_position) + view_vector;

	f32 gl_x      = camera_space_position.x;
	f32 gl_y      = camera_space_position.y;
	f32 gl_width  = quad_meter_width;
	f32 gl_height = quad_meter_height;

	f32 tex_x = texture_scissor_rect.x;
	f32 tex_w = texture_scissor_rect.w;
	f32 tex_y = texture_scissor_rect.y;
	f32 tex_h = texture_scissor_rect.h;

	Vertex vertices[4] = {
		//{ { gl_x, gl_y, 0.0f }, { 0, 1 } },
		{ { gl_x, gl_y, 0.0f }, { tex_x, tex_y + tex_h } },
		//{ { gl_x, gl_y + gl_height, 0.0f }, { 0, 0 } },
		{ { gl_x, gl_y + gl_height, 0.0f }, { tex_x, tex_y } },
		//{ { gl_x + gl_width, gl_y + gl_height, 0.0f }, { 1, 0 } },
		{ { gl_x + gl_width, gl_y + gl_height, 0.0f }, { tex_x + tex_w, tex_y } },
		//{ { gl_x + gl_width, gl_y, 0.0f }, { 1, 1 } },
		{ { gl_x + gl_width, gl_y, 0.0f }, { tex_x + tex_w, tex_y + tex_h } },
	};

	GLuint indices[6] {
		0, 1, 2, 0, 2, 3
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, position));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, uv));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rc.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

	glBindVertexArray(0);
	render_commands.push(rc);
}

void
add_sprite_render_commands(Sprite_Instance s, V2 world_position, V2 view_vector)
{
	Sprite_Asset *sprite_data = get_sprite(s.name);
	if (!sprite_data) {
		assert(0);
		return;
	}

	//if (!asset_and_dependencies_are_ready(s.sprite_asset_id)) {
	//	return;
	//}

	auto f = sprite_data->frames[s.current_frame];

	add_quad_render_commands(sprite_data->texture_name, f.meter_width, f.meter_height, f.texture_scissor, world_position + f.meter_offset, view_vector);
}

void
render_init()
{
	// Load OpenGL functions.
#define LOADPROC
#include "opengl_functions.h"
#undef LOADPROC

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_DEPTH_TEST);
	glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
	glViewport(0, 0, window_pixel_width, window_pixel_height);

	Memory_Arena init_arena = mem_make_arena();
	DEFER(mem_destroy_arena(&init_arena));

	String vert_source = read_entire_file("shader.vert", &init_arena);
	String frag_source = read_entire_file("shader.frag", &init_arena);
	assert(vert_source != STRING_ERROR && frag_source != STRING_ERROR);

	shader = make_shader(vert_source, frag_source);
	assert(shader);

	glUseProgram(shader);
	orthographic_projection = make_orthographic_projection(0, window_scaled_meter_width, 0, window_scaled_meter_height);
	glUniformMatrix4fv(glGetUniformLocation(shader, "projection_matrix"), 1, false, (GLfloat *)&orthographic_projection);
	glUseProgram(0);
}

void debug_render();

void
submit_render_commands_and_swap_backbuffer()
{
	// Render main.
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shader);

		for (auto rc : render_commands) {
			glBindVertexArray(rc.vao);
			glBindBuffer(GL_ARRAY_BUFFER, rc.vbo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rc.ebo);
			glBindTexture(GL_TEXTURE_2D, rc.texture_id);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (GLvoid *)0);

			glDeleteBuffers(1, &rc.vbo);
			glDeleteVertexArrays(1, &rc.vao);
		}

		render_commands.clear();

		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			if (err == GL_INVALID_ENUM)
				log_print(MAJOR_ERROR_LOG, "Opengl error: Invalid enum.");
			if (err == GL_INVALID_VALUE)
				log_print(MAJOR_ERROR_LOG, "Opengl error: Invalid value.");
			if (err == GL_INVALID_OPERATION)
				log_print(MAJOR_ERROR_LOG, "Opengl error: Invalid operation.");
			if (err == GL_STACK_OVERFLOW)
				log_print(MAJOR_ERROR_LOG, "Opengl error: Stack overflow.");
			if (err == GL_STACK_UNDERFLOW)
				log_print(MAJOR_ERROR_LOG, "Opengl error: Stack underflow.");
			if (err == GL_OUT_OF_MEMORY)
				log_print(MAJOR_ERROR_LOG, "Opengl error: Out of memory.");
			if (err == GL_INVALID_FRAMEBUFFER_OPERATION)
				log_print(MAJOR_ERROR_LOG, "Opengl error: Invalid framebuffer operation.");
			if (err == GL_CONTEXT_LOST)
				log_print(MAJOR_ERROR_LOG, "Opengl error: GL context lost due to a graphics card reset.");
			if (err == GL_TABLE_TOO_LARGE)
				log_print(MAJOR_ERROR_LOG, "Opengl error: Table too large. (This error was removed in OpenGl 3.1.)");
			if (err == 0)
				log_print(MAJOR_ERROR_LOG, "Opengl error: glGetError() failed.");
		}
	}

	// Render debug.
	{
		debug_render();
	}

	platform_swap_buffers();
}

void
render_cleanup()
{

}

void
render_resize_window()
{

}

