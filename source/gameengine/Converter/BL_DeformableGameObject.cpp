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

/** \file gameengine/Converter/BL_DeformableGameObject.cpp
 *  \ingroup bgeconv
 */

#include "BL_DeformableGameObject.h"

#include "KX_SoftBodyDeformer.h"
#include "KX_Mesh.h"

#include "CM_Message.h"

BL_DeformableGameObject::BL_DeformableGameObject(void *sgReplicationInfo, SG_Callbacks callbacks)
	:KX_GameObject(sgReplicationInfo, callbacks),
	m_deformer(nullptr),
	m_lastframe(0.0)
{
}

BL_DeformableGameObject::~BL_DeformableGameObject()
{
	if (m_deformer) {
		delete m_deformer;
	}
}

EXP_Value *BL_DeformableGameObject::GetReplica()
{
	BL_DeformableGameObject *replica = new BL_DeformableGameObject(*this);
	replica->ProcessReplica();
	return replica;
}

void BL_DeformableGameObject::ProcessReplica()
{
	KX_GameObject::ProcessReplica();

	if (m_deformer) {
		m_deformer = m_deformer->GetReplica();
	}
}

void BL_DeformableGameObject::Relink(std::map<KX_GameObject *, KX_GameObject *>& map)
{
	if (m_deformer) {
// 		m_deformer->Relink(map);
	}
// 	KX_GameObject::Relink(map);
}

double BL_DeformableGameObject::GetLastFrame() const
{
	return m_lastframe;
}

void BL_DeformableGameObject::SetLastFrame(double curtime)
{
	m_lastframe = curtime;
}

void BL_DeformableGameObject::SetDeformer(RAS_Deformer *deformer)
{
	// Make sure that the object doesn't already have a mesh user.
	BLI_assert(m_meshUser == nullptr);
	m_deformer = deformer;
}

RAS_Deformer *BL_DeformableGameObject::GetDeformer()
{
	return m_deformer;
}

bool BL_DeformableGameObject::IsDeformable() const
{
	return true;
}

void BL_DeformableGameObject::LoadDeformer()
{
	if (m_deformer) {
		delete m_deformer;
		m_deformer = nullptr;
	}

	if (m_meshes.empty()) {
		return;
	}

	KX_Mesh *meshobj = m_meshes.front();
	Mesh *mesh = meshobj->GetMesh();

	if (!mesh) {
		return;
	}

#ifdef WITH_BULLET
	const bool bHasSoftBody = false; //(!parentobj && (blenderobj->gameflag & OB_SOFT_BODY)); TODO BL_ConvertInfo or physics API

	if (bHasSoftBody) {
		m_deformer = new KX_SoftBodyDeformer(meshobj, this);
	}
#endif

	if (m_deformer) {
		m_deformer->InitializeDisplayArrays();
	}
}
