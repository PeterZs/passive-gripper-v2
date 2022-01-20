#include <vector>
#include <cstddef>

namespace psg {
namespace core {

class UnionFind {
 public:
  UnionFind(size_t size);
  void merge(size_t a, size_t b);
  size_t find(size_t a);
 private:
  std::vector<size_t> parent;
};

}  // namespace core
}  // namespace psg
