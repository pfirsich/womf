#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

class BufferBase {
public:
    using Ptr = std::shared_ptr<BufferBase>;

    virtual ~BufferBase() = default;
    virtual std::span<const uint8_t> data() const = 0;
    virtual size_t size() const = 0;
    virtual std::string path() const = 0;
    virtual std::string name() const = 0;
};

class Buffer final
    : public BufferBase
    , public std::enable_shared_from_this<Buffer> {
public:
    using Ptr = std::shared_ptr<Buffer>;

    template <typename... Args>
    [[nodiscard]] static Ptr create(Args&&... args)
    {
        return std::shared_ptr<Buffer>(new Buffer(std::forward<Args>(args)...));
    }

    std::span<const uint8_t> data() const override;

    size_t size() const override;

    std::string path() const override;
    std::string name() const override;

private:
    Buffer(std::string filename);

    std::vector<uint8_t> data_;
    std::string filename_;
};

class BufferView final
    : public BufferBase
    , public std::enable_shared_from_this<BufferView> {
public:
    using Ptr = std::shared_ptr<BufferView>;

    [[nodiscard]] static Ptr create(Buffer::Ptr buffer, size_t offset, size_t size);
    [[nodiscard]] static Ptr create(BufferView::Ptr buffer, size_t offset, size_t size);

    std::span<const uint8_t> data() const override;

    size_t size() const override;

    std::string path() const override;

    std::string name() const override;

private:
    BufferView(BufferBase::Ptr buffer, size_t offset = 0, size_t size = -1);

    BufferBase::Ptr buffer_;
    size_t offset_;
    size_t size_;
};
