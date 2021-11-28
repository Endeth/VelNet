#ifndef NETCOMMON_H
#define NETCONNOM_H

#include <memory>
#include <thread>
#include <mutex>
#include <optional>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <chrono>
#include <queue>
#include <vector>
#include <deque>
#include <unordered_set>
 
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#define ASIO_STANDALONE

#ifndef VEL_DEBUG_LOG
#define VEL_DEBUG_LOG 1
#endif

#ifdef VEL_DEBUG_LOG
#define VELLOG(x) { if(!(x)) { std::cout << "DEBUG: " << x << std::endl; } }
#endif

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#endif