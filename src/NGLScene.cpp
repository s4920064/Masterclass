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
const static int TEXTURE_WIDTH=1024;
const static int TEXTURE_HEIGHT=720;

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

  // ha no not yet
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

  // create the shader program
  shader->loadShader("TAA",                // Name of program
                     "shaders/TAAVertex.glsl",     // Vertex shader
                     "shaders/TAAFragment.glsl");    // Fragment shader

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

}

void NGLScene::createTextureObject()
{
  // First delete the FBO if it has been created previously
  glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE) {
      glDeleteTextures(1, &m_fboCurrentTexId);
      glDeleteTextures(1, &m_fboCurrentDepthId);
      glDeleteFramebuffers(1, &m_fboId);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  //------------Current Frame Texture Buffer-------------
  // create a texture object
  glGenTextures(1, &m_fboCurrentTexId);
  // bind it to make it active
  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_2D, m_fboCurrentTexId);
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

  //------------Current Frame Depth Texture Buffer-------------
  // create a texture object
  glGenTextures(1, &m_fboCurrentDepthId);
  // bind it to make it active
  glActiveTexture(GL_TEXTURE7);
  glBindTexture(GL_TEXTURE_2D, m_fboCurrentDepthId);
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

//  //-----------Previous Frame Texture Buffer-------------
//  // create a texture object
//  glGenTextures(1, &m_fboPreviousTexId);
//  // bind it to make it active
//  glActiveTexture(GL_TEXTURE6);
//  glBindTexture(GL_TEXTURE_2D, m_fboPreviousTexId);
//  // set params
//  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//  //glGenerateMipmapEXT(GL_TEXTURE_2D);
//  // set the data size but just set the buffer to 0 as we will fill it with the FBO
//  glTexImage2D(GL_TEXTURE_2D,
//               0,
//               GL_RGBA8,
//               TEXTURE_WIDTH,
//               TEXTURE_HEIGHT,
//               0,
//               GL_RGBA,
//               GL_UNSIGNED_BYTE,
//               0);
//  // now turn the texture off for now
//  glBindTexture(GL_TEXTURE_2D, 0);
//  //-----------------------------------------------------
}

void NGLScene::createFramebufferObject()
{
  // create a framebuffer object this is deleted in the dtor
  glGenFramebuffers(1, &m_fboId);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);

  // attatch the textures we created earlier to the FBO
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboCurrentTexId, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fboCurrentDepthId, 0);
  //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboPreviousTexId, 0);

  // Set the fragment shader output targets (DEPTH_ATTACHMENT is done automatically)
  GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, drawBufs);

  // now got back to the default render context
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void NGLScene::loadMatricesToShader()
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

   t.MVP=m_projection*t.M;
   t.normalMatrix=t.M;
   t.normalMatrix.inverse().transpose();
   shader->setUniformBuffer("TransformUBO",sizeof(transform),&t.MVP.m_00);

   if(m_transformLight)
   {
     shader->setUniform("lightPosition",(m_mouseGlobalTX*m_lightPos).toVec3());

   }
}

void NGLScene::paintGL()
{
  //----------------------------------------------------------------
  // draw to our FBO first
  //----------------------------------------------------------------

  createTextureObject();
  createFramebufferObject();

  // we are now going to draw to our FBO
  // set the rendering destination to FBO
  glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
  // set the background colour (using blue to show it up)
  glClearColor(0,0.4f,0.5f,1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // set our viewport to the size of the texture
  // if we want a different camera we would set this here
  glViewport(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);

  //----------------------------------------------------------------------------------------------------------------------
  // now we are going to draw to the normal GL buffer and use the texture created
  // in the previous render to draw to our objects
  //----------------------------------------------------------------------------------------------------------------------
  //shader->use("PBR");

  //-------------------------Draw Geometry------------------------------
  // grab an instance of the shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["PBR"]->use();

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

  shader->use(ngl::nglCheckerShader);
  ngl::Mat4 tx;
  tx.translate(0.0f,-0.45f,0.0f);
  ngl::Mat4 MVP=m_projection*m_view*m_mouseGlobalTX*tx;
  ngl::Mat3 normalMatrix=m_view*m_mouseGlobalTX;
  normalMatrix.inverse().transpose();
  shader->setUniform("MVP",MVP);
  shader->setUniform("normalMatrix",normalMatrix);
  if(m_transformLight)
  {
    shader->setUniform("lightPosition",(m_mouseGlobalTX*m_lightPos).toVec3());

  }

  prim->draw("floor");

  //draw
  loadMatricesToShader();
  prim->draw("teapot");
  //--------------------------------------------------------------------

  // first bind the normal render buffer
  //glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // do any mipmap generation
  //glGenerateMipmap(GL_TEXTURE_2D);

  // set the screen for a different clear colour
  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // clear this screen
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0,0,TEXTURE_WIDTH,TEXTURE_HEIGHT);

  // now enable the textures we just rendered to
  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_2D, m_fboCurrentTexId);

//  glActiveTexture(GL_TEXTURE7);
//  glBindTexture(GL_TEXTURE_2D, m_fboPreviousTexId);

  glActiveTexture(GL_TEXTURE6);
  glBindTexture(GL_TEXTURE_2D, m_fboCurrentDepthId);

  // get the new shader and set the new viewport size
  (*shader)["TAA"]->use();
  GLuint pid = shader->getProgramID("TAA");

  // send all the textures to the GPU
  glUniform1i(glGetUniformLocation(pid, "currentFrameTex"), 5);
  //glUniform1i(glGetUniformLocation(pid, "PreviousFrameTex"), 6);
  glUniform2f(glGetUniformLocation(pid, "windowSize"), TEXTURE_WIDTH, TEXTURE_HEIGHT);

  // this takes into account retina displays etc
  glViewport(0, 0, static_cast<GLsizei>(width() * devicePixelRatio()), static_cast<GLsizei>(height() * devicePixelRatio()));

  // set the MVP for the plane
  glm::mat4 MVP_plane = glm::rotate(glm::mat4(1.0f), glm::pi<float>() * 0.5f, glm::vec3(1.0f,0.0f,0.0f));
  glUniformMatrix4fv(glGetUniformLocation(pid, "MVP"), 1, false, glm::value_ptr(MVP_plane));

  // draw the plane
  prim->draw("plane");

  // bind the plane texture
  glBindTexture(GL_TEXTURE_2D, 0);
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
// turn on wirframe rendering
#ifndef USINGIOS_
    case Qt::Key_W:
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
      break;
    // turn off wire frame
    case Qt::Key_S:
      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
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
