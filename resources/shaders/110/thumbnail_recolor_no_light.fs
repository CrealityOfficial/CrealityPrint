#version 110

uniform sampler2D u_lit;
uniform sampler2D u_no_light;
uniform vec2      u_inv_size;
uniform int       u_color_count;
uniform vec3      u_colors[32];
uniform float     u_emission_factor;

void main()
{
    vec2 uv = gl_FragCoord.xy * u_inv_size;

    vec4 p = texture2D(u_lit, uv);
    vec4 n = texture2D(u_no_light, uv);

    float a_nl_f = floor(n.a * 255.0 + 0.5);
    int   a_nl   = int(a_nl_f);
    int   idx    = 255 - a_nl;
    if (idx < 0 || idx >= u_color_count) {
        gl_FragColor = p;
        return;
    }

    vec3 t    = u_colors[idx];
    vec3 prgb = p.rgb;
    vec3 nrgb = n.rgb;

    vec3 d = min(nrgb, prgb);
    vec3 s = prgb - d;

    float eps    = 1e-6;
    float ix_sum = 0.0;
    int   ix_n   = 0;
    if (nrgb.r > eps) { ix_sum += d.r / nrgb.r; ++ix_n; }
    if (nrgb.g > eps) { ix_sum += d.g / nrgb.g; ++ix_n; }
    if (nrgb.b > eps) { ix_sum += d.b / nrgb.b; ++ix_n; }
    float intensity_x = (ix_n > 0) ? (ix_sum / float(ix_n)) : 0.0;
    float intensity_y = (s.r + s.g + s.b) / 3.0;

    float brightness = dot(t, vec3(0.299, 0.587, 0.114));
    bool  is_black   = (brightness < 0.01);

    // Keep the original behavior: push a small base light into black filaments.
    if (is_black) {
        float base_light = 0.02;
        if (intensity_y <= 0.0)
            intensity_y = intensity_x * 0.1 + base_light;
        else if (intensity_y >= 0.001)
            intensity_y = intensity_x * 0.1 + 0.03;
    }
    if (intensity_y < 0.0)
        intensity_y = 0.0;

    // Preserve perceived brightness for non-black colors: (diffuse + emission) ~= 1 when intensity_x == 1.
    float emission       = clamp(u_emission_factor, 0.0, 1.0);
    float diffuse_coef   = is_black ? 1.0 : (1.0 - emission);
    float highlight_coef = is_black ? 0.8 : 1.2;

    vec3 out_rgb = t * intensity_x * diffuse_coef + vec3(intensity_y * highlight_coef) + t * emission;
    out_rgb      = clamp(out_rgb, 0.0, 1.0);
    gl_FragColor = vec4(out_rgb, p.a);
}
