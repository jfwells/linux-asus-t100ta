varying  vec2 texcoord;
varying  vec2 texsize;
attribute vec4 pos;
attribute vec2 itexcoord;
uniform mat4 modelviewProjection;
uniform vec2 u_texsize;
void main(void)
{
    texcoord = itexcoord;
    texsize = u_texsize;
    gl_Position = modelviewProjection * pos;
}