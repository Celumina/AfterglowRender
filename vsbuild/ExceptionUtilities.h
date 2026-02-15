
#include <stdexcept>
#include "DebugUtilities.h"

#define EXCEPT_RUNTIME(str) \
	DEBUG_FATAL(str); \
	throw std::runtime_error(str);

#define EXCEPT_CLASS_RUNTIME(str) \
	DEBUG_CLASS_FATAL(str);\
	throw std::runtime_error(str);

#define EXCEPT_TYPE_RUNTIME(type, str) \
	DEBUG_TYPE_FATAL(type, str);\
	throw std::runtime_error(str);

#define EXCEPT_INVALID_ARG(str) \
	DEBUG_FATAL(str); \
	throw std::invalid_argument(str);

#define EXCEPT_TYPE_INVALID_ARG(type, str) \
	DEBUG_TYPE_FATAL(type, str);\
	throw std::invalid_argument(str);

#define EXCEPT_CLASS_INVALID_ARG(str) \
	DEBUG_CLASS_FATAL(str);\
	throw std::invalid_argument(str);