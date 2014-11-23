
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


#define MaxSteps 40
#define MinimumDistance 0.00005
#define normalDistance     0.001

#define Iterations 7
#define Scale  3.0
#define FieldOfView 3.0
#define NonLinearPerspective 1.0
#define DebugNonlinearPerspective false

#define Ambient 0.32184
#define Diffuse 0.8
#define LightDir vec3(1.0)
#define LightColor vec3(1.0,0.7,0.358824)
#define LightDir2 vec3(1.0,-1.0,1.0)
#define LightColor2 vec3(0.5,0.633333,0.2)
#define Offset vec3(0.92858,0.92858,0.32858)

vec2 rotate(vec2 v, float a) {
	return vec2(cos(a)*v.x + sin(a)*v.y, -sin(a)*v.x + cos(a)*v.y);
}

// Two light sources. No specular
vec3 getLight(in vec3 color, in vec3 normal, in vec3 dir) {
	vec3 lightDir = normalize(LightDir);
	float diffuse = max(0.0,dot(-normal, lightDir)); // Lambertian

	vec3 lightDir2 = normalize(LightDir2);
	float diffuse2 = max(0.0,dot(-normal, lightDir2)); // Lambertian

	return
	(diffuse*Diffuse)*(LightColor*color) +
	(diffuse2*Diffuse)*(LightColor2*color);
}


float DE(in vec3 z)
{
	z  = abs(1.0-mod(z,2.0));
	float time = 0.3*iGlobalTime;
	float d = 1.0;
	for (int n = 0; n < Iterations; n++)
	{
		z.xy = rotate(z.xy,2.0*cos( time/8.0));
		z.yz = rotate(z.yz,2.0*sin( time/200.0));
		z = abs(z);
		if (z.x<z.y){ z.xy = z.yx;}
		if (z.x< z.z){ z.xz = z.zx;}
		if (z.y<z.z){ z.yz = z.zy;}
		z = Scale*z-Offset*(Scale-1.0);
		if( z.z<-0.5*Offset.z*(Scale-1.0))  z.z+=Offset.z*(Scale-1.0);
		d = min(d, length(z) * pow(Scale, float(-n)-1.0));
	}
	return d-normalDistance;
}

// Finite difference normal
vec3 getNormal(in vec3 pos) {
	vec3 e = vec3(0.0,normalDistance,0.0);
	return normalize(vec3(
			DE(pos+e.yxx)-DE(pos-e.yxx),
			DE(pos+e.xyx)-DE(pos-e.xyx),
			DE(pos+e.xxy)-DE(pos-e.xxy)
			)
		);
}

vec3 getColor(vec3 normal, vec3 pos) {
	return vec3(0.7, 0.8, 1.0);
}

vec3 hash3( float n )
{
	return fract(sin(vec3(n,n+1.0,n+2.0))*vec3(43758.5453123,22578.1459123,19642.3490423));
}

vec3 noise( in float x )
{
	float p = floor(x);
	float f = fract(x);
	f = f*f*(3.0-2.0*f);
	return mix( hash3(p+0.0), hash3(p+1.0),f);
}


mat4 rotationMat( in vec3 xyz )
{
	vec3 si = sin(xyz);
	vec3 co = cos(xyz);
	return mat4( co.y*co.z,                co.y*si.z,               -si.y,       0.0,
				 si.x*si.y*co.z-co.x*si.z, si.x*si.y*si.z+co.x*co.z, si.x*co.y,  0.0,
				 co.x*si.y*co.z+si.x*si.z, co.x*si.y*si.z-si.x*co.z, co.x*co.y,  0.0,
				 0.0,                      0.0,                      0.0,        1.0 );
}


vec4 rayMarch(in vec3 from, in vec3 dir)
{
	float totalDistance = 0.0;
	float distance;
	int steps = 0;
	vec3 pos;
	for (int i=0; i<MaxSteps; i++)
	{
		pos = from + totalDistance * dir;
		distance = DE(pos);
		totalDistance += distance;
		if (distance < MinimumDistance) break;
		steps = i;
	}
	if (distance>0.1) return exp(2.0*distance)*vec4(1.0, 0.5, 0.0, 1.0);
	float smoothStep =   float(steps) + distance/MinimumDistance;
	float ao = 1.1-smoothStep/float(MaxSteps);
	vec3 normal = getNormal(pos-dir*normalDistance*3.0);
	vec3 color = getColor(normal, pos);
	vec3 light = getLight(color, normal, dir);
	color = (color*Ambient+light)*ao;
	return vec4(color,1.0);
}

void main(void)
{
	vec2 ndc = vec2((gl_FragCoord.x-viewportX)/viewportW,
					(gl_FragCoord.y-viewportY)/viewportH);

	float Hup    = znear * UpTan;
	float Hdown  = znear * DownTan;
	float Wleft  = znear * LeftTan;
	float Wright = znear * RightTan;

	vec2 ll = vec2(-Wleft, -Hdown);
	vec2 ur = vec2(Wright, Hup);
	vec2 P = ll + ndc*(ur-ll);

	vec3 rd = normalize( -znear*camBasisZ + P.x*camBasisX + P.y*camBasisY );


	vec2 q = ndc;
	vec2 p = -1.0 + 2.0 * q;
	p.x *= iResolution.x/iResolution.y;
	vec2 m = vec2(0.5);

	// animation
	float time = iGlobalTime;
	time += 15.0*smoothstep(  15.0, 25.0, iGlobalTime );
	time += 20.0*smoothstep(  65.0, 80.0, iGlobalTime );
	time += 35.0*smoothstep( 105.0, 135.0, iGlobalTime );
	time += 20.0*smoothstep( 165.0, 180.0, iGlobalTime );
	time += 40.0*smoothstep( 220.0, 290.0, iGlobalTime );
	time +=  5.0*smoothstep( 320.0, 330.0, iGlobalTime );
	float time1 = (time-10.0)*1.5 - 167.0;
	float time2 = time;
	const float s = 1.1;
	mat4 mm;
	mm = rotationMat( vec3(0.4,0.1,3.4) +
					  0.15*sin(0.1*vec3(0.40,0.30,0.61)*time1) +
					  0.15*sin(0.1*vec3(0.11,0.53,0.48)*time1));
	mm[0].xyz *= s;
	mm[1].xyz *= s;
	mm[2].xyz *= s;
	mm[3].xyz = vec3( 0.15, 0.05, -0.07 ) + 0.05*sin(vec3(0.0,1.0,2.0) + 0.2*vec3(0.31,0.24,0.42)*time1);

	// camera
	float an = 1.0 + 0.1*time2 - 6.2*m.x;
	float cr = 0.15*sin(0.2*time2);
	float cr2 = 0.6*smoothstep(10.0,20.0,time2);
	vec3 dro = cr2 * vec3(sin(an),0.25,cos(an));

	vec3 ro;
	if (iPaused==0)
	{
		ro = 0.05*iGlobalTime*vec3(0.0,0.0,1.0) + 0.01*dro;
	}
	else
	{
		ro = camPos;
	}

	gl_FragColor = rayMarch(ro, rd);
}


