#include "NGLScene.h"
#include <glm/gtc/type_ptr.hpp>
#include <ngl/NGLInit.h>
#include <ngl/NGLStream.h>
#include <ngl/ShaderLib.h>
#include <ngl/VAOPrimitives.h>
#include <QGuiApplication>
#include <QMouseEvent>


NGLScene::NGLScene()
{
  setTitle( "Demo" );
}


NGLScene::~NGLScene()
{
  std::cout << "Shutting down NGL, removing VAO's and Shaders\n";
}


void NGLScene::resizeGL( int _w, int _h )
{
  m_projection=ngl::perspective( 45.0f, static_cast<float>( _w ) / _h, 0.1f, 200.0f );

  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}
const static int TEXTURE_WIDTH=640;
const static int TEXTURE_HEIGHT=480;

const static int WINDOW_WIDTH=1024;
const static int WINDOW_HEIGHT=720;

void NGLScene::initializeGL()
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

//  // enable multisampling for smoother drawing
//  glEnable( GL_MULTISAMPLE );

  // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib* shader = ngl::ShaderLib::instance();

  // create the shader program
  shader->createShaderProgram( "PBR" );
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader( "PBRVertex", ngl::ShaderType::VERTEX );
  shader->attachShader( "PBRFragment", ngl::ShaderType::FRAGMENT );
  // attach the source
  shader->loadShaderSource( "PBRVertex", "shaders/PBRVertex.glsl" );
  shader->loadShaderSource( "PBRFragment", "shaders/PBRFragment.glsl" );
  // compile the shaders
  shader->compileShader( "PBRVertex" );
  shader->compileShader( "PBRFragment" );
  // add them to the program
  shader->attachShaderToProgram( "PBR", "PBRVertex" );
  shader->attachShaderToProgram( "PBR", "PBRFragment" );
  // now we have associated that data we can link the shader
  shader->linkProgramObject( "PBR" );

  // create the TAA shader program
  shader->loadShader("TAA",                // Name of program
                     "shaders/TAAVertex.glsl",     // Vertex shader
                     "shaders/TAAFragment.glsl");    // Fragment shader

  // create a simple shader program to pass textures
  shader->loadShader("Texture",                // Name of program
                     "shaders/TextureVertex.glsl",     // Vertex shader
                     "shaders/TextureFragment.glsl");    // Fragment shader

  // Create a screen oriented plane
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
  prim->createTrianglePlane("plane",2,2,1,1,ngl::Vec3(0,1,0));

  ( *shader )[ "PBR" ]->use();
 // We now create our view matrix for a static camera
  ngl::Vec3 from( 0.0f, 2.0f, 2.0f );
  ngl::Vec3 to( 0.0f, 0.0f, 0.0f );
  ngl::Vec3 up( 0.0f, 1.0f, 0.0f );
  // now load to our new camera
  m_view=ngl::lookAt(from,to,up);
  shader->setUniform( "camPos", from );
  // now a light
  m_lightPos.set( 0.0, 2.0f, 2.0f ,1.0f);
  // setup the default shader material and light porerties
  // these are "uniform" so will retain their values
  shader->setUniform("lightPosition",m_lightPos.toVec3());
  shader->setUniform("lightColor",400.0f,400.0f,400.0f);
  shader->setUniform("exposure",2.2f);
  shader->setUniform("albedo",0.950f, 0.71f, 0.29f);

  shader->setUniform("metallic",1.02f);
  shader->setUniform("roughness",0.38f);
  shader->setUniform("ao",0.2f);
  ngl::VAOPrimitives::instance()->createTrianglePlane("floor",20,20,1,1,ngl::Vec3::up());
  shader->printRegisteredUniforms("PBR");
  shader->use(ngl::nglCheckerShader);
  shader->setUniform("lightDiffuse",1.0f,1.0f,1.0f,1.0f);
  shader->setUniform("checkOn",true);
  shader->setUniform("lightPos",m_lightPos.toVec3());
  shader->setUniform("colour1",0.9f,0.9f,0.9f,1.0f);
  shader->setUniform("colour2",0.6f,0.6f,0.6f,1.0f);
  shader->setUniform("checkSize",60.0f);
  shader->printRegisteredUniforms(ngl::nglCheckerShader);

  // create the fbos and their texture attach
  createTextureObject();
  createFramebufferObject();
}

void NGLScene::loadMatricesToShader(ngl::Mat4 _jitter)
{
  ngl::ShaderLib* shader = ngl::ShaderLib::instance();
  shader->use("PBR");
  struct transform
  {
    ngl::Mat4 MVP;
    ngl::Mat4 normalMatrix;
    ngl::Mat4 M;
  };

  transform t;
  t.M=m_view*m_mouseGlobalTX;

  t.MVP=_jitter*m_projection*t.M;
  t.normalMatrix=t.M;
  t.normalMatrix.inverse().transpose();
  shader->setUniformBuffer("TransformUBO",sizeof(transform),&t.MVP.m_00);

  if(m_transformLight)
  {
    shader->setUniform("lightPosition",(m_mouseGlobalTX*m_lightPos).toVec3());
  }
}

glm::mat4 NGLScene::jitterMatrix2x()
{
  float jitterDistance = 0.5;
  glm::vec3 jitterTranslation = glm::vec3((jitterDistance*(1-2*float(m_jitterCycle)))/TEXTURE_WIDTH,
                                          (jitterDistance*(1-2*float(m_jitterCycle)))/TEXTURE_HEIGHT,
                                          0.0f);
  glm::mat4 jitterMatrix = glm::translate(glm::mat4(1.0), jitterTranslation);
  return jitterMatrix;
}

glm::mat4 NGLScene::jitterMatrixQuincunx()
{
  float jitterDistance = 0.5;
  glm::vec2 jitterTranslation;
  switch(m_jitterCycle)
  {
    case 0:
      jitterTranslation = glm::vec2(-jitterDistance,-jitterDistance);
      break;
    case 1:
      jitterTranslation = glm::vec2(jitterDistance,-jitterDistance);
      break;
    case 2:
      jitterTranslation = glm::vec2(jitterDistance,jitterDistance);
      break;
    case 3:
      jitterTranslation = glm::vec2(-jitterDistance,jitterDistance);
      break;
    case 4:
      jitterTranslation = glm::vec2(0.0,0.0);
      break;
  }
  return glm::translate(glm::mat4(1.0),
                        glm::vec3(jitterTranslation[0]/TEXTURE_WIDTH,jitterTranslation[1]/TEXTURE_HEIGHT,0.0));
}
void NGLScene::updateJitter(int samples)
{
  m_jitterCycle = (m_jitterCycle + 1) % samples;
  printf("%d\n",m_jitterCycle);
}

void NGLScene::drawSceneGeometry()
{ 
  // grab an instance of the shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["PBR"]->use();

  // create the jitter matrix
  m_jitterMatrix = jitterMatrixQuincunx();

  // update the jitter matrix
  updateJitter(5);

  // Rotation based on the mouse position for our global transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX( m_win.spinXFace );
  rotY.rotateY( m_win.spinYFace );
  // multiply the rotations
  m_mouseGlobalTX = rotX * rotY;
  // add the translations
  m_mouseGlobalTX.m_m[ 3 ][ 0 ] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[ 3 ][ 1 ] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[ 3 ][ 2 ] = m_modelPos.m_z;

  // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives* prim = ngl::VAOPrimitives::instance();

  // send matrices to shader
  shader->use(ngl::nglCheckerShader);
  ngl::Mat4 tx;
  tx.translate(0.0f,-0.45f,0.0f);
  ngl::Mat4 MVP=m_jitterMatrix*m_projection*m_view*m_mouseGlobalTX*tx;
  ngl::Mat3 normalMatrix=m_view*m_mouseGlobalTX;
  normalMatrix.inverse().transpose();
  shader->setUniform("MVP",MVP);
  shader->setUniform("normalMatrix",normalMatrix);

  // send light information to shader
  if(m_transformLight)
  {
    shader->setUniform("lightPosition",(m_mouseGlobalTX*m_lightPos).toVec3());
  }

  // draw objects:
  // floor
  prim->draw("floor");
  // teapot
  loadMatricesToShader(m_jitterMatrix);
  prim->draw("teapot");
}

void NGLScene::drawScreenOrientedPlane(GLuint _pid)
{
  // grab an instance of ngl VAO primitives
  ngl::VAOPrimitives* prim = ngl::VAOPrimitives::instance();

  // the plane MVP
  glm::mat4 MVP_plane = glm::rotate(glm::mat4(1.0f), glm::pi<float>() * 0.5f, glm::vec3(1.0f,0.0f,0.0f));
  glUniformMatrix4fv(glGetUniformLocation(_pid, "MVP"), 1, false, glm::value_ptr(MVP_plane));

  // draw the screen-oriented plane (the final image)
  prim->draw("plane");
}



void NGLScene::createTextureObject()
{
   // first delete any fbos and texture attachments if it has been created previously
  for(int i=0;i<2;i++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId[i]);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE)
    {
        glDeleteTextures(1, &m_fboTexId[i]);
        glDeleteTextures(1, &m_fboDepthId[i]);
        glDeleteFramebuffers(1, &m_fboId[i]);
    }
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // create two of each texture attachments (color and depth)
  for(int i=0;i<3;i++)
  {
    //-------------Color Texture Attachments------------
    // create a texture object
    glGenTextures(1, &m_fboTexId[i]);
    // bind it to make it active

    //glActiveTexture(GL_TEXTURE2+i);
    glBindTexture(GL_TEXTURE_2D, m_fboTexId[i]);
    // set params
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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

    //--------------Depth Texture Attachments--------------
    // create a texture object
    glGenTextures(1, &m_fboDepthId[i]);
    // bind it to make it active
    //glActiveTexture(GL_TEXTURE4+i);
    glBindTexture(GL_TEXTURE_2D, m_fboDepthId[i]);
    // set params
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glGenerateMipmapEXT(GL_TEXTURE_2D);
    // set the data size but just set the buffer to 0 as we will fill it with the FBO
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_DEPTH_COMPONENT,
                 TEXTURE_WIDTH,
                 TEXTURE_HEIGHT,
                 0,
                 GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_BYTE,
                 0);
    // now turn the texture off for now
    glBindTexture(GL_TEXTURE_2D, 0);
    //-----------------------------------------------------
  }
}

void NGLScene::createFramebufferObject()
{
  // make two fbos: fbo[0] and fbo[1]
  // fbo[0] has attachments fboTexId[0] and fboDepthId[0], and vice versa for fbo[1]...
  for(int i=0;i<3;i++)
  {
    // create framebuffer objects, these are deleted in the dtor
    glGenFramebuffers(1, &m_fboId[i]);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId[i]);

    // attatch the textures we created earlier to the FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexId[i], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fboDepthId[i], 0);

    //Set the fragment shader output targets (DEPTH_ATTACHMENT is done automatically)
//    GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0};
//    glDrawBuffers(1, drawBufs);

    // now got back to the default render context
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // check FBO attachment success
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
    GL_FRAMEBUFFER_COMPLETE  || !m_fboDepthId[i]  || !m_fboTexId[i])
    {
      printf("Framebuffer %d OK\n", i);
    }
  }
}

void NGLScene::paintGL()
{
  // update the image
  update();

  //------------------CurrentFrame FBO-------------------

  // set the rendering target to the "current frame" fbo
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fboId[2]);

  // clear the current rendering target
  glClearColor(0,0.2f,0.8f,1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);

  // draw the scene to the current rendering target
  drawSceneGeometry();

  // bind the current fbo's color attachment to a texture slot in memory
  // always TEXTURE3
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, m_fboTexId[2]);

  // bind the current fbo's depth attachment to a texture slot in memory
  // always TEXTURE4
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, m_fboDepthId[2]);

  //-----------------PreviousHistory FBO-----------------

  // bind to the "previous history" fbo
  glBindFramebuffer(GL_FRAMEBUFFER, m_fboId[1-m_targetId]);

  // bind the accumulation fbo's color attachment to a texture slot in memory
  // (if it's fbo[0] to TEXTURE6, if it's fbo[1] TEXTURE5)
  glActiveTexture(GL_TEXTURE0 + (1-m_targetId)+5);
  glBindTexture(GL_TEXTURE_2D, m_fboTexId[1-m_targetId]);

  //-------------------NewHistory FBO--------------------

  // bind to the "new history" frame buffer to draw the blend between
  // the "previous history" and the "current frame"
  glBindFramebuffer(GL_FRAMEBUFFER, m_fboId[m_targetId]);
  // clear
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // grab an instance of ngl shader manager and ngl primitives
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  // use the TAA shader to draw our image onto the screen-oriented plane
  (*shader)["TAA"]->use();
  GLuint pid = shader->getProgramID("TAA");

  // send the uniforms to the TAA shader
  // textures
  glUniform1i(glGetUniformLocation(pid, "_currentFrameTex"), 3);
  glUniform1i(glGetUniformLocation(pid, "_currentDepthTex"), 4);
  glUniform1i(glGetUniformLocation(pid, "_previousFrameTex"), (1-m_targetId)+5);
  // window size
  glUniform2f(glGetUniformLocation(pid, "_textureSize"), TEXTURE_WIDTH, TEXTURE_HEIGHT);
  // view matrix
  shader->setUniform("_view", m_view);
  shader->setUniform("_viewInverse", m_view.inverse());
  shader->setUniform("_viewPrev", m_viewPrev);
  shader->setUniform("_projection", m_projection);
  shader->setUniform("_projectionInverse", m_projection.inverse());
  //shader->setUniform("_jitter", m_jitterMatrix.inverse()*glm::vec4(0.0,0.0,0.0,1.0));
  shader->setUniform("_jitter", glm::vec2(m_jitterMatrix[12],m_jitterMatrix[13]));

  std::cout << "prev V pos" << ngl::Vec3(m_viewPrev[12],m_viewPrev[13],m_viewPrev[14]) << "\n";
  std::cout << "curr V pos" << ngl::Vec3(m_view[12],m_view[13],m_view[14]) << "\n";
  std::cout << "jittermatrix" << ngl::Vec3(m_jitterMatrix[12],m_jitterMatrix[13],m_jitterMatrix[14]) << "\n";

  // store the current view matrix for next frame's "previous" view matrix
  m_viewPrev = m_view;

  // draw the plane
  drawScreenOrientedPlane(pid);

  // bind the blend fbo's color attachment to a texture slot in memory
  // (if it's fbo[0] to TEXTURE5, if it's fbo[1] to TEXTURE6)
  glActiveTexture(GL_TEXTURE0 + m_targetId + 5);
  glBindTexture(GL_TEXTURE_2D, m_fboTexId[m_targetId]);

  //--------------------Default FBO----------------------

  // bind to the default framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

  // use the Texture shader to draw our image onto the screen-oriented plane
  (*shader)["Texture"]->use();
  pid = shader->getProgramID("Texture");

  // send the NewHistory/output texture to the GPU
  glUniform1i(glGetUniformLocation(pid, "frameTex"), m_targetId+5);
  glUniform2f(glGetUniformLocation(pid, "windowSize"), WINDOW_WIDTH, WINDOW_HEIGHT);

  // draw the plane
  drawScreenOrientedPlane(pid);

  // switch the rendering target id (from 0 -> 1, or from 1 -> 0)
  m_targetId = 1-m_targetId;
}

//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent( QKeyEvent* _event )
{
  // that method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch ( _event->key() )
  {
    // escape key to quit
    case Qt::Key_Escape:
      QGuiApplication::exit( EXIT_SUCCESS );
      break;
  // move the camera
  #ifndef USINGIOS_

    // forward
    case Qt::Key_W:
    {
      ngl::Mat4 i = ngl::Mat4(1.0);
      i.translate(0,0,0.01);
      m_view = m_view * i;
    }
      break;
    // backward
    case Qt::Key_S:
    {
      ngl::Mat4 i = ngl::Mat4(1.0);
      i.translate(0,0,-0.01);
      m_view = m_view * i;
    }
      break;
    // left
    case Qt::Key_A:
    {
      ngl::Mat4 i = ngl::Mat4(1.0);
      i.translate(0.01,0,0);
      m_view = m_view * i;
    }
      break;
    // right
    case Qt::Key_D:
    {
      ngl::Mat4 i = ngl::Mat4(1.0);
      i.translate(-0.01,0,0);
      m_view = m_view * i;
    }
      break;
    // up
    case Qt::Key_Q:
    {
      ngl::Mat4 i = ngl::Mat4(1.0);
      i.translate(0,-0.01,0);
      m_view = m_view * i;
    }
      break;
    // down
    case Qt::Key_E:
    {
      ngl::Mat4 i = ngl::Mat4(1.0);
      i.translate(0,0.01,0);
      m_view = m_view * i;
    }
      break;
  #endif
    // show full screen
    case Qt::Key_F:
      showFullScreen();
      break;
    // show windowed
    case Qt::Key_N:
      showNormal();
      break;
    case Qt::Key_Space :
      m_win.spinXFace=0;
      m_win.spinYFace=0;
      m_modelPos.set(ngl::Vec3::zero());
    break;

    case Qt::Key_L :
    m_transformLight^=true;
    break;
    default:
      break;
  }
  update();
}
