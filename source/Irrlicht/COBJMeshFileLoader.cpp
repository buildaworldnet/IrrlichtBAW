// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_OBJ_LOADER_

#include "COBJMeshFileLoader.h"
#include "IMeshManipulator.h"
#include "IVideoDriver.h"
#include "SMesh.h"
#include "SVertexManipulator.h"
#include "IReadFile.h"
#include "coreutil.h"
#include "os.h"

/*
namespace std
{
    template <>
    struct hash<irr::scene::SObjVertex>
    {
        std::size_t operator()(const irr::scene::SObjVertex& k) const
        {
            using std::size_t;
            using std::hash;

            return hash(k.normal32bit)^
                    (reinterpret_cast<const uint32_t&>(k.pos[0])*4996156539000000107ull)^
                    (reinterpret_cast<const uint32_t&>(k.pos[1])*620612627000000023ull)^
                    (reinterpret_cast<const uint32_t&>(k.pos[2])*1231379668000000199ull)^
                    (reinterpret_cast<const uint32_t&>(k.uv[0])*1099543332000000001ull)^
                    (reinterpret_cast<const uint32_t&>(k.uv[1])*1123461104000000009ull);
        }
    };

}
*/


namespace irr
{
namespace scene
{

//#ifdef _DEBUG
#define _IRR_DEBUG_OBJ_LOADER_
//#endif

static const uint32_t WORD_BUFFER_LENGTH = 512;


//! Constructor
COBJMeshFileLoader::COBJMeshFileLoader(scene::ISceneManager* smgr, io::IFileSystem* fs)
: SceneManager(smgr), FileSystem(fs), useGroups(false), useMaterials(true)
{
	#ifdef _DEBUG
	setDebugName("COBJMeshFileLoader");
	#endif

	if (FileSystem)
		FileSystem->grab();
}


//! destructor
COBJMeshFileLoader::~COBJMeshFileLoader()
{
	if (FileSystem)
		FileSystem->drop();
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool COBJMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "obj" );
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call ICPUMesh::drop().
//! See IReferenceCounted::drop() for more information.
ICPUMesh* COBJMeshFileLoader::createMesh(io::IReadFile* file)
{
	const long filesize = file->getSize();
	if (!filesize)
		return 0;

	const uint32_t WORD_BUFFER_LENGTH = 512;

	core::array<core::vector3df> vertexBuffer;
	core::array<core::vector3df> normalsBuffer;
	core::array<core::vector2df> textureCoordBuffer;

	SObjMtl * currMtl = new SObjMtl();
	Materials.push_back(currMtl);
	uint32_t smoothingGroup=0;

	const io::path fullName = file->getFileName();
	const io::path relPath = io::IFileSystem::getFileDir(fullName)+"/";

	char* buf = new char[filesize];
	memset(buf, 0, filesize);
	file->read((void*)buf, filesize);
	const char* const bufEnd = buf+filesize;

	// Process obj information
	const char* bufPtr = buf;
	std::string grpName, mtlName;
	bool mtlChanged=false;
	while(bufPtr != bufEnd)
	{
		switch(bufPtr[0])
		{
		case 'm':	// mtllib (material)
		{
			if (useMaterials)
			{
				char name[WORD_BUFFER_LENGTH];
				bufPtr = goAndCopyNextWord(name, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
#ifdef _IRR_DEBUG_OBJ_LOADER_
				os::Printer::log("Reading material file",name);
#endif
				readMTL(name, relPath);
			}
		}
			break;

		case 'v':               // v, vn, vt
			switch(bufPtr[1])
			{
			case ' ':          // vertex
				{
					core::vector3df vec;
					bufPtr = readVec3(bufPtr, vec, bufEnd);
					vertexBuffer.push_back(vec);
				}
				break;

			case 'n':       // normal
				{
					core::vector3df vec;
					bufPtr = readVec3(bufPtr, vec, bufEnd);
					normalsBuffer.push_back(vec);
				}
				break;

			case 't':       // texcoord
				{
					core::vector2df vec;
					bufPtr = readUV(bufPtr, vec, bufEnd);
					textureCoordBuffer.push_back(vec);
				}
				break;
			}
			break;

		case 'g': // group name
			{
				char grp[WORD_BUFFER_LENGTH];
				bufPtr = goAndCopyNextWord(grp, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
#ifdef _IRR_DEBUG_OBJ_LOADER_
	os::Printer::log("Loaded group start",grp, ELL_DEBUG);
#endif
				if (useGroups)
				{
					if (0 != grp[0])
						grpName = grp;
					else
						grpName = "default";

                    mtlChanged=true;
				}
			}
			break;

		case 's': // smoothing can be a group or off (equiv. to 0)
			{
				char smooth[WORD_BUFFER_LENGTH];
				bufPtr = goAndCopyNextWord(smooth, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
#ifdef _IRR_DEBUG_OBJ_LOADER_
	os::Printer::log("Loaded smoothing group start",smooth, ELL_DEBUG);
#endif
				if (core::stringc("off")==smooth)
					smoothingGroup=0;
				else
                    sscanf(smooth,"%u",&smoothingGroup);
			}
			break;

		case 'u': // usemtl
			// get name of material
			{
				char matName[WORD_BUFFER_LENGTH];
				bufPtr = goAndCopyNextWord(matName, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
#ifdef _IRR_DEBUG_OBJ_LOADER_
	os::Printer::log("Loaded material start",matName, ELL_DEBUG);
#endif
				mtlName=matName;
				mtlChanged=true;
			}
			break;

		case 'f':               // face
		{
			char vertexWord[WORD_BUFFER_LENGTH]; // for retrieving vertex data
			SObjVertex v;
			// Assign vertex color from currently active material's diffuse color
			if (mtlChanged)
			{
				// retrieve the material
				SObjMtl *useMtl = findMtl(mtlName, grpName);
				// only change material if we found it
				if (useMtl)
					currMtl = useMtl;
				mtlChanged=false;
			}

			// get all vertices data in this face (current line of obj file)
			const core::stringc wordBuffer = copyLine(bufPtr, bufEnd);
			const char* linePtr = wordBuffer.c_str();
			const char* const endPtr = linePtr+wordBuffer.size();

			core::array<uint32_t> faceCorners;
			faceCorners.reallocate(32); // should be large enough

			// read in all vertices
			linePtr = goNextWord(linePtr, endPtr);
			while (0 != linePtr[0])
			{
				// Array to communicate with retrieveVertexIndices()
				// sends the buffer sizes and gets the actual indices
				// if index not set returns -1
				int32_t Idx[3];
				Idx[1] = Idx[2] = -1;

				// read in next vertex's data
				uint32_t wlength = copyWord(vertexWord, linePtr, WORD_BUFFER_LENGTH, endPtr);
				// this function will also convert obj's 1-based index to c++'s 0-based index
				retrieveVertexIndices(vertexWord, Idx, vertexWord+wlength+1, vertexBuffer.size(), textureCoordBuffer.size(), normalsBuffer.size());
				v.pos[0] = vertexBuffer[Idx[0]].X;
				v.pos[1] = vertexBuffer[Idx[0]].Y;
				v.pos[2] = vertexBuffer[Idx[0]].Z;
				//set texcoord
				if ( -1 != Idx[1] )
                {
					v.uv[0] = textureCoordBuffer[Idx[1]].X;
					v.uv[1] = textureCoordBuffer[Idx[1]].Y;
                }
				else
                {
					v.uv[0] = 0.f;
					v.uv[1] = 0.f;
                }
                //set normal
				if ( -1 != Idx[2] )
                {
					core::vectorSIMDf simdNormal;
					simdNormal.set(normalsBuffer[Idx[2]]);
					v.normal32bit = quantizeNormal2_10_10_10(simdNormal);
                }
				else
				{
					v.normal32bit = 0;
					currMtl->RecalculateNormals=true;
				}

				int vertLocation;
				std::map<SObjVertex, int>::iterator n = currMtl->VertMap.find(v);
				if (n!=currMtl->VertMap.end())
				{
					vertLocation = n->second;
				}
				else
				{
					currMtl->Vertices.push_back(v);
					vertLocation = currMtl->Vertices.size() -1;
					currMtl->VertMap.insert(std::pair<SObjVertex, int>(v, vertLocation));
				}

				faceCorners.push_back(vertLocation);

				// go to next vertex
				linePtr = goNextWord(linePtr, endPtr);
			}

			// triangulate the face
			for ( uint32_t i = 1; i < faceCorners.size() - 1; ++i )
			{
				// Add a triangle
				currMtl->Indices.push_back( faceCorners[i+1] );
				currMtl->Indices.push_back( faceCorners[i] );
				currMtl->Indices.push_back( faceCorners[0] );
			}
			faceCorners.set_used(0); // fast clear
			faceCorners.reallocate(32);
		}
		break;

		case '#': // comment
		default:
			break;
		}	// end switch(bufPtr[0])
		// eat up rest of line
		bufPtr = goNextLine(bufPtr, bufEnd);
	}	// end while(bufPtr && (bufPtr-buf<filesize))
	// Clean up the allocate obj file contents
	delete [] buf;

	SCPUMesh* mesh = new SCPUMesh();

	// Combine all the groups (meshbuffers) into the mesh
	for ( uint32_t m = 0; m < Materials.size(); ++m )
	{
		if ( Materials[m]->Indices.size() == 0 )
        {
            delete Materials[m];
            continue;
        }

        if (Materials[m]->RecalculateNormals)
        {
            core::irrAllocatorFast<core::vectorSIMDf> alctr;
            core::vectorSIMDf* newNormals = alctr.allocate(sizeof(core::vectorSIMDf)*Materials[m]->Vertices.size());
            memset(newNormals,0,sizeof(core::vectorSIMDf)*Materials[m]->Vertices.size());
            for (size_t i=0; i<Materials[m]->Indices.size(); i+=3)
            {
                core::vectorSIMDf v1;
                v1.set(Materials[m]->Vertices[Materials[m]->Indices[i+0]].pos);
                core::vectorSIMDf v2;
                v2.set(Materials[m]->Vertices[Materials[m]->Indices[i+1]].pos);
                core::vectorSIMDf v3;
                v3.set(Materials[m]->Vertices[Materials[m]->Indices[i+2]].pos);
                v1.makeSafe3D();
                v2.makeSafe3D();
                v3.makeSafe3D();
                core::vectorSIMDf normal;
                normal.set(core::plane3d<float>(v1.getAsVector3df(), v2.getAsVector3df(), v3.getAsVector3df()).Normal);
                newNormals[Materials[m]->Indices[i+0]] += normal;
                newNormals[Materials[m]->Indices[i+1]] += normal;
                newNormals[Materials[m]->Indices[i+2]] += normal;
            }
            for (size_t i=0; i<Materials[m]->Vertices.size(); i++)
            {
                Materials[m]->Vertices[i].normal32bit = quantizeNormal2_10_10_10(newNormals[i]);
            }
            alctr.deallocate(newNormals);
        }
        if (Materials[m]->Material.MaterialType == -1)
            os::Printer::log("Loading OBJ Models with normal maps and tangents not supported!\n",ELL_ERROR);/*
        {
            SMesh tmp;
            tmp.addMeshBuffer(Materials[m]->Meshbuffer);
            IMesh* tangentMesh = SceneManager->getMeshManipulator()->createMeshWithTangents(&tmp);
            mesh->addMeshBuffer(tangentMesh->getMeshBuffer(0));
            tangentMesh->drop();
        }
        else*/

        ICPUMeshBuffer* meshbuffer = new ICPUMeshBuffer();
        mesh->addMeshBuffer(meshbuffer);
        meshbuffer->drop();

        meshbuffer->getMaterial() = Materials[m]->Material;

        ICPUMeshDataFormatDesc* desc = new ICPUMeshDataFormatDesc();
        meshbuffer->setMeshDataAndFormat(desc);
        desc->drop();

        bool doesntNeedIndices = true;
        size_t baseVertex = Materials[m]->Indices[0];
        for (size_t i=1; i<Materials[m]->Indices.size(); i++)
        {
            if (baseVertex+i!=Materials[m]->Indices[i])
            {
                doesntNeedIndices = false;
                break;
            }
        }

        core::ICPUBuffer* vertexbuf;
        size_t actualVertexCount;
        if (doesntNeedIndices)
        {
            meshbuffer->setIndexCount(Materials[m]->Indices.size()-baseVertex);
            actualVertexCount = meshbuffer->getIndexCount();
        }
        else
        {
            baseVertex = 0;
            actualVertexCount = Materials[m]->Vertices.size();

            core::ICPUBuffer* indexbuf = new core::ICPUBuffer(Materials[m]->Indices.size()*4);
            desc->mapIndexBuffer(indexbuf);
            indexbuf->drop();
            memcpy(indexbuf->getPointer(),&Materials[m]->Indices[0],indexbuf->getSize());

            meshbuffer->setIndexType(video::EIT_32BIT);
            meshbuffer->setIndexCount(Materials[m]->Indices.size());
        }

        vertexbuf = new core::ICPUBuffer(actualVertexCount*sizeof(SObjVertex));
        desc->mapVertexAttrBuffer(vertexbuf,EVAI_ATTR0,ECPA_THREE,ECT_FLOAT,sizeof(SObjVertex),0);
        desc->mapVertexAttrBuffer(vertexbuf,EVAI_ATTR2,ECPA_TWO,ECT_FLOAT,sizeof(SObjVertex),12);
        desc->mapVertexAttrBuffer(vertexbuf,EVAI_ATTR3,ECPA_FOUR,ECT_INT_2_10_10_10_REV,sizeof(SObjVertex),20); //normal
        memcpy(vertexbuf->getPointer(),Materials[m]->Vertices.data()+baseVertex,vertexbuf->getSize());
        vertexbuf->drop();

        //memory is precious
        delete Materials[m];
	}

	// more cleaning up
	Materials.clear();

	if ( 0 != mesh->getMeshBufferCount() )
		mesh->recalculateBoundingBox(true);
	else
    {
		mesh->drop();
		mesh = NULL;
    }

	return mesh;
}


const char* COBJMeshFileLoader::readTextures(const char* bufPtr, const char* const bufEnd, SObjMtl* currMaterial, const io::path& relPath)
{
	uint8_t type=0; // map_Kd - diffuse color texture map
	// map_Ks - specular color texture map
	// map_Ka - ambient color texture map
	// map_Ns - shininess texture map
	if ((!strncmp(bufPtr,"map_bump",8)) || (!strncmp(bufPtr,"bump",4)))
		type=1; // normal map
	else if ((!strncmp(bufPtr,"map_d",5)) || (!strncmp(bufPtr,"map_opacity",11)))
		type=2; // opacity map
	else if (!strncmp(bufPtr,"map_refl",8))
		type=3; // reflection map
	// extract new material's name
	char textureNameBuf[WORD_BUFFER_LENGTH];
	bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);

	float bumpiness = 6.0f;
	bool clamp = false;
	// handle options
	while (textureNameBuf[0]=='-')
	{
		if (!strncmp(bufPtr,"-bm",3))
		{
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			sscanf(textureNameBuf,"%f",&currMaterial->Material.MaterialTypeParam);
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			continue;
		}
		else
		if (!strncmp(bufPtr,"-blendu",7))
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
		else
		if (!strncmp(bufPtr,"-blendv",7))
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
		else
		if (!strncmp(bufPtr,"-cc",3))
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
		else
		if (!strncmp(bufPtr,"-clamp",6))
			bufPtr = readBool(bufPtr, clamp, bufEnd);
		else
		if (!strncmp(bufPtr,"-texres",7))
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
		else
		if (!strncmp(bufPtr,"-type",5))
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
		else
		if (!strncmp(bufPtr,"-mm",3))
		{
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
		}
		else
		if (!strncmp(bufPtr,"-o",2)) // texture coord translation
		{
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			// next parameters are optional, so skip rest of loop if no number is found
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			if (!core::isdigit(textureNameBuf[0]))
				continue;
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			if (!core::isdigit(textureNameBuf[0]))
				continue;
		}
		else
		if (!strncmp(bufPtr,"-s",2)) // texture coord scale
		{
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			// next parameters are optional, so skip rest of loop if no number is found
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			if (!core::isdigit(textureNameBuf[0]))
				continue;
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			if (!core::isdigit(textureNameBuf[0]))
				continue;
		}
		else
		if (!strncmp(bufPtr,"-t",2))
		{
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			// next parameters are optional, so skip rest of loop if no number is found
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			if (!core::isdigit(textureNameBuf[0]))
				continue;
			bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
			if (!core::isdigit(textureNameBuf[0]))
				continue;
		}
		// get next word
		bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
	}

	if ((type==1) && (core::isdigit(textureNameBuf[0])))
	{
		sscanf(textureNameBuf,"%f",&currMaterial->Material.MaterialTypeParam);
		bufPtr = goAndCopyNextWord(textureNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
	}
	if (clamp)
    {
        for (size_t i=0; i<_IRR_MATERIAL_MAX_TEXTURES_; i++)
        {
            currMaterial->Material.TextureLayer[i].SamplingParams.TextureWrapU = video::ETC_CLAMP_TO_BORDER;
            currMaterial->Material.TextureLayer[i].SamplingParams.TextureWrapV = video::ETC_CLAMP_TO_BORDER;
            currMaterial->Material.TextureLayer[i].SamplingParams.TextureWrapW = video::ETC_CLAMP_TO_BORDER;
        }
    }

	io::path texname(textureNameBuf);
	handleBackslashes(&texname);

	video::ITexture * texture = 0;
	bool newTexture=false;
	if (texname.size())
	{
	    /*
 		io::path texnameWithUserPath( SceneManager->getParameters()->getAttributeAsString(OBJ_TEXTURE_PATH) );
 		if ( texnameWithUserPath.size() )
 		{
 			texnameWithUserPath += '/';
 			texnameWithUserPath += texname;
 		}
 		if (FileSystem->existFile(texnameWithUserPath))
 			texture = SceneManager->getVideoDriver()->getTexture(texnameWithUserPath);
		else */if (FileSystem->existFile(texname))
		{
			newTexture = SceneManager->getVideoDriver()->findTexture(texname) == 0;
			texture = SceneManager->getVideoDriver()->getTexture(texname);
		}
		else
		{
			newTexture = SceneManager->getVideoDriver()->findTexture(relPath + texname) == 0;
			// try to read in the relative path, the .obj is loaded from
			texture = SceneManager->getVideoDriver()->getTexture( relPath + texname );
		}
	}
	if ( texture )
	{
		if (type==0)
        {
			currMaterial->Material.setTexture(0, texture);
        }
		else if (type==1)
		{
			if (newTexture)
				SceneManager->getVideoDriver()->makeNormalMapTexture(texture, bumpiness);
			currMaterial->Material.setTexture(1, texture);
			currMaterial->Material.MaterialType=(video::E_MATERIAL_TYPE)-1;
			currMaterial->Material.MaterialTypeParam=0.035f;
		}
		else if (type==2)
		{
			currMaterial->Material.setTexture(0, texture);
			currMaterial->Material.MaterialType=video::EMT_TRANSPARENT_ADD_COLOR;
		}
		else if (type==3)
		{
//						currMaterial->Material.Textures[1] = texture;
//						currMaterial->Material.MaterialType=video::EMT_REFLECTION_2_LAYER;
		}
	}
	return bufPtr;
}


void COBJMeshFileLoader::readMTL(const char* fileName, const io::path& relPath)
{
	const io::path realFile(fileName);
	io::IReadFile * mtlReader;

	if (FileSystem->existFile(realFile))
		mtlReader = FileSystem->createAndOpenFile(realFile);
	else if (FileSystem->existFile(relPath + realFile))
		mtlReader = FileSystem->createAndOpenFile(relPath + realFile);
	else if (FileSystem->existFile(io::IFileSystem::getFileBasename(realFile)))
		mtlReader = FileSystem->createAndOpenFile(io::IFileSystem::getFileBasename(realFile));
	else
		mtlReader = FileSystem->createAndOpenFile(relPath + io::IFileSystem::getFileBasename(realFile));
	if (!mtlReader)	// fail to open and read file
	{
		os::Printer::log("Could not open material file", realFile.c_str(), ELL_WARNING);
		return;
	}

	const long filesize = mtlReader->getSize();
	if (!filesize)
	{
		os::Printer::log("Skipping empty material file", realFile.c_str(), ELL_WARNING);
		mtlReader->drop();
		return;
	}

	char* buf = new char[filesize];
	mtlReader->read((void*)buf, filesize);
	const char* bufEnd = buf+filesize;

	SObjMtl* currMaterial = 0;

	const char* bufPtr = buf;
	while(bufPtr != bufEnd)
	{
		switch(*bufPtr)
		{
			case 'n': // newmtl
			{
				// if there's an existing material, store it first
				if ( currMaterial )
					Materials.push_back( currMaterial );

				// extract new material's name
				char mtlNameBuf[WORD_BUFFER_LENGTH];
				bufPtr = goAndCopyNextWord(mtlNameBuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);

				currMaterial = new SObjMtl;
				currMaterial->Name = mtlNameBuf;
			}
			break;
			case 'i': // illum - illumination
			if ( currMaterial )
			{
				const uint32_t COLOR_BUFFER_LENGTH = 16;
				char illumStr[COLOR_BUFFER_LENGTH];

				bufPtr = goAndCopyNextWord(illumStr, bufPtr, COLOR_BUFFER_LENGTH, bufEnd);
				currMaterial->Illumination = (char)atol(illumStr);
			}
			break;
			case 'N':
			if ( currMaterial )
			{
				switch(bufPtr[1])
				{
				case 's': // Ns - shininess
					{
						const uint32_t COLOR_BUFFER_LENGTH = 16;
						char nsStr[COLOR_BUFFER_LENGTH];

						bufPtr = goAndCopyNextWord(nsStr, bufPtr, COLOR_BUFFER_LENGTH, bufEnd);
						float shininessValue;
						sscanf(nsStr,"%f",&shininessValue);

						// wavefront shininess is from [0, 1000], so scale for OpenGL
						shininessValue *= 0.128f;
						currMaterial->Material.Shininess = shininessValue;
					}
				break;
				case 'i': // Ni - refraction index
					{
						char tmpbuf[WORD_BUFFER_LENGTH];
						bufPtr = goAndCopyNextWord(tmpbuf, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
					}
				break;
				}
			}
			break;
			case 'K':
			if ( currMaterial )
			{
				switch(bufPtr[1])
				{
				case 'd':		// Kd = diffuse
					{
						bufPtr = readColor(bufPtr, currMaterial->Material.DiffuseColor, bufEnd);

					}
					break;

				case 's':		// Ks = specular
					{
						bufPtr = readColor(bufPtr, currMaterial->Material.SpecularColor, bufEnd);
					}
					break;

				case 'a':		// Ka = ambience
					{
						bufPtr=readColor(bufPtr, currMaterial->Material.AmbientColor, bufEnd);
					}
					break;
				case 'e':		// Ke = emissive
					{
						bufPtr=readColor(bufPtr, currMaterial->Material.EmissiveColor, bufEnd);
					}
					break;
				}	// end switch(bufPtr[1])
			}	// end case 'K': if ( 0 != currMaterial )...
			break;
			case 'b': // bump
			case 'm': // texture maps
			if (currMaterial)
			{
				bufPtr=readTextures(bufPtr, bufEnd, currMaterial, relPath);
			}
			break;
			case 'd': // d - transparency
			if ( currMaterial )
			{
				const uint32_t COLOR_BUFFER_LENGTH = 16;
				char dStr[COLOR_BUFFER_LENGTH];

				bufPtr = goAndCopyNextWord(dStr, bufPtr, COLOR_BUFFER_LENGTH, bufEnd);
				float dValue;
				sscanf(dStr,"%f",&dValue);

				currMaterial->Material.DiffuseColor.setAlpha( (int32_t)(dValue * 255) );
				if (dValue<1.0f)
					currMaterial->Material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
			}
			break;
			case 'T':
			if ( currMaterial )
			{
				switch ( bufPtr[1] )
				{
				case 'f':		// Tf - Transmitivity
					const uint32_t COLOR_BUFFER_LENGTH = 16;
					char redStr[COLOR_BUFFER_LENGTH];
					char greenStr[COLOR_BUFFER_LENGTH];
					char blueStr[COLOR_BUFFER_LENGTH];

					bufPtr = goAndCopyNextWord(redStr,   bufPtr, COLOR_BUFFER_LENGTH, bufEnd);
					bufPtr = goAndCopyNextWord(greenStr, bufPtr, COLOR_BUFFER_LENGTH, bufEnd);
					bufPtr = goAndCopyNextWord(blueStr,  bufPtr, COLOR_BUFFER_LENGTH, bufEnd);

					float red,green,blue;
					sscanf(redStr,"%f",&red);
					sscanf(greenStr,"%f",&green);
					sscanf(blueStr,"%f",&blue);
					float transparency = ( red+green+blue ) / 3;

					currMaterial->Material.DiffuseColor.setAlpha( (int32_t)(transparency * 255) );
					if (transparency < 1.0f)
						currMaterial->Material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
				}
			}
			break;
			default: // comments or not recognised
			break;
		} // end switch(bufPtr[0])
		// go to next line
		bufPtr = goNextLine(bufPtr, bufEnd);
	}	// end while (bufPtr)

	// end of file. if there's an existing material, store it
	if ( currMaterial )
		Materials.push_back( currMaterial );

	delete [] buf;
	mtlReader->drop();
}


//! Read RGB color
const char* COBJMeshFileLoader::readColor(const char* bufPtr, video::SColor& color, const char* const bufEnd)
{
	const uint32_t COLOR_BUFFER_LENGTH = 16;
	char colStr[COLOR_BUFFER_LENGTH];

	float tmp;

	color.setAlpha(255);
	bufPtr = goAndCopyNextWord(colStr, bufPtr, COLOR_BUFFER_LENGTH, bufEnd);
	sscanf(colStr,"%f",&tmp);
	color.setRed((int32_t)(tmp * 255.0f));
	bufPtr = goAndCopyNextWord(colStr,   bufPtr, COLOR_BUFFER_LENGTH, bufEnd);
	sscanf(colStr,"%f",&tmp);
	color.setGreen((int32_t)(tmp * 255.0f));
	bufPtr = goAndCopyNextWord(colStr,   bufPtr, COLOR_BUFFER_LENGTH, bufEnd);
	sscanf(colStr,"%f",&tmp);
	color.setBlue((int32_t)(tmp * 255.0f));
	return bufPtr;
}


//! Read 3d vector of floats
const char* COBJMeshFileLoader::readVec3(const char* bufPtr, core::vector3df& vec, const char* const bufEnd)
{
	const uint32_t WORD_BUFFER_LENGTH = 256;
	char wordBuffer[WORD_BUFFER_LENGTH];

	bufPtr = goAndCopyNextWord(wordBuffer, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
	sscanf(wordBuffer,"%f",&vec.X);
	bufPtr = goAndCopyNextWord(wordBuffer, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
	sscanf(wordBuffer,"%f",&vec.Y);
	bufPtr = goAndCopyNextWord(wordBuffer, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
	sscanf(wordBuffer,"%f",&vec.Z);

	vec.X = -vec.X; // change handedness
	return bufPtr;
}


//! Read 2d vector of floats
const char* COBJMeshFileLoader::readUV(const char* bufPtr, core::vector2df& vec, const char* const bufEnd)
{
	const uint32_t WORD_BUFFER_LENGTH = 256;
	char wordBuffer[WORD_BUFFER_LENGTH];

	bufPtr = goAndCopyNextWord(wordBuffer, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
	sscanf(wordBuffer,"%f",&vec.X);
	bufPtr = goAndCopyNextWord(wordBuffer, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
	sscanf(wordBuffer,"%f",&vec.Y);

	vec.Y = 1-vec.Y; // change handedness
	return bufPtr;
}


//! Read boolean value represented as 'on' or 'off'
const char* COBJMeshFileLoader::readBool(const char* bufPtr, bool& tf, const char* const bufEnd)
{
	const uint32_t BUFFER_LENGTH = 8;
	char tfStr[BUFFER_LENGTH];

	bufPtr = goAndCopyNextWord(tfStr, bufPtr, BUFFER_LENGTH, bufEnd);
	tf = strcmp(tfStr, "off") != 0;
	return bufPtr;
}


COBJMeshFileLoader::SObjMtl* COBJMeshFileLoader::findMtl(const std::string& mtlName, const std::string& grpName)
{
	COBJMeshFileLoader::SObjMtl* defMaterial = 0;
	// search existing Materials for best match
	// exact match does return immediately, only name match means a new group
	for (uint32_t i = 0; i < Materials.size(); ++i)
	{
		if ( Materials[i]->Name == mtlName )
		{
			if ( Materials[i]->Group == grpName )
				return Materials[i];
			else
				defMaterial = Materials[i];
		}
	}
	// we found a partial match
	if (defMaterial)
	{
		Materials.push_back(new SObjMtl(*defMaterial));
		Materials.getLast()->Group = grpName;
		return Materials.getLast();
	}
	// we found a new group for a non-existant material
	else if (grpName.length())
	{
		Materials.push_back(new SObjMtl(*Materials[0]));
		Materials.getLast()->Group = grpName;
		return Materials.getLast();
	}
	return 0;
}


//! skip space characters and stop on first non-space
const char* COBJMeshFileLoader::goFirstWord(const char* buf, const char* const bufEnd, bool acrossNewlines)
{
	// skip space characters
	if (acrossNewlines)
		while((buf != bufEnd) && core::isspace(*buf))
			++buf;
	else
		while((buf != bufEnd) && core::isspace(*buf) && (*buf != '\n'))
			++buf;

	return buf;
}


//! skip current word and stop at beginning of next one
const char* COBJMeshFileLoader::goNextWord(const char* buf, const char* const bufEnd, bool acrossNewlines)
{
	// skip current word
	while(( buf != bufEnd ) && !core::isspace(*buf))
		++buf;

	return goFirstWord(buf, bufEnd, acrossNewlines);
}


//! Read until line break is reached and stop at the next non-space character
const char* COBJMeshFileLoader::goNextLine(const char* buf, const char* const bufEnd)
{
	// look for newline characters
	while(buf != bufEnd)
	{
		// found it, so leave
		if (*buf=='\n' || *buf=='\r')
			break;
		++buf;
	}
	return goFirstWord(buf, bufEnd);
}


uint32_t COBJMeshFileLoader::copyWord(char* outBuf, const char* const inBuf, uint32_t outBufLength, const char* const bufEnd)
{
	if (!outBufLength)
		return 0;
	if (!inBuf)
	{
		*outBuf = 0;
		return 0;
	}

	uint32_t i = 0;
	while(inBuf[i])
	{
		if (core::isspace(inBuf[i]) || &(inBuf[i]) == bufEnd)
			break;
		++i;
	}

	uint32_t length = core::min_(i, outBufLength-1);
	for (uint32_t j=0; j<length; ++j)
		outBuf[j] = inBuf[j];

	outBuf[length] = 0;
	return length;
}


core::stringc COBJMeshFileLoader::copyLine(const char* inBuf, const char* bufEnd)
{
	if (!inBuf)
		return core::stringc();

	const char* ptr = inBuf;
	while (ptr<bufEnd)
	{
		if (*ptr=='\n' || *ptr=='\r')
			break;
		++ptr;
	}
	// we must avoid the +1 in case the array is used up
	return core::stringc(inBuf, (uint32_t)(ptr-inBuf+((ptr < bufEnd) ? 1 : 0)));
}


const char* COBJMeshFileLoader::goAndCopyNextWord(char* outBuf, const char* inBuf, uint32_t outBufLength, const char* bufEnd)
{
	inBuf = goNextWord(inBuf, bufEnd, false);
	copyWord(outBuf, inBuf, outBufLength, bufEnd);
	return inBuf;
}


bool COBJMeshFileLoader::retrieveVertexIndices(char* vertexData, int32_t* idx, const char* bufEnd, uint32_t vbsize, uint32_t vtsize, uint32_t vnsize)
{
	char word[16] = "";
	const char* p = goFirstWord(vertexData, bufEnd);
	uint32_t idxType = 0;	// 0 = posIdx, 1 = texcoordIdx, 2 = normalIdx

	uint32_t i = 0;
	while ( p != bufEnd )
	{
		if ( ( core::isdigit(*p)) || (*p == '-') )
		{
			// build up the number
			word[i++] = *p;
		}
		else if ( *p == '/' || *p == ' ' || *p == '\0' )
		{
			// number is completed. Convert and store it
			word[i] = '\0';
			// if no number was found index will become 0 and later on -1 by decrement
			sscanf(word,"%u",idx+idxType);
			if (idx[idxType]<0)
			{
				switch (idxType)
				{
					case 0:
						idx[idxType] += vbsize;
						break;
					case 1:
						idx[idxType] += vtsize;
						break;
					case 2:
						idx[idxType] += vnsize;
						break;
				}
			}
			else
				idx[idxType]-=1;

			// reset the word
			word[0] = '\0';
			i = 0;

			// go to the next kind of index type
			if (*p == '/')
			{
				if ( ++idxType > 2 )
				{
					// error checking, shouldn't reach here unless file is wrong
					idxType = 0;
				}
			}
			else
			{
				// set all missing values to disable (=-1)
				while (++idxType < 3)
					idx[idxType]=-1;
				++p;
				break; // while
			}
		}

		// go to the next char
		++p;
	}

	return true;
}




} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_OBJ_LOADER_

