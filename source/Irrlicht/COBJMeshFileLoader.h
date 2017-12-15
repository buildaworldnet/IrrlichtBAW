// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_OBJ_MESH_FILE_LOADER_H_INCLUDED__
#define __C_OBJ_MESH_FILE_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "IFileSystem.h"
#include "ISceneManager.h"
#include "irrString.h"
#include <vector>

namespace irr
{
namespace scene
{

#include "irrpack.h"
class SObjVertex
{
public:
    inline bool operator<(const SObjVertex& other) const
    {
        if (pos[0]==other.pos[0])
        {
            if (pos[1]==other.pos[1])
            {
                if (pos[2]==other.pos[2])
                {
                    if (uv[0]==other.uv[0])
                    {
                        if (uv[1]==other.uv[1])
                            return normal32bit<other.normal32bit;

                        return uv[1]<other.uv[1];
                    }
                    return uv[0]<other.uv[0];
                }
                return pos[2]<other.pos[2];
            }
            return pos[1]<other.pos[1];
        }

        return pos[0]<other.pos[0];
    }
    inline bool operator==(const SObjVertex& other) const
    {
        return pos[0]==other.pos[0]&&pos[1]==other.pos[1]&&pos[2]==other.pos[2]&&uv[0]==other.uv[0]&&uv[1]==other.uv[1]&&normal32bit==other.normal32bit;
    }
    float pos[3];
    float uv[2];
    uint32_t normal32bit;
} PACK_STRUCT;

class SObjVertex16
{
public:
    SObjVertex16(const SObjVertex& other)
    {
        pos[0] = other.pos[0];
        pos[1] = other.pos[1];
        pos[2] = other.pos[2];
        for (size_t i=0; i<2; i++)
        {
            double x = other.uv[i];
            x *= 0xffffu;
            uv[i] = x;
        }
        normal32bit = other.normal32bit;
    }
    float pos[3];
    uint16_t uv[2];
    uint32_t normal32bit;
} PACK_STRUCT;

class SObjVertex8
{
public:
    SObjVertex8(const SObjVertex& other)
    {
        pos[0] = other.pos[0];
        pos[1] = other.pos[1];
        pos[2] = other.pos[2];
        for (size_t i=0; i<2; i++)
        {
            double x = other.uv[i];
            x *= 0xffu;
            uv[i] = x;
        }
        normal32bit = other.normal32bit;
    }
    float pos[3];
    uint8_t uv[2];
    uint32_t normal32bit;
} PACK_STRUCT;
#include "irrunpack.h"

//! Meshloader capable of loading obj meshes.
class COBJMeshFileLoader : public IMeshLoader
{
protected:
	//! destructor
	virtual ~COBJMeshFileLoader();

public:
	//! Constructor
	COBJMeshFileLoader(scene::ISceneManager* smgr, io::IFileSystem* fs);

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".obj")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IReferenceCounted::drop() for more information.
	virtual ICPUMesh* createMesh(io::IReadFile* file);

private:

	class SObjMtl
	{
        public:
            SObjMtl() : Bumpiness (1.0f), Illumination(0),
                RecalculateNormals(false)
            {
                Material.Shininess = 0.0f;
                Material.AmbientColor = video::SColorf(0.2f, 0.2f, 0.2f, 1.0f).toSColor();
                Material.DiffuseColor = video::SColorf(0.8f, 0.8f, 0.8f, 1.0f).toSColor();
                Material.SpecularColor = video::SColorf(1.0f, 1.0f, 1.0f, 1.0f).toSColor();
            }

            SObjMtl(const SObjMtl& o)
                : Name(o.Name), Group(o.Group),
                Bumpiness(o.Bumpiness), Illumination(o.Illumination),
                RecalculateNormals(false)
            {
                Material = o.Material;
            }

            std::map<SObjVertex, int> VertMap;
            std::vector<SObjVertex> Vertices;
            std::vector<uint32_t> Indices;
            video::SMaterial Material;
            std::string Name;
            std::string Group;
            float Bumpiness;
            char Illumination;
            bool RecalculateNormals;
	};

	// helper method for material reading
	const char* readTextures(const char* bufPtr, const char* const bufEnd, SObjMtl* currMaterial, const io::path& relPath);

	// returns a pointer to the first printable character available in the buffer
	const char* goFirstWord(const char* buf, const char* const bufEnd, bool acrossNewlines=true);
	// returns a pointer to the first printable character after the first non-printable
	const char* goNextWord(const char* buf, const char* const bufEnd, bool acrossNewlines=true);
	// returns a pointer to the next printable character after the first line break
	const char* goNextLine(const char* buf, const char* const bufEnd);
	// copies the current word from the inBuf to the outBuf
	uint32_t copyWord(char* outBuf, const char* inBuf, uint32_t outBufLength, const char* const pBufEnd);
	// copies the current line from the inBuf to the outBuf
	core::stringc copyLine(const char* inBuf, const char* const bufEnd);

	// combination of goNextWord followed by copyWord
	const char* goAndCopyNextWord(char* outBuf, const char* inBuf, uint32_t outBufLength, const char* const pBufEnd);

	//! Read the material from the given file
	void readMTL(const char* fileName, const io::path& relPath);

	//! Find and return the material with the given name
	SObjMtl* findMtl(const std::string& mtlName, const std::string& grpName);

	//! Read RGB color
	const char* readColor(const char* bufPtr, video::SColor& color, const char* const pBufEnd);
	//! Read 3d vector of floats
	const char* readVec3(const char* bufPtr, core::vector3df& vec, const char* const pBufEnd);
	//! Read 2d vector of floats
	const char* readUV(const char* bufPtr, core::vector2df& vec, const char* const pBufEnd);
	//! Read boolean value represented as 'on' or 'off'
	const char* readBool(const char* bufPtr, bool& tf, const char* const bufEnd);

	// reads and convert to integer the vertex indices in a line of obj file's face statement
	// -1 for the index if it doesn't exist
	// indices are changed to 0-based index instead of 1-based from the obj file
	bool retrieveVertexIndices(char* vertexData, int32_t* idx, const char* bufEnd, uint32_t vbsize, uint32_t vtsize, uint32_t vnsize);


	scene::ISceneManager* SceneManager;
	io::IFileSystem* FileSystem;

	bool useGroups;
	bool useMaterials;

	core::array<SObjMtl*> Materials;
};

} // end namespace scene
} // end namespace irr

#endif

