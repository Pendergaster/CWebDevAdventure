
"precision highp float;\n"
STRINGIFY(


layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;
out vec2 out_uv;


void main() {
	gl_Position = vec4(pos, 0.0, 1.0); 
	out_uv = uv;
	out_uv.y = 1.0 - out_uv.y;

	 // float temp = out_uv.x;
	 // out_uv.x = out_uv.y;
	 // out_uv.y = temp;
}
)

