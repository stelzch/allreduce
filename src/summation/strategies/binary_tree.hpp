#include "summation_strategy.hpp"
#include <cstdint>
#include <vector>
#include <chrono>
#include <array>
#include <map>
#include <mpi.h>

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
    MessageBuffer();
    const void receive(const int sourceRank);
    void flush(void);
    void wait(void);

    void put(const int targetRank, const uint64_t index, const double value);
    const double get(const int sourceRank, const uint64_t index);

protected:
    array<MessageBufferEntry, MAX_MESSAGE_LENGTH> entries;
    map<uint64_t, double> inbox;
    int targetRank;
    vector<MessageBufferEntry> outbox;
    vector<MessageBufferEntry> buffer;
    vector<MPI_Request> reqs;
};

class BinaryTreeSummation : public SummationStrategy {
public:
    BinaryTreeSummation(uint64_t rank, vector<int> &n_summands);

    virtual ~BinaryTreeSummation();

    static const uint64_t parent(const uint64_t i);

    bool isLocal(uint64_t index) const;

    /** Determine which rank has the number with a given index */
    uint64_t rankFromIndex(uint64_t index) const;

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

protected:
    double accumulate(uint64_t index);

    const uint64_t largest_child_index(const uint64_t index) const;
    const uint64_t subtree_size(const uint64_t index) const;

    /** Figure out if the parts that make up a certain index are all local and form a subtree
     * of a specifc size */
    const bool is_local_subtree_of_size(const uint64_t expectedSubtreeSize, const uint64_t i) const;
    const double accumulate_local_8subtree(const uint64_t startIndex) const;


private:
    const uint64_t size,  begin, end;
    const vector<uint64_t> rankIntersectingSummands;
    std::chrono::duration<double> acquisitionDuration;
    long int acquisitionCount;

    MessageBuffer messageBuffer;
};

