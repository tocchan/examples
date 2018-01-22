#version 420 core

// Attributes
in vec3 POSITION;

// Entry point
void main( void )
{
	gl_Position = local_pos; 
}
