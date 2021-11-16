#include "summation_strategy.hpp"
#include <cstdint>
#include <vector>
#include <chrono>

using std::vector;

class BinaryTreeSummation : public SummationStrategy {
public:
    BinaryTreeSummation(uint64_t rank, vector<int> &n_summands);

    virtual ~BinaryTreeSummation();

    static const uint64_t parent(const uint64_t i);

    bool isLocal(uint64_t index) const;

    /** Determine which rank has the number with a given index */
    uint64_t rankFromIndex(uint64_t index) const;

    double acquireNumber(uint64_t index) const;


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

private:
    const uint64_t size,  begin, end;
    const vector<uint64_t> rankIntersectingSummands;
    std::chrono::duration<double> acquisitionDuration;
    long int acquisitionCount;

};

