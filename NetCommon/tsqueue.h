#ifndef _THREADSAFE_QUEUE_
#define _THREADSAFE_QUEUE_

#include "net_common.h"

BEGIN_NET_NS

template <typename T>
class tsqueue {
public:
	tsqueue() = default;
	tsqueue(const tsqueue<T>&) = delete;
	virtual ~tsqueue() { this->clear(); }

public:
	T& front() {
		std::scoped_lock<std::mutex> lock(muxQueue);
		return this->deqQueue.front();
	}

	T& back() {
		std::scoped_lock<std::mutex> lock(muxQueue);
		return this->deqQueue.back();
	}

	void push_back(const T& item) {
		std::scoped_lock<std::mutex> lock(muxQueue);
		this->deqQueue.emplace_back(std::move(item));

		std::unique_lock<std::mutex> ul(this->muxBlocking);
		this->blocking.notify_one();
	}

	void push_front(const T& item) {
		std::scoped_lock<std::mutex> lock(muxQueue);
		this->deqQueue.emplace_front(std::move(item));

		std::unique_lock<std::mutex> ul(this->muxBlocking);
		this->blocking.notify_one();
	}

	bool empty() {
		std::scoped_lock<std::mutex> lock(muxQueue);
		return this->deqQueue.empty();
	}

	size_t count() {
		std::scoped_lock<std::mutex> lock(muxQueue);
		return this->deqQueue.size();
	}

	void clear() {
		std::scoped_lock<std::mutex> lock(muxQueue);
		this->deqQueue.clear();
	}

	T pop_front() {
		std::scoped_lock<std::mutex> lock(muxQueue);
		auto i = std::move(this->deqQueue.front());
		deqQueue.pop_front();
		return i;
	}

	T pop_back() {
		std::scoped_lock<std::mutex> lock(muxQueue);
		auto i = std::move(this->deqQueue.back());
		deqQueue.pop_front();
		return i;
	}

	void wait() {
		while (this->empty()) {
			std::unique_lock<std::mutex> ul(this->muxBlocking);
			this->blocking.wait(ul);
		}
	}

private:
	std::mutex muxQueue;
	std::deque<T> deqQueue;
private:
	std::condition_variable blocking;
	std::mutex muxBlocking;
public:
	tsqueue<T>& operator = (const tsqueue<T>&) = delete;
};

END_NET_NS

#endif