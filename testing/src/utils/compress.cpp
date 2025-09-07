#include <compress.h>
#include <zlib.h>
#include <iostream>

std::string compressObject(const std::string& input) {
    if (input.empty()) return std::string();
    uLongf destinationCapacity = compressBound(static_cast<uLong>(input.size()));
    std::string compressed;
    compressed.resize(destinationCapacity);
    int result = compress2(
        reinterpret_cast<Bytef*>(&compressed[0]),
        &destinationCapacity,
        reinterpret_cast<const Bytef*>(input.data()),
        static_cast<uLong>(input.size()),
        Z_BEST_COMPRESSION
    );
    if (result != Z_OK) {
        std::cerr << "zlib compress2 failed with code: " << result << std::endl;
        return std::string();
    }
    compressed.resize(destinationCapacity);
    return compressed;
}

std::string decompressObject(const std::string& input) {
    if (input.empty()) return std::string();

    // Start with a buffer that's 4x the input size and grow as needed
    uLongf destinationCapacity = static_cast<uLongf>(input.size() * 4 + 64);
    std::string decompressed;
    int result = Z_OK;

    do {
        decompressed.resize(destinationCapacity);
        uLongf outSize = destinationCapacity;
        result = uncompress(
            reinterpret_cast<Bytef*>(&decompressed[0]),
            &outSize,
            reinterpret_cast<const Bytef*>(input.data()),
            static_cast<uLong>(input.size())
        );

        if (result == Z_BUF_ERROR) {
            destinationCapacity *= 2; // grow and retry
            continue;
        }

        if (result != Z_OK) {
            std::cerr << "zlib uncompress failed with code: " << result << std::endl;
            return std::string();
        }

        decompressed.resize(outSize);
        break;
    } while (true);

    return decompressed;
}

