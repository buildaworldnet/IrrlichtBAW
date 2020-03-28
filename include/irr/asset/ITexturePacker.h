#ifndef __IRR_C_TEXTURE_PACKER_H_INCLUDED__
#define __IRR_C_TEXTURE_PACKER_H_INCLUDED__

#include "irr/asset/format/EFormat.h"
#include "irr/asset/ICPUImage.h"
#include "irr/core/alloc/GeneralpurposeAddressAllocator.h"
#include "irr/core/alloc/PoolAddressAllocator.h"
#include "irr/core/math/morton.h"
#include "irr/core/alloc/address_allocator_traits.h"

namespace irr {
namespace asset
{

class ITexturePacker
{
protected:
    const uint32_t m_addr_layerShift;
    const uint32_t m_physPgOffset_xMask;

    const uint32_t m_pgSzxy;
    const uint32_t m_pgSzxy_log2;
    const uint32_t m_tilesPerDim;

    using pg_tab_addr_alctr_t = core::GeneralpurposeAddressAllocator<uint32_t>;
    core::smart_refctd_dynamic_array<uint8_t> m_pgTabAddrAlctr_reservedSpc;
    pg_tab_addr_alctr_t m_pgTabAddrAlctr;

public:
#include "irr/irrpack.h"
    //must be 64bit
    struct STextureData
    {
        //unorm16 page table texture UV
        uint16_t pgTab_x;
        uint16_t pgTab_y;
        //unorm16 originalTexSz/maxAllocatableTexSz ratio
        uint16_t scale_x;
        uint16_t scale_y;
    } PACK_STRUCT;
#include "irr/irrunpack.h"

    //! SPhysPgOffset is what is stored in texels of page table!
    struct SPhysPgOffset
    {
        _IRR_STATIC_INLINE_CONSTEXPR uint32_t invalid_addr = 0xffffu;

        SPhysPgOffset(uint32_t _addr) : addr(_addr) {}

        //upper 16 bits are used for address of mip-tail page
        uint32_t addr;
    };
    SPhysPgOffset makeTexOffset(uint32_t _x, uint32_t _y, uint32_t _layer)
    {
        uint32_t addr = (_x & m_physPgOffset_xMask);
        addr |= (_y & m_physPgOffset_xMask) << (m_addr_layerShift >> 1);
        addr |= (_layer << m_addr_layerShift);
        addr |= (SPhysPgOffset::invalid_addr << 16);
        return addr;
    }
    uint32_t physPgOffset_x(SPhysPgOffset _offset) const { return _offset.addr & m_physPgOffset_xMask; }
    uint32_t physPgOffset_y(SPhysPgOffset _offset) const { return (_offset.addr >> (m_addr_layerShift>>1)) & m_physPgOffset_xMask; }
    uint32_t physPgOffset_layer(SPhysPgOffset _offset) const { return (_offset.addr & 0xffffu)>>m_addr_layerShift; }
    bool physPgOffset_valid(SPhysPgOffset _offset) const { return (_offset.addr&0xffffu) != SPhysPgOffset::invalid_addr; }
    bool physPgOffset_hasMipTailAddr(SPhysPgOffset _offset) const { return physPgOffset_valid(physPgOffset_mipTailAddr(_offset)); }
    SPhysPgOffset physPgOffset_mipTailAddr(SPhysPgOffset _offset) const { return _offset.addr>>16; }


    using page_tab_offset_t = core::vector2du32_SIMD;
    static page_tab_offset_t page_tab_offset_invalid() { return page_tab_offset_t(~0u,~0u); }
    
    //m_pgTabSzxy is PoT because of allocation using morton codes
    //m_tilesPerDim is PoT actually physical addr tex doesnt even have to be square nor PoT, but it makes addr/offset encoding easier
    //m_pgSzxy is PoT because of packing mip-tail
    ITexturePacker(uint32_t _pgTabSzxy_log2 = 10u, uint32_t _pgSzxy_log2 = 8u, uint32_t _tilesPerDim_log2 = 5u) :
        m_addr_layerShift(_tilesPerDim_log2<<1),
        m_physPgOffset_xMask((1u<<(m_addr_layerShift>>1))-1u),
        m_pgSzxy(1u<<_pgSzxy_log2),
        m_pgSzxy_log2(_pgSzxy_log2),
        m_tilesPerDim(1u<<_tilesPerDim_log2),
        m_pgTabAddrAlctr_reservedSpc(core::make_refctd_dynamic_array<decltype(m_pgTabAddrAlctr_reservedSpc)>(pg_tab_addr_alctr_t::reserved_size((1u<<_pgTabSzxy_log2)*(1u<<_pgTabSzxy_log2), (1u<<_pgTabSzxy_log2)*(1u<<_pgTabSzxy_log2), 1u))),
        m_pgTabAddrAlctr(m_pgTabAddrAlctr_reservedSpc->data(), 0u, 0u, (1u<<_pgTabSzxy_log2)*(1u<<_pgTabSzxy_log2), (1u<<_pgTabSzxy_log2)*(1u<<_pgTabSzxy_log2), 1u)
    {
    }

    virtual ~ITexturePacker() = default;

    uint32_t getPageSize() const { return m_pgSzxy; }

    page_tab_offset_t alloc(const IImage* _img, const IImage::SSubresourceRange& _subres)
    {
        uint32_t szAndAlignment = computeSquareSz(_img, _subres);
        szAndAlignment *= szAndAlignment;

        uint32_t addr{};
        core::address_allocator_traits<pg_tab_addr_alctr_t>::multi_alloc_addr(m_pgTabAddrAlctr, 1u, &addr, &szAndAlignment, &szAndAlignment, nullptr);
        return (addr==pg_tab_addr_alctr_t::invalid_address) ? 
            page_tab_offset_invalid() :
            page_tab_offset_t(core::morton2d_decode_x(addr), core::morton2d_decode_y(addr));
    }
    virtual void free(page_tab_offset_t _addr, const IImage* _img, const IImage::SSubresourceRange& _subres)
    {
        if ((_addr==page_tab_offset_invalid()).all())
            return;

        uint32_t sz = computeSquareSz(_img, _subres);
        sz *= sz;
        const uint32_t addr = core::morton2d_encode(_addr.x, _addr.y);

        core::address_allocator_traits<pg_tab_addr_alctr_t>::multi_free_addr(m_pgTabAddrAlctr, 1u, &addr, &sz);
    }

protected:
    uint32_t neededPageCountForSide(uint32_t _sideExtent, uint32_t _level)
    {
        return (std::max(_sideExtent>>_level,1u)+m_pgSzxy-1u) / m_pgSzxy;
    }
private:
    uint32_t computeSquareSz(const IImage* _img, const ICPUImage::SSubresourceRange& _subres)
    {
        auto extent = _img->getCreationParameters().extent;

        const uint32_t w = neededPageCountForSide(extent.width, _subres.baseMipLevel);
        const uint32_t h = neededPageCountForSide(extent.height, _subres.baseMipLevel);

        return core::roundUpToPoT(std::max(w,h));
    }
};

class ICPUTexturePacker : public core::IReferenceCounted, public ITexturePacker
{
protected:
    virtual ~ICPUTexturePacker() = default;

public:
    _IRR_STATIC_INLINE_CONSTEXPR uint32_t MAX_PHYSICAL_PAGE_SIZE_LOG2 = 9u;
    _IRR_STATIC_INLINE_CONSTEXPR uint32_t MAX_PHYSICAL_PAGE_SIZE = 1u<<MAX_PHYSICAL_PAGE_SIZE_LOG2;

    uint32_t maxAllocatableTextureSz() const { return m_pgSzxy<<m_pageTable->getCreationParameters().mipLevels; }

    uint32_t physAddrTexLayerSz() const
    {
        return m_tilesPerDim * (2u*m_tilePadding+m_pgSzxy);
    }
    //! @returns texel-wise offset of physical page
    core::vector3du32_SIMD pageCoords(SPhysPgOffset _txoffset, uint32_t _pgSz) const
    {
        core::vector3du32_SIMD coords(physPgOffset_x(_txoffset), physPgOffset_y(_txoffset), 0u);
        coords *= (_pgSz + 2u*m_tilePadding);
        coords += m_tilePadding;
        coords.z = physPgOffset_layer(_txoffset);
        return coords;
    }
    struct SMiptailPacker
    {
        struct rect
        {
            int x, y, mx, my;
        };
        static bool computeMiptailOffsets(rect* res, int log2SIZE, int padding=9);
    };

    ICPUTexturePacker(E_FORMAT_CLASS _fclass, E_FORMAT _format, uint32_t _pgTabSzxy_log2 = 10u, uint32_t _pgTabMipLevels = 11u, uint32_t _pgSzxy_log2 = 8u, uint32_t _tilesPerDim_log2 = 5u, uint32_t _numLayers = 4u, uint32_t _tilePad = 9u/*max_aniso/2+1*/) :
        ITexturePacker(_pgTabSzxy_log2, _pgSzxy_log2, _tilesPerDim_log2), m_tilePadding(_tilePad),
        m_physPgAddrAlctr_reservedSpc(core::make_refctd_dynamic_array<decltype(m_physPgAddrAlctr_reservedSpc)>(phys_pg_addr_alctr_t::reserved_size(1u, _numLayers*(1u<<_tilesPerDim_log2)*(1u<<_tilesPerDim_log2), 1u))),
        m_physPgAddrAlctr(m_physPgAddrAlctr_reservedSpc->data(), 0u, 0u, 1u, _numLayers*(1u<<_tilesPerDim_log2)*(1u<<_tilesPerDim_log2), 1u)
    {
        assert(getFormatClass(_format)==_fclass);
        {
            const uint32_t SZ = physAddrTexLayerSz();

            ICPUImage::SCreationParams params;
            params.extent = { SZ,SZ,1u };
            params.format = _format;
            params.arrayLayers = _numLayers;
            params.mipLevels = 1u;
            params.type = IImage::ET_2D;
            params.samples = IImage::ESCF_1_BIT;
            params.flags = static_cast<IImage::E_CREATE_FLAGS>(0);

            m_physAddrTex = ICPUImage::create(std::move(params));
            auto regions = core::make_refctd_dynamic_array<core::smart_refctd_dynamic_array<IImage::SBufferCopy>>(1ull);
            auto& region = regions->front();
            region.imageSubresource.mipLevel = 0u;
            region.imageSubresource.baseArrayLayer = 0u;
            region.imageSubresource.layerCount = _numLayers;
            region.bufferOffset = 0u;
            region.bufferRowLength = SZ;
            region.bufferImageHeight = 0u; //tightly packed
            region.imageOffset = { 0u, 0u, 0u };
            region.imageExtent = params.extent;
            auto buffer = core::make_smart_refctd_ptr<ICPUBuffer>(getTexelOrBlockBytesize(_format) * params.extent.width*params.extent.height*params.arrayLayers);
            m_physAddrTex->setBufferAndRegions(std::move(buffer), regions);
        }
        {
            uint32_t pgTabSzxy = 1u<<_pgTabSzxy_log2;

            ICPUImage::SCreationParams params;
            params.arrayLayers = 1u;
            params.extent = {pgTabSzxy,pgTabSzxy,1u};
            params.flags = static_cast<ICPUImage::E_CREATE_FLAGS>(0);
            params.format = EF_R16G16_UINT;
            params.mipLevels = _pgTabMipLevels;
            params.samples = ICPUImage::ESCF_1_BIT;
            params.type = IImage::ET_2D;
            m_pageTable = ICPUImage::create(std::move(params));

            const uint32_t texelSz = getTexelOrBlockBytesize(m_pageTable->getCreationParameters().format);
        
            auto regions = core::make_refctd_dynamic_array<core::smart_refctd_dynamic_array<ICPUImage::SBufferCopy>>(m_pageTable->getCreationParameters().mipLevels);

            uint32_t bufOffset = 0u;
            for (uint32_t i = 0u; i < m_pageTable->getCreationParameters().mipLevels; ++i)
            {
                const uint32_t tilesPerLodDim = pgTabSzxy>>i;
                const uint32_t regionSz = tilesPerLodDim*tilesPerLodDim*texelSz;
                auto& region = (*regions)[i];
                region.bufferOffset = bufOffset;
                region.bufferImageHeight = 0u;
                region.bufferRowLength = tilesPerLodDim*texelSz;
                region.imageExtent = {tilesPerLodDim,tilesPerLodDim,1u};
                region.imageOffset = {0,0,0};
                region.imageSubresource.baseArrayLayer = 0u;
                region.imageSubresource.layerCount = 1u;
                region.imageSubresource.mipLevel = i;
                region.imageSubresource.aspectMask = static_cast<IImage::E_ASPECT_FLAGS>(0);

                bufOffset += regionSz;
            }
            auto buf = core::make_smart_refctd_ptr<ICPUBuffer>(bufOffset);
            uint32_t* bufptr = reinterpret_cast<uint32_t*>(buf->getPointer());
            std::fill(bufptr, bufptr+bufOffset/sizeof(uint32_t), SPhysPgOffset::invalid_addr);

            m_pageTable->setBufferAndRegions(std::move(buf), regions);
        }
        const uint32_t pgSzLog2 = core::findMSB(m_pgSzxy);
        bool ok = SMiptailPacker::computeMiptailOffsets(m_miptailOffsets, pgSzLog2, m_tilePadding);
        assert(ok);
    }

    void free(page_tab_offset_t _addr, const IImage* _img, const IImage::SSubresourceRange& _subres) override
    {
        //free physical pages
        auto extent = _img->getCreationParameters().extent;
        const uint32_t levelCount = countLevelsTakingAtLeastOnePage(extent, _subres);

        for (uint32_t i = 0u; i < levelCount; ++i)
        {
            const uint32_t w = neededPageCountForSide(extent.width, _subres.baseMipLevel+i);
            const uint32_t h = neededPageCountForSide(extent.height, _subres.baseMipLevel+i);

            uint32_t* const pgTab = reinterpret_cast<uint32_t*>(
                reinterpret_cast<uint8_t*>(m_pageTable->getBuffer()->getPointer()) + m_pageTable->getRegions().begin()[i].bufferOffset
                );
            const uint32_t pgtPitch = m_pageTable->getRegions().begin()[i].bufferRowLength;
            const uint32_t offset = (_addr.y>>i)*pgtPitch + (_addr.x>>i);

            for (uint32_t y = 0u; y < h; ++y)
                for (uint32_t x = 0u; x < w; ++x)
                {
                    const uint32_t linTexelAddr = offset + y*pgtPitch + x;

                    SPhysPgOffset physPgOffset = pgTab[linTexelAddr];
                    if (physPgOffset_valid(physPgOffset))
                    {
                        pgTab[linTexelAddr] = SPhysPgOffset::invalid_addr;

                        uint32_t addrs[2] { physPgOffset.addr&0xffffu, physPgOffset_mipTailAddr(physPgOffset).addr&0xffffu };
                        const uint32_t szs[2]{ 1u,1u };
                        core::address_allocator_traits<phys_pg_addr_alctr_t>::multi_free_addr(m_physPgAddrAlctr, physPgOffset_hasMipTailAddr(physPgOffset) ? 2u : 1u, addrs, szs);
                    }
                }
        }

        //free entries in page table
        ITexturePacker::free(_addr, _img, _subres);
    }

    STextureData offsetToTextureData(const page_tab_offset_t& _offset, const ICPUImage* _img)
    {
        STextureData texData;
        core::vector2df_SIMD scaleUnorm16(_img->getCreationParameters().extent.width, _img->getCreationParameters().extent.height);
        scaleUnorm16 /= core::vector2df_SIMD(m_pageTable->getCreationParameters().extent.width*m_pgSzxy);//taking just width into account because page table is always square anyway
        scaleUnorm16 *= core::vector2df_SIMD(0xffffu);

        texData.scale_x = scaleUnorm16.x;
        texData.scale_y = scaleUnorm16.y;

        core::vector2df_SIMD pgTabUnorm16(_offset.x, _offset.y);
		pgTabUnorm16 /= core::vector2df_SIMD(m_pageTable->getCreationParameters().extent.width,m_pageTable->getCreationParameters().extent.height, 1.f, 1.f);
		pgTabUnorm16 *= core::vector2df_SIMD(0xffffu);
		texData.pgTab_x = pgTabUnorm16.x;
		texData.pgTab_y = pgTabUnorm16.y;

        return texData;
    }
    page_tab_offset_t pack(const ICPUImage* _img, const ICPUImage::SSubresourceRange& _subres)
    {
        if (getFormatClass(_img->getCreationParameters().format)!=getFormatClass(m_physAddrTex->getCreationParameters().format))
            return page_tab_offset_invalid();

        const auto extent = _img->getCreationParameters().extent;

        if ((extent.width>>_subres.baseMipLevel) > maxAllocatableTextureSz() || (extent.height>>_subres.baseMipLevel) > maxAllocatableTextureSz())
            return page_tab_offset_invalid();

        const page_tab_offset_t pgtOffset = alloc(_img, _subres);
        if ((pgtOffset==page_tab_offset_invalid()).all())
            return pgtOffset;

        const uint32_t levelsTakingAtLeastOnePageCount = countLevelsTakingAtLeastOnePage(extent, _subres);
        const uint32_t levelsToPack = std::min(_subres.levelCount, m_pageTable->getCreationParameters().mipLevels+m_pgSzxy_log2);

        uint32_t miptailPgAddr = SPhysPgOffset::invalid_addr;

        const uint32_t texelSz = getTexelOrBlockBytesize(m_physAddrTex->getCreationParameters().format);
        //fill page table and pack present mips into physical addr texture
        for (uint32_t i = 0u; i < levelsToPack; ++i)
        {
            const uint32_t w = neededPageCountForSide(extent.width, _subres.baseMipLevel+i);
            const uint32_t h = neededPageCountForSide(extent.height, _subres.baseMipLevel+i);

            uint32_t* const pgTab = reinterpret_cast<uint32_t*>(
                reinterpret_cast<uint8_t*>(m_pageTable->getBuffer()->getPointer()) + m_pageTable->getRegions().begin()[i].bufferOffset
                );
            const uint32_t pgtPitch = m_pageTable->getRegions().begin()[i].bufferRowLength;
            const uint32_t pgtH = m_pageTable->getRegions().begin()[i].imageExtent.height;
            const uint32_t offset = (pgtOffset.y>>i)*pgtPitch + (pgtOffset.x>>i);

            for (uint32_t y = 0u; y < h; ++y)
                for (uint32_t x = 0u; x < w; ++x)
                {
                    uint32_t physPgAddr{};
                    if (i>=levelsTakingAtLeastOnePageCount)
                        physPgAddr = miptailPgAddr;
                    else
                    {
                        const uint32_t szAndAlignment = 1u;
                        core::address_allocator_traits<phys_pg_addr_alctr_t>::multi_alloc_addr(m_physPgAddrAlctr, 1u, &physPgAddr, &szAndAlignment, &szAndAlignment, nullptr);
                    }
                    //assert(physPgAddr<SPhysPgOffset::invalid_addr);
                    if (physPgAddr==phys_pg_addr_alctr_t::invalid_address)
                    {
                        free(pgtOffset, _img, _subres);
                        return pgtOffset;
                    }

                    if (i==(levelsTakingAtLeastOnePageCount-1u) && levelsTakingAtLeastOnePageCount<_subres.levelCount)
                    {
                        assert(w==1u && h==1u);
                        uint32_t physMiptailPgAddr{};
                        const uint32_t szAndAlignment = 1u;
                        core::address_allocator_traits<phys_pg_addr_alctr_t>::multi_alloc_addr(m_physPgAddrAlctr, 1u, &physMiptailPgAddr, &szAndAlignment, &szAndAlignment, nullptr);
                        assert(physMiptailPgAddr<SPhysPgOffset::invalid_addr);
                        miptailPgAddr = physMiptailPgAddr = (physMiptailPgAddr==phys_pg_addr_alctr_t::invalid_address) ? SPhysPgOffset::invalid_addr : physMiptailPgAddr;
                        physPgAddr |= (physMiptailPgAddr<<16);
                    }
                    else 
                        physPgAddr |= (SPhysPgOffset::invalid_addr<<16);
                    pgTab[offset + y*pgtPitch + x] = physPgAddr;

                    core::vector3du32_SIMD physPg = pageCoords(physPgAddr, m_pgSzxy);
                    for (const auto& reg : _img->getRegions())
                    {
                        if (reg.imageSubresource.mipLevel != (_subres.baseMipLevel+i))
                            continue;

                        auto src_txOffset = core::vector2du32_SIMD(x,y)*m_pgSzxy;

                        const uint32_t a_left = reg.imageOffset.x;
                        const uint32_t b_right = src_txOffset.x + m_pgSzxy;
                        const uint32_t a_right = a_left + reg.imageExtent.width;
                        const uint32_t b_left = src_txOffset.x;
                        const uint32_t a_bot = reg.imageOffset.y;
                        const uint32_t b_top = src_txOffset.y + m_pgSzxy;
                        const uint32_t a_top = a_bot + reg.imageExtent.height;
                        const uint32_t b_bot = src_txOffset.y;
                        if (a_left>b_right || a_right<b_left || a_top<b_bot || a_bot>b_top)
                            continue;

                        const core::vector2du32_SIMD regOffset = core::vector2du32_SIMD(a_left, a_bot);
                        const core::vector2du32_SIMD withinRegTxOffset = core::max(src_txOffset, regOffset)-regOffset;
                        const core::vector2du32_SIMD withinPgTxOffset = (withinRegTxOffset & core::vector2du32_SIMD(m_pgSzxy-1u));
                        src_txOffset += withinPgTxOffset;
                        const core::vector2du32_SIMD src_txOffsetEnd = core::min(core::vector2du32_SIMD(a_right,a_top), core::vector2du32_SIMD(b_right,b_top));
                        //special offset for packing tail mip levels into single page
                        const core::vector2du32_SIMD miptailOffset = (i>=levelsTakingAtLeastOnePageCount) ? core::vector2du32_SIMD(m_miptailOffsets[i-levelsTakingAtLeastOnePageCount].x,m_miptailOffsets[i-levelsTakingAtLeastOnePageCount].y)+core::vector2du32_SIMD(m_tilePadding,m_tilePadding) : core::vector2du32_SIMD(0u,0u);

                        physPg += (withinPgTxOffset + miptailOffset);

                        const core::vector2du32_SIMD cpExtent = src_txOffsetEnd - src_txOffset;
                        const uint32_t src_offset_lin = (withinRegTxOffset.y*reg.bufferRowLength + withinRegTxOffset.x) * texelSz;
                        const uint32_t dst_offset_lin = (m_physAddrTex->getCreationParameters().extent.width*m_physAddrTex->getCreationParameters().extent.height*physPg.z + physPg.y*m_physAddrTex->getCreationParameters().extent.width + physPg.x) * texelSz;
                        const uint8_t* src = reinterpret_cast<const uint8_t*>(_img->getBuffer()->getPointer()) + reg.bufferOffset + src_offset_lin;
                        uint8_t* dst = reinterpret_cast<uint8_t*>(m_physAddrTex->getBuffer()->getPointer()) + dst_offset_lin;
                        for (uint32_t j = 0u; j < cpExtent.y; ++j)
                        {
                            memcpy(dst + j*m_physAddrTex->getCreationParameters().extent.width*texelSz, src + j*reg.bufferRowLength*texelSz, cpExtent.x*texelSz);
                        }
                    }
                }
            //TODO fill borders according to wrapping mode, but i think i'll wait for your IImageManipulator @devsh
        }

        return pgtOffset;
    }

    ICPUImage* getPhysicalAddressTexture() { return m_physAddrTex.get(); }
    ICPUImage* getPageTable() { return m_pageTable.get(); }

private:
    uint32_t countLevelsTakingAtLeastOnePage(const VkExtent3D& _extent, const ICPUImage::SSubresourceRange& _subres)
    {
        const uint32_t baseMaxDim = core::roundUpToPoT(core::max(_extent.width, _extent.height))>>_subres.baseMipLevel;
        const int32_t lastFullMip = core::findMSB(baseMaxDim) - static_cast<int32_t>(m_pgSzxy_log2);

        return core::clamp(lastFullMip+1, 0, static_cast<int32_t>(m_pageTable->getCreationParameters().mipLevels));
    }

    core::smart_refctd_ptr<ICPUImage> m_physAddrTex;
    core::smart_refctd_ptr<ICPUImage> m_pageTable;
    const uint32_t m_tilePadding;

    using phys_pg_addr_alctr_t = core::PoolAddressAllocator<uint32_t>;
    core::smart_refctd_dynamic_array<uint8_t> m_physPgAddrAlctr_reservedSpc;
    phys_pg_addr_alctr_t m_physPgAddrAlctr;

    SMiptailPacker::rect m_miptailOffsets[MAX_PHYSICAL_PAGE_SIZE_LOG2];
};

bool ICPUTexturePacker::SMiptailPacker::computeMiptailOffsets(rect* res, int log2SIZE, int padding)
{
    if (log2SIZE<7 || log2SIZE>10)
        return false;
    
    int SIZE = 0x1u<<log2SIZE;
	if ((SIZE>>1)+(SIZE>>2) + padding * 4 > SIZE)
		return false;
	

	int x1 = 0, y1 = 0;
	res[0].x = 0;
	res[0].mx = (SIZE >> 1) + padding * 2 -1;
	res[0].y = 0;
	res[0].my = (SIZE >> 1) + padding * 2 -1;
	y1 = res[0].my + 1;
	x1 = res[0].mx + 1;

	bool ofx1 = false, ofx2 = false;
	int i = 1;
	while (i < log2SIZE)
	{

		int s = (SIZE >> (i + 1)) + padding * 2;
		int x, y;
		if (i % 2 == 0) {	//on right
			if (i == 2)
			{
				x = x1;
				y = 0;
			}
			else
			{
				if (res[i - 2].my + s > y1)
				{
					if (res[i - 6].mx + s > SIZE || ofx1)
					{
						x = res[i - 4].mx + 1;
						y = res[i - 4].y;
						ofx1 = true;
					}
					else {

					x = res[i - 6].mx + 1;
					y = res[i - 6].y;
					}
				}
				else
				{
					y = res[i - 2].my + 1;
					x = x1;
				}
			}
		}
		else //those placed below the first square and going right are ~x2 larger than those above
		{
			if (i == 1)
			{
				x = 0;
				y = y1;
			}
			else {
				if (res[i - 2].mx + s > SIZE)
				{
					if (res[i - 6].my + s > SIZE ||ofx2)
					{
						x = res[i - 4].x;
						y = res[i - 4].my + 1;
						ofx2 = true;
					}
					else {
						x = res[i - 6].x;
						y = res[i - 6].my + 1;
					}
				}
				else 
				{
				    x = res[i - 2].mx + 1;
				    y = y1;
				}
			}
		}
		res[i].x = x;
		res[i].y = y;
		res[i].mx = x + s -1;
		res[i].my = y + s -1 ;

		i++;
	}
    return true;
}

}}

#endif