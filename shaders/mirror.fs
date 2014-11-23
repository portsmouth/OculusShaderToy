



uniform float viewportW;
uniform float viewportH;
uniform float viewportX;
uniform float viewportY;

uniform vec3 camPos;
uniform vec3 camBasisX;
uniform vec3 camBasisY;
uniform vec3 camBasisZ;

uniform float UpTan;
uniform float DownTan;
uniform float LeftTan;
uniform float RightTan;

uniform float znear;
uniform float zfar;

uniform float iGlobalTime;
uniform int iPaused;
uniform vec2 iResolution;

#define REFLECTIONS 16

#define FOG_STRENGTH 1.0
#define REFLECTION_STRENGTH 100000.0
#define COLOR_STRENGTH 10.0
#define BRIGHTNESS 10.0

struct ray {
	vec3 p;
	vec3 d;
};

struct material {
	vec3 color;
	vec3 emmision;
	float diffusion;
};

struct hit {
	vec3 p;
	vec3 n;
	float t;
	material m;
};

void plane (vec3 v, float f, ray r, material m, inout hit h) {
	float t = (f - dot (v, r.p)) / dot (v, r.d);

	if (t < 0.0)
		return;

	if (t < h.t) {
		h.t = t;
		h.p = r.p + r.d * t;
		h.n = /*normalize*/ (faceforward (v, v, r.d));

		h.m = m;
	}
}

void sphere (vec3 v, float f, ray r, material m, inout hit h) {
	vec3 d = r.p - v;

	float a = dot (r.d, r.d);
	float b = dot (r.d, d);
	float c = dot (d, d) - f * f;

	float g = b*b - a*c;

	if (g < 0.0)
		return;

	float t = (-sqrt (g) - b) / a;

	if (t < 0.0)
		return;

	if (t < h.t) {
		h.t = t;
		h.p = r.p + r.d * t;
		h.n = (h.p - v) / f;

		h.m = m;
	}
}

hit scene (ray r) {
	hit h;
	h.t = 1e20;

	material m0 = material (vec3 (1), vec3 (0), 0.5);

	plane	(vec3 ( 1.0, 0.1, 0.0), 3.0, r, m0, h);
	plane	(vec3 ( 1.0, 0.4, 0.0),-3.0, r, m0, h);
	plane	(vec3 ( 0.0, 1.0, 0.0), 0.0, r, m0, h);
	plane	(vec3 ( 0.0, 1.0, 0.0), 5.0, r, m0, h);
	plane	(vec3 ( 0.0, 0.0, 1.0), 5.0, r, m0, h);
	plane	(vec3 ( 1.0, 0.0, 1.0),-6.0, r, m0, h);
	sphere	(vec3 (-1.0, sin (iGlobalTime) + 3.0,0), 1.0, r, m0, h);
	sphere	(vec3 ( 1.0, 1.0, 2.0), 1.0, r, m0, h);

	h.m.color *= h.n * h.n;

	return h;
}

vec3 surface (ray r)
{
	vec3 color = vec3 (0);
	float depth = 0.0;
	float l = 0.0;

	for (int i = 0; i < REFLECTIONS; i++)
	{
		hit h = scene (r);
		float c = dot (h.n, r.d);
		depth += (1.0 / REFLECTION_STRENGTH) + h.t * FOG_STRENGTH;
		r = ray (h.p + h.n * 0.0001, reflect (normalize (r.d), h.n));
		float d = 1.0 / (depth * depth);
		color = (color + c*c*d) * (1.0 - h.m.color * d * COLOR_STRENGTH);
	}

	return color * BRIGHTNESS;
}

void main (void) {

	vec2 ndc = vec2((gl_FragCoord.x-viewportX)/viewportW,
					(gl_FragCoord.y-viewportY)/viewportH);

	float Hup    = znear * UpTan;
	float Hdown  = znear * DownTan;
	float Wleft  = znear * LeftTan;
	float Wright = znear * RightTan;

	vec2 ll = vec2(-Wleft, -Hdown);
	vec2 ur = vec2(Wright, Hup);
	vec2 P = ll + ndc*(ur-ll);

	vec3 direction = normalize( -znear*camBasisZ + P.x*camBasisX + P.y*camBasisY );
	vec3 origin = camPos;

	ray r;
	r.d = direction;
	r.p = origin;

	gl_FragColor = vec4 (surface (r) * (1.3 - max (abs (ndc.x), abs (ndc.y * 1.5))), 1.0);
}
