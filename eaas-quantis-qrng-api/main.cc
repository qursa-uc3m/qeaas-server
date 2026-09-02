#include "RandomSource.h"

#include <drogon/drogon.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
struct AppOptions
{
    RandomSourceOptions random;
    unsigned short port = 6065;
};

bool parseOnOff(const std::string &value, const std::string &option)
{
    if (value == "on")
    {
        return true;
    }
    if (value == "off")
    {
        return false;
    }
    throw std::runtime_error(option + " expects 'on' or 'off'");
}

void printUsage(const char *program)
{
    std::cout
        << "Usage: " << program << " [options]\n\n"
        << "  --source usb|pcie|os    Primary randomness source (default: pcie)\n"
        << "  --pcie-output MODE      extracted|raw; must match host driver (default: extracted)\n"
        << "  --device-number N       Quantis USB index (default: 0)\n"
        << "  --qrandom PATH          PCIe device path (default: /dev/qrandom0)\n"
        << "  --extract on|off        Matrix extraction (default: off)\n"
        << "  --matrix PATH           Extraction matrix file\n"
        << "  --xor-os on|off         XOR with Linux getrandom() (default: off)\n"
        << "  --fallback on|off       Fall back to Linux getrandom() (default: on)\n"
        << "  --port N                HTTP port (default: 6065)\n"
        << "  --help                  Show this help\n";
}

AppOptions parseArguments(int argc, char **argv)
{
    AppOptions options;

    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--help")
        {
            printUsage(argv[0]);
            std::exit(0);
        }
        if (i + 1 >= argc)
        {
            throw std::runtime_error("Missing value for " + argument);
        }

        const std::string value = argv[++i];
        if (argument == "--source")
        {
            if (value == "usb")
            {
                options.random.source = QrngSource::Usb;
            }
            else if (value == "pcie")
            {
                options.random.source = QrngSource::Pcie;
            }
            else if (value == "os")
            {
                options.random.source = QrngSource::Os;
            }
            else
            {
                throw std::runtime_error("--source expects 'usb', 'pcie', or 'os'");
            }
        }
        else if (argument == "--pcie-output")
        {
            if (value == "extracted")
            {
                options.random.pcieOutput = PcieOutput::Extracted;
            }
            else if (value == "raw")
            {
                options.random.pcieOutput = PcieOutput::Raw;
            }
            else
            {
                throw std::runtime_error("--pcie-output expects 'extracted' or 'raw'");
            }
        }
        else if (argument == "--device-number")
        {
            options.random.deviceNumber = static_cast<unsigned int>(std::stoul(value));
        }
        else if (argument == "--qrandom")
        {
            options.random.qrandomPath = value;
        }
        else if (argument == "--extract")
        {
            options.random.extraction = parseOnOff(value, argument);
        }
        else if (argument == "--matrix")
        {
            options.random.matrixPath = value;
        }
        else if (argument == "--xor-os")
        {
            options.random.xorOs = parseOnOff(value, argument);
        }
        else if (argument == "--fallback")
        {
            options.random.fallback = parseOnOff(value, argument);
        }
        else if (argument == "--port")
        {
            const auto port = std::stoul(value);
            if (port == 0 || port > 65535)
            {
                throw std::runtime_error("--port must be between 1 and 65535");
            }
            options.port = static_cast<unsigned short>(port);
        }
        else
        {
            throw std::runtime_error("Unknown option: " + argument);
        }
    }

    return options;
}
} // namespace

int main(int argc, char **argv)
{
    try
    {
        const auto options = parseArguments(argc, argv);
        initializeRandomSource(options.random);

        std::cout << "random source=" << sourceName(options.random.source);
        if (options.random.source == QrngSource::Pcie)
        {
            std::cout << " pcie_output=" << pcieOutputName(options.random.pcieOutput);
        }
        std::cout << " extraction=" << (options.random.extraction ? "on" : "off")
                  << " xor_os=" << (options.random.xorOs ? "on" : "off")
                  << " fallback=" << (options.random.fallback ? "on" : "off") << '\n';

        drogon::app().addListener("0.0.0.0", options.port).run();
    }
    catch (const std::exception &exception)
    {
        std::cerr << "error: " << exception.what() << '\n';
        return 1;
    }
    return 0;
}
