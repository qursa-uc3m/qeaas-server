#include "RandomSource.h"

#include <Quantis.h>
#include <QuantisExtractor.h>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unistd.h>
#include <utility>

namespace
{
std::unique_ptr<RandomSource> configuredSource;

std::string systemError(const std::string &operation)
{
    return operation + ": " + std::strerror(errno);
}
} // namespace

std::string sourceName(QrngSource source)
{
    return source == QrngSource::Usb ? "usb" : "pcie";
}

std::string pcieOutputName(PcieOutput output)
{
    return output == PcieOutput::Extracted ? "extracted" : "raw";
}

RandomSource::RandomSource(RandomSourceOptions options) : options_(std::move(options))
{
#ifndef QRNG_ENABLE_OS_XOR
    if (options_.xorOs)
    {
        throw std::runtime_error(
            "OS XOR was disabled at build time; rebuild with -DENABLE_OS_XOR=ON");
    }
#endif

    if (options_.source == QrngSource::Pcie && options_.pcieOutput == PcieOutput::Extracted &&
        options_.extraction)
    {
        throw std::runtime_error(
            "PCIe extracted mode already uses the card's hardware post-processing; "
            "use --extract off, or select PCIe raw/sample mode first");
    }

    if (!options_.extraction)
    {
        return;
    }

    const auto status =
        QuantisExtractorInitializeMatrix(options_.matrixPath.c_str(), &extractorMatrix_, 1024, 768);
    if (status != QUANTIS_SUCCESS)
    {
        QuantisExtractorUninitializeMatrix(&extractorMatrix_);
        extractorMatrix_ = nullptr;
        throw std::runtime_error("Cannot initialize Quantis extraction matrix '" +
                                 options_.matrixPath + "': " + std::to_string(status));
    }
}

RandomSource::~RandomSource()
{
    if (extractorMatrix_ != nullptr)
    {
        QuantisExtractorUninitializeMatrix(&extractorMatrix_);
    }
}

const RandomSourceOptions &RandomSource::options() const
{
    return options_;
}

bool RandomSource::readFile(const std::string &path, std::size_t size,
                            std::vector<std::uint8_t> &output, std::string &error)
{
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0)
    {
        error = systemError("Cannot open " + path);
        return false;
    }

    output.resize(size);
    std::size_t offset = 0;
    while (offset < size)
    {
        const ssize_t count = ::read(fd, output.data() + offset, size - offset);
        if (count > 0)
        {
            offset += static_cast<std::size_t>(count);
            continue;
        }
        if (count < 0 && errno == EINTR)
        {
            continue;
        }

        error = count == 0 ? "Unexpected end of file while reading " + path
                           : systemError("Cannot read " + path);
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

bool RandomSource::readRaw(std::size_t size, std::vector<std::uint8_t> &output, std::string &error)
{
    if (options_.source == QrngSource::Pcie)
    {
        return readFile(options_.qrandomPath, size, output, error);
    }

    if (size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        error = "USB request is too large";
        return false;
    }

    output.resize(size);
    const int count = QuantisRead(QUANTIS_DEVICE_USB, options_.deviceNumber, output.data(), size);
    if (count < 0)
    {
        error = std::string("Quantis USB read failed: ") +
                QuantisStrError(static_cast<QuantisError>(count));
        return false;
    }
    if (static_cast<std::size_t>(count) != size)
    {
        error = "Quantis USB returned " + std::to_string(count) + " bytes; expected " +
                std::to_string(size);
        return false;
    }
    return true;
}

bool RandomSource::readPrimary(std::size_t size, std::vector<std::uint8_t> &output,
                               std::string &error)
{
    if (!options_.extraction)
    {
        return readRaw(size, output, error);
    }

    if (size > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        error = "Extraction request is too large";
        return false;
    }

    std::uint32_t outputSize = 0;
    std::uint32_t inputSize = 0;
    const int status = QuantisExtractorComputeBufferSize(static_cast<std::uint32_t>(size),
                                                         &outputSize, &inputSize);
    if (status != QUANTIS_SUCCESS)
    {
        error = "Cannot compute Quantis extraction buffer sizes: " + std::to_string(status);
        return false;
    }

    std::vector<std::uint8_t> input;
    if (!readRaw(inputSize, input, error))
    {
        return false;
    }

    std::vector<std::uint8_t> extracted(outputSize);
    QuantisExtractorGetDataFromBuffer(input.data(), extracted.data(), extractorMatrix_, outputSize);
    output.assign(extracted.begin(), extracted.begin() + static_cast<std::ptrdiff_t>(size));
    return true;
}

bool RandomSource::read(std::size_t size, RandomReadResult &result, std::string &error)
{
    std::lock_guard<std::mutex> lock(mutex_);

    result = RandomReadResult{};
    if (!readPrimary(size, result.bytes, error))
    {
        if (!options_.fallback)
        {
            return false;
        }

        const std::string primaryError = error;
        if (!readFile("/dev/urandom", size, result.bytes, error))
        {
            error = primaryError + "; fallback also failed: " + error;
            return false;
        }

        result.source = "/dev/urandom";
        result.extractionMode = "none";
        result.warning = primaryError;
        result.fallbackUsed = true;
        return true;
    }

    result.source =
        options_.source == QrngSource::Usb ? "Quantis USB library" : options_.qrandomPath;
    result.extractionApplied = options_.extraction;
    if (options_.extraction)
    {
        result.extractionMode = "software-matrix";
    }
    else if (options_.source == QrngSource::Pcie && options_.pcieOutput == PcieOutput::Extracted)
    {
        result.extractionMode = "pcie-hardware";
    }
    else if (options_.source == QrngSource::Pcie)
    {
        result.warning = "PCIe raw/sample output is being returned without randomness extraction";
    }

    if (options_.xorOs)
    {
#ifdef QRNG_ENABLE_OS_XOR
        std::vector<std::uint8_t> osBytes;
        if (!readFile("/dev/urandom", size, osBytes, error))
        {
            return false;
        }
        for (std::size_t i = 0; i < size; ++i)
        {
            result.bytes[i] ^= osBytes[i];
        }
        result.xorOsApplied = true;
#endif
    }

    return true;
}

void initializeRandomSource(RandomSourceOptions options)
{
    configuredSource = std::make_unique<RandomSource>(std::move(options));
}

RandomSource &randomSource()
{
    if (!configuredSource)
    {
        throw std::logic_error("Random source was not initialized");
    }
    return *configuredSource;
}
