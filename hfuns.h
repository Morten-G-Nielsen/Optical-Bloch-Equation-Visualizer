#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

namespace hfuns{

std::string loadFile(const char* path)
{
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(GLenum type, const char* src){
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);

  GLint Success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &Success);
  if(!Success)
  {
    char log[1024];
    glGetShaderInfoLog(shader, 1024, NULL, log);
    std::cout << "Shader compile error:\n" << log << "\n";
  }
  return shader;
}

GLuint createGraphicsProgram(const char* vsSrc, const char* fsSrc){
  GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);

  GLuint program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  GLint Success;
  glGetProgramiv(program, GL_LINK_STATUS, &Success);
  if(!Success)
  {
    char log[1024];
    glGetProgramInfoLog(program, 1024, NULL, log);
    std::cout << "Program link error:\n" << log << "\n";
  }

  glDeleteShader(vs);
  glDeleteShader(fs);
  return program;
}
}
