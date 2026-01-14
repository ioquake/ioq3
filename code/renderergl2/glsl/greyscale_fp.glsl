uniform sampler2D u_TextureMap;
uniform float     u_Greyscale;
varying vec2      var_TexCoords;

const vec3 LUMA = vec3(0.2125, 0.7154, 0.0721);

void main() {
    vec4 color = texture2D(u_TextureMap, var_TexCoords);
    if (u_Greyscale > 0.0) {
        float y = dot(color.rgb, LUMA);
        color.rgb = mix(color.rgb, vec3(y), clamp(u_Greyscale, 0.0, 1.0));
    }
    gl_FragColor = color;
}