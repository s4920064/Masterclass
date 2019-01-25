#include "Camera.h"
#include "Scene.h"
#include <QMouseEvent>
#include <QGuiApplication>
#include <ngl/Mat4.h>
#include <iostream>

//----------------------------------------------------------------------------------------------------------------------
void Scene::mouseMoveEvent( QMouseEvent* _event )
{
  // if there has been a rotation
  if(m_cam.rotate)
  {
    // set pitch and yaw according to cursor movement
    float deltaYaw = mapToGlobal(QPoint(width() / 2, height() / 2)).x() - m_cam.cursor.pos().x();
    float deltaPitch = mapToGlobal(QPoint(width() / 2, height() / 2)).y() - m_cam.cursor.pos().y();

    // reset the cursor to the center of the screen
    m_cam.cursor.setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));

    // scale by sensitivity
    deltaYaw *= -SENSITIVITY;
    deltaPitch *= SENSITIVITY;

    // set the total pitch and yaw of the camera
    m_cam.yaw += deltaYaw;
    m_cam.pitch += deltaPitch;

    // clamp the pitch so that the camera can't do a filp
    if(m_cam.pitch > 89.0f)
        m_cam.pitch = 89.0f;
    if(m_cam.pitch < -89.0f)
        m_cam.pitch = -89.0f;

    // temporary vector to apply transformations to
    glm::vec3 front;

    front.x = cos(glm::radians(m_cam.yaw)) * cos(glm::radians(m_cam.pitch));
    front.y = sin(glm::radians(m_cam.pitch));
    front.z = sin(glm::radians(m_cam.yaw)) * cos(glm::radians(m_cam.pitch));

    // apply the camera rotations to our camera's directional vector
    m_cam.front = glm::normalize(front);
  }

  // if the zoom is on (left mouse button has been clicked)
  if(m_cam.zoom)
  {
    // update the cursor coordinates
    m_cam.cursorX = -_event->x()+width()/2;
    m_cam.cursorY = -_event->y()+height()/2;

    // update the zoom square
    m_cam.zoomSquare = glm::vec4(-m_cam.cursorX+m_cam.zoomSize+0.5f*TEXTURE_WIDTH,
                                 -m_cam.cursorX-m_cam.zoomSize+0.5f*TEXTURE_WIDTH,
                                 m_cam.cursorY+m_cam.zoomSize+0.5f*TEXTURE_HEIGHT,
                                 m_cam.cursorY-m_cam.zoomSize+0.5f*TEXTURE_HEIGHT );

  }
}

//----------------------------------------------------------------------------------------------------------------------
void Scene::mousePressEvent( QMouseEvent* _event )
{
  if(_event->button() == Qt::RightButton)
  {
    // the camera is rotating
    m_cam.rotate = true;
    // set the cursor to the center of the screen
    m_cam.cursor.setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
    // hide the cursor (not working?)
    m_cam.cursor.setShape(Qt::BlankCursor);
  }
  if(_event->button() == Qt::LeftButton)
  {
    m_cam.zoom = true;
    // update the cursor coordinates
    m_cam.cursorX = -_event->x()+width()/2;
    m_cam.cursorY = -_event->y()+height()/2;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void Scene::mouseReleaseEvent( QMouseEvent* _event )
{
  if(_event->button() == Qt::RightButton)
  {
    // stop rotating
    m_cam.rotate = false;
    // show cursor
    m_cam.cursor.setShape(Qt::ArrowCursor);
  }
  if(_event->button() == Qt::LeftButton)
  {
    m_cam.zoom = false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void Scene::keyPressEvent( QKeyEvent* _event )
{
  // that method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow

  switch ( _event->key() )
  {
    // move the camera
    #ifndef USINGIOS_

    // forward
    case Qt::Key_W:
    {
      m_cam.translateF = 1;
    }
      break;
    // backward
    case Qt::Key_S:
    {
      m_cam.translateB = -1;
    }
      break;
    // left
    case Qt::Key_A:
    {
      m_cam.translateL = -1;
    }
      break;
    // right
    case Qt::Key_D:
    {
      m_cam.translateR = 1;
    }
      break;
    // up
    case Qt::Key_Q:
    {

    }
      break;
    // down
    case Qt::Key_E:
    {

    }
    #endif
      break;
    case Qt::Key_T:
      // toggle TAA on/off
      m_TAA = !m_TAA;
      break;
    case Qt::Key_M:
      break;
    // show full screen
    case Qt::Key_F:
      showFullScreen();
      break;
    // show windowed
    case Qt::Key_Escape:
      showNormal();
      break;
    // toggle neighborhood clamping
    case Qt::Key_N:
      m_subNbrClamp = !m_subNbrClamp;
      break;
    // toggle reprojection
    case Qt::Key_R:
      m_subReproject = !m_subReproject;
      break;

    case Qt::Key_L :
    m_transformLight^=true;
    break;
    default:
      break;
  }

}

void Scene::keyReleaseEvent( QKeyEvent* _event )
{
  // that method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  // move the camera
  #ifndef USINGIOS_
  switch ( _event->key() )
  {
    // forward
    case Qt::Key_W:
    {
      m_cam.translateF = 0;
    }
      break;
    // backward
    case Qt::Key_S:
    {
      m_cam.translateB = 0;
    }
      break;
    // left
    case Qt::Key_A:
    {
      m_cam.translateL = 0;
    }
      break;
    // right
    case Qt::Key_D:
    {
      m_cam.translateR = 0;
    }
      break;
    // up
    case Qt::Key_Q:
    {

    }
      break;
    // down
    case Qt::Key_E:
    {

    }
  #endif
  }
}
