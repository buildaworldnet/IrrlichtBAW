#include "IrrCompileConfig.h"
#include "IMeshBuffer.h"

// Based on zeux's meshoptimizer (https://github.com/zeux/meshoptimizer) available under MIT license

namespace irr { namespace scene 
{

class COverdrawMeshOptimizer
{
	enum
	{
		CACHE_SIZE = 16
	};

	struct ClusterSortData
	{
		uint32_t cluster;
		float dot;

		bool operator>(const ClusterSortData& other) const
		{
			// high product = possible occluder, render early
			return dot > other.dot;
		}
	};

	// private, undefined constructor
	COverdrawMeshOptimizer();

public:
	//! Creates new or modifies given mesh reordering indices to reduce pixel overdraw and vertex shader invocations.
	/**
	@param _inbuffer Input mesh buffer.
	@param _createNew Flag deciding whether to create new mesh (not modifying given one) or just optimize given mesh. Defaulted to true (i.e. create new).
	@param _threshold Indicates how much the overdraw optimizer can degrade vertex cache efficiency (1.05 = up to 5%) to reduce overdraw more efficiently. Defaulted to 1.05 (i.e. 5%).
	*/
	static ICPUMeshBuffer* createOptimized(ICPUMeshBuffer* _inbuffer, bool _createNew = true, float _threshold = 1.05f);

private:
	template<typename IdxT>
	static size_t genHardBoundaries(uint32_t* _dst, const IdxT* _indices, size_t _idxCount, size_t _vtxCount);
	template<typename IdxT>
	static size_t genSoftBoundaries(uint32_t* _dst, const IdxT* _indices, size_t _idxCount, size_t _vtxCount, const uint32_t* _clusters, size_t _clusterCount, float _threshold);

	template<typename IdxT>
	static void calcSortData(ClusterSortData* _dst, const IdxT* _indices, size_t _idxCount, const std::vector<core::vectorSIMDf>& _positions, const uint32_t* _clusters, size_t _clusterCount);

	static size_t updateCache(uint32_t _a, uint32_t _b, uint32_t _c, size_t* _cacheTimestamps, size_t& _timestamp);
};

}}