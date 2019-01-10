uniform sampler2D utexture;
//in vec3 frag_color;
in vec2 frag_uv;

out vec4 out_color;

void main() {
	//out_color = vec4(frag_color, 1.0);
	out_color = texture(utexture, frag_uv);
	//out_color = vec4(1.0, 1.0, 0.0f, 1.0);
}
