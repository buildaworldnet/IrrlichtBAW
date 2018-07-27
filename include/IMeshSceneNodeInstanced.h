// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_MESH_SCENE_NODE_INSTANCED_H_INCLUDED__
#define __I_MESH_SCENE_NODE_INSTANCED_H_INCLUDED__


#include "ISceneNode.h"
#include "SMesh.h"
#include <vector>

namespace irr
{
namespace scene
{

class ISceneManager;

//! A scene node displaying a static mesh
//! default instance data is interleaved
class IMeshSceneNodeInstanced : public ISceneNode
{
protected:
    bool wantBBoxUpdate;

    void* cpuCullingUserData;
public:
    struct MeshLoD
    {
        IGPUMesh* mesh;
        void* userDataForVAOSetup; //put array of vertex attribute mappings here or something
        float lodDistance;
    };

    typedef scene::IMeshDataFormatDesc<video::IGPUBuffer>* (*VaoSetupOverrideFunc)(ISceneManager*,video::IGPUBuffer*,const size_t&,const scene::IMeshDataFormatDesc<video::IGPUBuffer>*, void* userData);
    typedef void (*CPUCullingFunc)(uint8_t**,const size_t&,const core::aabbox3df&,const size_t&,const core::matrix4x3&,const uint8_t*,const size_t&,scene::ISceneManager*,void*);

	//! Constructor
	/** Use setMesh() to set the mesh to display.
	*/
	IMeshSceneNodeInstanced(IDummyTransformationSceneNode* parent, ISceneManager* mgr, int32_t id,
			const core::vector3df& position = core::vector3df(0,0,0),
			const core::vector3df& rotation = core::vector3df(0,0,0),
			const core::vector3df& scale = core::vector3df(1,1,1))
		: ISceneNode(parent, mgr, id, position, rotation, scale), wantBBoxUpdate(false), cpuCullingUserData(NULL)
    {
        setAutomaticCulling(EAC_OFF);
    }

    //!
    virtual const uint32_t& getGPUCullingThreshold() const =0;
    virtual void setGPUCullingThresholdMultiplier(const double& multiplier) =0;

	//! Sets a new mesh to display
    /** Extra Per-Instance input data is passed along as floating point components filling attribute slot 5 yzw components, and all components slots in attribute 6 and up
    Any remaining data after attribute 15 W component will not be passed as vertex attribute but will be retained in the input array which can be accessed by
    creating a Texture Buffer Object and grabbing the data with texelFetch inside the "lodSelectionShader"
    Lod selection is done by shader supplied, your callback can extract which Lod{s}'s instance data is being computer by casting MaterialTypeParam and MaterialTypeParam2 to uint32_t
    together they give the first LoD index and the last LoD index in the LoD range currently having its buffers filled.
    FUTURE:
    The compute shader mode can compute ALL LoDs at once and possibly all IMeshSceneNodeInstanced' nodes' culling and instance data at once.
	\param mesh Mesh to display. */
	virtual bool setLoDMeshes(std::vector<MeshLoD> levelsOfDetail, const size_t& dataSizePerInstanceOutput, const video::SMaterial& lodSelectionShader, VaoSetupOverrideFunc vaoSetupOverride, const size_t shaderLoDsPerPass=1, void* overrideUserData=NULL, const size_t& extraDataSizePerInstanceInput=0, CPUCullingFunc cpuCullFunc=NULL) = 0;

	//! Get the currently defined mesh for display.
	/** \return Pointer to mesh which is displayed by this node. */
	virtual SGPUMesh* getLoDMesh(const size_t &lod) = 0;

	//!
	virtual void setUserCPUCullingData(void* data)
	{
	    cpuCullingUserData = data;
	}

	virtual const size_t& getInstanceCount() const = 0;

	virtual const core::aabbox3df& getLoDInvariantBBox() const = 0;


    inline void setBBoxUpdateEnabled() {wantBBoxUpdate = true;}
    inline void setBBoxUpdateDisabled() {wantBBoxUpdate = false;}
    inline const bool& getBBoxUpdateMode() {return wantBBoxUpdate;}


    virtual uint32_t addInstance(const core::matrix4x3& relativeTransform, void* extraData=NULL) = 0;

    virtual bool addInstances(uint32_t* instanceIDs, const size_t& instanceCount, const core::matrix4x3* relativeTransforms, void* extraData) = 0;

    virtual void setInstanceTransform(const uint32_t& instanceID, const core::matrix4x3& relativeTransform) = 0;

    virtual core::matrix4x3 getInstanceTransform(const uint32_t& instanceID) = 0;

    virtual void setInstanceVisible(const uint32_t& instanceID, const bool& visible) = 0;

    virtual void setInstanceData(const uint32_t& instanceID, void* data) = 0;

    virtual void removeInstance(const uint32_t& instanceID) = 0;

    virtual void removeInstances(const size_t& instanceCount, const uint32_t* instanceIDs)= 0;
};

} // end namespace scene
} // end namespace irr


#endif


