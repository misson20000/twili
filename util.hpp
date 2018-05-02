#include<optional>
#include<vector>

namespace twili {
namespace util {

std::optional<std::vector<uint8_t>> ReadFile(const char *path);

}
}
