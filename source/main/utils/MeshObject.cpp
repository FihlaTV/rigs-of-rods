/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   1st of May 2010

#include "MeshObject.h"

#include "Application.h"
#include "OgreSubsystem.h"
#include "TerrainManager.h"
#include "MeshLodGenerator/OgreMeshLodGenerator.h"

using namespace Ogre;
using namespace RoR;

MeshObject::MeshObject(Ogre::String meshName, Ogre::String entityName, Ogre::SceneNode* sceneNode)
    : meshName(meshName)
    , entityName(entityName)
    , sceneNode(sceneNode)
    , ent(0)
    , loaded(false)
    , materialName()
    , castshadows(true)
    , enabled(true)
    , visible(true)
{
    // create a new sceneNode if not existing
    if (!sceneNode)
        sceneNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();

    loadMesh();
}

MeshObject::~MeshObject()
{
    if (!mesh.isNull())
        mesh->unload();
}

void MeshObject::setMaterialName(Ogre::String m)
{
    if (m.empty())
        return;
    materialName = m;
    if (loaded && ent)
    {
        ent->setMaterialName(materialName);
    }
}

void MeshObject::setCastShadows(bool b)
{
    castshadows = b;
    if (loaded && sceneNode && ent && sceneNode->numAttachedObjects())
    {
        sceneNode->getAttachedObject(0)->setCastShadows(b);
    }
}

void MeshObject::setMeshEnabled(bool e)
{
    setVisible(e);
    enabled = e;
}

void MeshObject::setVisible(bool b)
{
    if (!enabled)
        return;
    visible = b;
    if (loaded && sceneNode)
        sceneNode->setVisible(b);
}

void MeshObject::postProcess()
{
    loaded = true;
    if (!sceneNode)
        return;

    // important: you need to add the LODs before creating the entity
    // now find possible LODs, needs to be done before calling createEntity()
    if (!mesh.isNull())
    {
        String basename, ext;
        StringUtil::splitBaseFilename(meshName, basename, ext);

        String group = ResourceGroupManager::getSingleton().findGroupContainingResource(meshName);

        // the classic LODs
        FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(group, basename + "_lod*.mesh");
        for (FileInfoList::iterator iterFiles = files->begin(); iterFiles != files->end(); ++iterFiles)
        {
            String format = basename + "_lod%d.mesh";
            int i = -1;
            int r = sscanf(iterFiles->filename.c_str(), format.c_str(), &i);

            if (r <= 0 || i < 0)
                continue;

            float distance = 3;

            // we need to tune this according to our sightrange
            if (App::gfx_sight_range.GetActive() > App::GetSimTerrain()->UNLIMITED_SIGHTRANGE)
            {
                // unlimited
                if (i == 1)
                    distance = 200;
                else if (i == 2)
                    distance = 600;
                else if (i == 3)
                    distance = 2000;
                else if (i == 4)
                    distance = 5000;
            }
            else
            {
                // limited
                int sightrange = App::gfx_sight_range.GetActive();
                if (i == 1)
                    distance = std::max(20.0f, sightrange * 0.1f);
                else if (i == 2)
                    distance = std::max(20.0f, sightrange * 0.2f);
                else if (i == 3)
                    distance = std::max(20.0f, sightrange * 0.3f);
                else if (i == 4)
                    distance = std::max(20.0f, sightrange * 0.4f);
            }

            Ogre::MeshManager::getSingleton().load(iterFiles->filename, mesh->getGroup());
            //mesh->createManualLodLevel(distance, iterFiles->filename);
            //MeshLodGenerator::getSingleton().generateAutoconfiguredLodLevels(mesh);
        }

        // the custom LODs
        FileInfoListPtr files2 = ResourceGroupManager::getSingleton().findResourceFileInfo(group, basename + "_clod_*.mesh");
        for (FileInfoList::iterator iterFiles = files2->begin(); iterFiles != files2->end(); ++iterFiles)
        {
            // and custom LODs
            String format = basename + "_clod_%d.mesh";
            int i = -1;
            int r = sscanf(iterFiles->filename.c_str(), format.c_str(), &i);
            if (r <= 0 || i < 0)
                continue;

            Ogre::MeshManager::getSingleton().load(iterFiles->filename, mesh->getGroup());
            //mesh->createManualLodLevel(i, iterFiles->filename);
            //MeshLodGenerator::getSingleton().generateAutoconfiguredLodLevels(mesh);
        }
    }

    // now create an entity around the mesh and attach it to the scene graph
    try
    {
        if (entityName.empty())
            ent = gEnv->sceneManager->createEntity(meshName);
        else
            ent = gEnv->sceneManager->createEntity(entityName, meshName);
        if (ent)
            sceneNode->attachObject(ent);
    }
    catch (Ogre::Exception& e)
    {
        LOG("error loading mesh: " + meshName + ": " + e.getFullDescription());
        return;
    }

    // only set it if different from default (true)
    if (!castshadows && sceneNode && sceneNode->numAttachedObjects() > 0)
        sceneNode->getAttachedObject(0)->setCastShadows(castshadows);

    sceneNode->setVisible(visible);
}

void MeshObject::loadMesh()
{
    try
    {
        Ogre::String resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
        mesh = static_cast<Ogre::MeshPtr>(Ogre::MeshManager::getSingleton().create(meshName, resourceGroup));
        postProcess();
    }
    catch (Ogre::Exception* e)
    {
        LOG("exception while loading mesh: " + e->getFullDescription());
    }
}
