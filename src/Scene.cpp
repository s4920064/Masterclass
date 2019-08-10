#include "Scene.h"
#include "GLFW/glfw3.h"
#include "glm/ext.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <ngl/NGLInit.h>
#include <ngl/NGLStream.h>
#include <ngl/ShaderLib.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/Obj.h>
#include <QMouseEvent>
#include <QGLWidget>
#include <QPainter>

Scene::Scene()
{
  setTitle( "Demo" );
}


Scene::~Scene()
{
  std::cout << "Shutting down NGL, removing VAO's and Shaders\n";

  // delete all the textures and fbos
  for(int i=0;i<3;i++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId[i]);
    glDeleteTextures(1, &m_fboTexId[i]);
    glDeleteTextures(1, &m_fboDepthId);
    glDeleteFramebuffers(1, &m_fboId[i]);
  }
}

void Scene::initializeGL()
{
  // we must call that first before any other GL commands to load and link the
  // gl commands from the lib, if that is not done program will crash
  ngl::NGLInit::instance();
  // uncomment this line to make ngl less noisy with debug info
  // ngl::NGLInit::instance()->setCommunicationMode( ngl::CommunicationMode::NULLCONSUMER);
  glClearColor( 0.4f, 0.4f, 0.4f, 1.0f ); // Grey Background
  // Enable 2D texturing
  glEnable(GL_TEXTURE_2D);

  // enable depth testing for drawing
  glEnable( GL_DEPTH_TEST );

  // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib* shader = ngl::ShaderLib::instance();

  // create the PBR shader program
  shader->loadShader("PBR",                // Name of program
                     "shaders/PBRVertex.glsl",     // Vertex shader
                     "shaders/PBRFragment.glsl");    // Fragment shader

  // create the TAA shader program
  shader->loadShader("TAA",                // Name of program
                     "shaders/TAAVertex.glsl",     // Vertex shader
                     "shaders/TAAFragment.glsl");    // Fragment shader

  // create the zoom/blitting texture (final screen output)
  shader->loadShader("Zoom",                // Name of program
                     "shaders/ZoomVertex.glsl",     // Vertex shader
                     "shaders/ZoomFragment.glsl");    // Fragment shader

  // create a simple shader program to pass textures
  shader->loadShader("Texture",                // Name of program
                     "shaders/TextureVertex.glsl",     // Vertex shader
                     "shaders/TextureFragment.glsl");    // Fragment shader

  // Create a screen oriented plane
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
  prim->createTrianglePlane("plane",2,2,1,1,ngl::Vec3(0,1,0));

  // create projection matrix
  m_projection=ngl::perspective( 45.0f, static_cast<float>( TEXTURE_WIDTH ) / TEXTURE_HEIGHT, 0.1f, 200.0f );

  // Set the initial camera position and direction
  m_cam.pos = glm::vec3(0.0f, 2.0f, 2.0f);
  m_cam.front = glm::vec3(0.0f, -2.0f, -2.0f);
  m_cam.up = glm::vec3(0.0f, 1.0f, 0.0f);

  // set shading parameters for the PBR shader
  ( *shader )[ "PBR" ]->use();
  shader->setUniform( "camPos", m_cam.pos );
  shader->printRegisteredUniforms("PBR");

//  // set shading parameters for the Checker shader
//  shader->use(ngl::nglCheckerShader);
//  shader->setUniform("lightDiffuse",1.0f,1.0f,1.0f,1.0f);
//  shader->setUniform("checkOn",true);
//  shader->setUniform("lightPos",m_lightPos.toVec3());
//  shader->setUniform("colour1",0.9f,0.9f,0.9f,1.0f);
//  shader->setUniform("colour2",0.6f,0.6f,0.6f,1.0f);
//  shader->setUniform("checkSize",60.0f);
//  shader->printRegisteredUniforms(ngl::nglCheckerShader);

  // create a mesh from an obj passing in the obj file and texture
  m_meshStreet.reset(  new ngl::Obj("geo/street.obj"));
  // now we need to create this as a VAO so we can draw it
  m_meshStreet->createVAO();

  // create a mesh from an obj passing in the obj file and texture
  m_meshGround.reset(  new ngl::Obj("geo/ground.obj"));
  // now we need to create this as a VAO so we can draw it
  m_meshGround->createVAO();

  // create a mesh from an obj passing in the obj file and texture
  m_meshWalls.reset(  new ngl::Obj("geo/walls.obj"));
  // now we need to create this as a VAO so we can draw it
  m_meshWalls->createVAO();

//  // create a mesh from an obj passing in the obj file and texture
//  m_meshFence.reset(  new ngl::Obj("geo/fence.obj"));
//  // now we need to create this as a VAO so we can draw it
//  m_meshFence->createVAO();

  // create the fbos and their texture attach

  initEnvironment();
  createTextureObjects();
  createFramebufferObject();
  loadTexture();

  // start the timer
  m_time.start();
}

void Scene::loadMatricesToShader(const char* _shaderName, ngl::Mat4 _M)
{
  // grab an instance of the shader manager
  ngl::ShaderLib* shader = ngl::ShaderLib::instance();
  shader->use(_shaderName);

  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;

  // set the MVP with the jitter as the final transformation
  MVP=m_jitter.getMatrix()*m_projection*m_view*_M;

  // set the normal matrix
  normalMatrix=m_view;
  normalMatrix.inverse().transpose();

  // send uniforms to the GPU
  shader->setUniform("MVP", MVP);
  shader->setUniform("normalMatrix", normalMatrix);
  shader->setUniform("V", m_view);
  shader->setUniform("M", _M);
}

// function from jon Macey
void Scene::loadTexture()
{
  QImage image;
  bool loaded=image.load("textures/brick.png");
  if(loaded == true)
  {
    int width=image.width();
    int height=image.height();
    // note this method is depracted as it uses the Older GLWidget but does work
    image = QGLWidget::convertToGLFormat(image);

    glGenTextures(1,&m_brickTexture);
    glBindTexture(GL_TEXTURE_2D,m_brickTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    glGenerateMipmap(GL_TEXTURE_2D); //  Allocate the mipmaps
  }
}

void Scene::drawSceneGeometry(ngl::VAOPrimitives* _prim)
{
  // if TAA is enabled
  if(m_TAA)
  {
    // create the jitter matrix
    m_jitter.makeQuincunx();
  }

  // get time elapsed and restart the timer
  m_deltaTime = m_time.restart();
  // store FPS
  m_framerate = 1000/m_deltaTime;
  // print framerate
  std::cout << m_framerate << "\n";

  // scale distance by framerate
  float dist = SPEED*m_deltaTime;

  // translate the camera according to which buttons were pressed
  m_cam.pos += m_cam.translateF * dist*m_cam.front;
  m_cam.pos += m_cam.translateB * dist*m_cam.front;
  m_cam.pos += m_cam.translateL * glm::normalize(glm::cross(m_cam.front, m_cam.up)) * dist;
  m_cam.pos += m_cam.translateR * glm::normalize(glm::cross(m_cam.front, m_cam.up)) * dist;

  // update the view matrix with camera movements
  m_view = ngl::lookAt(m_cam.pos, m_cam.pos + m_cam.front, m_cam.up);

  //----------------draw objects------------------
  // make a model matrix
  ngl::Mat4 M;

//  // teapot
//  // update model matrix for teapot
//  M = ngl::Mat4(1.0f);
//  M.translate(0.0f,0.45f,0.0f);
//  // send the matrices to PBR shader
//  loadMatricesToShader("PBR", M);
//  // draw
//  _prim->draw("teapot");
//  // reset the model matrix for the other objects
//  M = ngl::Mat4(1.0f);

  glBindTexture(GL_TEXTURE_2D,m_brickTexture);

  loadMatricesToShader("PBR",M);
  m_meshStreet->draw();

  loadMatricesToShader("Texture",M);
  m_meshWalls->draw();

  loadMatricesToShader("Texture",M);
  m_meshGround->draw();

//  loadMatricesToShader("PBR",M);
//  m_meshFence->draw();

  //----------------------------------------------
}

void Scene::drawScreenOrientedPlane(GLuint _pid, ngl::VAOPrimitives* _prim)
{
  // the plane MVP
  glm::mat4 MVP_plane = glm::rotate(glm::mat4(1.0f), glm::pi<float>() * 0.5f, glm::vec3(1.0f,0.0f,0.0f));
  glUniformMatrix4fv(glGetUniformLocation(_pid, "MVP"), 1, false, glm::value_ptr(MVP_plane));

  // draw the screen-oriented plane (the final image)
  _prim->draw("plane");
}

void Scene::createTextureObjects()
{
   // first delete any fbos and texture attachments if it has been created previously
  for(int i=0;i<3;i++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId[i]);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE)
    {
        glDeleteTextures(1, &m_fboTexId[i]);
        glDeleteTextures(1, &m_fboDepthId);
        glDeleteFramebuffers(1, &m_fboId[i]);
    }
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // create three color texture attachments
  // two for the two history ping pong fbos and one for the current frame fbo
  for(int i=0;i<3;i++)
  {
    //-------------Color Texture Attachments------------
    // create a texture object
    glGenTextures(1, &m_fboTexId[i]);

    // bind it to make it active
    glBindTexture(GL_TEXTURE_2D, m_fboTexId[i]);

    // set params
    // nearest neighbor filtering
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // clamped edges
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //glGenerateMipmapEXT(GL_TEXTURE_2D);
    // set the data size but just set the buffer to 0 as we will fill it with the FBO
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 TEXTURE_WIDTH,
                 TEXTURE_HEIGHT,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 NULL);
    // now turn the texture off for now
    glBindTexture(GL_TEXTURE_2D, 0);
    //-----------------------------------------------------
  }

  // create the depth attachment for the current frame (fboId[2])
  //--------------Depth Texture Attachment--------------
  glGenTextures(1, &m_fboDepthId);
  glBindTexture(GL_TEXTURE_2D, m_fboDepthId);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_DEPTH_COMPONENT,
               TEXTURE_WIDTH,
               TEXTURE_HEIGHT,
               0,
               GL_DEPTH_COMPONENT,
               GL_UNSIGNED_BYTE,
               NULL);
  // now turn the texture off for now
  glBindTexture(GL_TEXTURE_2D, 0);
  //-----------------------------------------------------

}

void Scene::createFramebufferObject()
{
  // make two fbos: fbo[0] and fbo[1], the history ping pong buffers
  // fbo[0] has attachments fboTexId[0] and fboDepthId[0], and similarly for fbo[1]...
  for(int i=0;i<2;i++)
  {
    // create framebuffer objects, these are deleted in the dtor
    glGenFramebuffers(1, &m_fboId[i]);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId[i]);

    // attatch the textures we created earlier to the FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexId[i], 0);

    // now got back to the default render context
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // check FBO attachment success
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
    GL_FRAMEBUFFER_COMPLETE  || !m_fboTexId[i])
    {
      printf("Framebuffer %d OK\n", i);
    }
  }

  // do the same for our current frame fbos
  glGenFramebuffers( 1, &m_fboId[2] );
  glBindFramebuffer( GL_FRAMEBUFFER, m_fboId[2] );
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexId[2], 0 );
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fboDepthId, 0 );
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  // check FBO attachment success
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
  GL_FRAMEBUFFER_COMPLETE  || !m_fboTexId[2] || !m_fboDepthId )
  {
    printf("Framebuffer 2 OK\n");
  }
}

void Scene::initEnvironment()
{
  // Enable seamless cube mapping
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  // Placing our environment map texture in texture unit 0
  glActiveTexture (GL_TEXTURE0);

  // Generate storage and a reference for our environment map texture
  glGenTextures (1, &m_envTex);

  // Bind this texture to the active texture unit
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_envTex);

  // Now load up the sides of the cube
//  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, "textures/sky_zneg.png");
//  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, "textures/sky_zpos.png");
//  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, "textures/sky_ypos.png");
//  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, "textures/sky_yneg.png");
//  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, "textures/sky_xneg.png");
//  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_POSITIVE_X, "textures/sky_xpos.png");

  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, "textures/yoko_zneg.jpg");
  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, "textures/yoko_zpos.jpg");
  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, "textures/yoko_ypos.jpg");
  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, "textures/yoko_yneg.jpg");
  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, "textures/yoko_xneg.jpg");
  initEnvironmentSide(GL_TEXTURE_CUBE_MAP_POSITIVE_X, "textures/yoko_xpos.jpg");

  // Generate mipmap levels
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  // Set the texture parameters for the cube map
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  GLfloat anisotropy;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);

  // Set our cube map texture to on the shader so we can use it
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->use("PBR");
  shader->setUniform("envTex", 0);
}

void Scene::initEnvironmentSide(GLenum target, const char *filename) {
    // Load up the image using NGL routine
    ngl::Image img(filename);

    // Transfer image data onto the GPU using the teximage2D call
    glTexImage2D (
      target,           // The target (in this case, which side of the cube)
      0,                // Level of mipmap to load
      img.format(),     // Internal format (number of colour components)
      img.width(),      // Width in pixels
      img.height(),     // Height in pixels
      0,                // Border
      GL_RGBA,          // Format of the pixel data
      GL_UNSIGNED_BYTE, // Data type of pixel data
      img.getPixels()   // Pointer to image data in memory
    );
}

void Scene::renderText()
{
  // set up our QPainter object
  QPainter painter(this);

  QString framerate = QString::number(m_framerate);
  painter.drawText(QPoint(20,30), "FPS: "+framerate );
}

void Scene::paintGL()
{
  // grab an instance of ngl shader manager and ngl primitives
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  ngl::VAOPrimitives* prim = ngl::VAOPrimitives::instance();
  // shader program id
  GLuint pid;

  // to help distinguish the ping pong fbos and their texture locations
  int pong = 1-m_ping;
  // the locations of the ping pong textures starts at location 4
  int historyLocation = 4;

  //------------------CurrentFrame FBO-------------------

  // set the rendering target to the "current frame" fbo
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fboId[2]); // always fbo[2]

  // clear the current rendering target
  glClearColor(0.2f,0.5f,0.8f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);

  // bind the current fbo's color attachment to a texture slot in memory
  // always TEXTURE2
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_fboTexId[2]);

  // bind the current fbo's depth attachment to a texture slot in memory
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, m_fboDepthId);

  // draw the scene to the current rendering target
  drawSceneGeometry(prim);

  if(m_TAA)
  {
    //-------------------History FBO--------------------

    // bind to one of the history ping-pong buffers (either fbo[1] or fbo[0])
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId[m_ping]);
    // clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // bind the blend fbo's color attachment to a texture slot in memory
    // (if it's fbo[0] to TEXTURE4, if it's fbo[1] to TEXTURE5)
    // because of the texture's location, it will become previous history in next frame
    glActiveTexture(GL_TEXTURE0 + historyLocation + m_ping);
    glBindTexture(GL_TEXTURE_2D, m_fboTexId[m_ping]);

    // use the TAA shader to draw our image onto the screen-oriented plane
    (*shader)["TAA"]->use();
    pid = shader->getProgramID("TAA");

    // send subroutine uniforms to the shader
    //GLuint *indices;
    GLuint nbrClamp_subroutine_index;
    if(m_subNbrClamp)
    {
      nbrClamp_subroutine_index = glGetSubroutineIndex(pid, GL_FRAGMENT_SHADER, "nbrClampOn" );
    }
    else
    {
      nbrClamp_subroutine_index = glGetSubroutineIndex(pid, GL_FRAGMENT_SHADER, "nbrClampOff" );
    }
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &nbrClamp_subroutine_index);

    // send textures to the shader
    glUniform1i(glGetUniformLocation(pid, "_currentFrameTex"),    2);
    glUniform1i(glGetUniformLocation(pid, "_currentDepthTex"),    3);
    glUniform1i(glGetUniformLocation(pid, "_previousFrameTex"),   historyLocation + pong); //either 5 or 4

    ngl::Vec3 currPos(m_view[12],m_view[13],m_view[14]);
    ngl::Vec3 prevPos(m_viewPrev[12],m_viewPrev[13],m_viewPrev[14]);

    // send other parameter uniforms
    shader->setUniform("_windowSize",           ngl::Vec2(TEXTURE_WIDTH,TEXTURE_HEIGHT) );
    shader->setUniform("_start",                m_start                                 );
    shader->setUniform("_viewInverse",          m_view.inverse()                        );
    shader->setUniform("_viewPrev",             m_viewPrev                              );
    shader->setUniform("_deltaDist",            currPos-prevPos                         );
    shader->setUniform("_projection",           m_projection                            );
    shader->setUniform("_projectionInverse",    m_projection.inverse()                  );
    // multiply the jitter offset by 0.5 to fit the canonical view volume
    shader->setUniform("_jitter",               m_jitter.getOffset()*ngl::Vec2(0.5,0.5) );

    // draw the plane
    drawScreenOrientedPlane(pid,prim);
  }

  //--------------------Default FBO----------------------

  // bind to the default framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);

  // use the Texture shader to draw our image onto the screen-oriented plane
  (*shader)["Zoom"]->use();
  pid = shader->getProgramID("Zoom");

  // send zoom subroutine to the shader
  GLuint zoom_subroutine_index;
  if(m_cam.zoom)
  {
    zoom_subroutine_index = glGetSubroutineIndex(pid, GL_FRAGMENT_SHADER, "on" );
  }
  else
  {
    zoom_subroutine_index = glGetSubroutineIndex(pid, GL_FRAGMENT_SHADER, "off" );
  }
  glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &zoom_subroutine_index);

  // send the history output texture to the GPU
  glUniform1i(glGetUniformLocation(pid, "_frameTex"), (historyLocation+m_ping)*m_TAA + !m_TAA*2 ); // either 3 or 4, or 1 (current frame, no TAA)

  // send parameters for zoom
  shader->setUniform("_mousePos",             ngl::Vec2(m_cam.cursorX, -m_cam.cursorY)     );
  shader->setUniform("_windowSize",           ngl::Vec2(TEXTURE_WIDTH,TEXTURE_HEIGHT)      );
  // the dimensions of the texture when zoomed-in
  shader->setUniform("_zoomSize",             ngl::Vec2(TEXTURE_WIDTH*m_cam.zoomScale,
                                                        TEXTURE_HEIGHT*m_cam.zoomScale)    );
  shader->setUniform("_zoomSquare",           m_cam.zoomSquare                             );

  // draw the plane
  drawScreenOrientedPlane(pid, prim);

  //-----------------------------------------------------

  // switch the rendering target index (from 0 -> 1, or from 1 -> 0)
  m_ping = pong;

  // store the current view matrix for next frame's "previous" view matrix
  m_viewPrev = m_view;

  // no longer the first frame
  m_start = false;

  // render the text that displays the current TAA settings
  //renderText();

  // update the image
  update();
}
