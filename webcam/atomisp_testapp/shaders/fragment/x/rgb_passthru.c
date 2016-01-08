uniform sampler2D u_textureRGBP;
varying mediump vec2 texcoord;
varying mediump vec2 texsize;
void main(void) {
    lowp vec4 resultcolor;
    lowp vec4 raw = texture2D(u_textureRGBP, texcoord);
    resultcolor.r = raw.r;
    resultcolor.g = raw.g;
    resultcolor.b = raw.b;
    resultcolor.a = 1.0;
    gl_FragColor = resultcolor;
}
