
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


vec2 rot(vec2 p, float r)
{
	return vec2(
		cos(r) * p.x - sin(r) * p.y,
		sin(r) * p.x + cos(r) * p.y);
}

float siso(vec3 p)
{
	float h = iGlobalTime * 1.5 + sin(iGlobalTime * 1.3) * 0.3;
	float k = 0.05;
	vec2  m = vec2(6, 5);
	vec2  b = vec2(1, 0.5);
	for(int i = 0 ; i < 3; i++)
	{
		p.xz += vec2(
			cos(p.y * m.x + h * b.x),
			sin(p.y * m.y + h * b.x)) * k;
		k *= 13.0 + sin(h);
		m = -m.yx * 0.125;
		b = -b.yx * 1.07;
		h *= 1.5;
	}
	return length(mod(p.xz, 60.0) - 30.0) - 3.0;
}

float iso(vec3 p)
{
	return min(siso(p.zxy + 15.0), min(siso(-p.xzy), siso(-p + 15.0)));
}

void main( void )
{
	float h = iGlobalTime;

	vec2 ndc = vec2((gl_FragCoord.x-viewportX)/viewportW,
					(gl_FragCoord.y-viewportY)/viewportH);

	vec2 uv  = -1.0 + 2.0 * ndc;

	float Hup    = znear * UpTan;
	float Hdown  = znear * DownTan;
	float Wleft  = znear * LeftTan;
	float Wright = znear * RightTan;

	vec2 ll = vec2(-Wleft, -Hdown);
	vec2 ur = vec2(Wright, Hup);
	vec2 P = ll + ndc*(ur-ll);

	vec3 dir = normalize( -znear*camBasisZ + P.x*camBasisX + P.y*camBasisY );
	vec3 pos = camPos;

	dir.yz = rot(dir.yz, h * 0.1);
	dir.xy = rot(dir.xy, h * 0.1);

	float t = 0.0;

	for(int i = 0 ; i < 50; i++)
		t += iso(dir * t + pos) * 0.9;

	vec3 ip  = t * dir + pos;

	vec3 col = vec3(t * 0.0005 + abs(iso(ip + 0.05)) ) * vec3(5, 2, 1);

	col = sqrt(col + dir * 0.2);
	gl_FragColor = vec4(col * (1.0 - pow(dot(uv*uv, uv*uv), 4.0)), 1.0);

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
