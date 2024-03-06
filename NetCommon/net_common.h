/*
 * NetWeave - C++ Networking Library
 * Copyright 2024 - Jessy van Polanen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NET_COMMON_
#define _NET_COMMON_

#define __NET_MAJOR__		1.0
#define __NET_MINOR__		0.1
#define __NET_PATCH__		0.0
#define __NET_REVISION__	0.0
#define __NET_VERSION__		1.0
#define __NET_MESSAGE__		"Net: V0.1"

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <algorithm>
#include <queue>
#include <deque>
#include <functional>
#include <string>

#ifdef _WIN32
#   define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#define BEGIN_NET_NS namespace net {
#define END_NET_NS }

BEGIN_NET_NS

template <typename T>
using scope = std::unique_ptr<T>;
 
template <typename T>
using ref = std::shared_ptr<T>;

END_NET_NS

#endif