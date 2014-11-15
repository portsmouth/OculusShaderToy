
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


#define MaxSteps 40
#define MinimumDistance 0.0001
#define normalDistance 0.0002

#define Iterations 6
#define Scale 3.2
#define Offset vec3(0.5,0.5,0.5)

#define Ambient 0.72184
#define Diffuse 0.5
#define LightDir vec3(1.0)
#define LightColor vec3(1.0,1.0,0.858824)
#define LightDir2 vec3(1.0,-1.0,1.0)
#define LightColor2 vec3(0.0,0.333333,1.0)


vec2 rotate(vec2 v, float a)
{
	return vec2(cos(a)*v.x + sin(a)*v.y, -sin(a)*v.x + cos(a)*v.y);
}


float DE(in vec3 z)
{
	// Folding 'tiling' of 3D space;
	//z = abs(1.0-mod(z,2.0));

	float d = 1.0;
	for (int n = 0; n < Iterations; n++)
	{
		//z.xy = rotate(z.xy,4.0+2.0*cos(iGlobalTime/8.0));
		z = abs(z);
		if (z.x < z.y) { z.xy = z.yx;}
		if (z.x < z.z) { z.xz = z.zx;}
		if (z.y < z.z) { z.yz = z.zy;}

		z = Scale*z-Offset*(Scale-1.0);
		if( z.z < -0.5*Offset.z*(Scale-1.0) )
		{
			z.z+=Offset.z*(Scale-1.0);
			z.zy = rotate(z.zy,4.0+2.0*cos(iGlobalTime/8.0));
			z.xy = rotate(z.xy,4.0+2.0*sin(iGlobalTime/8.0));
		}
		d = min(d, length(z) * pow(Scale, float(-n)-1.0));
	}

	return d-0.001;
}

// Finite difference normal
vec3 getNormal(in vec3 pos) 
{
	vec3 e = vec3(0.0,normalDistance,0.0);
	return normalize(vec3(	DE(pos+e.yxx)-DE(pos-e.yxx),
							DE(pos+e.xyx)-DE(pos-e.xyx),
							DE(pos+e.xxy)-DE(pos-e.xxy))	);
}

// Solid color
vec3 getColor(vec3 normal, vec3 pos) {
return vec3(1.0);
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




vec3 rayMarch(in vec3 from, in vec3 dir)
{
	float totalDistance = 0.0;
	vec3 dir2 = dir;
	float distance;
	int steps = 0;
	vec3 pos;
	for (int i=0; i<MaxSteps; i++)
	{
		pos = from + totalDistance*dir;
		distance = DE(pos);
		totalDistance += distance;
		if (distance < MinimumDistance) break;
		steps = i;
	}

	// 'AO' is based on number of steps.
	// Try to smooth the count, to combat banding.
	float smoothStep = float(steps) + distance/MinimumDistance;
	float ao = 1.1-smoothStep/float(MaxSteps);

	// Since our distance field is not signed,
	// backstep when calc'ing normal
	vec3 normal = getNormal(pos-dir*normalDistance*3.0);

	vec3 color = getColor(normal, pos);

vec3 light = getLight(color, normal, dir);
color = (color*Ambient+light)*ao;

	return color;
}


void main()
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

	vec3 ray_direction = normalize( -znear*camBasisZ + P.x*camBasisX + P.y*camBasisY );
	vec3 ray_origin = camPos;

	vec3 col = rayMarch(ray_origin, ray_direction);

	vec4 sample = vec4(col, 1.0);
	gl_FragColor = sample;
}

