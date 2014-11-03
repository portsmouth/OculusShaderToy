attribute vec2 a_position;
void main()
{
	// output the transformed vertex
	//gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_Position = vec4(a_position, 0, 1);
}
