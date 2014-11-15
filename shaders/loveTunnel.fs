#ifdef GL_ES
precision highp float;
#endif



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
uniform vec2 iResolution;

#define DELTA				0.01
#define RAY_LENGTH_MAX		50.0
#define RAY_STEP_MAX		50
#define LIGHT				vec3 (0.5, 0.0, -2.0)
#define AMBIENT				0.5
#define SPECULAR_POWER		4.0
#define SPECULAR_INTENSITY	0.2
#define FADE_POWER			3.0
#define GAMMA				(1.0 / 2.2)
#define M_PI				3.1415926535897932384626433832795

vec3 vRotateZ (in vec3 p, in float angle) {
	float c = cos (angle);
	float s = sin (angle);
	return vec3 (c * p.x + s * p.y, c * p.y - s * p.x, p.z);
}

float sphere (in vec3 p, in float r) {
	return length (p) - r;
}

float box (in vec3 p, in vec3 b, in float r) {
	return length (max (abs (p) - b + r, 0.0)) - r;
}

float torusZ (in vec3 p, in float r1, in float r2) {
	vec2 q = vec2 (length (p.xy) - r1, p.z);
	return length (q) - r2;
}

float cylinderZ (in vec3 p, in float r) {
	return length (p.xy) - r;
}

vec3 twistZ (in vec3 p, in float k, in float angle) {
	return vRotateZ (p, angle + k * p.z);
}

float fixDistance (in float d, in float correction, in float k) {
	correction = max (correction, 0.0);
	k = clamp (k, 0.0, 1.0);
	return min (d, max ((d - DELTA) * k + DELTA, d - correction));
}

float getDistance (in vec3 p, out vec4 q) {

	// Global deformation
	p += vec3 (2.0 * sin (p.z * 0.2 + iGlobalTime * 2.0), sin (p.z * 0.1 + iGlobalTime), 0.0);

	// Cylinder
	q.xyz = p;
	q.w = -1.0;
	float d = fixDistance (-cylinderZ (q.xyz, 4.0) + 0.5 * sin (atan (q.y, q.x) * 6.0) * sin (q.z), 0.4, 0.8);

	// Twisted boxes
	vec3 q_;
	q_.xy = mod (p.xy, 5.0) - 0.5 * 5.0;
	q_.z = mod (p.z, 12.0) - 0.5 * 12.0;
	q_ = twistZ (q_, 1.0, iGlobalTime);
	float d_ = fixDistance (box (q_, vec3 (0.6, 0.6, 1.5), 0.3), 0.4, 0.8);
	if (d_ < d) {
		q.xyz = q_;
		d = d_;
	}

	// Rotating spheres
	q_ = p;
	q_.z += 12.0;
	q_ = vRotateZ (q_, sin (iGlobalTime * 4.0));
	q_.xy = mod (q_.xy, 4.5) - 0.5 * 4.5;
	q_.z = mod (q_.z, 24.0) - 0.5 * 24.0;
	d_ = sphere (q_, 0.5);
	if (d_ < d) {
		q.xyz = q_;
		d = d_;
	}

	// Torus
	q_ = p;
	q_.z = mod (q_.z + 12.0, 24.0) - 0.5 * 24.0;
	d_ = torusZ (q_, 3.5, 0.4);
	if (d_ < d) {
		q.xyz = q_;
		d = d_;
	}

	// Flow of boxes and spheres
	q_ = p;
	q_.z += iGlobalTime * 20.0;
	const float spacing = 0.5;
	const float stepCount = 3.0;
	const float period = spacing * stepCount;
	for (float step = 0.0; step < stepCount; ++step) {
		float k1 = floor (q_.z / period + 0.5);
		float k2 = k1 * stepCount + step;
		vec3 qq = q_ - vec3 (0.4 * sin (k2), 0.4 * sin (k2 * 13.0), period * k1);
		if (mod (k2, 2.0) > 0.5) {
			d_ = box (vRotateZ (qq, k2), vec3 (0.08), 0.01);
		} else {
			d_ = sphere (qq, 0.08);
		}
		if (d_ < d) {
			q.xyz = qq;
			q.w = 1.0;
			d = d_;
		}
		q_.z += spacing;
	}

	// Final distance
	return d;
}

vec3 getNormal (in vec3 p) {
	vec4 q;
	const vec2 h = vec2 (DELTA, 0.0);
	return normalize (vec3 (
		getDistance (p + h.xyy, q) - getDistance (p - h.xyy, q),
		getDistance (p + h.yxy, q) - getDistance (p - h.yxy, q),
		getDistance (p + h.yyx, q) - getDistance (p - h.yyx, q)
	));
}

vec3 rgb (in vec3 hsv) {
	hsv.yz = clamp (hsv.yz, 0.0, 1.0);
	return hsv.z * (1.0 + hsv.y * clamp (abs (fract (hsv.xxx + vec3 (0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0) - 2.0, -1.0, 0.0));
}

void main () {

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
	vec3 origin = vec3 (0.0, 0.0, iGlobalTime * 6.0);//camPos;

	// Ray marching
	vec3 p = origin;
	vec4 q;
	float rayLength = 0.0;
	int rayStepCounter = 0;
	for (int rayStep = 0; rayStep < RAY_STEP_MAX; ++rayStep) {
		float dist = getDistance (p, q);
		rayLength += dist;
		if (dist < DELTA || rayLength > RAY_LENGTH_MAX) {
			break;
		}
		p += dist * direction;
		++rayStepCounter;
	}

	// Compute the fragment color
	vec3 color;
	float bass = 1.0; //texture2D (iChannel0, vec2 (0.0)).x;
	if (rayLength > RAY_LENGTH_MAX) {
		color = vec3 (0.0);
	} else {

		// Object color
		vec3 normal = getNormal (p);
		float hue = (p.z + iGlobalTime) * 0.1;
		if (q.w < 0.0) {
			float treble = 0.5;//texture2D (iChannel0, vec2 (0.8, 0.0)).x;
			float saturation = 0.8 + (0.2 + 0.8 * treble) * 0.4 * sin (q.x * 10.0) * sin (q.y * 10.0) * sin (q.z * 10.0);
			float value = 0.2 + 0.8 * bass;
			color = rgb (vec3 (hue, saturation, value));
		} else {
			color = rgb (vec3 (hue, 1.0, 1.0));
		}

		// Lighting
		vec3 lightDirection = normalize (LIGHT);
		vec3 reflectDirection = reflect (direction, normal);
		float diffuse = max (0.0, dot (normal, lightDirection));
		float specular = pow (max (0.0, dot (reflectDirection, lightDirection)), SPECULAR_POWER) * SPECULAR_INTENSITY;
		float fade = pow (1.0 - rayLength / RAY_LENGTH_MAX, FADE_POWER);
		color = ((AMBIENT + diffuse) * color + specular) * fade;

		// Special effect
		color *= max (1.0, 10.0 * sin (p.z * 0.1 - iGlobalTime * 4.0) - 7.0);

		// Gamma correction
		color = pow (color, vec3 (GAMMA));
	}

	// Another special effect
	color.r = mix (color.r, float (rayStepCounter) / float (RAY_STEP_MAX / 2), 1.0 - bass);

	// Set the fragment color
	gl_FragColor = vec4 (color, 1.0);
}
