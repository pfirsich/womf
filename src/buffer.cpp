#include "buffer.hpp"

#include "die.hpp"
#include "util.hpp"

std::span<const uint8_t> Buffer::data() const
{
    return std::span<const uint8_t>(data_);
}

size_t Buffer::size() const
{
    return data_.size();
}

std::string Buffer::path() const
{
    return filename_;
}
std::string Buffer::name() const
{
    return filename_;
}

Buffer::Buffer(std::string filename)
    : filename_(std::move(filename))
    , data_(readFile<std::vector<uint8_t>>(filename_))
{
}

BufferView::Ptr BufferView::create(Buffer::Ptr buffer, size_t offset, size_t size)
{
    return std::shared_ptr<BufferView>(
        new BufferView(std::static_pointer_cast<BufferBase>(std::move(buffer)), offset, size));
}

BufferView::Ptr BufferView::create(BufferView::Ptr buffer, size_t offset, size_t size)
{
    return std::shared_ptr<BufferView>(
        new BufferView(std::static_pointer_cast<BufferBase>(std::move(buffer)), offset, size));
}

std::span<const uint8_t> BufferView::data() const
{
    return buffer_->data().subspan(offset_, size_);
}

size_t BufferView::size() const
{
    return size_;
}

std::string BufferView::path() const
{
    return buffer_->path();
}

std::string BufferView::name() const
{
    return buffer_->name() + fmt::format("[{}:{}]", offset_, size_);
}

BufferView::BufferView(BufferBase::Ptr buffer, size_t offset, size_t size)
    : buffer_(std::move(buffer))
    , offset_(offset)
    , size_(std::min(size, buffer_->size()))
{
}
