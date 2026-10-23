#include <ark/storage/object_format.h>

#include <ark/core/exceptions.h>
#include <charconv>
#include <system_error>

namespace ark {

std::string frameObject(ObjectType type, std::string_view body) {
  std::string framed(toString(type));
  framed += ' ';
  framed += std::to_string(body.size());
  framed.push_back('\0');
  framed.append(body.data(), body.size());
  return framed;
}

RawObject parseObjectFrame(const ObjectHash &objectHash,
                           std::string_view data) {
  const std::string &hash = objectHash.hex();
  size_t nul = data.find('\0');
  if (nul == std::string_view::npos)
    throw ObjectFormatException(hash, "missing header");

  std::string_view header = data.substr(0, nul);
  std::string_view body = data.substr(nul + 1);

  size_t space = header.find(' ');
  if (space == std::string_view::npos)
    throw ObjectFormatException(hash, "malformed header");

  std::string typeName(header.substr(0, space));
  std::optional<ObjectType> type = parseObjectType(typeName);
  if (!type)
    throw ObjectFormatException(hash, "unsupported type " + typeName);

  const char *first = header.data() + space + 1;
  const char *last = header.data() + header.size();
  size_t declared = 0;
  auto [ptr, ec] = std::from_chars(first, last, declared);
  if (ec != std::errc() || ptr != last)
    throw ObjectFormatException(hash, "invalid size in header");

  if (declared != body.size())
    throw ObjectFormatException(hash, "size mismatch");

  return RawObject{*type, std::string(body)};
}

} // namespace ark
