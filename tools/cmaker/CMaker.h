#pragma once

#include <memory>
#include <string>
#include <vector>

namespace ga {

class CMaker {
  public:
    CMaker();
    ~CMaker();

    int exec(const std::vector<std::string> &args, const std::string &pwd = "");

  private:
    struct Impl;
    std::shared_ptr<Impl> _impl;
};

} // namespace ga
