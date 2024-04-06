#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <algorithm>
#include <chrono>
#include <random>

#define NUM_SENSORS 8

// how many minutes to simulate the sensors for
#define MAX_TIME 1440

struct Node {
	double value = -1;
	Node *next = nullptr;
};

Node *heads[NUM_SENSORS] = { nullptr };
std::atomic<Node *> tails[NUM_SENSORS] = { nullptr };
std::atomic<size_t> recordLengths[NUM_SENSORS] = { 0 };

std::atomic<size_t> runningSensors;

void sensor(int sensorNumber) {
	std::random_device rdev;
	std::mt19937 rng(rdev());
	std::uniform_real_distribution<double> distr(-100, 70);

	for (int time = 0; time < MAX_TIME; ++time) {
		// generate a reading
		double temp = distr(rng);

		// create a new node for the reading
		Node *node = new Node();
		node->value = temp;
		node->next = nullptr;

		// and append it locklessly to the linked list
		tails[sensorNumber].exchange(node)->next = node;
		recordLengths[sensorNumber] += 1;
	}

	runningSensors -= 1;
}

int main() {
	for (int sensor = 0; sensor < NUM_SENSORS; ++sensor) {
		// start every sensor's linked list off with a dummy node
		heads[sensor] = new Node();
		tails[sensor].store(heads[sensor]);
	}

	// create the threads for all the sensors
	runningSensors = NUM_SENSORS;

	std::vector<std::thread> threads;
	for (int id = 0; id < NUM_SENSORS; ++id) {
		threads.emplace_back(sensor, id);
	}

	int hour = 0;
	while (true) {
		// check if we have an hour's worth of data from all the sensors

		bool dataAvailable = true;
		for (int i = 0; i < NUM_SENSORS; ++i) {
			if (recordLengths[i].load() < 60) {
				dataAvailable = false;
				break;
			}
		}

		if (!dataAvailable) {
			if (runningSensors.load() > 0) {
				continue;
			}
			else {
				break;
			}
		}

		// if we have at least an hour's worth of data from all the sensors, collect the data and analyze it

		for (int i = 0; i < NUM_SENSORS; ++i) {
			recordLengths[i] -= 60;
		}

		std::vector<std::vector<double>> readings(60, std::vector<double>(NUM_SENSORS));
		for (int i = 0; i < 60; ++i) {
			for (int j = 0; j < NUM_SENSORS; ++j) {
				Node *oldHead = heads[j];
				while (heads[j]->next == nullptr)
					{}
				heads[j] = heads[j]->next;
				readings[i][j] = heads[j]->value;
				delete oldHead;
			}
		}

		std::vector<double> flattenedReadings;
		for (auto &reading : readings)
			for (double temp : reading)
				flattenedReadings.push_back(temp);
		std::sort(flattenedReadings.begin(), flattenedReadings.end());

		std::cout << "Hour #" << hour << " Report:" << std::endl;
		hour += 1;

		std::cout << "  Top 5 highest temperatures:" << std::endl;
		for (size_t j = 1, i = flattenedReadings.size() - 1; i >= flattenedReadings.size() - 5; --i, ++j)
			std::cout << "  " << j << ": " << flattenedReadings[i] << "F" << std::endl;

		std::cout << "  Top 5 lowest temperatures:" << std::endl;
		for (size_t j = 1, i = 0; i < 5; ++i, ++j)
			std::cout << "  " << j << ": " << flattenedReadings[i] << "F" << std::endl;

		size_t diffMinute = 0;
		double bestDiff = 0;
		for (size_t i = 0; i < flattenedReadings.size() - NUM_SENSORS * 10; ++i) {
			size_t j = i + NUM_SENSORS * 10;
			size_t minute = i / NUM_SENSORS;

			double diff = abs(flattenedReadings[i] - flattenedReadings[j]);
			if (diff > bestDiff) {
				bestDiff = diff;
				diffMinute = minute;
			}
		}

		std::cout << "  The largest 10-minute temperature difference was observed between minutes "
			<< diffMinute << " and " << diffMinute + 10 << std::endl;

		std::cout << std::endl;
	}

	for (std::thread &thr : threads)
		thr.join();

	return 0;
}