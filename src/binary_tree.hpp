#include <vector>

using std::vector;

class DistributedBinaryTree {
public:
    DistributedBinaryTree(uint64_t rank, vector<uint64_t> n_summands);

    // Debug call to read data from array of all summands
    void read_from_array(double *array);

    const uint64_t parent(uint64_t i) const;

    const bool isLocal(uint64_t index) const;

    /** Determine which rank has the number with a given index */
    const uint64_t rankFromIndex(uint64_t index) const;

    const double acquireNumber(uint64_t index) const;

    /* Calculate all rank-intersecting summands that must be sent out because
     * their parent is non-local and located on another rank
     */
    const vector<uint64_t> rankIntersectingSummands(void) const;

    /* Sum all numbers. Will return the total sum on rank 0
     */
    double accumulate(void);

protected:
    double accumulate(uint64_t index);

private:
    uint64_t size, globalSize, rank, n_ranks, begin, end, nodeIndex;
    vector<uint64_t> n_summands;
    vector<double> summands;

};

