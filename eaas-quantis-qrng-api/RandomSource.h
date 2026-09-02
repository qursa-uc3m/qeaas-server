#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

enum class QrngSource
{
    Usb,
    Pcie
};

struct RandomSourceOptions
{
    QrngSource source = QrngSource::Pcie;
    unsigned int deviceNumber = 0;
    std::string qrandomPath = "/dev/qrandom0";
    bool extraction = false;
    bool xorOs = false;
    bool fallback = true;
    std::string matrixPath = "/opt/quantis/share/quantis/default_idq_matrix.dat";
};

struct RandomReadResult
{
    std::vector<std::uint8_t> bytes;
    std::string source;
    std::string warning;
    bool extractionApplied = false;
    bool xorOsApplied = false;
    bool fallbackUsed = false;
};

class RandomSource
{
  public:
    explicit RandomSource(RandomSourceOptions options);
    ~RandomSource();

    RandomSource(const RandomSource &) = delete;
    RandomSource &operator=(const RandomSource &) = delete;

    bool read(std::size_t size, RandomReadResult &result, std::string &error);
    const RandomSourceOptions &options() const;

  private:
    bool readPrimary(std::size_t size, std::vector<std::uint8_t> &output, std::string &error);
    bool readRaw(std::size_t size, std::vector<std::uint8_t> &output, std::string &error);
    bool readFile(const std::string &path,
                  std::size_t size,
                  std::vector<std::uint8_t> &output,
                  std::string &error);

    RandomSourceOptions options_;
    std::uint64_t *extractorMatrix_ = nullptr;
    std::mutex mutex_;
};

void initializeRandomSource(RandomSourceOptions options);
RandomSource &randomSource();
std::string sourceName(QrngSource source);
