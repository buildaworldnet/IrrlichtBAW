// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_STL_WRITER_

#include "CSTLMeshWriter.h"
#include "os.h"
#include "IMesh.h"
#include "IMeshBuffer.h"
#include "ISceneManager.h"
#include "IMeshCache.h"
#include "IWriteFile.h"
#include "IFileSystem.h"
#include <sstream>

namespace irr
{
namespace scene
{

CSTLMeshWriter::CSTLMeshWriter(scene::ISceneManager* smgr)
	: SceneManager(smgr)
{
	#ifdef _DEBUG
	setDebugName("CSTLMeshWriter");
	#endif

	if (SceneManager)
		SceneManager->grab();
}


CSTLMeshWriter::~CSTLMeshWriter()
{
	if (SceneManager)
		SceneManager->drop();
}


//! Returns the type of the mesh writer
EMESH_WRITER_TYPE CSTLMeshWriter::getType() const
{
	return EMWT_STL;
}


//! writes a mesh
bool CSTLMeshWriter::writeMesh(io::IWriteFile* file, scene::ICPUMesh* mesh, int32_t flags)
{
	if (!file)
		return false;

	os::Printer::log("Writing mesh", file->getFileName().c_str());

	if (flags & scene::EMWF_WRITE_COMPRESSED)
		return writeMeshBinary(file, mesh, flags);
	else
		return writeMeshASCII(file, mesh, flags);
}


template <class I>
inline void writePositions(ICPUMeshBuffer* buffer, const bool& noIndices, io::IWriteFile* file)
{
    const uint32_t indexCount = buffer->getIndexCount();
    const uint16_t attributes = 0;
    for (uint32_t j=0; j<indexCount; j+=3)
    {
        core::vectorSIMDf v1,v2,v3;
        if (noIndices)
        {
            v1 = buffer->getPosition(j);
            v1 = buffer->getPosition(j+1);
            v1 = buffer->getPosition(j+2);
        }
        else
        {
            v1 = buffer->getPosition(((I*)buffer->getIndices())[j]);
            v2 = buffer->getPosition(((I*)buffer->getIndices())[j+1]);
            v3 = buffer->getPosition(((I*)buffer->getIndices())[j+2]);
        }
        const core::plane3df tmpplane(v1.getAsVector3df(),v2.getAsVector3df(),v3.getAsVector3df());
        file->write(&tmpplane.Normal, 12);
        file->write(&v1, 12);
        file->write(&v2, 12);
        file->write(&v3, 12);
        file->write(&attributes, 2);
    }
}

bool CSTLMeshWriter::writeMeshBinary(io::IWriteFile* file, scene::ICPUMesh* mesh, int32_t flags)
{
	// write STL MESH header

	file->write("binary ",7);
	const core::stringc name(io::IFileSystem::getFileBasename(file->getFileName(),false));
	const int32_t sizeleft = 73-name.size(); // 80 byte header
	if (sizeleft<0)
		file->write(name.c_str(),73);
	else
	{
		const char buf[80] = {0};
		file->write(name.c_str(),name.size());
		file->write(buf,sizeleft);
	}
	uint32_t facenum = 0;
	for (uint32_t j=0; j<mesh->getMeshBufferCount(); ++j)
		facenum += mesh->getMeshBuffer(j)->getIndexCount()/3;
	file->write(&facenum,4);

	// write mesh buffers

	for (uint32_t i=0; i<mesh->getMeshBufferCount(); ++i)
	{
		ICPUMeshBuffer* buffer = mesh->getMeshBuffer(i);
		if (buffer&&buffer->getMeshDataAndFormat())
		{
			video::E_INDEX_TYPE type = buffer->getIndexType();
			if (!buffer->getMeshDataAndFormat()->getIndexBuffer())
                type = video::EIT_UNKNOWN;
			if (type==video::EIT_16BIT)
            {
                //os::Printer::log("Writing mesh with 16bit indices");
                writePositions<uint16_t>(buffer,false,file);
            }
			else if (type==video::EIT_32BIT)
            {
                //os::Printer::log("Writing mesh with 32bit indices");
                writePositions<uint32_t>(buffer,false,file);
            }
			else
            {
                //os::Printer::log("Writing mesh with 32bit indices");
                writePositions<uint64_t>(buffer,true,file); //uint64_t dummy
            }
		}
	}
	return true;
}


bool CSTLMeshWriter::writeMeshASCII(io::IWriteFile* file, scene::ICPUMesh* mesh, int32_t flags)
{
	// write STL MESH header

	file->write("solid ",6);
	const core::stringc name(io::IFileSystem::getFileBasename(file->getFileName(),false));
	file->write(name.c_str(),name.size());
	file->write("\n\n",2);

	// write mesh buffers

	for (uint32_t i=0; i<mesh->getMeshBufferCount(); ++i)
	{
		ICPUMeshBuffer* buffer = mesh->getMeshBuffer(i);
		if (buffer&&buffer->getMeshDataAndFormat())
		{
			video::E_INDEX_TYPE type = buffer->getIndexType();
			if (!buffer->getMeshDataAndFormat()->getIndexBuffer())
                type = video::EIT_UNKNOWN;
			const uint32_t indexCount = buffer->getIndexCount();
			if (type==video::EIT_16BIT)
			{
                //os::Printer::log("Writing mesh with 16bit indices");
                for (uint32_t j=0; j<indexCount; j+=3)
                {
                    writeFace(file,
                        buffer->getPosition(((uint16_t*)buffer->getIndices())[j]).getAsVector3df(),
                        buffer->getPosition(((uint16_t*)buffer->getIndices())[j+1]).getAsVector3df(),
                        buffer->getPosition(((uint16_t*)buffer->getIndices())[j+2]).getAsVector3df());
                }
			}
			else if (type==video::EIT_32BIT)
			{
                //os::Printer::log("Writing mesh with 32bit indices");
                for (uint32_t j=0; j<indexCount; j+=3)
                {
                    writeFace(file,
                        buffer->getPosition(((uint32_t*)buffer->getIndices())[j]).getAsVector3df(),
                        buffer->getPosition(((uint32_t*)buffer->getIndices())[j+1]).getAsVector3df(),
                        buffer->getPosition(((uint32_t*)buffer->getIndices())[j+2]).getAsVector3df());
                }
			}
			else
            {
                //os::Printer::log("Writing mesh with no indices");
                for (uint32_t j=0; j<indexCount; j+=3)
                {
                    writeFace(file,
                        buffer->getPosition(j).getAsVector3df(),
                        buffer->getPosition(j+1).getAsVector3df(),
                        buffer->getPosition(j+2).getAsVector3df());
                }
            }
			file->write("\n",1);
		}
	}

	file->write("endsolid ",9);
	file->write(name.c_str(),name.size());

	return true;
}


void CSTLMeshWriter::getVectorAsStringLine(const core::vector3df& v, core::stringc& s) const
{
    std::ostringstream tmp;
    tmp << v.X << " " << v.Y << " " << v.Z << "\n";
    s = core::stringc(tmp.str().c_str());
}


void CSTLMeshWriter::writeFace(io::IWriteFile* file,
		const core::vector3df& v1,
		const core::vector3df& v2,
		const core::vector3df& v3)
{
	core::stringc tmp;
	file->write("facet normal ",13);
	getVectorAsStringLine(core::plane3df(v1,v2,v3).Normal, tmp);
	file->write(tmp.c_str(),tmp.size());
	file->write("  outer loop\n",13);
	file->write("    vertex ",11);
	getVectorAsStringLine(v1, tmp);
	file->write(tmp.c_str(),tmp.size());
	file->write("    vertex ",11);
	getVectorAsStringLine(v2, tmp);
	file->write(tmp.c_str(),tmp.size());
	file->write("    vertex ",11);
	getVectorAsStringLine(v3, tmp);
	file->write(tmp.c_str(),tmp.size());
	file->write("  endloop\n",10);
	file->write("endfacet\n",9);
}

} // end namespace
} // end namespace

#endif

