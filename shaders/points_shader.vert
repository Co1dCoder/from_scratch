attribute vec4 a_vertices;

uniform vec4 u_v4_color;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;

void main ( void )
{
	mat4 mvpMatrix = u_projectionMatrix * u_viewMatrix;
	gl_Position = mvpMatrix * a_vertices;

	gl_FrontColor = u_v4_color;
}

