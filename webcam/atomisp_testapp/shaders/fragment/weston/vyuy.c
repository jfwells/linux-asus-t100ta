uniform sampler2D u_textureYUV;
varying  vec2 texcoord;
varying  vec2 texsize;
void main(void)
{
    float y, u, v;
    vec4 resultcolor;
    vec4 raw = texture2D(u_textureYUV, texcoord);
    
    if (fract(texcoord.x * texsize.x ) < 0.5)
        raw.a = raw.g;
    
    u = raw.b-0.5;
    v = raw.r-0.5;
    y = 1.1643*(raw.a-0.0625);
    resultcolor.r = (y+1.5958*(v));
    resultcolor.g = (y-0.39173*(u)-0.81290*(v));
    resultcolor.b = (y+2.017*(u));
    resultcolor.a = 1.0;
    
    gl_FragColor=resultcolor;
}