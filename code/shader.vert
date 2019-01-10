layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

uniform mat4 projection_matrix;

out vec2 frag_uv;

void main()
{
	gl_Position = projection_matrix * vec4(position, 1.0f);
	//gl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	//frag_color  = color;
	frag_uv     = uv;
}

