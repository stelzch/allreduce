#include "summation_strategy.hpp"
#include "util.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
#include <chrono>
#include <array>
#include <map>
#include <utility>
#include <mpi.h>
#include <util.hpp>

using std::vector;
using std::array;
using std::map;

const uint8_t MAX_MESSAGE_LENGTH = 4;

struct MessageBufferEntry {
    uint64_t index;
    double value;
};

class MessageBuffer {

public:
    MessageBuffer(MPI_Comm comm);
    const void receive(const int sourceRank);
    void flush(void);
    void wait(void);

    void put(const int targetRank, const uint64_t index, const double value);
    const double get(const int sourceRank, const uint64_t index);

    const void printStats(void) const;

protected:
    array<MessageBufferEntry, MAX_MESSAGE_LENGTH> entries;
    map<uint64_t, double> inbox;
    int targetRank;
    vector<MessageBufferEntry> outbox;
    vector<MessageBufferEntry> buffer;
    vector<MPI_Request> reqs;
    size_t awaitedNumbers;
    size_t sentMessages;
    size_t sentSummands;
    bool sendBufferClear;
    MPI_Comm comm;
};

class BinaryTreeSummation : public SummationStrategy {
public:
    BinaryTreeSummation(uint64_t rank, vector<int> &n_summands, MPI_Comm comm = MPI_COMM_WORLD);

    virtual ~BinaryTreeSummation();

    static const uint64_t parent(const uint64_t i);

    bool isLocal(uint64_t index) const;

    /** Determine which rank has the number with a given index */
    uint64_t rankFromIndex(uint64_t index) const;
    uint64_t rankFromIndexMap(const uint64_t index) const;

    static double global_sum(const vector<double> &data,
            MPI_Comm comm = MPI_COMM_WORLD);

    const double acquireNumber(const uint64_t index);


    /* Sum all numbers. Will return the total sum on rank 0
     */
    double accumulate(void);

    /* Calculate all rank-intersecting summands that must be sent out because
     * their parent is non-local and located on another rank
     */
    vector<uint64_t> calculateRankIntersectingSummands(void) const;

    /* Return the average number of nanoseconds spend in total on waiting for intermediary
     * results from other hosts
     */
    const double acquisitionTime(void) const;

    double accumulate(uint64_t index);
    double recursiveAccumulate(const uint64_t index);
    double nocheckAccumulate(void);

    const void printStats(void) const;

    const int rankFromIndexClosedForm(const uint64_t index) const;

protected:
    const uint64_t largest_child_index(const uint64_t index) const;
    const uint64_t subtree_size(const uint64_t index) const;

    /** Figure out if the parts that make up a certain index are all local and form a subtree
     * of a specifc size */
    const bool is_local_subtree_of_size(const uint64_t expectedSubtreeSize, const uint64_t i) const;
    const double accumulate_local_8subtree(const uint64_t startIndex) const;

    inline const double sum_remaining_8tree(const uint64_t bufferStartIndex,
            const uint64_t initialRemainingElements,
            const int y,
            const uint64_t maxX,
	    double *srcBuffer,
            double *dstBuffer) {
        uint64_t remainingElements = initialRemainingElements;

        for (int level = 0; level < 3; level++) {
            const int stride = 1 << (y - 1 + level);
            int elementsWritten = 0;
            for (int i = 0; (i + 1) < remainingElements; i += 2) {
                dstBuffer[elementsWritten++] = srcBuffer[i] + srcBuffer[i + 1];
            }


            if (remainingElements % 2 == 1) {
                const uint64_t bufferIndexA = remainingElements - 1;
                const uint64_t bufferIndexB = remainingElements;
                const uint64_t indexB = bufferStartIndex + bufferIndexB * stride;
                const double a = srcBuffer[bufferIndexA];

                if (indexB > maxX) {
                    // indexB is the last element because the subtree ends there
                    dstBuffer[elementsWritten++] = a;
                } else {
                    // indexB must be fetched from another rank
                    const double b = messageBuffer.get(rankFromIndexMap(indexB), indexB);
                    dstBuffer[elementsWritten++] = a + b;
                }

                remainingElements += 1;
            }

	    srcBuffer = dstBuffer;
            remainingElements /= 2;
        }
        assert(remainingElements == 1);

        return dstBuffer[0];
    }


private:
    const uint64_t size,  begin, end;
    const vector<uint64_t> rankIntersectingSummands;
    const int nonResidualRanks;
    const uint64_t fairShare, splitIndex;
    static vector<double, Util::AlignedAllocator<double>> accumulationBuffer;
    std::chrono::duration<double> acquisitionDuration;
    std::map<uint64_t, int> startIndices;
    long int acquisitionCount;


    MessageBuffer messageBuffer;
};
