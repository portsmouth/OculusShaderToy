
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


// A quick test of several repeated objects with different repetition offsets.
//
// Version 1.0 (2013-03-19)
// Simon Stelling-de San Antonio
//
// Many thanks to Iñigo Quílez (iq) for articles and example source codes.

float camtime = 0.4*iGlobalTime;
vec3 camo;
vec3 camd;

float maxcomp( vec3 p ) { return max(p.x,max(p.y,p.z));}

float sdBox( vec3 p, vec3 b )
{
  vec3  di = abs(p) - b;
  float mc = maxcomp(di);
  return min(mc,length(max(di,0.0)));
}

float sdSphere( vec3 p, float s )
{
  return length(p)-s;
}

float repeatedSphere( vec3 p, vec3 c, float s )
{
  return length( mod(p,c)-0.5*c ) - s;
}

float repeatedBox( vec3 p, vec3 c, float b, float r )
{
  return length(max(abs(mod(p,c)-0.5*c)-b,0.0))-r;
}

float repeatedGrid( vec3 p, vec3 c, float b )
{
  vec3 d = abs(mod(p,c)-0.5*c)-b;
  return min(min(max(d.x,d.y), max(d.x,d.z)), max(d.y,d.z));
}

float repeatedCone( vec3 p, vec3 c, vec2 cone )
{
  vec3 q = mod(p,c)-0.5*c;
  // "cone" must be normalized
  float qq = length(q.xy);
  return max( dot(cone,vec2(qq,q.z)),  // cone
			  length(q)-0.125 // sphere
			);
}

void attributedUnion( inout vec2 c, float d, float a)
{
  if (d < c.x) {
	c.x = d;
	c.y = a;
  }
}

void attributedIntersection( inout vec2 c, float d, float a)
{
  if (d > c.x) {
	c.x = d;
	c.y = a;
  }
}

vec2 map( vec3 p )
{
	float M = abs(1.0 + 0.5*sin(iGlobalTime/200.0));

  vec2 ret = vec2(min(
						// repeatedSphere(p, vec3(2.0), 0.25),
						// repeatedBox(p, vec3(0.811), 0.071, 0.031)  ),
						repeatedBox(p, vec3(1.2), 0.071, 0.031),
						max( repeatedGrid(p, vec3(1.2), 0.0271),
							 repeatedBox(p+0.05,  vec3(0.1), 0.015, 0.0035)
						   )
						//repeatedBox(p, vec3(0.524), 0.041, 0.017)
				), 1.0);

  attributedUnion(ret, repeatedCone(vec3(p.x, p.y, p.z - camtime),
									M*vec3(1.611, 1.9, 5.0),
									normalize(vec2(1.0, 0.3))),
					   2.0);

  attributedUnion(ret, max( repeatedSphere(p+0.25, M*vec3(2.0), M*0.25),
							repeatedSphere(p, vec3(0.1)/M, M*0.046)
						  ), 3.0);

  attributedIntersection(ret, -sdSphere(p - camo, 0.03), -1.0);
  return ret;
}

vec2 intersect( in vec3 ro, in vec3 rd)
{
  float t = 0.0;
  vec2 res = vec2(-1.0, 0.0);
  bool search = true;
  for(int i=0; i<40; i++) {
	if (search) {
	  vec2 d = map(ro + rd*t);
	  if (d.x < 0.001) {
		res = vec2(t, d.y);
		if (d.x < 0.0001) {
		  search = false;
		}
	  }
	  t += d.x;
	}
  }
  return res;
}

vec3 calcNormal(in vec3 pos)
{
  vec2 eps = vec2(0.002, 0.0);
  vec3 nor;
  nor.x = map(pos+eps.xyy).x - map(pos-eps.xyy).x;
  nor.y = map(pos+eps.yxy).x - map(pos-eps.yxy).x;
  nor.z = map(pos+eps.yyx).x - map(pos-eps.yyx).x;
  return normalize(nor);
}

vec3 getCam(float t)
{
  return (3.91+1.51*cos(0.61*t))
   * vec3(0.01+2.51*sin(0.25*t),
		  0.12+1.05*cos(0.17*t),
		  0.04+2.51*cos(0.25*t));
}

void main(void)
{
	vec2 p = -1.0 + 2.0 * gl_FragCoord.xy / iResolution.xy;
	p.x *= (iResolution.x / iResolution.y);

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


	// camera
	vec3 ro = ray_origin;
	camo = ro;

	vec3 rd = ray_direction;
	camd = rd;

	float sh = 0.0;
	vec2 hit = intersect(ro,rd);
	if (hit.x > 0.0) {
	  vec3 pos = ro + hit.x*rd;
	  rd = calcNormal(pos);
	  ro = pos;
	  sh = clamp( pow(2.0/length(pos - camo), 2.0), 0.0, 1.0);
	}

	// light
	vec3 light = normalize(vec3(1.0,0.9,0.3));
	float lif = dot(rd,light);
	float li1 = pow( max( lif,0.0), 3.1);
	float li2 = pow( max(-lif,0.0), 3.1);
	// color
	vec3 col;
	if (hit.y < 0.0) {
	  col = vec3(1.0, sin(ro.x*30.0+ro.y*24.0+ro.z*16.0)*0.4+0.5, 0.0); // yellow-red
	} else {
	  if (hit.y == 2.0) {
		col = vec3(0.7, 0.7, 0.7); // silver bullets
	  } else {
		col = sin(ro*3.0*hit.y)*0.25+0.25;
	  }
	  col = mix(col, vec3(1.0        ), li1); // add light1 (white)
	  col = mix(col, vec3(0.0,0.1,0.3), li2); // add light2 (dark blue)
	}
	// fog color
	float shf = dot(camd,light);
	float sh1 = pow( max( shf,0.0), 3.1);
	vec3 shcol = mix(vec3(0.5,0.6,0.7), vec3(1.0,0.9,0.7), sh1);
	col = mix(shcol, col, sh); // add fog
	gl_FragColor = vec4(col,1.0);
}


/*
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

	vec3 col = render( ray_origin, ray_direction );

	vec4 sample = vec4(col, 1.0);
	gl_FragColor = sample;
}
*/
