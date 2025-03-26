// Entradas
$input v_color0, v_color1, v_fog, v_refl, v_texcoord0, v_lightmapUV, v_extra

#include <bgfx_shader.sh>
#include <newb/main.sh>

// Función para detectar el tiempo en el mundo
vec4 worldTimeDetection(vec3 v_FogColor, vec3 v_FogControl) {
  // Detección de tiempo dinámico
  float day = pow(max(min(1.0 - v_FogColor.r * 1.2, 1.0), 0.0), 0.4);
  float night = pow(max(min(1.0 - v_FogColor.r * 1.5, 1.0), 0.0), 1.2);
  float dusk = max(v_FogColor.r - v_FogColor.b, 0.0);
  float rain = mix(smoothstep(0.66, 0.3, v_FogControl.x), 0.0, step(v_FogControl.x, 0.0));

  return vec4(dusk, day, night, rain);
}

// Declaración de texturas
SAMPLER2D_AUTOREG(s_MatTexture);
SAMPLER2D_AUTOREG(s_SeasonsTexture);
SAMPLER2D_AUTOREG(s_LightMapTexture);

void main() {
  #if defined(DEPTH_ONLY_OPAQUE) || defined(DEPTH_ONLY) || defined(INSTANCING)
    gl_FragColor = vec4(1.0,1.0,1.0,1.0);
    return;
  #endif

  // Obtener la textura base y el color
  vec4 diffuse = texture2D(s_MatTexture, v_texcoord0);
  vec4 color = v_color0;

  // Prueba de alfa
  #ifdef ALPHA_TEST
    if (diffuse.a < 0.6) {
      discard;
    }
  #endif

  // Si la opción de estaciones está habilitada, mezcla la textura de estaciones
  #if defined(SEASONS) && (defined(OPAQUE) || defined(ALPHA_TEST))
    diffuse.rgb *= mix(vec3(1.0, 1.0, 1.0), texture2D(s_SeasonsTexture, v_color1.xy).rgb * 2.0, v_color1.z);
  #endif

  // Efecto de brillo
  vec3 glow = nlGlow(s_MatTexture, v_texcoord0, v_extra.a);

  // Aplica el efecto de difuso
  diffuse.rgb *= diffuse.rgb;

  // Obtener la detección del tiempo del mundo
  vec4 worldTime = worldTimeDetection(v_fog.rgb, v_extra.xyz);
  vec4 diff2 = texture2D(s_MatTexture, v_texcoord0);
  float dif2 = (diff2.r + diff2.g + diff2.b) / 3.0;
  float hl = smoothstep(0.57, 1.0, dif2);

  // Modificar la intensidad de la luz según el tiempo del mundo
  hl *= mix(1.0, 0.3, worldTime.z);
  hl *= mix(1.0, 0.6, worldTime.x);

  hl *= 1.0;

  // Ajustar el color difuso con la intensidad de la luz
  diffuse.rgb += 1.03 * hl;
  diffuse.a += 0.53 * hl;

  // Modificar el tinte de luz con la textura de mapa de luz
  vec3 lightTint = texture2D(s_LightMapTexture, v_lightmapUV).rgb;
  lightTint = mix(lightTint.bbb, lightTint * lightTint, 0.35 + 0.65 * v_lightmapUV.y * v_lightmapUV.y * v_lightmapUV.y);

  color.rgb *= lightTint;

  #if defined(TRANSPARENT) && !(defined(SEASONS) || defined(RENDER_AS_BILLBOARDS))
    if (v_extra.b > 0.9) {
      diffuse.rgb = vec3_splat(1.0 - NL_WATER_TEX_OPACITY * (1.0 - diffuse.b * 1.8));
      diffuse.a = color.a;
    }
  #else
    diffuse.a = 1.0;
  #endif
  
  // Ajustar la sombra usando smoothstep
  vec2 uvl = v_lightmapUV;
  float shadowmap = smoothstep(0.915, 0.890, uvl.y);

  // CORRECCIÓN: Se usa vec3 en lugar de float3
  diffuse.rgb *= mix(vec3(1.0), vec3(0.3, 0.4, 0.425), shadowmap);

  // Agregar un efecto de luz adicional
  diffuse.rgb += diffuse.rgb * (vec3(1.5, 0.5, 0.0) * 1.15) * pow(uvl.x * 1.2, 6.0);

  // Multiplicar por el color base
  diffuse.rgb *= color.rgb;
  diffuse.rgb += glow;

  // Efecto de reflexión si está habilitado
  if (v_extra.b > 0.9) {
    diffuse.rgb += v_refl.rgb * v_refl.a;
  } else if (v_refl.a > 0.0) {
    // Efecto reflectante - solo en el plano xz
    float dy = abs(dFdy(v_extra.g));
    if (dy < 0.0002) {
      float mask = v_refl.a * (clamp(v_extra.r * 10.0, 8.2, 8.8) - 7.8);
      diffuse.rgb *= 1.0 - 0.6 * mask;
      diffuse.rgb += v_refl.rgb * mask;
    }
  }

  // Aplicar niebla
  diffuse.rgb = mix(diffuse.rgb, v_fog.rgb, v_fog.a);

  // Corrección de color
  diffuse.rgb = colorCorrection(diffuse.rgb);

  // Finalizar color del fragmento
  gl_FragColor = diffuse;
}
