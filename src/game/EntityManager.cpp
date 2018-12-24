#include <OgreRoot.h>
#include "../core/CameraManager.h"
#include "../core/DebugDraw.h"
#include "../core/Logger.h"
#include "../core/Timer.h"
#include "Entity.h"
#include "EntityManager.h"
#include "EntityManagerCommands.h"
#include "EntityXmlFile.h"
#include "MapXmlFile.h"



template<>EntityManager *Ogre::Singleton< EntityManager >::msSingleton = NULL;

std::vector< Ogre::Vector3 > place_finder_ignore;



EntityManager::EntityManager()
{
    LOG_TRIVIAL( "EntityManager created." );

    InitCmd();

    m_Hud = new HudManager();

    EntityXmlFile* desc_file = new EntityXmlFile( "data/entities.xml" );
    desc_file->LoadDesc();
    delete desc_file;

    m_SceneManager = Ogre::Root::getSingletonPtr()->getSceneManager( "Scene" );
    m_SceneNode = m_SceneManager->getRootSceneNode()->createChildSceneNode( "EntityManager" );

    MapXmlFile* map_loader = new MapXmlFile( "data/map/test.xml" );
    map_loader->LoadMap( m_MapSector );
    map_loader->LoadEntities();
    delete map_loader;

    Ogre::SceneNode* node = m_SceneNode->createChildSceneNode( "Map" );
    node->attachObject( &m_MapSector );
}



EntityManager::~EntityManager()
{
    for( unsigned int i = 0; i < m_Entities.size(); ++i )
    {
        delete m_Entities[ i ];
    }

    m_SceneManager->getRootSceneNode()->removeAndDestroyChild( "EntityManager" );

    delete m_Hud;

    LOG_TRIVIAL( "EntityManager destroyed." );
}



void
EntityManager::Input( const Event& event )
{
    m_Hud->Input( event );
}



void
EntityManager::Update()
{
    float delta = Timer::getSingleton().GetGameTimeDelta();

    for( size_t i = 0; i < m_EntitiesMovable.size(); ++i )
    {
        Ogre::Vector3 start = m_EntitiesMovable[ i ]->GetPosition();
        Ogre::Vector3 end = m_EntitiesMovable[ i ]->GetMoveNextPosition();

        float speed = 2.0f;

        if( start != end )
        {
            Ogre::Vector3 end_start = end - start;
            float length = end_start.length();
            end_start.normalise();

            if( length <= ( end_start * speed * delta ).length() )
            {
                m_EntitiesMovable[ i ]->SetPosition( end );
                Ogre::Vector3 pos_e = m_EntitiesMovable[ i ]->GetMoveEndPosition();

                LOG_ERROR( "Update for entity " + Ogre::StringConverter::toString( i ) + ": cur_pos=" + Ogre::StringConverter::toString( end ) + ", final_pos=" + Ogre::StringConverter::toString( pos_e ) );

                if( pos_e != end )
                {
                    Ogre::Vector3 pos_s = m_EntitiesMovable[ i ]->GetMoveNextPosition();
                    place_finder_ignore.clear();
                    m_EntitiesMovable[ i ]->SetMovePath( AStarFinder( m_EntitiesMovable[ i ], pos_e.x, pos_e.y ) );

                    LOG_ERROR( "    path for entity " + Ogre::StringConverter::toString( m_EntitiesMovable[ i ] ) + ":" );
                    std::vector< Ogre::Vector3 > path = m_EntitiesMovable[ i ]->GetMovePath();
                    for( size_t j = 0; j < path.size(); ++j )
                    {
                        LOG_ERROR( "        " + Ogre::StringConverter::toString( path[ j ] ) );
                    }

                    std::vector< Ogre::Vector3 > occupation;
                    occupation.push_back( end );
                    occupation.push_back( pos_s );
                    LOG_ERROR( "    occupation " + Ogre::StringConverter::toString( end ) );
                    LOG_ERROR( "    occupation " + Ogre::StringConverter::toString( pos_s ) );
                    m_EntitiesMovable[ i ]->SetOccupation( occupation );
                }
                else
                {
                    LOG_ERROR( "    occupation " + Ogre::StringConverter::toString( end ) );
                    std::vector< Ogre::Vector3 > occupation;
                    occupation.push_back( end );
                    m_EntitiesMovable[ i ]->SetOccupation( occupation );
                }
            }
            else
            {
                m_EntitiesMovable[ i ]->SetPosition( start + end_start * speed * delta );
            }
        }
    }

    m_Hud->Update();

    UpdateDebug();
}



void
EntityManager::UpdateDebug()
{
    for( size_t i = 0; i < m_Entities.size(); ++i )
    {
        std::vector< Ogre::Vector3 > occupation = m_Entities[ i ]->GetOccupation();
        for( size_t j = 0; j < occupation.size(); ++j )
        {
            Ogre::Vector3 pos_s = CameraManager::getSingleton().ProjectPointToScreen( Ogre::Vector3( occupation[ j ].x - 0.5f, occupation[ j ].y - 0.5f, 0 ) );
            Ogre::Vector3 pos_e = CameraManager::getSingleton().ProjectPointToScreen( Ogre::Vector3( occupation[ j ].x + 0.5f, occupation[ j ].y + 0.5f, 0 ) );
            DEBUG_DRAW.SetColour( Ogre::ColourValue( 1, 1, 1, 0.5f ) );
            DEBUG_DRAW.Quad( pos_s.x, pos_s.y, pos_e.x, pos_s.y, pos_e.x, pos_e.y, pos_s.x, pos_e.y );
        }

    }

    for( size_t i = 0; i < m_EntitiesMovable.size(); ++i )
    {
        Ogre::Vector3 pos = m_EntitiesMovable[ i ]->GetPosition();
        std::vector< Ogre::Vector3 > path = m_EntitiesMovable[ i ]->GetMovePath();
        for( size_t i = 0; i < path.size(); ++i )
        {
            Ogre::Vector3 pos_s;
            Ogre::Vector3 pos_e;
            if( i == 0 )
            {
                pos_s = CameraManager::getSingleton().ProjectPointToScreen( Ogre::Vector3( pos.x, pos.y, 0 ) );
                pos_e = CameraManager::getSingleton().ProjectPointToScreen( Ogre::Vector3( path.back().x, path.back().y, 0 ) );
            }
            else
            {
                pos_s = CameraManager::getSingleton().ProjectPointToScreen( Ogre::Vector3( path[ i - 1 ].x, path[ i - 1 ].y, 0 ) );
                pos_e = CameraManager::getSingleton().ProjectPointToScreen( Ogre::Vector3( path[ i ].x, path[ i ].y, 0 ) );
            }
            DEBUG_DRAW.SetColour( Ogre::ColourValue( 1, 1, 1, 1 ) );
            DEBUG_DRAW.Line( pos_s.x, pos_s.y, pos_e.x, pos_e.y );
        }
    }

    for( size_t i = 0; i < m_EntitiesSelected.size(); ++i )
    {
        Ogre::Vector4 col = m_EntitiesSelected[ i ]->GetDrawBox();
        Ogre::Vector3 pos = m_EntitiesSelected[ i ]->GetPosition();
        Ogre::Vector3 pos_s = CameraManager::getSingleton().ProjectPointToScreen( pos + Ogre::Vector3( col.x, col.y, 0 ) );
        Ogre::Vector3 pos_e = CameraManager::getSingleton().ProjectPointToScreen( pos + Ogre::Vector3( col.z, col.w, 0 ) );
        DEBUG_DRAW.SetColour( Ogre::ColourValue( 0.5f, 1, 0, 0.3f ) );
        DEBUG_DRAW.Quad( pos_s.x, pos_s.y, pos_e.x, pos_s.y, pos_e.x, pos_e.y, pos_s.x, pos_e.y );
    }

    for( size_t i = 0; i < 100; ++i )
    {
        for( size_t j = 0; j < 100; ++j )
        {
            //Ogre::Vector3 pos_s = CameraManager::getSingleton().ProjectPointToScreen( Ogre::Vector3( i - 0.5f, j - 0.5f, 0 ) );
            //Ogre::Vector3 pos_e = CameraManager::getSingleton().ProjectPointToScreen( Ogre::Vector3( i + 0.5f, j + 0.5f, 0 ) );
            //int pass = m_MapSector.GetPass( i, j );
            //DEBUG_DRAW.SetColour( Ogre::ColourValue( 1, 1, 1, 1 ) );
            //DEBUG_DRAW.Text( Ogre::Vector3( i, j, 0 ), 0, 0, Ogre::StringConverter::toString( pass ) );
            //DEBUG_DRAW.SetColour( Ogre::ColourValue( 1, 1, 0, 0.3f ) );
            //DEBUG_DRAW.Quad( pos_s.x, pos_s.y, pos_e.x, pos_s.y, pos_e.x, pos_e.y, pos_s.x, pos_e.y );
        }
    }

    //Ogre::SceneNode::ChildNodeIterator node = m_SceneNode->getChildIterator();
    //int row = 0;
    //DEBUG_DRAW.SetColour( Ogre::ColourValue( 1, 1, 1, 1 ) );
    //while( node.hasMoreElements() )
    //{
        //Ogre::SceneNode* n = ( Ogre::SceneNode* )node.getNext();
        //DEBUG_DRAW.Text( 10, 10 + row * 20, n->getName() );
        //++row;
    //}

    m_Hud->UpdateDebug();
}



void
EntityManager::AddEntityByName( const Ogre::String& name, const float x, const float y )
{
    static int UNIQUE_NODE_ID = 0;

    for( size_t i = 0; i < m_EntityDescs.size(); ++i )
    {
        if( m_EntityDescs[ i ].name == name )
        {
            Entity* entity;
            Ogre::SceneNode* node = m_SceneNode->createChildSceneNode( "Entity" + m_EntityDescs[ i ].entity_class + Ogre::StringConverter::toString( UNIQUE_NODE_ID ) );
            ++UNIQUE_NODE_ID;


            if( m_EntityDescs[ i ].entity_class == "Movable" )
            {
                entity = new EntityMovable( node );
                m_EntitiesMovable.push_back( ( EntityMovable* )entity );
                std::vector< Ogre::Vector3 > occupation;
                occupation.push_back( Ogre::Vector3( x, y, 0 ) );
                entity->SetOccupation( occupation );
            }
            else if( m_EntityDescs[ i ].entity_class == "Stand" )
            {
                entity = new EntityStand( node );
                std::vector< Ogre::Vector3 > occupation;
                for( size_t j = 0; j < m_EntityDescs[ i ].occupation.size(); ++j )
                {
                    occupation.push_back( Ogre::Vector3( x, y, 0 ) + m_EntityDescs[ i ].occupation[ j ] );
                }
                entity->SetOccupation( occupation );
            }
            else
            {
                LOG_ERROR( "EntityManager::AddEntityByName: entity class \"" + m_EntityDescs[ i ].entity_class + "\" not found." );
                return;
            }

            entity->SetPosition( Ogre::Vector3( x, y, 0 ) );
            entity->SetDrawBox( m_EntityDescs[ i ].draw_box );
            entity->SetTexture( m_EntityDescs[ i ].texture );
            entity->UpdateGeometry();
            m_Entities.push_back( entity );

            node->attachObject( entity );

            return;
        }
    }
    LOG_ERROR( "EntityManager::AddEntityByName: entity \"" + name + "\" not found in desc." );
}



void
EntityManager::AddEntityDesc( const EntityDesc& desc )
{
    for( size_t i = 0; i < m_EntityDescs.size(); ++i )
    {
        if( m_EntityDescs[ i ].name == desc.name )
        {
            m_EntityDescs[ i ] = desc;
            return;
        }
    }
    m_EntityDescs.push_back( desc );
}



void
EntityManager::SetEntitySelection( const Ogre::Vector3& start, const Ogre::Vector3& end )
{
    //LOG_ERROR( "SetEntitySelection: " + Ogre::StringConverter::toString( start ) + " " + Ogre::StringConverter::toString( end ) );

    m_EntitiesSelected.clear();

    for( size_t i = 0; i < m_EntitiesMovable.size(); ++i )
    {
        Ogre::Vector3 pos = m_EntitiesMovable[ i ]->GetPosition();
        Ogre::Vector4 col = m_EntitiesMovable[ i ]->GetDrawBox();

        float lhs_left = ( start.x < end.x ) ? start.x : end.x;
        float lhs_right = ( start.x < end.x ) ? end.x : start.x;
        float lhs_top = ( start.y < end.y ) ? start.y : end.y;
        float lhs_bottom = ( start.y < end.y ) ? end.y : start.y;

        float rhs_left = pos.x + col.x;
        float rhs_right = pos.x + col.z;
        float rhs_top = pos.y + col.y;
        float rhs_bottom = pos.y + col.w;

        if( rhs_left < lhs_right && rhs_right > lhs_left && rhs_top < lhs_bottom && rhs_bottom > lhs_top )
        {
            m_EntitiesSelected.push_back( m_EntitiesMovable[ i ] );
        }
    }
}



void
EntityManager::SetEntitySelectionMove( const Ogre::Vector3& move )
{
    LOG_ERROR( "Start move: target=" + Ogre::StringConverter::toString( move ) );
    for( size_t i = 0; i < m_EntitiesSelected.size(); ++i )
    {
        place_finder_ignore.clear();
        m_EntitiesSelected[ i ]->SetMovePath( AStarFinder( m_EntitiesSelected[ i ], move.x, move.y ) );

        LOG_ERROR( "    path for entity " + Ogre::StringConverter::toString( i ) + ":" );
        std::vector< Ogre::Vector3 > path = m_EntitiesSelected[ i ]->GetMovePath();
        for( size_t j = 0; j < path.size(); ++j )
        {
            LOG_ERROR( "        " + Ogre::StringConverter::toString( path[ j ] ) );
        }

        std::vector< Ogre::Vector3 > occupation_old = m_EntitiesSelected[ i ]->GetOccupation();
        std::vector< Ogre::Vector3 > occupation;
        if( occupation_old.size() > 0 )
        {
            occupation.push_back( occupation_old[ 0 ] );
            LOG_ERROR( "    occupation " + Ogre::StringConverter::toString( occupation_old[ 0 ] ) );
            Ogre::Vector3 pos = m_EntitiesSelected[ i ]->GetMoveNextPosition();
            if( pos != occupation_old[ 0 ] )
            {
                LOG_ERROR( "    occupation " + Ogre::StringConverter::toString( pos ) );
                occupation.push_back( pos );
            }
            m_EntitiesSelected[ i ]->SetOccupation( occupation );
        }
    }
}



std::vector< Ogre::Vector3 >
EntityManager::AStarFinder( EntityMovable* entity, const int x, const int y ) const
{
    std::vector< Ogre::Vector3 > move_path;

    if( entity == NULL )
    {
        return move_path;
    }

    Ogre::Vector3 pos_e = PlaceFinder( entity, x, y );
    if( pos_e.z == -1 )
    {
        return move_path;
    }

    Ogre::Vector3 pos_s = entity->GetMoveNextPosition();
    if( ( pos_s.x == pos_e.x ) && ( pos_s.y == pos_e.y ) )
    {
        return move_path;
    }

    std::vector< AStarNode* > grid;
    for( size_t i = 0; i < 100; ++i )
    {
        for( size_t j = 0; j < 100; ++j )
        {
            AStarNode* node = new AStarNode();
            node->x = i;
            node->y = j;
            node->g = 0.0f;
            node->h = 0.0f;
            node->f = 0.0f;
            node->opened = false;
            node->closed = false;
            node->parent = NULL;
            grid.push_back( node );
        }
    }

    AStarNode* start_node = grid[ pos_s.x * 100 + pos_s.y ];
    start_node->opened = true;

    std::vector< AStarNode* > open_list;
    open_list.push_back( start_node );

    while( open_list.size() != 0 )
    {
        AStarNode* node = open_list.back();
        open_list.pop_back();
        node->closed = true;

        //LOG_ERROR( "cycle for node: " + Ogre::StringConverter::toString( node->x ) + " " + Ogre::StringConverter::toString( node->y ) );

        // if reached the end position, construct the path and return it
        if( node->x == pos_e.x && node->y == pos_e.y )
        {
            while( node->parent != NULL )
            {
                Ogre::Vector3 point;
                point.x = node->x;
                point.y = node->y;
                point.z = 0;
                move_path.push_back( point );
                node = node->parent;
            }
            break;
        }

        std::vector< AStarNode* > neighbors;
        if( IsPassable( entity, node->x - 1, node->y - 1 ) && IsPassable( entity, node->x, node->y - 1 ) && IsPassable( entity, node->x - 1, node->y ) )
        {
            neighbors.push_back( grid[ ( node->x - 1 ) * 100 + ( node->y -  1) ] );
        }
        if( IsPassable( entity, node->x, node->y - 1 ) )
        {
            neighbors.push_back( grid[ node->x * 100 + ( node->y - 1 ) ] );
        }
        if( IsPassable( entity, node->x + 1, node->y - 1 ) && IsPassable( entity, node->x, node->y - 1 ) && IsPassable( entity, node->x + 1, node->y ) )
        {
            neighbors.push_back( grid[ ( node->x + 1 ) * 100 + ( node->y - 1 ) ] );
        }
        if( IsPassable( entity, node->x - 1, node->y ) )
        {
            neighbors.push_back( grid[ ( node->x - 1 ) * 100 + node->y ] );
        }
        if( IsPassable( entity, node->x + 1, node->y ) )
        {
            neighbors.push_back( grid[ ( node->x + 1 ) * 100 + node->y ] );
        }
        if( IsPassable( entity, node->x - 1, node->y + 1 ) && IsPassable( entity, node->x, node->y + 1 ) && IsPassable( entity, node->x - 1, node->y ) )
        {
            neighbors.push_back( grid[ ( node->x - 1 ) * 100 + ( node->y + 1 ) ] );
        }
        if( IsPassable( entity, node->x, node->y + 1 ) )
        {
            neighbors.push_back( grid[ node->x * 100 + ( node->y + 1 ) ] );
        }
        if( IsPassable( entity, node->x + 1, node->y + 1 ) && IsPassable( entity, node->x, node->y + 1 ) && IsPassable( entity, node->x + 1, node->y ) )
        {
            neighbors.push_back( grid[ ( node->x + 1 ) * 100 + ( node->y + 1 ) ] );
        }
        for( size_t i = 0; i < neighbors.size(); ++i )
        {
            AStarNode* neighbor = neighbors[ i ];

            if( neighbor->closed == true )
            {
                continue;
            }

            // get the distance between current node and the neighbor and calculate the next g score
            float ng = node->g + sqrt( ( neighbor->x - node->x ) * ( neighbor->x - node->x ) + ( neighbor->y - node->y ) * ( neighbor->y - node->y ) );

            // check if the neighbor has not been inspected yet, or can be reached with smaller cost from the current node
            if( neighbor->opened == false || ng < neighbor->g )
            {
                neighbor->g = ng;
                neighbor->h = sqrt( ( neighbor->x - x ) * ( neighbor->x - x ) + ( neighbor->y - y ) * ( neighbor->y - y ) );
                neighbor->f = neighbor->g + neighbor->h;
                neighbor->parent = node;

                if( neighbor->opened == false )
                {
                    open_list.push_back( neighbor );
                    neighbor->opened = true;
                }

                struct
                {
                    bool operator()( AStarNode* a, AStarNode* b ) const
                    {
                        return a->f > b->f;
                    }
                } less;
                std::sort( open_list.begin(), open_list.end(), less );
            }
        }
    }

    for( size_t i = 0; i < grid.size(); ++i )
    {
        delete grid[ i ];
    }

    if( move_path.size() == 0 )
    {
        place_finder_ignore.push_back( pos_e );
        move_path = AStarFinder( entity, x, y );
    }

    return move_path;
}



const Ogre::Vector3
EntityManager::PlaceFinder( Entity* entity, const int x, const int y ) const
{
    std::vector< Ogre::Vector3 > searched;
    std::vector< Ogre::Vector3 > neighbors;

    neighbors.push_back( Ogre::Vector3( x, y, 0 ) );

    for( size_t i = 0; i < neighbors.size(); ++i )
    {
        Ogre::Vector3 neighbor = neighbors[ i ];

        size_t j = 0;
        for( ; j < searched.size(); ++j )
        {
            if( searched[ j ].x == neighbor.x && searched[ j ].y == neighbor.y )
            {
                break;
            }
        }
        if( j == searched.size() )
        {
            float dist = sqrt( ( x - neighbor.x ) * ( x - neighbor.x ) + ( y - neighbor.y ) * ( y - neighbor.y ) );
            if( dist <= 5.0f )
            {
                if( IsPassable( entity, neighbor.x, neighbor.y ) == true )
                {
                    return Ogre::Vector3( neighbor.x, neighbor.y, 0 );
                }

                neighbors.push_back( Ogre::Vector3( neighbor.x, neighbor.y - 1, 0 ) );
                neighbors.push_back( Ogre::Vector3( neighbor.x - 1, neighbor.y, 0 ) );
                neighbors.push_back( Ogre::Vector3( neighbor.x + 1, neighbor.y, 0 ) );
                neighbors.push_back( Ogre::Vector3( neighbor.x, neighbor.y + 1, 0 ) );
            }

            searched.push_back( neighbor );
        }
    }

    return Ogre::Vector3( 0, 0, -1 );
}



const bool
EntityManager::IsPassable( Entity* entity, const int x, const int y ) const
{
    // we use self position collision to not to move to same point as we already stand
    if( entity != NULL )
    {
        Ogre::Vector3 pos = entity->GetPosition();
        if( ( pos.x == x ) && ( pos.y == y ) )
        {
            return false;
        }
    }

    for( size_t i = 0; i < place_finder_ignore.size(); ++i )
    {
        if( ( place_finder_ignore[ i ].x == x ) && ( place_finder_ignore[ i ].y == y ) )
        {
            return false;
        }
    }

    if( m_MapSector.GetPass( x, y ) == 0 )
    {
        for( size_t i = 0; i < m_Entities.size(); ++i )
        {
            if( m_Entities[ i ] != entity )
            {
                std::vector< Ogre::Vector3 > occupation = m_Entities[ i ]->GetOccupation();
                for( size_t j = 0; j < occupation.size(); ++j )
                {
                    if( ( occupation[ j ].x == x ) && ( occupation[ j ].y == y ) )
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    return false;
}
