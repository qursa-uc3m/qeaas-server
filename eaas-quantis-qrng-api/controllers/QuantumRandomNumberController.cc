#include "QuantumRandomNumberController.h"
#include "RandomSource.h"

#include <cstring>

void QuantumRandomNumberController::generateRandomNumber(
    const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback,
    int num_bytes)
{
    Json::Value result;
    if (num_bytes <= 0 || num_bytes > 8)
    {
        result["return"] = 1;
        result["error_message"] = "Invalid num_bytes. Allowed range: 1 to 8.";
    }
    else
    {
        RandomReadResult random;
        std::string error;
        if (randomSource().read(static_cast<std::size_t>(num_bytes), random, error))
        {
            std::uint64_t randomNumber = 0;
            std::memcpy(&randomNumber, random.bytes.data(), random.bytes.size());

            result["return"] = 0;
            result["random_number"] = Json::Value::UInt64(randomNumber);
            result["source"] = random.source;
            result["extraction"] = random.extractionApplied;
            result["extraction_mode"] = random.extractionMode;
            if (randomSource().options().source == QrngSource::Pcie)
            {
                result["pcie_output"] = pcieOutputName(randomSource().options().pcieOutput);
            }
            result["xor_os"] = random.xorOsApplied;
            result["fallback"] = random.fallbackUsed;
            if (!random.warning.empty())
            {
                result["warning"] = random.warning;
            }
        }
        else
        {
            result["return"] = 1;
            result["error_message"] = error;
        }
    }

    auto resp = HttpResponse::newHttpJsonResponse(result);
    if (result["return"].asInt() != 0)
    {
        resp->setStatusCode(k503ServiceUnavailable);
    }
    callback(resp);
}
