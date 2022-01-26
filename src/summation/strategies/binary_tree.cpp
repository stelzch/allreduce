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
#include <memory>
#include <functional>
#include <chrono>
#include <io.hpp>
#include <util.hpp>
#include "binary_tree.hpp"

#ifdef AVX
#include <immintrin.h>
#endif

#undef DEBUG_OUTPUT_TREE

using namespace std;
using namespace std::string_literals;

const int MESSAGEBUFFER_MPI_TAG = 1;

MessageBuffer::MessageBuffer(MPI_Comm comm) : targetRank(-1),
    inbox(),
    awaitedNumbers(0),
    sentMessages(0),
    sentSummands(0),
    sendBufferClear(true),
    comm(comm)
    {
    outbox.reserve(MAX_MESSAGE_LENGTH + 1);
    buffer.resize(MAX_MESSAGE_LENGTH);
    reqs.reserve(16);
}

void MessageBuffer::wait() {
    for (MPI_Request &r : reqs) {
        MPI_Wait(&r, MPI_STATUS_IGNORE);
    }

    reqs.clear();
    sendBufferClear = true;
}


void MessageBuffer::flush() {
    if(targetRank == -1) return;
    MPI_Request r;
    reqs.push_back(r);

    const int messageByteSize = sizeof(MessageBufferEntry) * outbox.size();

    assert(0 < targetRank < 128);
    MPI_Isend(static_cast<void *>(&outbox[0]), messageByteSize, MPI_BYTE, targetRank,
            MESSAGEBUFFER_MPI_TAG, comm, &reqs.back());
    sentMessages++;

    targetRank = -1;
    outbox.clear();
    sendBufferClear = false;
}

const void MessageBuffer::receive(const int sourceRank) {
    assert(0 < sourceRank < 128);
    MPI_Status status;

    MPI_Recv(static_cast<void *>(&buffer[0]), sizeof(MessageBufferEntry) * MAX_MESSAGE_LENGTH, MPI_BYTE,
            sourceRank, MESSAGEBUFFER_MPI_TAG, comm, &status);
    awaitedNumbers++;

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

    /* Since we send asynchronously, we must check whether the buffer can currently be written to */
    if(!sendBufferClear) {
        wait();
    }

    if (this->targetRank == -1) {
        this->targetRank = targetRank;
    }

    MessageBufferEntry e;
    e.index = index;
    e.value = value;
    outbox.push_back(e);

    if (outbox.size() == MAX_MESSAGE_LENGTH) flush();

    sentSummands++;
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

const void MessageBuffer::printStats() const {
    int rank;
    MPI_Comm_rank(comm, &rank);

    size_t globalStats[] {0, 0, 0};
    size_t localStats[] {sentMessages, sentMessages, sentSummands};

    MPI_Reduce(localStats, globalStats, 3, MPI_LONG, MPI_SUM,
            0, comm);

    if (rank == 0) {
        printf("sentMessages=%li\naverageSummandsPerMessage=%f\n",
                globalStats[0],
                globalStats[2] / static_cast<double>(globalStats[0]));

    }

}


BinaryTreeSummation::BinaryTreeSummation(uint64_t rank, vector<int> &n_summands, MPI_Comm comm)
    : SummationStrategy(rank, n_summands, comm),
      size(n_summands[rank]),
      begin (startIndex[rank]),
      end (begin +  size),
      rankIntersectingSummands(calculateRankIntersectingSummands()),
      acquisitionDuration(std::chrono::duration<double>::zero()),
      acquisitionCount(0L),
      accumulationBuffer(size + 8),
      messageBuffer(comm)
{
    /* Initialize start indices map */
    int startIndex = 0;
    int rankNumber = 0;
    for (const int& n : n_summands) {
        startIndices[startIndex] = rankNumber++;
        startIndex += n;
    }
    // guardian element
    startIndices[startIndex] = rankNumber;

    int c_size;
    MPI_Comm_size(comm, &c_size);
    assert(c_size == n_summands.size());

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

uint64_t BinaryTreeSummation::rankFromIndexMap(const uint64_t index) const {
    auto it = startIndices.upper_bound(index);

    assert(it != startIndices.end());
    const int nextRank = it->second;
    return nextRank - 1;
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

    assert(begin != 0);

    uint64_t index = begin;
    while (index < end) {
        assert(parent(index) < begin);
        result.push_back(index);

        index = index + subtree_size(index);
    }

    return result;
}

/* Sum all numbers. Will return the total sum on rank 0
    */
double BinaryTreeSummation::accumulate(void) {
    for (auto summand : rankIntersectingSummands) {
        if (subtree_size(summand) > 32) {
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
              ROOT_RANK, comm);

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

    uint64_t elementsInBuffer = n_local_elements;

    double *sourceBuffer = static_cast<double *>(&summands[index - begin]);
    double *destinationBuffer = static_cast<double *>(&accumulationBuffer[0]);


    for (int y = 1; y <= maxY; y += 3) {
        uint64_t elementsWritten = 0;

        for (uint64_t i = 0; i + 8 <= elementsInBuffer; i += 8) {
            __m256d a = _mm256_loadu_pd(static_cast<double *>(&sourceBuffer[i]));
            __m256d b = _mm256_loadu_pd(static_cast<double *>(&sourceBuffer[i+4]));
            __m256d level1Sum = _mm256_hadd_pd(a, b);

            __m128d c = _mm256_extractf128_pd(level1Sum, 1); // Fetch upper 128bit
            __m128d d = _mm256_castpd256_pd128(level1Sum); // Fetch lower 128bit
            __m128d level2Sum = _mm_add_pd(c, d);

            __m128d level3Sum = _mm_hadd_pd(level2Sum, level2Sum);

            destinationBuffer[elementsWritten++] = _mm_cvtsd_f64(level3Sum);
        }

        // number of remaining elements
        const uint64_t remainder = elementsInBuffer - 8 * elementsWritten;
        assert(0 <= remainder);
        assert(remainder < 8);

        if (remainder > 0) {
            const uint64_t bufferIdx = 8 * elementsWritten;
            const uint64_t indexOfRemainingTree = index + bufferIdx * (1UL << (y - 1));
            const double a = sum_remaining_8tree(indexOfRemainingTree,
                    remainder, y, maxX,
		    &sourceBuffer[0] + bufferIdx,
                    &destinationBuffer[0] + bufferIdx);
            destinationBuffer[elementsWritten++] = a;
        }

	// After first iteration, read only from accumulation buffer
	sourceBuffer = destinationBuffer;

        elementsInBuffer = elementsWritten;
    }
        
    assert(elementsInBuffer == 1);

    return destinationBuffer[0];
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

const void BinaryTreeSummation::printStats() const {
    messageBuffer.printStats();
}
