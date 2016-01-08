uniform sampler2D u_textureRGBP;
varying vec2 texcoord;
varying vec2 texsize;
void main(void) {
    vec4 resultcolor;
    vec4 raw = texture2D(u_textureRGBP, texcoord);
    resultcolor.r = raw.r;
    resultcolor.g = raw.g;
    resultcolor.b = raw.b;
    resultcolor.a = 1.0;
    gl_FragColor = resultcolor;
}
