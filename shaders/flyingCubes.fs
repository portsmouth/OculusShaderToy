
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


#define FOV_MORPH 1

float box(vec3 p)
{
	return length(max(abs(p)-vec3(.5),0.0)) - 0.15;
}

vec3 rot(vec3 p, float f) {
	float s = sin(f);
	float c = cos(f);
	p.xy *= mat2(c, -s, s, c);
	p.yz *= mat2(c, -s, s, c);
	return p;
}


vec3 trans(vec3 p, out float rotout)
{
	p.zx += iGlobalTime*8.0;

	vec3 b = vec3(4.);
	vec3 rep = floor(p/b);

	p = mod(p,b)-0.5*b;

	rotout = iGlobalTime*1.88 + (rep.x+rep.z+rep.y)*0.3;
	p = rot(p, rotout);
	return p;
}

float scene(vec3 p) {
	float dummy;
	return box(trans(p,dummy));
}


vec3 normal(vec3 p, float d)
{
	vec3 e = vec3(0.04,.0,.0);
	return normalize(vec3(
		scene(p+e.xyy)-d,
		scene(p+e.yxy)-d,
		scene(p+e.yyx)-d));

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
	vec3 ro = camPos;
	vec3 p = ro + rd;

	float dall,d;
	for(int i = 0; i < 64; i++) {
		d = scene(p);
		if(d < 0.06) break;
		p += d*rd;
		dall += d;
	}

	vec3 bg = normalize(p).zzz + 0.1;

	if(d < 0.06) {
		vec3 n = normal(p,d);

		float S = 100.0;
		vec3 C = vec3(mod(p.x,S),mod(p.y,S),mod(p.z,S));

		vec3 col = C * vec3(dot(vec3(0.0,0.0,1.0), n));
		float objrot;
		vec3 objp = trans(p,objrot);
		vec3 objn = abs(rot(n,objrot));

		vec2 uv =
			(objn.y > 0.707) ? vec2(objp.zx) :
			(objn.x > 0.707) ? vec2(objp.zy) :
							   vec2(objp.xy) ;
		vec3 tex = vec3(0.5, 0.5, 0.5);//texture2D(iChannel0, uv).rgb;
		vec3 hl = smoothstep(0.6, 1.0, col);
		col *= clamp(tex.xyz+0.3, 0.0, 1.0);

		col = col + hl*.4;
		float fog = clamp(dall/mix(90.0,40.0,((rd.z+1.0)*0.5)), 0.0, 1.0);

		gl_FragColor = vec4(mix(col, bg, fog),1.0);
	}
	else {
		gl_FragColor = vec4(bg, 1.0);
	}



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
