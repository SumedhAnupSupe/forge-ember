#ifndef FORGE_COMMON_RESULT_HPP
#define FORGE_COMMON_RESULT_HPP
#include "forge/common/error.hpp"
namespace forge { namespace common { template<typename T> struct Result { T value; Error error; bool success() const { return error.ok; } }; } }
#endif
