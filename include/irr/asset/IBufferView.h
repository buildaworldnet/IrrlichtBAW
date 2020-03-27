#ifndef __IRR_I_BUFFER_VIEW_H_INCLUDED__
#define __IRR_I_BUFFER_VIEW_H_INCLUDED__

#include "irr/macros.h"

#include "irr/asset/format/EFormat.h"
#include "irr/asset/IDescriptor.h"

namespace irr
{
namespace asset
{

template<typename BufferType>
class IBufferView : public IDescriptor
{
	public:
		_IRR_STATIC_INLINE_CONSTEXPR size_t whole_buffer = ~static_cast<size_t>(0u);

	protected:
		IBufferView(core::smart_refctd_ptr<BufferType>&& _buffer, E_FORMAT _format, size_t _offset, size_t _size) :
			m_buffer(std::move(_buffer)), m_format(_format), m_offset(_offset), m_size(_size)
		{
			if (m_size == whole_buffer)
				m_size = m_buffer->getSize() - m_offset;
		}
		virtual ~IBufferView() = default;

		core::smart_refctd_ptr<BufferType> m_buffer;
		E_FORMAT m_format;
		size_t m_offset;
		size_t m_size;

	public:
		E_CATEGORY getTypeCategory() const override { return EC_BUFFER_VIEW; }

		const BufferType* getUnderlyingBuffer() const { return m_buffer.get(); }

		E_FORMAT getFormat() const { return m_format; }
		size_t getOffsetInBuffer() const { return m_offset; }
		size_t getByteSize() const { return m_size; }
};

}
}

#endif