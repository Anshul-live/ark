#include <ark/storage/sha256.h>

#include <ark/core/object_hash.h>
#include <openssl/evp.h>
#include <stdexcept>

namespace ark {

std::array<unsigned char, 32> sha256Digest(std::string_view data) {
  std::array<unsigned char, 32> digest{};
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    throw std::runtime_error("sha256: failed to allocate digest context");
  unsigned int length = 0;
  bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
            EVP_DigestUpdate(ctx, data.data(), data.size()) == 1 &&
            EVP_DigestFinal_ex(ctx, digest.data(), &length) == 1;
  EVP_MD_CTX_free(ctx);
  if (!ok)
    throw std::runtime_error("sha256: digest computation failed");
  return digest;
}

std::string sha256Hex(std::string_view data) {
  return ObjectHash::fromDigest(sha256Digest(data)).hex();
}

} // namespace ark
