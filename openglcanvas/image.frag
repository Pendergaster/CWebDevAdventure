"precision highp float;\n"
STRINGIFY(


in vec2 out_uv;
uniform sampler2D image;

out vec4 color;

void main() {
	color = texture(image, out_uv);
}



)