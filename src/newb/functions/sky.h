#ifndef SKY_H
#define SKY_H

#include "detection.h"
#include "noise.h"

struct nl_skycolor {
  vec3 zenith;
  vec3 horizon;
  vec3 horizonEdge;
};

// rainbow spectrum
vec3 spectrum(float x) {
    vec3 s = vec3(x-0.5, x, x+0.5);
    s = smoothstep(1.0,0.0,abs(s));
    return s*s;
}

vec3 getUnderwaterCol(vec3 FOG_COLOR) {
  return 2.0*NL_UNDERWATER_TINT*FOG_COLOR*FOG_COLOR;
}

vec3 getEndZenithCol() {
  return NL_END_ZENITH_COL;
}

vec3 getEndHorizonCol() {
  return NL_END_HORIZON_COL;
}

// values used for getting sky colors
vec3 getSkyFactors(vec3 FOG_COLOR) {
  vec3 factors = vec3(
    max(FOG_COLOR.r*0.6, max(FOG_COLOR.g, FOG_COLOR.b)), // intensity val
    1.5*max(FOG_COLOR.r-FOG_COLOR.b, 0.0), // viewing sun
    min(FOG_COLOR.g, 0.26) // rain brightness
  );

  factors.z *= factors.z;

  return factors;
}

vec3 getZenithCol(float rainFactor, vec3 FOG_COLOR, vec3 fs) {
  vec3 zenithCol = NL_NIGHT_ZENITH_COL*(1.0-FOG_COLOR.b);
  zenithCol += NL_DAWN_ZENITH_COL*((0.7*fs.x*fs.x) + (0.4*fs.x) + fs.y);
  zenithCol = mix(zenithCol, (0.7*fs.x*fs.x + 0.3*fs.x)*NL_DAY_ZENITH_COL, fs.x*fs.x);
  zenithCol = mix(zenithCol*(1.0+0.5*rainFactor), NL_RAIN_ZENITH_COL*fs.z*13.2, rainFactor);

  return zenithCol;
}

vec3 getHorizonCol(float rainFactor, vec3 FOG_COLOR, vec3 fs) {
  vec3 horizonCol = NL_NIGHT_HORIZON_COL*(1.0-FOG_COLOR.b); 
  horizonCol += NL_DAWN_HORIZON_COL*(((0.7*fs.x*fs.x) + (0.3*fs.x) + fs.y)*1.9); 
  horizonCol = mix(horizonCol, 2.0*fs.x*NL_DAY_HORIZON_COL, fs.x*fs.x);
  horizonCol = mix(horizonCol, NL_RAIN_HORIZON_COL*fs.z*19.6, rainFactor);

  return horizonCol;
}

// tinting on horizon col
vec3 getHorizonEdgeCol(vec3 horizonCol, float rainFactor, vec3 FOG_COLOR) {
  float val = 2.1*(1.1-FOG_COLOR.b)*FOG_COLOR.g*(1.0-rainFactor);
  horizonCol *= vec3_splat(1.0-val) + NL_DAWN_EDGE_COL*val;

  return horizonCol;
}

// 1D sky with three color gradient
vec3 renderOverworldSky(nl_skycolor skycol, vec3 viewDir) {
  float h = 1.0-viewDir.y*viewDir.y;
  float hsq = h*h;
  if (viewDir.y < 0.0) {
    hsq = 0.4 + 0.6*hsq*hsq;
  }

  // gradient 1  h^16
  // gradient 2  h^8 mix h^2
  float gradient1 = hsq*hsq;
  gradient1 *= gradient1;
  float gradient2 = 0.6*gradient1 + 0.4*hsq;
  gradient1 *= gradient1;

  vec3 sky = mix(skycol.horizon, skycol.horizonEdge, gradient1);
  sky = mix(skycol.zenith, skycol.horizon, gradient2);

  return sky;
}

// sunrise/sunset bloom
vec3 getSunBloom(float viewDirX, vec3 horizonEdgeCol, vec3 FOG_COLOR) {
  float factor = FOG_COLOR.r/(0.01 + length(FOG_COLOR));
  factor *= factor;
  factor *= factor;

  float spread = smoothstep(0.0, 1.0, abs(viewDirX));
  float sunBloom = spread*spread;
  sunBloom = 0.5*spread + sunBloom*sunBloom*sunBloom*1.5;

  return NL_MORNING_SUN_COL*horizonEdgeCol*(sunBloom*factor*factor);
}


vec3 renderEndSky(vec3 horizonCol, vec3 zenithCol, vec3 v, float t) {
    vec3 sky = vec3(0.02, 0.01, 0.04);

    // Ajuste para elevar las púas y aplicar distorsión
    v.y = smoothstep(-0.4, 1.0, abs(v.y));  // Ajuste para elevar más las púas

    // Suavizar el ángulo para eliminar la línea del medio
    float a = atan(v.x, v.z);
    a = mod(a + 6.14159265, 10.28318531); // Convierte el rango de [-π, π] a [0, 2π] de forma continua

    // Configuración base para las púas
    float puaAncho = 20.0; // Mayor densidad
    float puaLargo = 0.4;  // Más largas

    // Primera capa de púas
    float s1 = sin(a * puaAncho + t * 0.2); // Velocidad de Puas
    s1 *= s1;
    s1 *= puaLargo * sin(a * 30.0 - 0.4 * t);
    float g1 = smoothstep(1.0 - s1, -4.2, v.y);
    float f1 = (4.5 * g1 + 1.0 * smoothstep(1.0, -0.8, v.y));

    // Segunda capa de púas
    float s2 = sin(a * (puaAncho + 15.0) + t * 0.2);
    s2 *= s2;
    s2 *= (puaLargo * 1.2) * sin(a * 0.0 - 0.8 * t);
    float g2 = smoothstep(1.0 - s2, -6.0, v.y);
    float f2 = (2.0 * g2 + 0.8 * smoothstep(1.0, -0.4, v.y));

    // Tercera capa de púas (más difusas)
    float s3 = sin(a * (puaAncho - 5.0) + t * 0.2);
    s3 *= s3;
    s3 *= (puaLargo * 0.6) * sin(a * 20.0 - 0.2 * t);
    float g3 = smoothstep(1.08 - s3, -3.8, v.y);
    float f3 = (18.5 * g3 + 0.5 * smoothstep(2.0, -0.4, v.y));

    // Colores dinámicos
    vec3 colorBase1 = vec3(0.1, 0.3, 0.8); // Púrpura intenso
    vec3 colorBase2 = vec3(0.1, 0.4, 0.9); // Azul-violeta
    vec3 colorBase3 = vec3(0.2, 0.4, 1.0); // Magenta-rosado

    vec3 puaColor1 = mix(colorBase1, colorBase2, sin(t * 0.9) * 0.5 + 0.5);
    vec3 puaColor2 = mix(colorBase2, colorBase3, cos(t * 0.4) * 0.5 + 0.5);
    vec3 puaColor3 = mix(colorBase1, colorBase3, sin(t * 0.3) * 0.5 + 0.5);

    // Gradiente de fond
    vec3 upperGradientCol = mix(zenithCol, vec3(0.048, 0.02, 0.04), smoothstep(0.6, 1.0, v.y));
    vec3 lowerGradientCol = mix(horizonCol, vec3(0.8, 0.2, 0.28), smoothstep(0.0, 0.5, v.y));

    // Mezcla de gradientes y púas con los colores
    sky += mix(upperGradientCol, lowerGradientCol, pow(f1, 2.0));  // Gradiente combinado
    sky += puaColor1 * f1 * 0.2;  // Capa de púas 1
    sky += puaColor2 * f2 * 0.2;  // Capa de púas 2
    sky += puaColor3 * f3 * 0.08; // Capa de púas 3

    return sky;
}

vec3 nlRenderSky(nl_skycolor skycol, nl_environment env, vec3 viewDir, vec3 FOG_COLOR, float t) {
  vec3 sky;
  viewDir.y = -viewDir.y;

  if (env.end) {
    sky = renderEndSky(skycol.horizon, skycol.zenith, viewDir, t);
  } else {
    sky = renderOverworldSky(skycol, viewDir);
    #ifdef NL_RAINBOW
      sky += mix(NL_RAINBOW_CLEAR, NL_RAINBOW_RAIN, env.rainFactor)*spectrum((viewDir.z+0.6)*8.0)*max(viewDir.y, 0.0)*FOG_COLOR.g;
    #endif
    #ifdef NL_UNDERWATER_STREAKS
      if (env.underwater) {
        float a = atan2(viewDir.x, viewDir.z);
        float grad = 0.5 + 0.5*viewDir.y;
        grad *= grad;
        float spread = (0.5 + 0.5*sin(3.0*a + 0.2*t + 2.0*sin(5.0*a - 0.4*t)));
        spread *= (0.5 + 0.5*sin(3.0*a - sin(0.5*t)))*grad;
        spread += (1.0-spread)*grad;
        float streaks = spread*spread;
        streaks *= streaks;
        streaks = (spread + 3.0*grad*grad + 4.0*streaks*streaks);
        sky += 2.0*streaks*skycol.horizon;
      } else 
    #endif
    if (!env.nether) {
      sky += getSunBloom(viewDir.x, skycol.horizonEdge, FOG_COLOR);
    }
  }

  return sky;
}

// sky reflection on plane
vec3 getSkyRefl(nl_skycolor skycol, nl_environment env, vec3 viewDir, vec3 FOG_COLOR, float t) {
  vec3 refl = nlRenderSky(skycol, env, viewDir, FOG_COLOR, t);

  if (!(env.underwater || env.nether)) {
    float specular = smoothstep(0.7, 0.0, abs(viewDir.z));
    specular *= specular*viewDir.x;
    specular *= specular;
    specular += specular*specular*specular*specular;
    specular *= max(FOG_COLOR.r-FOG_COLOR.b, 0.0);
    refl += 5.0 * skycol.horizonEdge * specular * specular;
  }

  return refl;
}

// shooting star
vec3 nlRenderShootingStar(vec3 viewDir, vec3 FOG_COLOR, float t) {
  // transition vars
  float h = t / (NL_SHOOTING_STAR_DELAY + NL_SHOOTING_STAR_PERIOD);
  float h0 = floor(h);
  t = (NL_SHOOTING_STAR_DELAY + NL_SHOOTING_STAR_PERIOD) * (h-h0);
  t = min(t/NL_SHOOTING_STAR_PERIOD, 1.0);
  float t0 = t*t;
  float t1 = 1.0-t0;
  t1 *= t1; t1 *= t1; t1 *= t1;

  // randomize size, rotation, add motion, add skew
  float r = fract(sin(h0) * 43758.545313);
  float a = 6.2831*r;
  float cosa = cos(a);
  float sina = sin(a);
  vec2 uv = viewDir.xz * (6.0 + 4.0*r);
  uv = vec2(cosa*uv.x + sina*uv.y, -sina*uv.x + cosa*uv.y);
  uv.x += t1 - t;
  uv.x -= 2.0*r + 3.5;
  uv.y += viewDir.y * 3.0;

  // draw star
  float g = 1.0-min(abs((uv.x-0.95))*20.0, 1.0); // source glow
  float s = 1.0-min(abs(8.0*uv.y), 1.0); // line
  s *= s*s*smoothstep(-1.0+1.96*t1, 0.98-t, uv.x); // decay tail
  s *= s*s*smoothstep(1.0, 0.98-t0, uv.x); // decay source
  s *= 1.0-t1; // fade in
  s *= 1.0-t0; // fade out
  s *= 0.7 + 16.0*g*g;
  s *= max(1.0-FOG_COLOR.r-FOG_COLOR.g-FOG_COLOR.b, 0.0); // fade out during day
  return s*vec3(0.8, 0.9, 1.0);
}

// Galaxy stars - needs further optimization
vec3 nlRenderGalaxy(vec3 vdir, vec3 fogColor, nl_environment env, float t) {
  if (env.underwater) {
    return vec3_splat(0.0);
  }

  t *= NL_GALAXY_SPEED;

  // rotate space
  float cosb = sin(0.2*t);
  float sinb = cos(0.2*t);
  vdir.xy = mul(mat2(cosb, sinb, -sinb, cosb), vdir.xy);

  // noise
  float n0 = 0.5 + 0.5*sin(5.0*vdir.x)*sin(5.0*vdir.y - 0.5*t)*sin(5.0*vdir.z + 0.5*t);
  float n1 = noise3D(15.0*vdir + sin(0.85*t + 1.3));
  float n2 = noise3D(50.0*vdir + 1.0*n1 + sin(0.7*t + 1.0));
  float n3 = noise3D(200.0*vdir - 10.0*sin(0.4*t + 0.500));

  // stars
  n3 = smoothstep(0.04,0.3,n3+0.02*n2);
  float gd = vdir.x + 0.1*vdir.y + 0.1*sin(10.0*vdir.z + 0.2*t);
  float st = n1*n2*n3*n3*(1.0+70.0*gd*gd);
  st = (1.0-st)/(1.0+400.0*st);
  vec3 stars = (0.8 + 0.2*sin(vec3(8.0,6.0,10.0)*(2.0*n1+0.8*n2) + vec3(0.0,0.4,0.82)))*st;

  // glow
  float gfmask = abs(vdir.x)-0.15*n1+0.04*n2+0.25*n0;
  float gf = 1.0 - (vdir.x*vdir.x + 0.03*n1 + 0.2*n0);
  gf *= gf;
  gf *= gf*gf;
  gf *= 1.0-0.3*smoothstep(0.2, 0.3, gfmask);
  gf *= 1.0-0.2*smoothstep(0.3, 0.4, gfmask);
  gf *= 1.0-0.1*smoothstep(0.2, 0.1, gfmask);
  vec3 gfcol = normalize(vec3(n0, cos(2.0*vdir.y), sin(vdir.x+n0)));
  stars += (0.4*gf + 0.012)*mix(vec3(0.5, 0.5, 0.5), gfcol*gfcol, NL_GALAXY_VIBRANCE);

  stars *= mix(1.0, NL_GALAXY_DAY_VISIBILITY, min(dot(fogColor, vec3(0.5,0.7,0.5)), 1.0)); // maybe add day factor to env for global use?

  return stars*(1.0-env.rainFactor);
}

nl_skycolor nlUnderwaterSkyColors(float rainFactor, vec3 FOG_COLOR) {
  nl_skycolor s;
  s.zenith = getUnderwaterCol(FOG_COLOR);
  s.horizon = s.zenith;
  s.horizonEdge = s.zenith;
  return s;
}

nl_skycolor nlEndSkyColors(float rainFactor, vec3 FOG_COLOR) {
  nl_skycolor s;
  s.zenith = getEndZenithCol();
  s.horizon = getEndHorizonCol();
  s.horizonEdge = s.horizon;
  return s;
}

nl_skycolor nlOverworldSkyColors(float rainFactor, vec3 FOG_COLOR) {
  nl_skycolor s;
  vec3 fs = getSkyFactors(FOG_COLOR);
  s.zenith= getZenithCol(rainFactor, FOG_COLOR, fs);
  s.horizon= getHorizonCol(rainFactor, FOG_COLOR, fs);
  s.horizonEdge= getHorizonEdgeCol(s.horizon, rainFactor, FOG_COLOR);
  return s;
}

nl_skycolor nlSkyColors(nl_environment env, vec3 FOG_COLOR) {
  nl_skycolor s;
  if (env.underwater) {
    s = nlUnderwaterSkyColors(env.rainFactor, FOG_COLOR);
  } else if (env.end) {
    s = nlEndSkyColors(env.rainFactor, FOG_COLOR);
  } else {
    s = nlOverworldSkyColors(env.rainFactor, FOG_COLOR);
  }
  return s;
}

#endif
