/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file BL_SceneConverter.h
 *  \ingroup bgeconv
 */

#ifndef __KX_BLENDERSCENECONVERTER_H__
#define __KX_BLENDERSCENECONVERTER_H__

#include "CM_Message.h"

#include <map>
#include <vector>

class KX_Mesh;
class KX_BlenderMaterial;
class BL_ResourceCollection;
class BL_ConvertObjectInfo;
class KX_GameObject;
class KX_Scene;
struct Object;
struct Mesh;
struct Material;
struct bAction;

class BL_SceneConverter
{
	friend BL_ResourceCollection;

private:
	KX_Scene *m_scene;

	std::vector<KX_BlenderMaterial *> m_materials;
	std::vector<KX_Mesh *> m_meshes;
	std::vector<BL_ConvertObjectInfo *> m_objectInfos;
	std::vector<bAction *> m_actions;

	std::map<Object *, BL_ConvertObjectInfo *> m_blenderToObjectInfos;
	std::map<Object *, KX_GameObject *> m_map_blender_to_gameobject;
	std::map<Mesh *, KX_Mesh *> m_map_mesh_to_gamemesh;
	std::map<Material *, KX_BlenderMaterial *> m_map_mesh_to_polyaterial;

public:
	BL_SceneConverter(KX_Scene *scene);
	~BL_SceneConverter() = default;

	// Disable dangerous copy.
	BL_SceneConverter(const BL_SceneConverter& other) = delete;

	BL_SceneConverter(BL_SceneConverter&& other);

	KX_Scene *GetScene() const;

	/** Generate shaders and mesh attributes depending on.
	 * This function is separated from ConvertScene to be synchronized when compiling shaders
	 * and select a scene to generate shaders with. This last point is used for scene libload
	 * merging.
	 * \param scene The scene used to generate shaders.
	 */
	void Finalize(KX_Scene *scene);

	void RegisterObject(KX_GameObject *gameobject, Object *for_blenderobject);
	void UnregisterObject(KX_GameObject *gameobject);
	KX_GameObject *FindObject(Object *for_blenderobject);

	void RegisterMesh(KX_Mesh *gamemesh, Mesh *for_blendermesh);
	KX_Mesh *FindMesh(Mesh *for_blendermesh);

	void RegisterMaterial(KX_BlenderMaterial *blmat, Material *mat);
	KX_BlenderMaterial *FindMaterial(Material *mat);

	void RegisterAction(bAction *action);

	BL_ConvertObjectInfo *GetObjectInfo(Object *blenderobj);
};

#endif  // __KX_BLENDERSCENECONVERTER_H__
