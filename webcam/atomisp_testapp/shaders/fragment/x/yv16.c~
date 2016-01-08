uniform sampler2D u_textureY;
uniform sampler2D u_textureU;
uniform sampler2D u_textureV;
varying mediump vec2 texcoord;
varying mediump vec2 texsize;
void main(void) 
{
    highp float y, u, v;
    lowp vec4 resultcolor;
    y=texture2D(u_textureY,texcoord).r;
    u=texture2D(u_textureU,texcoord).r;
    v=texture2D(u_textureV,texcoord).r;
    
    u = u-0.5;
    v = v-0.5;
    y = 1.1643*(y-0.0625);
    resultcolor.r = (y+1.5958*(v));
    resultcolor.g = (y-0.39173*(u)-0.81290*(v));
    resultcolor.b = (y+2.017*(u));
    resultcolor.a = 1.0;
    
    gl_FragColor=resultcolor;
}
