#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec3 uVector;
uniform bool uIsVector;
uniform mat4 modelY;
uniform mat4 modelX;

void main()
{
  vec3 pos;
  if(uIsVector){
    pos = (gl_VertexID == 0) ? vec3(0.0) : uVector;
  }
  else {
    pos = aPos;
  }
  gl_Position = modelX * modelY * vec4(pos, 1.0);
}

