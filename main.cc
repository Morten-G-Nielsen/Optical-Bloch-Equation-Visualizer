#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <complex>
#include <functional>

#include "hfuns.h"

const double PI = 3.1415926535897932;

struct Pulse{
  double t_center, A, w, beta=0;

  double square(double t){
    if(t <= t_center+w/2 && t>=t_center-w/2) return A/w;
    return 0;
  }

  double gauss(double t){
    double sigma = w/2.335;
    double term1 = A/(sigma*std::sqrt(2*PI));
    double term2 = std::exp(-std::pow(t-t_center, 2)/(2*std::pow(sigma, 2)));
    return term1*term2;
  }

  std::complex<double> chirped(double t){
    std::complex<double> i(0,1);
    return gauss(t)*std::exp(-i*beta/2.0 * std::pow(t-t_center,2));
  }
};


struct Bloc{
  double Delta, T1, T2, tau;
  glm::dvec3 R;
  Pulse p;
  std::function<std::complex<double>(double)> Omega;

  Bloc(){
    p.t_center = 1;
    p.A = PI/2;
    p.w = 0.01;
    R.x = 0; R.y = 0; R.z = -1.0;
    set_gauss();
  }

  void set_gauss() {
    Omega = [&](double t){return std::complex<double>(p.gauss(t), 0.0);};
  }
  void set_square() {
    Omega = [&](double t){return std::complex<double>(p.square(t), 0.0);};
  }
  void set_chirped(){
    Omega = [&](double t){return p.chirped(t);};
  }

  glm::dvec3 f(double t, glm::dvec3 R){
    std::complex<double> Om = Omega(t);
    double re_om = std::real(Om);
    double im_om = std::imag(Om);

    double du = Delta*R.y - im_om*R.z-R.x/T1;
    double dv = -Delta*R.x - re_om*R.z - R.y/T2;
    double dw = re_om*R.y + im_om*R.x - (1.0+R.z)/T1;
    return glm::dvec3(du, dv, dw);
  }

  glm::dvec3 RK4(double t, double dt, glm::dvec3 R){
    glm::dvec3 k1 = f(t, R);
    glm::dvec3 k2 = f(t + dt/2.0, R + (dt / 2.0) * k1);
    glm::dvec3 k3 = f(t + dt/2.0, R + (dt / 2.0) * k2);
    glm::dvec3 k4 = f(t + dt, R + dt * k3);
    return R + (dt/6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
  }
};

int main(){
  Bloc System;
  System.Delta = 0.1;
  System.T1 = 1e6;
  System.T2 = 1e6;

  // Vertices for Bloch sphere
  std::vector<float> vertices;
  int segments = 32;
  // Horizontal rings
  for(int i = 1; i<=10; ++i){
    float phi = PI * i/10.0f;
    for(int j = 0; j<=segments; ++j){
      float theta = 2.0*PI*j/float(segments);
      vertices.push_back(std::sin(phi)*std::cos(theta));
      vertices.push_back(std::sin(phi)*std::sin(theta));
      vertices.push_back(std::cos(phi));
    }
  }
  // Vertical rings
  for(int i = 1; i<=10; ++i){
    float theta = 2.0*PI*i/10.0f;
    for(int j = 0; j<=segments; ++j){
      float phi = PI * j/float(segments);
      vertices.push_back(std::sin(phi)*std::cos(theta));
      vertices.push_back(std::sin(phi)*std::sin(theta));
      vertices.push_back(std::cos(phi));
    }
  }
  // Inilising window and loading glad
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window = glfwCreateWindow(800, 600, "", NULL, NULL);
  glfwMakeContextCurrent(window);

  if(!gladLoadGL()){
    std::cout << "Failed to initilaize glad" << std::endl;
    return -1;
  };

  // Compiling shaders and linking
  std::string vStr = hfuns::loadFile("shaders/v.vert");
  std::string fStr = hfuns::loadFile("shaders/fragment.frag");
  const char* vSrc = vStr.c_str();
  const char* fSrc = fStr.c_str();
  GLuint renderprogram = hfuns::createGraphicsProgram(vSrc, fSrc);

  // Empty vao for bloch vector and sphereVAO with VBO
  GLuint vao;
  glGenVertexArrays(1, &vao);

  GLuint sphereVAO;
  glGenVertexArrays(1, &sphereVAO);
  glBindVertexArray(sphereVAO);

  GLuint VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);

  // Locations for uniforms in renderprogram
  GLuint uVectorLoc = glGetUniformLocation(renderprogram, "uVector");
  GLuint isVectorLoc = glGetUniformLocation(renderprogram, "uIsVector");
  GLuint uColor = glGetUniformLocation(renderprogram, "uColor");
  GLuint modelXLoc = glGetUniformLocation(renderprogram, "modelX");
  GLuint modelYLoc = glGetUniformLocation(renderprogram, "modelY");

  // Initilising variables
  double t = 0;
  double dt = 0.001;
  float phi = 0;
  float phi_speed = 0.01;
  float theta = 0;
  float theta_speed = 0.01;
  while(!glfwWindowShouldClose(window)){
    // Solves next step using Runge-Kutta 4
    System.R = System.RK4(t, dt, System.R);
    t+=dt;

    // Creates rotation matrix
    glm::mat4 modelX = glm::rotate(glm::mat4(1.0f), phi, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 modelY = glm::rotate(glm::mat4(1.0f), theta, glm::vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(modelXLoc, 1, GL_FALSE, &modelX[0][0]);
    glUniformMatrix4fv(modelYLoc, 1, GL_FALSE, &modelY[0][0]);

    // Keybinds
    if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS){
      phi += phi_speed;
    }
    if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS){
      phi -= phi_speed;
    }
    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS){
      theta += theta_speed;
    }
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){
      theta -= theta_speed;
    }
    if(glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS){
      System.p.t_center = t + 1.5*System.p.w;
    }
    if(glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS){
      System.p.w += 0.01;
    }
    if(glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS){
      System.p.w -= 0.01;
    }
    if(glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS){
      System.p.A += 0.01;
    }
    if(glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS){
      System.p.A -= 0.01;
    }
    if(glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS){
      System.set_gauss();
    }
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
      System.set_square();
    }
    if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS){
      System.set_chirped();
    }

    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draws the Bloch vector
    glUseProgram(renderprogram);
    glUniform1i(isVectorLoc, 1);
    glUniform3f(uVectorLoc, System.R.y, System.R.z, System.R.x);
    glUniform4f(uColor, 1, 0, 0, 1);
    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, 2);

    // Draws the Bloch sphere
    glUniform1i(isVectorLoc, 0);
    glUniform4f(uColor, 1, 1, 1, 0.5);
    glBindVertexArray(sphereVAO);
    glDrawArrays(GL_LINES, 0, vertices.size()/3);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}
