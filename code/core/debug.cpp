GLuint debug_vao;
GLuint debug_vbo;
GLuint debug_shader_program;

u8 debug_draw = false;

struct Debug_Render_Command {
	u32 num_verts;
	u32 gl_mode;
	V3f color;
	u8 is_screen_space;
};

constexpr GLuint DEBUG_MAX_RENDER_VERTS = 1<<28;
constexpr GLuint DEBUG_MAX_RENDER_COMMANDS = 1<<16;

struct Debug_Render_Commands {
	Debug_Render_Command data[DEBUG_MAX_RENDER_COMMANDS];
	size_t count;
	GLuint num_total_verts;
} debug_render_commands;

const char *debug_shader_vert_source = R"(
	layout (std140) uniform Matrices
	{
		mat4 u_view;
		mat4 u_perspective_proj;
		mat4 u_ortho_proj;
	};
	uniform vec3 u_color;
	uniform bool u_is_screen_space;

	layout (location = 0) in vec3 pos;

	void main()
	{
		//t_color = u_color;
		if (u_is_screen_space)
			gl_Position = u_ortho_proj * vec4(pos, 1.0f);
		else
			gl_Position = u_perspective_proj * u_view * vec4(pos, 1.0f);
		//gl_Position = vec4(pos, 1.0f);
		//t_normal = normal;
		//t_normal = normalize(mat3(transpose(inverse(model))) * normal);
	}
)";
const char *debug_shader_frag_source = R"(
	//in vec3 t_color;
	uniform vec3 u_color;
	//in vec3 t_normal;
	out vec4 color;

	void main()
	{
		color = vec4(u_color, 1.0f);
	}
)";

void
debug_init()
{
	glGenVertexArrays(1, &debug_vao);
	glBindVertexArray(debug_vao);
	glGenBuffers(1, &debug_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
	glBufferData(GL_ARRAY_BUFFER, DEBUG_MAX_RENDER_VERTS, NULL, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V3f), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	debug_shader_program = gpu_make_shader(debug_shader_vert_source, debug_shader_frag_source);
	glUseProgram(debug_shader_program);
	glUniform1i(glGetUniformLocation(debug_shader_program, "u_is_screen_space"), false);
}

// @TODO: This is all pretty terrible. Passing back the offset if caller wants to update the data. Should group debug render elements
// into "debug groups" and have one vbo for each group. Pass that back as an id and make functions that operate on the id.
s64
debug_push_vertices(V3f *verts, u32 num_verts, V3f color, u32 gl_render_mode, u8 is_screen_space = false, s64 ofs = -1)
{
	glUseProgram(debug_shader_program);
	glBindVertexArray(debug_vao);
	glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
	if (ofs == -1) {
		glBufferSubData(GL_ARRAY_BUFFER, debug_render_commands.num_total_verts * sizeof(V3f), num_verts * sizeof(V3f), verts);
		u32 start_offset = debug_render_commands.num_total_verts * sizeof(V3f);
		debug_render_commands.num_total_verts += num_verts;
		assert(debug_render_commands.num_total_verts < DEBUG_MAX_RENDER_VERTS);
		debug_render_commands.data[debug_render_commands.count] = { num_verts, gl_render_mode, color, is_screen_space };
		++debug_render_commands.count;
		return start_offset;
	}
	glBufferSubData(GL_ARRAY_BUFFER, ofs, num_verts * sizeof(V3f), verts);
	return ofs;
}

s64
debug_draw_lines(Line *lines, size_t num_lines, s64 ofs)
{
	return debug_push_vertices((V3f *)lines, num_lines * 2, { 1.0f, 0.0f, 0.0f }, GL_LINES, false, ofs);
}

void
debug_draw_line(V3f origin, V3f direction, int length)
{
	V3f line[2] = { origin, origin + (length * direction) };
	debug_push_vertices(line, 2, { 1.0f, 0.0f, 0.0f }, GL_LINES);
}

void
debug_draw_opaque_circle(float radius, V3f center, V3f normal)
{
	V3f circle[362];
	circle[0] = center;
	normal = normalize(normal); // Just to be safe.
	//V3f x_axis = { normal.y, normal.z, 0.0f };
	//V3f z_axis = cross_product(x_axis, normal);

	for (int i = 0; i < 361; ++i)
		circle[i+1] = { _cos(M_DEG_TO_RAD * i) * radius + center.x, center.y, _sin(M_DEG_TO_RAD * i) * radius + center.z };
	debug_push_vertices(circle, 362, { 0.0f, 1.0f, 0.0f }, GL_TRIANGLE_FAN);
}

void
debug_draw_opaque_cylinder(float top_radius, float bottom_radius, float height)
{

}

void
debug_draw_opaque_cone()
{

}

// @TODO: Move debug_print into here.
void
debug_draw_arrow(V3f origin, V3f direction, int length)
{
	debug_draw_line(origin, direction, length); // @@@@ Should be - len of cone.
	debug_draw_opaque_cone();
}

void
debug_draw_ui_panel()
{
	V3f v[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.5f }, { 1.0f, 0.5f }, { 0.0f, 0.5f }, { 1.0f, 0.0f } };
	debug_push_vertices(v, ARRAY_LENGTH(v), { 0.0f, 0.0f, 1.0f }, GL_TRIANGLES, true);
}

void
debug_update(const Input input)
{
	if (platform_was_key_pressed_toggle(input.keyboard.keys, F_KEY))
		debug_draw = !debug_draw;
}

void
debug_render()
{
	if (debug_draw) {
		glUseProgram(debug_shader_program);
		glBindVertexArray(debug_vao);

		GLuint num_verts_drawn = 0;
		glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
		for (u32 i = 0; i < debug_render_commands.count; ++i) {
			glUniform1i(glGetUniformLocation(debug_shader_program, "u_is_screen_space"), debug_render_commands.data[i].is_screen_space ? 1 : 0);
			V3f color = debug_render_commands.data[i].color;
			glUniform3f(glGetUniformLocation(debug_shader_program, "u_color"), color.x, color.y, color.z);
			glDrawArrays(debug_render_commands.data[i].gl_mode, num_verts_drawn, debug_render_commands.data[i].num_verts);
			num_verts_drawn += debug_render_commands.data[i].num_verts;
		}
		//debug_render_commands.count = debug_render_commands.num_total_verts = 0;
	}
}

