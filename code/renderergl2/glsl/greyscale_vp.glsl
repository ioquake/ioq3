attribute vec4 attr_Position;
attribute vec2 attr_TexCoord0;
varying   vec2 var_TexCoords;

void main() {
    vec2 clipXY = (attr_Position.xy * r_FBufScale) * 2.0 - 1.0;
    clipXY.y = -clipXY.y;

    gl_Position   = vec4(clipXY, 0.0, 1.0);
    var_TexCoords = attr_TexCoord0;
}
