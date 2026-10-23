#include "support/ark_test.h"

#include <ark/storage/compression.h>

#include <random>

using namespace arktest;

namespace {

std::string randomBytes(size_t n, uint32_t seed) {
  std::mt19937 rng(seed);
  std::string s(n, '\0');
  for (auto &c : s) {
    c = static_cast<char>(rng() & 0xff);
  }
  return s;
}

} // namespace

class CompressRoundTrip : public ::testing::TestWithParam<std::string> {};

TEST_P(CompressRoundTrip, DecompressRestoresInputExactly) {
  const std::string &input = GetParam();
  const std::string packed = compressObject(input);
  ASSERT_FALSE(packed.empty());
  EXPECT_EQ(decompressObject(packed), input);
}

TEST_P(CompressRoundTrip, OutputIsAStandardZlibStream) {
  // Objects must be readable by any zlib implementation, not only by Ark.
  const std::string &input = GetParam();
  EXPECT_EQ(inflate(compressObject(input)), input);
}

INSTANTIATE_TEST_SUITE_P(
    Inputs, CompressRoundTrip,
    ::testing::Values(std::string("a"), std::string("hello world\n"),
                      std::string("blob 3\0abc", 10),
                      std::string(100000, 'x'),         // highly compressible
                      randomBytes(1 << 20, 7),          // incompressible, 1 MiB
                      std::string("\0\0\0\xff\xfe", 5)) // binary
);

TEST(Compress, EmptyInputGivesEmptyOutput) {
  EXPECT_EQ(compressObject(""), "");
  EXPECT_EQ(decompressObject(""), "");
}

TEST(Compress, DecompressHandlesOutputManyTimesLargerThanInput) {
  // 10 MB of zeros compresses to ~10 KB, so the buffer has to grow a lot.
  const std::string big(10 * 1024 * 1024, '\0');
  const std::string packed = compressObject(big);
  ASSERT_LT(packed.size(), big.size() / 100);
  EXPECT_EQ(decompressObject(packed), big);
}

TEST(Compress, CorruptInputReturnsEmptyInsteadOfGarbage) {
  std::string packed = compressObject("some content that will be damaged");
  packed[packed.size() / 2] ^= 0x5a;
  EXPECT_EQ(decompressObject(packed), "");
  EXPECT_EQ(decompressObject("definitely not zlib"), "");
}

TEST(Compress, TruncatedInputReturnsEmpty) {
  const std::string packed = compressObject(randomBytes(4096, 3));
  EXPECT_EQ(decompressObject(packed.substr(0, packed.size() / 2)), "");
}
