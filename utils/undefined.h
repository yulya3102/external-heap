#pragma once

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define __AT__ __FILE__ ":" TOSTRING(__LINE__)

#define undefined throw std::runtime_error(__AT__ ": not implemented")
#define undefined_expr(T) []() -> T { undefined; }()
