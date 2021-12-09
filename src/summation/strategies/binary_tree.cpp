#define NDEBUG
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <exception>
#include <vector>
#include <numeric>
#include <cstring>
#include <cassert>
#include <cmath>
#include <unistd.h>
#include <functional>
#include <chrono>
#include <io.hpp>
#include <util.hpp>
#include "binary_tree.hpp"

#undef DEBUG_OUTPUT_TREE

using namespace std;
using namespace std::string_literals;

const int MESSAGEBUFFER_MPI_TAG = 1;

MessageBuffer::MessageBuffer() : targetRank(-1), inbox() {
    outbox.reserve(MAX_MESSAGE_LENGTH + 1);
    buffer.resize(MAX_MESSAGE_LENGTH);
    reqs.reserve(16);
}

void MessageBuffer::wait() {
    for (MPI_Request &r : reqs) {
        MPI_Wait(&r, MPI_STATUS_IGNORE);
    }

    reqs.clear();
}


void MessageBuffer::flush() {
    if(targetRank == -1) return;
    MPI_Request r;
    reqs.push_back(r);

    const int messageByteSize = sizeof(MessageBufferEntry) * outbox.size();

    assert(0 < targetRank < 128);
    MPI_Isend(static_cast<void *>(&outbox[0]), messageByteSize, MPI_BYTE, targetRank,
            MESSAGEBUFFER_MPI_TAG, MPI_COMM_WORLD, &reqs.back());

    targetRank = -1;
    outbox.clear();
}

const void MessageBuffer::receive(const int sourceRank) {
    assert(0 < sourceRank < 128);
    MPI_Status status;

    MPI_Recv(static_cast<void *>(&buffer[0]), sizeof(MessageBufferEntry) * MAX_MESSAGE_LENGTH, MPI_BYTE,
            sourceRank, MESSAGEBUFFER_MPI_TAG, MPI_COMM_WORLD, &status);

    const int receivedEntries = status._ucount / sizeof(MessageBufferEntry);

    for (int i = 0; i < receivedEntries; i++) {
        MessageBufferEntry entry = buffer[i];
        inbox[entry.index] = entry.value;
    }
}

void MessageBuffer::put(const int targetRank, const uint64_t index, const double value) {
    if (outbox.size() >= MAX_MESSAGE_LENGTH || this->targetRank != targetRank) {
        flush();
    }

    if (this->targetRank == -1) {
        this->targetRank = targetRank;
    }

    MessageBufferEntry e;
    e.index = index;
    e.value = value;
    outbox.push_back(e);

    if (outbox.size() == MAX_MESSAGE_LENGTH) flush();
}

const double MessageBuffer::get(const int sourceRank, const uint64_t index) {
    // If we have the number in our inbox, directly return it
    if (inbox.contains(index)) {
        double result = inbox[index];
        inbox.erase(index);
        return result;
    }

    // If not, we will wait for a message, but make sure no one is waiting for our results.
    flush();
    wait();
    receive(sourceRank);

    //cout << "Waiting for rank " << sourceRank << ", index " << index ;

    // Our computation order should guarantee that the number is contained within
    // the next package
    assert(inbox.contains(index));

    //cout << " [RECEIVED]" << endl;
    double result = inbox[index];
    inbox.erase(index);
    return result;
}


BinaryTreeSummation::BinaryTreeSummation(uint64_t rank, vector<int> &n_summands)
    : SummationStrategy(rank, n_summands),
      acquisitionDuration(std::chrono::duration<double>::zero()),
      acquisitionCount(0L),
      size(n_summands[rank]),
      begin (startIndex[rank]),
      end (begin +  size),
      rankIntersectingSummands(calculateRankIntersectingSummands()),
      accumulationBuffer(size) {
#ifdef DEBUG_OUTPUT_TREE
    printf("Rank %lu has %lu summands, starting from index %lu to %lu\n", rank, size, begin, end);
    printf("Rank %lu rankIntersectingSummands: ", rank);
    for (int ri : rankIntersectingSummands)
        printf("%u ", ri);
    printf("\n");
#endif
}

BinaryTreeSummation::~BinaryTreeSummation() {
#ifdef ENABLE_INSTRUMENTATION
    cout << "Rank " << rank << " avg. acquisition time: "
        << acquisitionTime() / acquisitionCount << "  ns\n";
#endif
}

const uint64_t BinaryTreeSummation::parent(const uint64_t i) {
    assert(i != 0);

    // clear least significand set bit
    return i & (i - 1);
}

bool BinaryTreeSummation::isLocal(uint64_t index) const {
    return (index >= begin && index < end);
}

/** Determine which rank has the number with a given index */
uint64_t BinaryTreeSummation::rankFromIndex(uint64_t index) const {
    int64_t rankLocalIndex = index;

    for (int64_t sourceRank = 0; sourceRank < clusterSize; sourceRank++) {
        if (rankLocalIndex < n_summands[sourceRank]) {
            //cout << "Idx " << index << " is on rank " << sourceRank
            //     << " with local idx " << rankLocalIndex << endl;

            return sourceRank;
        }

        rankLocalIndex -= n_summands[sourceRank];
    }


    throw logic_error(string("Number ") + to_string(index) + " cannot be found on any node");
}

const double BinaryTreeSummation::acquireNumber(const uint64_t index) {
    if (isLocal(index)) {
        // We have that number locally
        return summands[index - begin];
    }

    // Otherwise, receive it
    const double result = messageBuffer.get(rankFromIndex(index), index);

    return result;
}

/* Calculate all rank-intersecting summands that must be sent out because
    * their parent is non-local and located on another rank
    */
vector<uint64_t> BinaryTreeSummation::calculateRankIntersectingSummands(void) const {
    vector<uint64_t> result;

    if (rank == 0) {
        return result;
    }

    // ignore index 0
    uint64_t startIndex = (begin == 0) ? 1 : begin;
    for (uint64_t index = startIndex; index < end; index++) {
        if (!isLocal(parent(index))) {
            result.push_back(index);
        }
    }

    return result;
}

/* Sum all numbers. Will return the total sum on rank 0
    */
double BinaryTreeSummation::accumulate(void) {
    for (auto summand : rankIntersectingSummands) {
        if (subtree_size(summand) > 64) {
            // If we are about to do some considerable amount of work, make sure
            // the send buffer is empty so noone is waiting for our results
            messageBuffer.flush();
        }

        double result = accumulate(summand);

        messageBuffer.put(rankFromIndex(parent(summand)), summand, result);
    }
    messageBuffer.flush();
    messageBuffer.wait();

    double result = (rank == ROOT_RANK) ? accumulate(0) : 0.0;
    MPI_Bcast(&result, 1, MPI_DOUBLE,
              ROOT_RANK, MPI_COMM_WORLD);

    return result;
}


double BinaryTreeSummation::recursiveAccumulate(uint64_t index) {
#ifdef ENABLE_INSTRUMENTATION
    auto t1 = std::chrono::high_resolution_clock::now();
#endif

    if (is_local_subtree_of_size(8, index)) {
        return accumulate_local_8subtree(index);
    }

    double accumulator = acquireNumber(index);

#ifdef ENABLE_INSTRUMENTATION
    auto t2 = std::chrono::high_resolution_clock::now();
    acquisitionDuration += t2 - t1;
    acquisitionCount++;
#endif

#ifdef DEBUG_OUTPUT_TREE
    if (isLocal(index))
        printf("rk%i C%i.0 = %f\n", rank, index, accumulator);
#endif

    if (!isLocal(index)) {
        return accumulator;
    }

    int lsb_index = ffs(index);
    if (lsb_index == 0) {
        // If no bits are set, iterate over all zeros
        lsb_index = 31;
    }

    // Iterate over all variants where one of the least significant zeros
    // has been replaced with a one.
    for (int j = 1; j < lsb_index; j++) {
        int zero_position = j - 1;
        uint64_t child_index = index | (1 << zero_position);

        if (child_index < globalSize) {
            double num = recursiveAccumulate(child_index);
            accumulator += num;
#ifdef DEBUG_OUTPUT_TREE
            printf("rk%i C%i.%i = C%i.%i + %f\n", rank, index, j, index, j - 1, num);
#endif
        }
    }

    return accumulator;
}

double BinaryTreeSummation::accumulate(const uint64_t index) {
    if (index & 1) {
        // no accumulation needed
        return summands[index - begin];
    }

    const uint64_t maxX = (index == 0) ? globalSize - 1
        : min(globalSize - 1, index + subtree_size(index) - 1);
    const int maxY = (index == 0) ? ceil(log2(globalSize)) : log2(subtree_size(index));

    const uint64_t largest_local_index = min(maxX, end - 1);
    const uint64_t n_local_elements = largest_local_index + 1 - index;

    memcpy(&accumulationBuffer[0], &summands[index - begin], n_local_elements * sizeof(double));

    uint64_t elementsInBuffer = n_local_elements;

    for (int y = 1; y <= maxY; y++) {
        uint64_t elementsWritten = 0;

        // Do one less iteration if we have a remainder
        const uint64_t maxI = (elementsInBuffer >> 1) << 1;

        for (uint64_t i = 0; i < maxI; i += 2) {
            const double a = accumulationBuffer[i];
            const double b = accumulationBuffer[i + 1];

            accumulationBuffer[elementsWritten++] = a + b;
        }

        // Deal with the remainder
        if (2 * elementsWritten < elementsInBuffer) {
            const uint64_t indexA = index + (elementsInBuffer - 1) * (1 << (y - 1));
            const uint64_t indexB = index + (elementsInBuffer + 0) * (1 << (y - 1));
            const double a = accumulationBuffer[elementsInBuffer - 1];

            if (indexB > maxX) {
                // the remainder is the last element of our tree, we just copy it over
                accumulationBuffer[elementsWritten++] = a;
            } else {
                // otherwise, there is still one addition to calculate, but its second part is not local
                assert(indexB >= end);
                const double b = acquireNumber(indexB);
                accumulationBuffer[elementsWritten++] = a + b;
            }
        }

        elementsInBuffer = elementsWritten;
    }
    assert(elementsInBuffer == 1);

    return accumulationBuffer[0];
}

double BinaryTreeSummation::nocheckAccumulate(void) {
    assert(begin == 0);
    assert(globalSize == end);

    uint64_t maxX = globalSize - 1;
    int maxY = ceil(log2(globalSize));

    uint64_t largest_local_index = min(maxX, end - 1);
    uint64_t n_local_elements = largest_local_index + 1;

    for (size_t i = 0; i < n_local_elements; i++) {
        accumulationBuffer[i] = summands[i];
    }

    uint64_t elementsInBuffer = n_local_elements;

    for (int y = 1; y <= maxY; y++) {
        uint64_t elementsWritten = 0;

        for (uint64_t i = 0; i < elementsInBuffer; i += 2) {
            const uint64_t indexA = (i + 0) * (1 << (y - 1));
            const uint64_t indexB = (i + 1) * (1 << (y - 1));

            const double a = accumulationBuffer[i];
            const double b = (indexB > maxX) ? 0.0 : accumulationBuffer[i+1];

            accumulationBuffer[elementsWritten++] = a + b;
        }

        elementsInBuffer = elementsWritten;
    }
    assert(elementsInBuffer == 1);

    return accumulationBuffer[0];

}

const double BinaryTreeSummation::acquisitionTime(void) const {
    return std::chrono::duration_cast<std::chrono::nanoseconds> (acquisitionDuration).count();
}

const uint64_t BinaryTreeSummation::largest_child_index(const uint64_t index) const {
    return index | (index - 1);
}

const uint64_t BinaryTreeSummation::subtree_size(const uint64_t index) const {
    assert(index != 0);
    return largest_child_index(index) + 1 - index;
}

const bool BinaryTreeSummation::is_local_subtree_of_size(const uint64_t expectedSubtreeSize, const uint64_t i) const {
    const auto lci = largest_child_index(i);
    const auto subtreeSize = lci + 1 -i;
    return (subtreeSize == expectedSubtreeSize && isLocal(lci));
}

const double BinaryTreeSummation::accumulate_local_8subtree(const uint64_t startIndex) const {
    assert(isLocal(startIndex) && isLocal(largest_child_index(startIndex)));
    const double level1a = summands[startIndex + 0 - begin] + summands[startIndex + 1 - begin];
    const double level1b = summands[startIndex + 2 - begin] + summands[startIndex + 3 - begin];
    const double level1c = summands[startIndex + 4 - begin] + summands[startIndex + 5 - begin];
    const double level1d = summands[startIndex + 6 - begin] + summands[startIndex + 7 - begin];

    const double level2a = level1a + level1b;
    const double level2b = level1c + level1d;

    return level2a + level2b;
}
