
#shader vertex
#version 440 core


layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;

out vec2 itpl_uv;

uniform mat4 u_mpv_mat;


void main() {
    itpl_uv = uv;
    gl_Position = u_mpv_mat * position;
}



#shader fragment
#version 440 core


layout(location = 0) out vec4 color;
in vec2 itpl_uv;

uniform sampler2D u_texture;
uniform vec4 u_color;

void main() {
    color = texture(u_texture, itpl_uv) * u_color;
}
