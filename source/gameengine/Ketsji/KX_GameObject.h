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

/** \file KX_GameObject.h
 *  \ingroup ketsji
 *  \brief General KX game object.
 */

#ifndef __KX_GAMEOBJECT_H__
#define __KX_GAMEOBJECT_H__

#ifdef _MSC_VER
   /* get rid of this stupid "warning 'this' used in initialiser list", generated by VC when including Solid/Sumo */
#  pragma warning (disable:4355)
#endif 

#include <stddef.h>

#include "EXP_ListValue.h"
#include "SG_Node.h"
#include "SG_CullingNode.h"
#include "mathfu.h"
#include "KX_Scene.h"
#include "KX_KetsjiEngine.h" /* for m_anim_framerate */
#include "KX_ClientObjectInfo.h"

class KX_RayCast;
class KX_LodManager;
class KX_PythonComponent;
class KX_Mesh;
class RAS_MeshUser;
class PHY_IGraphicController;
class PHY_IPhysicsEnvironment;
class PHY_IPhysicsController;
class BL_ActionManager;
class BL_ConvertObjectInfo;
class KX_ObstacleSimulation;
class KX_CollisionContactPointList;
struct Object;
struct bRigidBodyJointConstraint;

#ifdef WITH_PYTHON
/* utility conversion function */
bool ConvertPythonToGameObject(void *logicmgr, PyObject *value, KX_GameObject **object, bool py_none_ok, const char *error_prefix);
#endif

#ifdef USE_MATHUTILS
void KX_GameObject_Mathutils_Callback_Init(void);
#endif

/**
 * KX_GameObject is the main class for dynamic objects.
 */
class KX_GameObject : public EXP_Value, public mt::SimdClassAllocator
{
	Py_Header
public:
	struct ActivityCullingInfo
	{
		ActivityCullingInfo();

		enum Flag {
			ACTIVITY_NONE = 0,
			ACTIVITY_PHYSICS = (1 << 0),
			ACTIVITY_LOGIC = (1 << 1)
		} m_flags;

		/// Squared physics culling radius.
		float m_physicsRadius;
		/// Squared logic culling radius.
		float m_logicRadius;
	};

	enum ObjectTypes
	{
		OBJECT_TYPE_OBJECT,
		OBJECT_TYPE_ARMATURE,
		OBJECT_TYPE_CAMERA,
		OBJECT_TYPE_LIGHT,
		OBJECT_TYPE_TEXT,
		OBJECT_TYPE_NAVMESH
	};

protected:

	KX_ClientObjectInfo m_clientInfo;
	bool m_suspended;
	std::string							m_name;
	int									m_layer;
	std::vector<KX_Mesh *>		m_meshes;
	KX_LodManager						*m_lodManager;
	short								m_currentLodLevel;
	RAS_MeshUser						*m_meshUser;
	/// Info about blender object convert from.
	BL_ConvertObjectInfo *m_convertInfo;

	bool								m_bIsNegativeScaling;
	mt::vec4							m_objectColor;

	// visible = user setting
	// culled = while rendering, depending on camera
	bool       							m_bVisible; 
	bool								m_bOccluder;

	/// Object activity culling settings converted from blender objects.
	ActivityCullingInfo m_activityCullingInfo;

	bool								m_autoUpdateBounds;

	std::unique_ptr<PHY_IPhysicsController> m_physicsController;
	std::unique_ptr<PHY_IGraphicController> m_graphicController;

	SG_CullingNode m_cullingNode;
	std::unique_ptr<SG_Node> m_sgNode;

	EXP_ListValue<KX_PythonComponent> *m_components;

	EXP_ListValue<KX_GameObject> *m_instanceObjects;
	KX_GameObject*						m_dupliGroupObject;

	// The action manager is used to play/stop/update actions
	std::unique_ptr<BL_ActionManager> m_actionManager;

	BL_ActionManager* GetActionManager();

public:
	/**
	 * KX_GameObject custom infos for ray cast, it contains property name,
	 * collision mask, xray flag and hited object.
	 * This structure is created during ray cast and passed as argument 
	 * "data" to functions KX_GameObject::NeedRayCast and KX_GameObject::RayHit.
	 */
	struct RayCastData;

	/**
	 * Helper function for modules that can't include KX_ClientObjectInfo.h
	 */
	static KX_GameObject* GetClientObject(KX_ClientObjectInfo* info);

#ifdef WITH_PYTHON
	// Python attributes that wont convert into EXP_Value
	//
	// there are 2 places attributes can be stored, in the EXP_Value,
	// where attributes are converted into BGE's EXP_Value types
	// these can be used with property actuators
	//
	// For the python API, For types that cannot be converted into EXP_Values (lists, dicts, GameObjects)
	// these will be put into "m_attr_dict", logic bricks cannot access them.
	//
	// rules for setting attributes.
	//
	// * there should NEVER be a EXP_Value and a m_attr_dict attribute with matching names. get/sets make sure of this.
	// * if EXP_Value conversion fails, use a PyObject in "m_attr_dict"
	// * when assigning a value, first see if it can be a EXP_Value, if it can remove the "m_attr_dict" and set the EXP_Value
	//
	PyObject*							m_attr_dict;
	PyObject*							m_collisionCallbacks;
#endif

	/**
	 * Used for constraint replication for group instances.
	 * The list of constraints is filled during data conversion.
	 */
	const std::vector<bRigidBodyJointConstraint *>& GetConstraints();

	void ReplicateConstraints(PHY_IPhysicsEnvironment *physEnv, const std::vector<KX_GameObject *>& constobj);


	/** 
	 * Get a pointer to the game object that is the parent of 
	 * this object. Or nullptr if there is no parent. The returned
	 * object is part of a reference counting scheme. Calling
	 * this function ups the reference count on the returned 
	 * object. It is the responsibility of the caller to decrement
	 * the reference count when you have finished with it.
	 */
		KX_GameObject*
	GetParent(
	);

	/** 
	 * Sets the parent of this object to a game object
	 */
	void SetParent(KX_GameObject *obj, bool addToCompound, bool ghost);

	/** 
	 * Removes the parent of this object to a game object
	 */
	void RemoveParent();

	/*********************************
	 * group reference API
	 *********************************/

		KX_GameObject*
	GetDupliGroupObject(
	);

		EXP_ListValue<KX_GameObject>*
	GetInstanceObjects(
	);

		void
	SetDupliGroupObject(KX_GameObject*
	);

		void
	AddInstanceObjects(KX_GameObject*
	);
		
		void 
	RemoveDupliGroupObject(
	);

		void
	RemoveInstanceObject(KX_GameObject*
	);
	/*********************************
	 * Animation API
	 *********************************/

	/**
	 * Adds an action to the object's action manager
	 */
	bool PlayAction(const std::string& name,
					float start,
					float end,
					short layer=0,
					short priority=0,
					float blendin=0.f,
					short play_mode=0,
					float layer_weight=0.f,
					short ipo_flags=0,
					float playback_speed=1.f,
					short blend_mode=0);

	/**
	 * Gets the current frame of an action
	 */
	float GetActionFrame(short layer);

	/**
	 * Gets the name of the current action
	 */
	const std::string GetActionName(short layer);

	/**
	 * Sets the current frame of an action
	 */
	void SetActionFrame(short layer, float frame);

	/**
	 * Sets play mode of the action on the given layer
	 */
	void SetPlayMode(short layer, short mode);

	/**
	 * Stop playing the action on the given layer
	 */
	void StopAction(short layer);

	/**
	 * Remove playing tagged actions.
	 */
	void RemoveTaggedActions();

	/**
	 * Check if an action has finished playing
	 */
	bool IsActionDone(short layer);

	bool IsActionsSuspended();

	/**
	 * Kick the object's action manager
	 * \param curtime The current time used to compute the actions frame.
	 * \param applyObject Set to true if the actions must transform this object, else it only manages actions' frames.
	 */
	void UpdateActionManager(float curtime, bool applyObject);

	/*********************************
	 * End Animation API
	 *********************************/

	/**
	 * Construct a game object. This class also inherits the 
	 * default constructors - use those with care!
	 */
	KX_GameObject(
		void* sgReplicationInfo,
		SG_Callbacks callbacks
	);

	KX_GameObject(const KX_GameObject& other);

	virtual 
	~KX_GameObject(
	);

	/** 
	 * \section Stuff which is here due to poor design.
	 * Inherited from EXP_Value and needs an implementation. 
	 * Do not expect these functions do to anything sensible.
	 */

	/**
	 * \section Inherited from EXP_Value. These are the useful
	 * part of the EXP_Value interface that this class implements. 
	 */

	/**
	 * Inherited from EXP_Value -- returns the name of this object.
	 */
	virtual std::string GetName();

	/**
	 * Inherited from EXP_Value -- set the name of this object.
	 */
	virtual void SetName(const std::string& name);

	/** 
	 * Inherited from EXP_Value -- return a new copy of this
	 * instance allocated on the heap. Ownership of the new 
	 * object belongs with the caller.
	 */
	virtual EXP_Value *GetReplica();
	
	/** 
	 * Return the linear velocity of the game object.
	 */
		mt::vec3 
	GetLinearVelocity(
		bool local=false
	);

	/** 
	 * Return the linear velocity of a given point in world coordinate
	 * but relative to center of object ([0,0,0]=center of object)
	 */
		mt::vec3 
	GetVelocity(
		const mt::vec3& position
	);

	/**
	 * Return the mass of the object
	 */
		float
	GetMass();

	/**
	 * Return the local inertia vector of the object
	 */
		mt::vec3
	GetLocalInertia();

	/** 
	 * Return the angular velocity of the game object.
	 */
		mt::vec3 
	GetAngularVelocity(
		bool local=false
	);

	/**
	 * Return object's physics controller gravity
	 */
	mt::vec3 GetGravity() const;

	/**
	 * Set object's physics controller gravity
	 */
	void SetGravity(const mt::vec3 &gravity);
	/** 
	 * Align the object to a given normal.
	 */
		void 
	AlignAxisToVect(
		const mt::vec3& vect,
		int axis = 2,
		float fac = 1.0
	);

	/** 
	 * Quick'n'dirty obcolor ipo stuff
	 */

		void
	SetObjectColor(
		const mt::vec4& rgbavec
	);

		const mt::vec4&
	GetObjectColor();

	/**
	 * \return a pointer to the physics controller owned by this class.
	 */

	PHY_IPhysicsController* GetPhysicsController();
	void SetPhysicsController(PHY_IPhysicsController *physicscontroller);

	virtual class RAS_Deformer* GetDeformer()
	{
		return 0;
	}
	/// Return true when the game object is a BL_DeformableGameObject.
	virtual bool IsDeformable() const
	{
		return false;
	}

	virtual void LoadDeformer()
	{
	}

	/**
	 * \return a pointer to the graphic controller owner by this class 
	 */
	PHY_IGraphicController* GetGraphicController();
	void SetGraphicController(PHY_IGraphicController* graphiccontroller);

	/*
	 * @add/remove the graphic controller to the physic system
	 */
	void ActivateGraphicController(bool recurse);

	/** Set the object's collison group
	 * \param filter The group bitfield
	 */
	void SetCollisionGroup(unsigned short filter);

	/** Set the object's collison mask
	 * \param mask The mask bitfield
	 */
	void SetCollisionMask(unsigned short mask);
	unsigned short GetCollisionGroup() const;
	unsigned short GetCollisionMask() const;

	/**
	 * \section Coordinate system manipulation functions
	 */

	void	NodeSetLocalPosition(const mt::vec3& trans	);

	void	NodeSetLocalOrientation(const mt::mat3& rot	);
	void	NodeSetGlobalOrientation(const mt::mat3& rot	);

	void	NodeSetLocalScale(	const mt::vec3& scale	);
	void	NodeSetWorldScale(	const mt::vec3& scale );

	void	NodeSetRelativeScale(	const mt::vec3& scale	);

	// adapt local position so that world position is set to desired position
	void	NodeSetWorldPosition(const mt::vec3& trans);

	void NodeUpdateGS();

	const mt::mat3& NodeGetWorldOrientation(  ) const;
	const mt::vec3& NodeGetWorldScaling(  ) const;
	const mt::vec3& NodeGetWorldPosition(  ) const;
	mt::mat3x4 NodeGetWorldTransform() const;

	const mt::mat3& NodeGetLocalOrientation(  ) const;
	const mt::vec3& NodeGetLocalScaling(  ) const;
	const mt::vec3& NodeGetLocalPosition(  ) const;
	mt::mat3x4 NodeGetLocalTransform() const;

	/**
	 * \section scene graph node accessor functions.
	 */

	SG_Node*	GetSGNode(	) 
	{ 
		return m_sgNode.get();
	}

	const 	SG_Node* GetSGNode(	) const
	{ 
		return m_sgNode.get();
	}

	Object *GetBlenderObject() const;

	BL_ConvertObjectInfo *GetConvertObjectInfo() const;
	void SetConvertObjectInfo(BL_ConvertObjectInfo *info);

	bool IsDupliGroup()
	{ 
		// TODO: store in BL_ConvertObjectInfo.
		/*Object *blenderobj = GetBlenderObject();
		return (blenderobj &&
				(blenderobj->transflag & OB_DUPLIGROUP) &&
				blenderobj->dup_group != nullptr) ? true : false;*/
		return false;
	}

	/**
	 * Set the Scene graph node for this game object.
	 * warning - it is your responsibility to make sure
	 * all controllers look at this new node. You must
	 * also take care of the memory associated with the
	 * old node. This class takes ownership of the new
	 * node.
	 */
	void SetSGNode(SG_Node *node);
	
	/// Is it a dynamic/physics object ?
	bool IsDynamic() const;

	bool IsDynamicsSuspended() const;

	/**
	 * Check if this object has a vertex parent relationship
	 */
	bool IsVertexParent( )
	{
		return (m_sgNode && m_sgNode->GetSGParent() && m_sgNode->GetSGParent()->IsVertexParent());
	}

	/// \see KX_RayCast
	bool RayHit(KX_ClientObjectInfo *client, KX_RayCast *result, RayCastData *rayData);
	/// \see KX_RayCast
	bool NeedRayCast(KX_ClientObjectInfo *client, RayCastData *rayData);


	/**
	 * \section Physics accessors for this node.
	 *
	 * All these calls get passed directly to the physics controller 
	 * owned by this object.
	 * This is real interface bloat. Why not just use the physics controller
	 * directly? I think this is because the python interface is in the wrong
	 * place.
	 */

		void
	ApplyForce(
		const mt::vec3& force,	bool local
	);

		void
	ApplyTorque(
		const mt::vec3& torque,
		bool local
	);

		void
	ApplyRotation(
		const mt::vec3& drot,
		bool local
	);

		void
	ApplyMovement(
		const mt::vec3& dloc,
		bool local
	);

		void
	AddLinearVelocity(
		const mt::vec3& lin_vel,
		bool local
	);

		void
	SetLinearVelocity(
		const mt::vec3& lin_vel,
		bool local
	);

		void
	SetAngularVelocity(
		const mt::vec3& ang_vel,
		bool local
	);

	virtual float	GetLinearDamping() const;
	virtual float	GetAngularDamping() const;
	virtual void	SetLinearDamping(float damping);
	virtual void	SetAngularDamping(float damping);
	virtual void	SetDamping(float linear, float angular);

	/**
	 * Update the physics object transform based upon the current SG_Node
	 * position.
	 */
		void
	UpdateTransform(
	);

	static void UpdateTransformFunc(SG_Node* node, void* gameobj, void* scene);

	/**
	 * only used for sensor objects
	 */
	void SynchronizeTransform();

	static void SynchronizeTransformFunc(SG_Node* node, void* gameobj, void* scene);

	/**
	 * Function to set IPO option at start of IPO
	 */ 
		void
	InitIPO(
		bool ipo_as_force,
		bool ipo_add,
		bool ipo_local
	);

	/**
	 * Odd function to update an ipo. ???
	 */ 
		void
	UpdateIPO(
		float curframetime,
		bool recurse
	);

	/**
	 * \section Mesh accessor functions.
	 */

	/**
	 * Update buckets to indicate that there is a new
	 * user of this object's meshes.
	 */
	virtual void
	AddMeshUser(
	);
	
	/**
	 * Update buckets with data about the mesh after
	 * creating or duplicating the object, changing
	 * visibility, object color, .. .
	 */
	virtual void UpdateBuckets();

	/**
	 * Clear the meshes associated with this class
	 * and remove from the bucketing system.
	 * Don't think this actually deletes any of the meshes.
	 */
		void
	RemoveMeshes(
	);

	/**
	 * Add a mesh to the set of meshes associated with this
	 * node. Meshes added in this way are not deleted by this class.
	 * Make sure you call RemoveMeshes() before deleting the
	 * mesh though,
	 */
		void
	AddMesh(
		KX_Mesh* mesh
	) {
		m_meshes.push_back(mesh);
	}

	void ReplaceMesh(KX_Mesh *mesh, bool use_gfx, bool use_phys);

	/** Set current lod manager, can be nullptr.
	 * If nullptr the object's mesh backs to the mesh of the previous first lod level.
	 */
	void SetLodManager(KX_LodManager *lodManager);
	/// Get current lod manager.
	KX_LodManager *GetLodManager() const;

	/**
	 * Updates the current lod level based on distance from camera.
	 */
	void UpdateLod(KX_Scene *scene, const mt::vec3& cam_pos, float lodfactor);

	/** Update the activity culling of the object.
	 * \param distance Squared nearest distance to the cameras of this object.
	 */
	void UpdateActivity(float distance);

	const std::vector<KX_Mesh *>& GetMeshList() const;

	/// Return the mesh user of this game object.
	RAS_MeshUser *GetMeshUser() const;

	/// Return true when the object can be culled.
	bool UseCulling() const;

	/**
	 * Was this object marked visible? (only for the explicit
	 * visibility system).
	 */
		bool
	GetVisible(
		void
	);

	/**
	 * Set visibility flag of this object
	 */
		void
	SetVisible(
		bool b,
		bool recursive
	);

	/**
	 * Was this object culled?
	 */
	inline bool
	GetCulled(
		void
	) { return m_cullingNode.GetCulled(); }

	/**
	 * Set culled flag of this object
	 */
	inline void
	SetCulled(
		bool c
	) { m_cullingNode.SetCulled(c); }
	
	/**
	 * Is this object an occluder?
	 */
	inline bool
	GetOccluder(
		void
	) { return m_bOccluder; }

	/**
	 * Set occluder flag of this object
	 */
	void
	SetOccluder(
		bool v,
		bool recursive
	);
	
	/**
	 * Change the layer of the object (when it is added in another layer
	 * than the original layer)
	 */
	virtual void
	SetLayer(
		int l
	);

	/**
	 * Get the object layer
	 */
		int
	GetLayer(
		void
	);

	/// Allow auto updating bounding volume box.
	inline void SetAutoUpdateBounds(bool autoUpdate)
	{
		m_autoUpdateBounds = autoUpdate;
	}

	inline bool GetAutoUpdateBounds() const
	{
		return m_autoUpdateBounds;
	}

	/** Update the game object bounding box (AABB) by using the one existing in the
	 * mesh or the mesh deformer.
	 * \param force Force the AABB update even if the object doesn't allow auto update or if the mesh is
	 * not modified like in the case of mesh replacement or object duplication.
	 * \warning Should be called when the game object contains a valid scene graph node
	 * and a valid graphic controller (if it exists).
	 */
	void UpdateBounds(bool force);
	void SetBoundsAabb(const mt::vec3 &aabbMin, const mt::vec3 &aabbMax);
	void GetBoundsAabb(mt::vec3 &aabbMin, mt::vec3 &aabbMax) const;

	SG_CullingNode *GetCullingNode();

	ActivityCullingInfo& GetActivityCullingInfo();
	void SetActivityCullingInfo(const ActivityCullingInfo& cullingInfo);
	/// Enable or disable a category of object activity culling.
	void SetActivityCulling(ActivityCullingInfo::Flag flag, bool enable);

	void SuspendPhysics(bool freeConstraints);
	void RestorePhysics();

	/**
	 * Get the negative scaling state
	 */
		bool
	IsNegativeScaling(
		void
	) { return m_bIsNegativeScaling; }

	/**
	 * \section Logic bubbling methods.
	 */

	void RegisterCollisionCallbacks();
	void UnregisterCollisionCallbacks();
	void RunCollisionCallbacks(KX_GameObject *collider, KX_CollisionContactPointList& contactPointList);

	/// Suspend all progress.
	void SuspendLogic();

	/// Resume progress.
	void ResumeLogic();

	virtual ObjectTypes GetGameObjectType() const;

	/**
	 * add debug object to the debuglist.
	 */
	void SetUseDebugProperties(bool debug, bool recursive);

	KX_ClientObjectInfo& GetClientInfo() { return m_clientInfo; }
	
	std::vector<KX_GameObject *> GetChildren() const;
	std::vector<KX_GameObject *> GetChildrenRecursive() const;

	/// Returns the component list.
	EXP_ListValue<KX_PythonComponent> *GetComponents() const;
	/// Add a components.
	void SetComponents(EXP_ListValue<KX_PythonComponent> *components);

	/// Updates the components.
	void UpdateComponents();

	KX_Scene*	GetScene();

#ifdef WITH_PYTHON
	/**
	 * \section Python interface functions.
	 */

	EXP_PYMETHOD_O(KX_GameObject,SetWorldPosition);
	EXP_PYMETHOD_VARARGS(KX_GameObject, ApplyForce);
	EXP_PYMETHOD_VARARGS(KX_GameObject, ApplyTorque);
	EXP_PYMETHOD_VARARGS(KX_GameObject, ApplyRotation);
	EXP_PYMETHOD_VARARGS(KX_GameObject, ApplyMovement);
	EXP_PYMETHOD_VARARGS(KX_GameObject,GetLinearVelocity);
	EXP_PYMETHOD_VARARGS(KX_GameObject,SetLinearVelocity);
	EXP_PYMETHOD_VARARGS(KX_GameObject,GetAngularVelocity);
	EXP_PYMETHOD_VARARGS(KX_GameObject,SetAngularVelocity);
	EXP_PYMETHOD_VARARGS(KX_GameObject,GetVelocity);
	EXP_PYMETHOD_VARARGS(KX_GameObject,SetDamping);

	EXP_PYMETHOD_NOARGS(KX_GameObject,GetReactionForce);


	EXP_PYMETHOD_NOARGS(KX_GameObject,GetVisible);
	EXP_PYMETHOD_VARARGS(KX_GameObject,SetVisible);
	EXP_PYMETHOD_VARARGS(KX_GameObject,SetOcclusion);
	EXP_PYMETHOD(KX_GameObject,AlignAxisToVect);
	EXP_PYMETHOD_O(KX_GameObject,GetAxisVect);
	EXP_PYMETHOD_VARARGS(KX_GameObject,SuspendPhysics);
	EXP_PYMETHOD_NOARGS(KX_GameObject,RestorePhysics);
	EXP_PYMETHOD_VARARGS(KX_GameObject,SuspendDynamics);
	EXP_PYMETHOD_NOARGS(KX_GameObject,RestoreDynamics);
	EXP_PYMETHOD_NOARGS(KX_GameObject,EnableRigidBody);
	EXP_PYMETHOD_NOARGS(KX_GameObject,DisableRigidBody);
	EXP_PYMETHOD_VARARGS(KX_GameObject,ApplyImpulse);
	EXP_PYMETHOD_O(KX_GameObject,SetCollisionMargin);
	EXP_PYMETHOD_O(KX_GameObject,Collide);
	EXP_PYMETHOD_NOARGS(KX_GameObject,GetParent);
	EXP_PYMETHOD(KX_GameObject,SetParent);
	EXP_PYMETHOD_NOARGS(KX_GameObject,RemoveParent);
	EXP_PYMETHOD_NOARGS(KX_GameObject,GetChildren);
	EXP_PYMETHOD_NOARGS(KX_GameObject,GetChildrenRecursive);
	EXP_PYMETHOD_VARARGS(KX_GameObject,GetMesh);
	EXP_PYMETHOD_NOARGS(KX_GameObject,GetPhysicsId);
	EXP_PYMETHOD_NOARGS(KX_GameObject,GetPropertyNames);
	EXP_PYMETHOD(KX_GameObject,ReplaceMesh);
	EXP_PYMETHOD_NOARGS(KX_GameObject,EndObject);
	EXP_PYMETHOD_DOC(KX_GameObject,rayCastTo);
	EXP_PYMETHOD_DOC(KX_GameObject,rayCast);
	EXP_PYMETHOD_DOC_O(KX_GameObject,getDistanceTo);
	EXP_PYMETHOD_DOC_O(KX_GameObject,getVectTo);
	EXP_PYMETHOD_DOC(KX_GameObject, sendMessage);
	EXP_PYMETHOD(KX_GameObject, ReinstancePhysicsMesh);
	EXP_PYMETHOD_O(KX_GameObject, ReplacePhysicsShape);
	EXP_PYMETHOD_DOC(KX_GameObject, addDebugProperty);

	EXP_PYMETHOD_DOC(KX_GameObject, playAction);
	EXP_PYMETHOD_DOC(KX_GameObject, stopAction);
	EXP_PYMETHOD_DOC(KX_GameObject, getActionFrame);
	EXP_PYMETHOD_DOC(KX_GameObject, getActionName);
	EXP_PYMETHOD_DOC(KX_GameObject, setActionFrame);
	EXP_PYMETHOD_DOC(KX_GameObject, isPlayingAction);
	
	/* Dict access */
	EXP_PYMETHOD_VARARGS(KX_GameObject,get);
	
	/* attributes */
	static PyObject*	pyattr_get_name(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_name(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_parent(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);

	static PyObject*	pyattr_get_group_object(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_group_members(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_scene(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);

	static PyObject*	pyattr_get_life(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_mass(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_mass(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_is_suspend_dynamics(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_lin_vel_min(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_lin_vel_min(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_lin_vel_max(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_lin_vel_max(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_ang_vel_min(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_ang_vel_min(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_ang_vel_max(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_ang_vel_max(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_layer(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_layer(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_visible(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_visible(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_culled(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_cullingBox(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_physicsCulling(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_physicsCulling(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_logicCulling(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_logicCulling(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_physicsCullingRadius(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_physicsCullingRadius(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_logicCullingRadius(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_logicCullingRadius(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldPosition(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldPosition(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localPosition(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localPosition(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localInertia(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localInertia(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldOrientation(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldOrientation(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localOrientation(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localOrientation(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldScaling(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldScaling(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localScaling(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localScaling(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localTransform(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localTransform(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldTransform(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldTransform(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldLinearVelocity(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldLinearVelocity(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localLinearVelocity(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localLinearVelocity(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldAngularVelocity(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldAngularVelocity(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localAngularVelocity(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localAngularVelocity(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_gravity(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_gravity(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_timeOffset(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_timeOffset(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_meshes(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_batchGroup(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_children(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_children_recursive(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_attrDict(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_obcolor(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_obcolor(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_components(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_collisionCallbacks(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_collisionCallbacks(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_collisionGroup(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_collisionGroup(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_collisionMask(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_collisionMask(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_debug(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_debug(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_debugRecursive(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_debugRecursive(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_linearDamping(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_linearDamping(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_angularDamping(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_angularDamping(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_lodManager(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_lodManager(EXP_PyObjectPlus *self_v, const EXP_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	
	/* getitem/setitem */
	static PyMappingMethods	Mapping;
	static PySequenceMethods	Sequence;
#endif
};



#endif  /* __KX_GAMEOBJECT_H__ */
