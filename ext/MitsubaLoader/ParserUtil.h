#ifndef __I_PARSER_UTIL_H_INCLUDED__
#define __I_PARSER_UTIL_H_INCLUDED__

#include "irr/core/core.h"
#include "irr/asset/IAssetLoader.h"
#include "irr/asset/CCPUMesh.h"

#include "../../ext/MitsubaLoader/CElementFactory.h"
#include "../../ext/MitsubaLoader/CGlobalMitsubaMetadata.h"

#include "expat/lib/expat.h"

#include <stack>


namespace irr
{
namespace ext
{
namespace MitsubaLoader
{

	   	  
//now unsupported elements (like  <sensor> (for now), for example) and its children elements will be ignored
class ParserFlowController
{
public:
	ParserFlowController()
		:isParsingSuspendedFlag(false) {};

	bool suspendParsingIfElNotSupported(const std::string& _el);
	void checkForUnsuspend(const std::string& _el);

	inline bool isParsingSuspended() const { return isParsingSuspendedFlag; }

private:
	_IRR_STATIC_INLINE_CONSTEXPR const char* unsElements[] =
	{ 
		"animation", "medium", "default", nullptr 
	};
	bool isParsingSuspendedFlag;
	std::string notSupportedElement;

};

class ParserLog
{
public:
	/*prints this message:
	Mitsuba loader error:
	Invalid .xml file structure: message */
	static void invalidXMLFileStructure(const std::string& errorMessage);

};


template<typename... types>
class ElementPool // : public std::tuple<core::vector<types>...>
{
		core::SimpleBlockBasedAllocatorST<core::LinearAddressAllocator<uint32_t> > poolAllocator;
	public:
		ElementPool() : poolAllocator(4096u*1024u, 256u) {}

		template<typename T, typename... Args>
		inline T* construct(Args&& ... args)
		{
			T* ptr = reinterpret_cast<T*>(poolAllocator.allocate(sizeof(T), alignof(T)));
			return new (ptr) T(std::forward<Args>(args)...);
		}
};

//struct, which will be passed to expat handlers as user data (first argument) see: XML_StartElementHandler or XML_EndElementHandler in expat.h
class ParserManager
{
	public:
		//! Constructor 
		ParserManager(asset::IAssetLoader::IAssetLoaderOverride* _override, XML_Parser _parser) :
								m_override(_override), m_parser(_parser), m_sceneDeclCount(0),
								m_globalMetadata(core::make_smart_refctd_ptr<CGlobalMitsubaMetadata>())
		{
		}

		auto& getParserFlowController() { return pfc; }
		auto& getParserFlowController() const { return pfc; }

		inline void killParseWithError(const std::string& message)
		{
			_IRR_DEBUG_BREAK_IF(true);
			ParserLog::invalidXMLFileStructure(message);
			XML_StopParser(m_parser, false);
		}

		void parseElement(const char* _el, const char** _atts);

		void onEnd(const char* _el);

		inline auto&& releaseTopLevelResources() { return std::move(shapegroups); }

	private:
		void processProperty(const char* _el, const char** _atts);

	private:
		asset::IAssetLoader::IAssetLoaderOverride* m_override;
		XML_Parser m_parser;

		//
		uint32_t m_sceneDeclCount;
		//
		core::smart_refctd_ptr<CGlobalMitsubaMetadata> m_globalMetadata;
		//
		core::vector<core::smart_refctd_ptr<asset::CCPUMesh>> shapegroups;
		//
		ElementPool<
			CElementIntegrator,
			CElementSensor,
			CElementFilm,
			CElementRFilter,
			CElementSampler,
			CElementShape,
			CElementBSDF,
			CElementTexture,
			CElementEmitter
					> objects;
		// aliases and names
		core::unordered_map<std::string,IElement*,core::CaseInsensitiveHash,core::CaseInsensitiveEquals> handles;

		/*stack of currently processed elements
		each element of index N is parent of the element of index N+1
		the scene element is a parent of all elements of index 0 */
		core::stack<IElement*> elements; 

		ParserFlowController pfc;

		friend class CElementFactory;
};

void elementHandlerStart(void* _data, const char* _el, const char** _atts);
void elementHandlerEnd(void* _data, const char* _el);

}
}
}

#endif