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

/** \file BL_Converter.h
 *  \ingroup bgeconv
 */

#ifndef __KX_BLENDERCONVERTER_H__
#define __KX_BLENDERCONVERTER_H__

#include <map>
#include <vector>

#include "CM_Thread.h"

class BL_SceneConverter;
class KX_KetsjiEngine;
class KX_LibLoadStatus;
struct Main;
struct BlendHandle;
struct Scene;
struct TaskPool;

class BL_Converter
{
private:
	struct ThreadInfo {
		TaskPool *m_pool;
		CM_ThreadMutex m_mutex;
	} m_threadinfo;

	// Saved KX_LibLoadStatus objects
	std::map<std::string, KX_LibLoadStatus *> m_status_map;
	std::vector<KX_LibLoadStatus *> m_mergequeue;

	Main *m_maggie;
	std::vector<Main *> m_DynamicMaggie;

	KX_KetsjiEngine *m_ketsjiEngine;
	bool m_alwaysUseExpandFraming;

public:
	BL_Converter(Main *maggie, KX_KetsjiEngine *engine);
	virtual ~BL_Converter();

	void ConvertScene(BL_SceneConverter& converter, bool libloading);

	void SetAlwaysUseExpandFraming(bool to_what);

	Scene *GetBlenderSceneForName(const std::string& name);
	std::vector<std::string> GetInactiveSceneNames() const;

	Main *CreateMainDynamic(const std::string& path);
	Main *GetMainDynamicPath(const std::string& path) const;
	const std::vector<Main *> &GetMainDynamic() const;

	KX_LibLoadStatus *LinkBlendFileMemory(void *data, int length, const char *path, char *group, KX_Scene *scene_merge, char **err_str, short options);
	KX_LibLoadStatus *LinkBlendFilePath(const char *path, char *group, KX_Scene *scene_merge, char **err_str, short options);
	KX_LibLoadStatus *LinkBlendFile(BlendHandle *blendlib, const char *path, char *group, KX_Scene *scene_merge, char **err_str, short options);

	bool FreeBlendFile(Main *maggie);
	bool FreeBlendFile(const std::string& path);

	KX_Mesh *ConvertMeshSpecial(KX_Scene *kx_scene, Main *maggie, const std::string& name);

	void MergeAsyncLoads();
	void FinalizeAsyncLoads();
	void AddScenesToMergeQueue(KX_LibLoadStatus *status);

	void PrintStats();

	// LibLoad Options.
	enum
	{
		LIB_LOAD_LOAD_ACTIONS = 1,
		LIB_LOAD_VERBOSE = 2,
		LIB_LOAD_LOAD_SCRIPTS = 4,
		LIB_LOAD_ASYNC = 8,
	};
};

#endif  // __KX_BLENDERCONVERTER_H__
