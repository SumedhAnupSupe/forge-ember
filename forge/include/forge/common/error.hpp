#ifndef FORGE_COMMON_ERROR_HPP
#define FORGE_COMMON_ERROR_HPP
#include <string>
namespace forge { namespace common { struct Error { bool ok = true; std::string message; }; } }
#endif
