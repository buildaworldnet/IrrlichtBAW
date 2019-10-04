#ifndef __IRR_I_PIPELINE_LAYOUT_H_INCLUDED__
#define __IRR_I_PIPELINE_LAYOUT_H_INCLUDED__

#include "irr/core/IReferenceCounted.h"
#include "irr/macros.h"
#include "irr/asset/ShaderCommons.h"
#include "irr/core/memory/refctd_dynamic_array.h"
#include "irr/core/SRange.h"
#include <algorithm>
#include <array>

namespace irr {
namespace asset
{

struct SPushConstantRange
{
    E_SHADER_STAGE stageFlags;
    uint32_t offset;
    uint32_t size;
};

template<typename DescLayoutType>
class IPipelineLayout
{
public:
    _IRR_STATIC_INLINE_CONSTEXPR size_t DESCRIPTOR_SET_COUNT = 4u;

    const DescLayoutType* getDescriptorSetLayout(uint32_t _set) const { return m_descSetLayouts[_set].get(); }
    core::SRange<const SPushConstantRange> getPushConstantRanges() const { return {m_pushConstantRanges->data(), m_pushConstantRanges->data()+m_pushConstantRanges->size()}; }

protected:
    virtual ~IPipelineLayout() = default;

    IPipelineLayout(
        const SPushConstantRange* const _pcRangesBegin = nullptr, const SPushConstantRange* const _pcRangesEnd = nullptr,
        core::smart_refctd_ptr<DescLayoutType>&& _layout0 = nullptr, core::smart_refctd_ptr<DescLayoutType>&& _layout1 = nullptr,
        core::smart_refctd_ptr<DescLayoutType>&& _layout2 = nullptr, core::smart_refctd_ptr<DescLayoutType>&& _layout3 = nullptr
    ) : m_descSetLayouts{{std::move(_layout0), std::move(_layout1), std::move(_layout2), std::move(_layout3)}},
        m_pushConstantRanges(_pcRangesBegin==_pcRangesEnd ? nullptr : core::make_refctd_dynamic_array<core::smart_refctd_dynamic_array<SPushConstantRange>>(_pcRangesEnd-_pcRangesBegin))
    {
        std::copy(_pcRangesBegin, _pcRangesEnd, m_pushConstantRanges->begin());
    }


    std::array<core::smart_refctd_ptr<DescLayoutType>, DESCRIPTOR_SET_COUNT> m_descSetLayouts;
    core::smart_refctd_dynamic_array<SPushConstantRange> m_pushConstantRanges;
};

}
}

#endif