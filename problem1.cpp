#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <algorithm>
#include <unordered_set>
#include <chrono>
#include <random>
#include <cassert>

#define NUM_SERVANTS 4

struct Node {
	Node *prev = nullptr, *next = nullptr;
	int tag;
};

class List {

	std::mutex mut;

	Node *head = nullptr;

public:

	void add(int tag) {
		std::unique_lock lock(mut);

		Node *node = new Node();
		node->tag = tag;

		Node *insertAfter = nullptr;

		if (head && head->tag < tag) {
			insertAfter = head;
			while (insertAfter && insertAfter->next && insertAfter->next->tag < tag) {
				insertAfter = insertAfter->next;
			}
		}

		if (insertAfter == nullptr) {
			node->next = head;
			if (head)
				head->prev = node;
			head = node;
		}
		else {
			node->prev = insertAfter;
			node->next = insertAfter->next;
			if (insertAfter->next)
				insertAfter->next->prev = node;
			insertAfter->next = node;
		}
	}

	int getFirst() {
		std::unique_lock lock(mut);

		return head ? head->tag : -1;
	}

	bool remove(int tag) {
		std::unique_lock lock(mut);

		Node *at = head;
		while (at) {
			if (at->tag == tag) {
				if (at == head)
					head = at->next;

				if (at->next)
					at->next->prev = at->prev;
				if (at->prev)
					at->prev->next = at->next;

				delete at;

				return true;
			}

			at = at->next;
		}

		return false;
	}

	bool includes(int tag) {
		std::unique_lock lock(mut);

		Node *at = head;
		while (at) {
			if (at->tag == tag) {
				return true;
			}

			at = at->next;
		}

		return false;
	}

};

std::mutex tagSetLock;
std::unordered_set<int> tagSet;

int numPresents = 0;
int numThankYous = 0;

List list;

void servant(int id) {
	// use this boolean to alternate actions
	bool placedLastTime = false;

	while (true) {
		int value = -1;

		if (!placedLastTime) {
			tagSetLock.lock();
			if (tagSet.size() > 0) {
				auto it = tagSet.begin();
				value = *it;
				tagSet.erase(it);
			}
			tagSetLock.unlock();
		}

		// write a thank you card
		if (value == -1) {
			placedLastTime = false;

			int gift = list.getFirst();
			if (gift == -1) {
				bool tagSetEmpty = false;

				{
					tagSetLock.lock();
					if (tagSet.size() == 0) {
						tagSetEmpty = true;
					}
					tagSetLock.unlock();
				}

				if (tagSetEmpty)
					break;
				else
					continue;
			}

			if (list.remove(gift))
				numThankYous += 1;
		}

		// place a gift in the chain
		else {
			placedLastTime = true;

			numPresents += 1;
			list.add(value);
		}

		std::cout << "presents: " << numPresents << " / thank yous: " << numThankYous << std::endl;
	}
}

int main() {
	for (int i = 0; i < 500'000; ++i)
		tagSet.insert(i);

	std::vector<std::thread> threads;
	for (int i = 0; i < NUM_SERVANTS; ++i)
		threads.emplace_back(servant, i);

	for (std::thread &thr : threads)
		thr.join();

	if (numThankYous == numPresents) {
		std::cout << "Number of thank yous matches the number of presents!" << std::endl;
	}
	else {
		std::cout << "Number of thank yous DOES NOT match the number of presents :(" << std::endl;
	}

	return 0;
}