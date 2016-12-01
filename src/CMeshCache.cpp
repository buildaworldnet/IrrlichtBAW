// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CMeshCache.h"
#include "IAnimatedMesh.h"
#include "IMesh.h"

namespace irr
{
namespace scene
{

static const io::SNamedPath emptyNamedPath;



//! adds a mesh to the list
template<class T>
void CMeshCache<T>::addMesh(const io::path& filename, T* mesh)
{
	mesh->grab();

	MeshEntry<T> e ( filename );
	e.Mesh = mesh;

	Meshes.push_back(e);
	Meshes.sort();
}


//! Removes a mesh from the cache.
template<class T>
void CMeshCache<T>::removeMesh(const T* const mesh)
{
	if ( !mesh )
		return;


	for (u32 i=0; i<Meshes.size(); ++i)
	{
		if (Meshes[i].Mesh == mesh)
		{
			Meshes[i].Mesh->drop();
			Meshes.erase(i);
			return;
		}
	}
}


//! Returns amount of loaded meshes
template<class T>
u32 CMeshCache<T>::getMeshCount() const
{
	return Meshes.size();
}


//! Returns current number of the mesh
template<class T>
s32 CMeshCache<T>::getMeshIndex(const T* const mesh) const
{
	for (u32 i=0; i<Meshes.size(); ++i)
	{
		if (Meshes[i].Mesh == mesh)
			return (s32)i;
	}

	return -1;
}


//! Returns a mesh based on its index number
template<class T>
T* CMeshCache<T>::getMeshByIndex(u32 number)
{
	if (number >= Meshes.size())
		return 0;

	return Meshes[number].Mesh;
}


//! Returns a mesh based on its name.
template<class T>
T* CMeshCache<T>::getMeshByName(const io::path& name)
{
	MeshEntry<T> e ( name );
	s32 id = Meshes.binary_search(e);
	return (id != -1) ? Meshes[id].Mesh : 0;
}


//! Get the name of a loaded mesh, based on its index.
template<class T>
const io::SNamedPath& CMeshCache<T>::getMeshName(u32 index) const
{
	if (index >= Meshes.size())
		return emptyNamedPath;

	return Meshes[index].NamedPath;
}


//! Get the name of a loaded mesh, if there is any.
template<class T>
const io::SNamedPath& CMeshCache<T>::getMeshName(const T* const mesh) const
{
	if (!mesh)
		return emptyNamedPath;

	for (u32 i=0; i<Meshes.size(); ++i)
	{
		if (Meshes[i].Mesh == mesh)
			return Meshes[i].NamedPath;
	}

	return emptyNamedPath;
}

//! Renames a loaded mesh.
template<class T>
bool CMeshCache<T>::renameMesh(u32 index, const io::path& name)
{
	if (index >= Meshes.size())
		return false;

	Meshes[index].NamedPath.setPath(name);
	Meshes.sort();
	return true;
}


//! Renames a loaded mesh.
template<class T>
bool CMeshCache<T>::renameMesh(const T* const mesh, const io::path& name)
{
	for (u32 i=0; i<Meshes.size(); ++i)
	{
		if (Meshes[i].Mesh == mesh)
		{
			Meshes[i].NamedPath.setPath(name);
			Meshes.sort();
			return true;
		}
	}

	return false;
}


//! returns if a mesh already was loaded
template<class T>
bool CMeshCache<T>::isMeshLoaded(const io::path& name)
{
	return getMeshByName(name) != 0;
}


//! Clears the whole mesh cache, removing all meshes.
template<class T>
void CMeshCache<T>::clear()
{
	for (u32 i=0; i<Meshes.size(); ++i)
		Meshes[i].Mesh->drop();

	Meshes.clear();
}

//! Clears all meshes that are held in the mesh cache but not used anywhere else.
template<class T>
void CMeshCache<T>::clearUnusedMeshes()
{
	for (u32 i=0; i<Meshes.size(); ++i)
	{
		if (Meshes[i].Mesh->getReferenceCount() == 1)
		{
			Meshes[i].Mesh->drop();
			Meshes.erase(i);
			--i;
		}
	}
}
// Instantiate CMeshCache for the supported template type parameters
template class CMeshCache<ICPUMesh>;
template class CMeshCache<IGPUMesh>;


} // end namespace scene
} // end namespace irr

