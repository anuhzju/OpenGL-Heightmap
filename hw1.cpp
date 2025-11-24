/*
  CSCI 420 Computer Graphics, Computer Science, USC
  Assignment 1: Height Fields with Shaders.
  C/C++ starter code

  Student username: annazhu
*/

#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "pipelineProgram.h"
#include "vbo.h"
#include "vao.h"

#include <iostream>
#include <cstring>
#include <memory>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// Transformations of the terrain.
float terrainRotate[3] = { 0.0f, 0.0f, 0.0f }; 
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = { 0.0f, 0.0f, 0.0f };
float terrainScale[3] = { 1.0f, 1.0f, 1.0f };

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework 1";


// Number of vertices in the single triangle (starter code).
int numVertices;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram pipelineProgram;
VBO vboVertices;
VBO vboColors;
VAO vao;

// global params for different render modes
enum class RenderMode { Points = 1, Grid = 2, Tris = 3, Smooth = 4 };
RenderMode rm = RenderMode::Points;
int imageW = 0, imageH = 0;
const unsigned char* pixels = nullptr;
void geometryForMode();
VBO vboLeft, vboRight, vboUp, vboDown;
float uScale = 0.1f;
float uExponent = 1.0f;
int colormapMode = 0;

// Write a screenshot to the specified filename.
void saveScreenshot(const char * filename)
{
  std::unique_ptr<unsigned char[]> screenshotData = std::make_unique<unsigned char[]>(windowWidth * windowHeight * 3);
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData.get());

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData.get());

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;
}

void idleFunc()
{
  // Do some stuff... 
  // For example, here, you can save the screenshots to disk (to make the animation).

  // Notify GLUT that it should call displayFunc.
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // When the window has been resized, we need to re-set our projection matrix.
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  // You need to be careful about setting the zNear and zFar. 
  // Anything closer than zNear, or further than zFar, will be culled.
  const float zNear = 0.1f;
  const float zFar = 10000.0f;
  const float humanFieldOfView = 60.0f;
  matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
  // Mouse has moved, and one of the mouse buttons is pressed (dragging).

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the terrain
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        terrainTranslate[0] += mousePosDelta[0] * 0.01f;
        terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        terrainTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the terrain
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        terrainRotate[0] += mousePosDelta[1];
        terrainRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        terrainRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the terrain
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // Mouse has moved.
  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // A mouse button has has been pressed or depressed.

  // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // Keep track of whether CTRL and SHIFT keys are pressed.
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // If CTRL and SHIFT are not pressed, we are in rotate mode.
    default:
      controlState = ROTATE;
    break;
  }

  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // Take a screenshot.
      saveScreenshot("screenshot.jpg");
    break;

    //switching render modes
    case '1': 
        rm = RenderMode::Points;
        break;
    case '2':
        rm = RenderMode::Grid;
        break;
    case '3':
        rm = RenderMode::Tris;
        break;
    case '4':
        rm = RenderMode::Smooth;
        break;

    //scale&exponent manip for smooth mode
    case '=':
        uScale *= 2.0;
        break;
    case '-':
        uScale /= 2.0;
        break;
    case '9':
        uExponent *= 2.0;
        break;
    case '0':
        uExponent /= 2.0;
        break;

    //cycles through additional colormaps
    case ']':
        if (colormapMode < 4)
            colormapMode++;
        else
            colormapMode = 0;
        break;
    case '[':
        if (colormapMode > 0)
            colormapMode--;
        else
            colormapMode = 4;
        break;
  }
}

void displayFunc()
{
  // This function performs the actual rendering.

  // First, clear the screen.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up the camera position, focus point, and the up vector.
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.LookAt(0.8, 0.8, -0.8,
                0.1, 0.0, -0.1,
                0.0, 1.0, 0.0);

  // In here, you can do additional modeling on the object, such as performing translations, rotations and scales.
  // ...

  // Read the current modelview and projection matrices from our helper class.
  // The matrices are only read here; nothing is actually communicated to OpenGL yet.
  float modelViewMatrix[16];

  //apply translations, rotations, and scale
  matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);
  matrix.Rotate(terrainRotate[2], 0, 0, 1);
  matrix.Rotate(terrainRotate[1], 0, 1, 0);
  matrix.Rotate(terrainRotate[0], 1, 0, 0);
  matrix.Scale(terrainScale[0], terrainScale[1], terrainScale[2]);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  // Upload the modelview and projection matrices to the GPU. Note that these are "uniform" variables.
  // Important: these matrices must be uploaded to *all* pipeline programs used.
  // In hw1, there is only one pipeline program, but in hw2 there will be several of them.
  // In such a case, you must separately upload to *each* pipeline program.
  // Important: do not make a typo in the variable name below; otherwise, the program will malfunction.
  pipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  pipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  pipelineProgram.Bind();

  int smoothMode = (rm == RenderMode::Smooth) ? 1 : 0;
  pipelineProgram.SetUniformVariablei("smoothMode", smoothMode);
  pipelineProgram.SetUniformVariablef("scale", uScale);
  pipelineProgram.SetUniformVariablef("exponent", uExponent);
  pipelineProgram.SetUniformVariablei("colormapMode", colormapMode);

  // Execute the rendering.
  // Bind the VAO that we want to render. Remember, one object = one VAO. 

  geometryForMode();
  vao.Bind();
  
  switch (rm) {
    case RenderMode::Points:
        glEnable(GL_PROGRAM_POINT_SIZE);
        glDrawArrays(GL_POINTS, 0, numVertices);
        break;

    case RenderMode::Grid:
        glDrawArrays(GL_LINES, 0, numVertices);
        break;

    case RenderMode::Tris:
        glDrawArrays(GL_TRIANGLES, 0, numVertices);
        break;
    case RenderMode::Smooth:
        glDrawArrays(GL_TRIANGLES, 0, numVertices);
        break;
  }
  
  // Swap the double-buffers.
  glutSwapBuffers();
}

//make different vertex arrays for each mode
void geometryForMode() {
    const float heightScale = 0.1f;
    //center the height map
    const float sx = (imageW > 1) ? 1.0f / (imageW - 1) : 1.0f;
    const float sy = (imageH > 1) ? 1.0f / (imageH - 1) : 1.0f;

    //"1" - points mode
    if (rm == RenderMode::Points) {
        numVertices = imageW * imageH; //one vertex per pixel

        std::unique_ptr<float[]> positions = std::make_unique<float[]>(numVertices * 3); //positions
        std::unique_ptr<float[]> colors = std::make_unique<float[]>(numVertices * 4); //colors

        for (int y = 0; y < imageH; ++y) {
            for (int x = 0; x < imageW; ++x) {
                int ind = (y * imageW + x) * 3;
                const float pixelHeight = pixels[y * imageW + x] / 255.0f;
                positions[ind] = x * sx - 0.5f;
                positions[ind + 1] = pixelHeight * heightScale;
                positions[ind + 2] = y * sy - 0.5f;

                ind = (y * imageW + x) * 4;
                colors[ind] = 1.0f; colors[ind + 1] = 1.0f; colors[ind + 2] = 1.0f;
                colors[ind + 3] = pixelHeight;
            }
        }

        vboVertices.Gen(numVertices, 3, positions.get(), GL_STATIC_DRAW); // 3 values per position
        vboColors.Gen(numVertices, 4, colors.get(), GL_STATIC_DRAW); // 4 values per color
    }

    //"2" - grid mode
    else if (rm == RenderMode::Grid) {
        numVertices = imageW * (imageW * 4 - 4); //derived this

        std::unique_ptr<float[]> positions = std::make_unique<float[]>(numVertices * 3); //positions
        std::unique_ptr<float[]> colors = std::make_unique<float[]>(numVertices * 4); //colors

        int ind = 0;
        //horizontal lines
        for (int y = 0; y < imageH; ++y) {
            for (int x = 0; x < imageW - 1; ++x) {
                float pixelHeightA = pixels[y * imageW + x] / 255.0f;
                positions[ind * 3] = x * sx - 0.5f;
                positions[ind * 3 + 1] = pixelHeightA * heightScale;
                positions[ind * 3 + 2] = y * sy - 0.5f;
                colors[ind * 4] = 1.0f; colors[ind * 4 + 1] = 1.0f; colors[ind * 4 + 2] = 1.0f;
                colors[ind * 4 + 3] = pixelHeightA; 
                ++ind;

                float pixelHeightB = pixels[y * imageW + (x + 1)] / 255.0f;
                positions[ind * 3] = (x + 1) * sx - 0.5f;
                positions[ind * 3 + 1] = pixelHeightB * heightScale;
                positions[ind * 3 + 2] = y * sy - 0.5f;
                colors[ind * 4] = 1.0f; colors[ind * 4 + 1] = 1.0f; colors[ind * 4 + 2] = 1.0f;
                colors[ind * 4 + 3] = pixelHeightB;
                ++ind;
            }
        }

        //vertical lines
        for (int x = 0; x < imageW; ++x) {
            for (int y = 0; y < imageH - 1; ++y) {
                float pixelHeightA = pixels[y * imageW + x] / 255.0f;
                positions[ind * 3] = x * sx - 0.5f;
                positions[ind * 3 + 1] = pixelHeightA * heightScale;
                positions[ind * 3 + 2] = y * sy - 0.5f;
                colors[ind * 4] = 1.0f; colors[ind * 4 + 1] = 1.0f; colors[ind * 4 + 2] = 1.0f;
                colors[ind * 4 + 3] = pixelHeightA; 
                ++ind;

                float pixelHeightB = pixels[(y + 1) * imageW + x] / 255.0f;
                positions[ind * 3] = x * sx - 0.5f;
                positions[ind * 3 + 1] = pixelHeightB * heightScale;
                positions[ind * 3 + 2] = (y + 1) * sy - 0.5f;
                colors[ind * 4] = 1.0f; colors[ind * 4 + 1] = 1.0f; colors[ind * 4 + 2] = 1.0f;
                colors[ind * 4 + 3] = pixelHeightB;
                ++ind;
            }
        }

        vboVertices.Gen(numVertices, 3, positions.get(), GL_STATIC_DRAW); // 3 values per position
        vboColors.Gen(numVertices, 4, colors.get(), GL_STATIC_DRAW); // 4 values per color
    }

    //"3" - triangles mode
    else if (rm == RenderMode::Tris) {
        numVertices = 6 * (imageW - 1) * (imageH - 1); //each square has 6 vertices

        std::unique_ptr<float[]> positions = std::make_unique<float[]>(numVertices * 3); //positions
        std::unique_ptr<float[]> colors = std::make_unique<float[]>(numVertices * 4); //colors

        int ind = 0;
        for (int y = 0; y < imageH - 1; ++y) {
            for (int x = 0; x < imageW - 1; ++x) {
                float height00 = pixels[y * imageW + x] / 255.0f;
                float height10 = pixels[y * imageW + (x + 1)] / 255.0f;
                float height01 = pixels[(y + 1) * imageW + x] / 255.0f;
                float height11 = pixels[(y + 1) * imageW + (x + 1)] / 255.0f;

                //top right? tri
                positions[ind * 3] = x * sx - 0.5f;
                positions[ind * 3 + 1] = height00 * heightScale;
                positions[ind * 3 + 2] = y * sy - 0.5f;
                colors[ind * 4] = 1.0f; colors[ind * 4 + 1] = 1.0f; colors[ind * 4 + 2] = 1.0f;
                colors[ind * 4 + 3] = height00; 
                ++ind;

                positions[ind * 3] = (x + 1) * sx - 0.5f;
                positions[ind * 3 + 1] = height10 * heightScale;
                positions[ind * 3 + 2] = y * sy - 0.5f;
                colors[ind * 4] = 1.0f; colors[ind * 4 + 1] = 1.0f; colors[ind * 4 + 2] = 1.0f;
                colors[ind * 4 + 3] = height10;
                ++ind;

                positions[ind * 3] = (x + 1) * sx - 0.5f;
                positions[ind * 3 + 1] = height11 * heightScale;
                positions[ind * 3 + 2] = (y + 1) * sy - 0.5f;
                colors[ind * 4] = 1.0f; colors[ind * 4 + 1] = 1.0f; colors[ind * 4 + 2] = 1.0f;
                colors[ind * 4 + 3] = height11;
                ++ind;

                //bot left? tri
                positions[ind * 3] = x * sx - 0.5f;
                positions[ind * 3 + 1] = height00 * heightScale;
                positions[ind * 3 + 2] = y * sy - 0.5f;
                colors[ind * 4] = 1.0f; colors[ind * 4 + 1] = 1.0f; colors[ind * 4 + 2] = 1.0f;
                colors[ind * 4 + 3] = height00;
                ++ind;

                positions[ind * 3] = (x + 1) * sx - 0.5f;
                positions[ind * 3 + 1] = height11 * heightScale;
                positions[ind * 3 + 2] = (y + 1) * sy - 0.5f;
                colors[ind * 4] = 1.0f; colors[ind * 4 + 1] = 1.0f; colors[ind * 4 + 2] = 1.0f;
                colors[ind * 4 + 3] = height11;
                ++ind;

                positions[ind * 3] = x * sx - 0.5f;
                positions[ind * 3 + 1] = height01 * heightScale;
                positions[ind * 3 + 2] = (y + 1) * sy - 0.5f;
                colors[ind * 4] = 1.0f; colors[ind * 4 + 1] = 1.0f;  colors[ind * 4 + 2] = 1.0f;
                colors[ind * 4 + 3] = height01;
                ++ind;
            }
        }
        vboVertices.Gen(numVertices, 3, positions.get(), GL_STATIC_DRAW); // 3 values per position
        vboColors.Gen(numVertices, 4, colors.get(), GL_STATIC_DRAW); // 4 values per color
    }

    vao.Gen();

    //"4" - smooth mode
    if (rm == RenderMode::Smooth) {
        numVertices = 6 * (imageW - 1) * (imageH - 1);

        std::unique_ptr<float[]> positions = std::make_unique<float[]>(numVertices * 3); //positions
        std::unique_ptr<float[]> colors = std::make_unique<float[]>(numVertices * 4); //colors

        //additional vbos for smoothing
        std::unique_ptr<float[]> pLeft = std::make_unique<float[]>(numVertices * 3);
        std::unique_ptr<float[]> pRight = std::make_unique<float[]>(numVertices * 3);
        std::unique_ptr<float[]> pUp = std::make_unique<float[]>(numVertices * 3);
        std::unique_ptr<float[]> pDown = std::make_unique<float[]>(numVertices * 3);

        //helper function to write self and neighbors
        auto smoothHelper = [&](int ind, int x, int y) {
            float pixelHeight = pixels[y * imageW + x] / 255.0f;
            positions[ind * 3] = x * sx - 0.5f;
            positions[ind * 3 + 1] = pixelHeight;
            positions[ind * 3 + 2] = y * sy - 0.5f;

            colors[ind * 4 + 0] = 1.0f; colors[ind * 4 + 1] = 1.0f; colors[ind * 4 + 2] = 1.0f; 
            colors[ind * 4 + 3] = pixelHeight;

            //bounds check - if its an edge vertex, use self
            int leftX = (x > 0) ? x - 1 : x;
            int rightX = (x < imageW - 1) ? x + 1 : x;
            int downY = (y > 0) ? y - 1 : y;
            int upY = (y < imageH - 1) ? y + 1 : y;

            auto setNeighbors = [&](float* arr, int newX, int newY) {
                float newHeight = pixels[newY * imageW + newX] / 255.0f;
                arr[ind * 3 + 0] = newX * sx - 0.5f;
                arr[ind * 3 + 1] = newHeight;                 
                arr[ind * 3 + 2] = newY * sy - 0.5f;
                };

            setNeighbors(pLeft.get(), leftX, y);
            setNeighbors(pRight.get(), rightX, y);
            setNeighbors(pDown.get(), x, downY);
            setNeighbors(pUp.get(), x, upY);
            };

        int ind = 0;
        for (int y = 0; y < imageH - 1; ++y) {
            for (int x = 0; x < imageW - 1; ++x) {
                //tri 1
                smoothHelper(ind++, x, y);
                smoothHelper(ind++, x + 1, y);
                smoothHelper(ind++, x + 1, y + 1);

                //tri 2
                smoothHelper(ind++, x, y);
                smoothHelper(ind++, x + 1, y + 1);
                smoothHelper(ind++, x, y + 1);
            }
        }

        vboVertices.Gen(numVertices, 3, positions.get(), GL_STATIC_DRAW);
        vboColors.Gen(numVertices, 4, colors.get(), GL_STATIC_DRAW); 
        vboLeft.Gen(numVertices, 3, pLeft.get(), GL_STATIC_DRAW); 
        vboRight.Gen(numVertices, 3, pRight.get(), GL_STATIC_DRAW); 
        vboUp.Gen(numVertices, 3, pUp.get(), GL_STATIC_DRAW); 
        vboDown.Gen(numVertices, 3, pDown.get(), GL_STATIC_DRAW);

        vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboLeft, "pLeft");
        vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboRight, "pRight");
        vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboUp, "pUp");
        vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboDown, "pDown");
    }

    vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboVertices, "position");
    vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboColors, "color");
}

void initScene(int argc, char *argv[])
{
  // Load the image from a jpeg disk file into main memory.
  std::unique_ptr<ImageIO> heightmapImage = std::make_unique<ImageIO>();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Create a pipeline program. This operation must be performed BEFORE we initialize any VAOs.
  // A pipeline program contains our shaders. Different pipeline programs may contain different shaders.
  // In this homework, we only have one set of shaders, and therefore, there is only one pipeline program.
  // In hw2, we will need to shade different objects with different shaders, and therefore, we will have
  // several pipeline programs (e.g., one for the rails, one for the ground/sky, etc.).
  // Load and set up the pipeline program, including its shaders.
  if (pipelineProgram.BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the pipeline program." << endl;
    throw 1;
  } 
  cout << "Successfully built the pipeline program." << endl;
    
  // Bind the pipeline program that we just created. 
  // The purpose of binding a pipeline program is to activate the shaders that it contains, i.e.,
  // any object rendered from that point on, will use those shaders.
  // When the application starts, no pipeline program is bound, which means that rendering is not set up.
  // So, at some point (such as below), we need to bind a pipeline program.
  // From that point on, exactly one pipeline program is bound at any moment of time.

  int smoothMode = (rm == RenderMode::Smooth) ? 1 : 0;
  pipelineProgram.Bind();
  pipelineProgram.SetUniformVariablei("smoothMode", smoothMode);
  pipelineProgram.SetUniformVariablef("scale", uScale);
  pipelineProgram.SetUniformVariablef("exponent", uExponent);
  pipelineProgram.SetUniformVariablei("colormapMode", colormapMode);

  imageW = heightmapImage->getWidth();
  imageH = heightmapImage->getHeight();
  pixels = heightmapImage->getPixels();
  geometryForMode();

  // Check for any OpenGL errors.
  std::cout << "GL error status is: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // Tells GLUT to use a particular display function to redraw.
  glutDisplayFunc(displayFunc);
  // Perform animation inside idleFunc.
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // Perform the initialization.
  initScene(argc, argv);

  // Sink forever into the GLUT loop.
  glutMainLoop();
}