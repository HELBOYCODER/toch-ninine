#include "core/State.h"

#include <cstring>
#include <iterator>

namespace sf {

namespace {

constexpr std::uint8_t kMagicBytes[4] = {'S', 'F', 'S', 'T'};
constexpr std::uint32_t kMaxShaderBytes = 1u << 20; // 1 MiB of GLSL is far beyond any real shader
constexpr std::uint32_t kMaxParams = 4096;
constexpr std::uint32_t kMaxBlobBytes = 64u << 20; // 64 MiB

void putU32(std::vector<std::uint8_t> &out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
    }
}

void putF32(std::vector<std::uint8_t> &out, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    putU32(out, bits);
}

void putBlob(std::vector<std::uint8_t> &out, const void *data, std::size_t size) {
    putU32(out, static_cast<std::uint32_t>(size));
    const auto *bytes = static_cast<const std::uint8_t *>(data);
    out.insert(out.end(), bytes, bytes + size);
}

// Bounds-checked cursor over the input buffer.
class Reader {
public:
    Reader(const std::uint8_t *data, std::size_t size) : data_(data), size_(size) {}

    bool readU32(std::uint32_t &value) {
        if (size_ - pos_ < 4) {
            return false;
        }
        value = 0;
        for (int i = 0; i < 4; ++i) {
            value |= static_cast<std::uint32_t>(data_[pos_ + i]) << (8 * i);
        }
        pos_ += 4;
        return true;
    }

    bool readF32(float &value) {
        std::uint32_t bits = 0;
        if (!readU32(bits)) {
            return false;
        }
        std::memcpy(&value, &bits, sizeof(value));
        return true;
    }

    bool readBytes(std::size_t count, const std::uint8_t *&out) {
        if (size_ - pos_ < count) {
            return false;
        }
        out = data_ + pos_;
        pos_ += count;
        return true;
    }

    bool readBlob(std::uint32_t maxBytes, const std::uint8_t *&out, std::uint32_t &length) {
        if (!readU32(length) || length > maxBytes) {
            return false;
        }
        return readBytes(length, out);
    }

private:
    const std::uint8_t *data_;
    std::size_t size_;
    std::size_t pos_ = 0;
};

StateLoadResult fail(const std::string &why) {
    StateLoadResult result;
    result.error = why;
    return result;
}

} // namespace

std::vector<std::uint8_t> serializeState(const SessionState &state) {
    std::vector<std::uint8_t> out;
    out.insert(out.end(), std::begin(kMagicBytes), std::end(kMagicBytes));
    putU32(out, kStateVersion);
    putBlob(out, state.shaderSource.data(), state.shaderSource.size());
    putU32(out, static_cast<std::uint32_t>(state.paramValues.size()));
    for (const auto &[id, value] : state.paramValues) {
        putU32(out, id);
        putF32(out, value);
    }
    putBlob(out, state.fxChain.data(), state.fxChain.size());
    putBlob(out, state.uiBlob.data(), state.uiBlob.size());
    return out;
}

StateLoadResult deserializeState(const std::uint8_t *data, std::size_t size, SessionState &out) {
    if (data == nullptr || size < 4 + 4) {
        return fail("truncated header");
    }
    if (std::memcmp(data, kMagicBytes, 4) != 0) {
        return fail("bad magic");
    }

    Reader reader(data + 4, size - 4);
    std::uint32_t version = 0;
    if (!reader.readU32(version) || version == 0) {
        return fail("invalid version");
    }

    SessionState parsed;
    std::uint32_t shaderLen = 0;
    const std::uint8_t *shaderBytes = nullptr;
    if (!reader.readBlob(kMaxShaderBytes, shaderBytes, shaderLen)) {
        return fail("bad shader block");
    }
    parsed.shaderSource.assign(reinterpret_cast<const char *>(shaderBytes), shaderLen);

    std::uint32_t paramCount = 0;
    if (!reader.readU32(paramCount) || paramCount > kMaxParams) {
        return fail("bad parameter count");
    }
    for (std::uint32_t i = 0; i < paramCount; ++i) {
        std::uint32_t id = 0;
        float value = 0.0f;
        if (!reader.readU32(id) || !reader.readF32(value)) {
            return fail("truncated parameter table");
        }
        parsed.paramValues[id] = value;
    }

    std::uint32_t fxLen = 0;
    const std::uint8_t *fxBytes = nullptr;
    if (!reader.readBlob(kMaxBlobBytes, fxBytes, fxLen)) {
        return fail("bad FX chain block");
    }
    parsed.fxChain.assign(fxBytes, fxBytes + fxLen);

    std::uint32_t uiLen = 0;
    const std::uint8_t *uiBytes = nullptr;
    if (!reader.readBlob(kMaxBlobBytes, uiBytes, uiLen)) {
        return fail("bad UI block");
    }
    parsed.uiBlob.assign(uiBytes, uiBytes + uiLen);

    // Anything after uiLen is ignored, which is how newer versions can append data.
    out = std::move(parsed);
    StateLoadResult result;
    result.ok = true;
    result.version = version;
    return result;
}

} // namespace sf
