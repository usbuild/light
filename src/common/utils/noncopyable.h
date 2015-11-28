#pragma once
namespace light {
namespace utils {
class NonCopyable {
protected:
  NonCopyable() = default;
  ~NonCopyable() = default;

private:
  NonCopyable(const NonCopyable &);
  NonCopyable &operator=(const NonCopyable &);
};
}
}
