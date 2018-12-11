#include <OgreRoot.h>
#include "../core/CameraManager.h"
#include "../core/DebugDraw.h"
#include "../core/Logger.h"
#include "../core/Timer.h"
#include "HudManager.h"



template<>HudManager *Ogre::Singleton< HudManager >::msSingleton = NULL;



HudManager::HudManager()
{
    m_Camera = CameraManager::getSingleton().GetCurrentCamera();
    m_Viewport = CameraManager::getSingleton().GetCurrentViewport();

    LOG_TRIVIAL( "HudManager created." );
}



HudManager::~HudManager()
{
    LOG_TRIVIAL( "HudManager destroyed." );
}



void
HudManager::Input( const Event& event )
{
    if( event.type == ET_PRESS && event.button == OIS::MB_Left )
    {
        Ogre::Ray ray = m_Camera->getCameraToViewportRay( event.param3 / m_Viewport->getActualWidth(), event.param4 / m_Viewport->getActualHeight() );
        Ogre::Plane plane( Ogre::Vector3::UNIT_Z, 0 );
        std::pair< bool, Ogre::Real > res = ray.intersects( plane );
        Ogre::Vector3 point = ray.getPoint( res.second );

        LOG_ERROR( "HUD: " + Ogre::StringConverter::toString( point ) + " " + Ogre::StringConverter::toString( ray.getDirection() ) );

        m_Selection = true;
        m_SelectionStart = point;
    }
    else if( event.type == ET_RELEASE && event.button == OIS::MB_Left )
    {
        EntityManager::getSingleton().SetEntitySelection( m_SelectionStart, m_SelectionEnd );

        m_Selection = false;
        m_SelectionEnd = Ogre::Vector3::ZERO;
    }
    else if( event.type == ET_MOUSE_MOVE && m_Selection == true )
    {
        Ogre::Ray ray = m_Camera->getCameraToViewportRay( event.param3 / m_Viewport->getActualWidth(), event.param4 / m_Viewport->getActualHeight() );
        Ogre::Plane plane( Ogre::Vector3::UNIT_Z, 0 );
        std::pair< bool, Ogre::Real > res = ray.intersects( plane );
        Ogre::Vector3 point = ray.getPoint( res.second );

        m_SelectionEnd = point;
    }
}



void
HudManager::Update()
{
}



void
HudManager::UpdateDebug()
{
    if( m_Selection == true )
    {
        Ogre::Vector3 start = m_Camera->ProjectPointToScreen( m_SelectionStart );
        Ogre::Vector3 end = m_Camera->ProjectPointToScreen( m_SelectionEnd );

        DEBUG_DRAW.SetColour( Ogre::ColourValue( 0, 1, 0, 0.3f) );
        DEBUG_DRAW.Quad( start.x, start.y, end.x, start.y, end.x, end.y, start.x, end.y );
    }
}
