#include <vector>

using std::vector;

class DistributedBinaryTree {
public:
    DistributedBinaryTree(uint64_t rank, vector<uint64_t> n_summands);

    // Debug call to read data from array of all summands
    void read_from_array(double *array);

    uint64_t parent(uint64_t i);

    // TODO: replace with memory-efficient generator or co-routine
    vector<uint64_t> children(uint64_t i);

    bool isLocal(uint64_t index);

    /** Determine which rank has the number with a given index */
    uint64_t rankFromIndex(uint64_t index);

    double acquireNumber(uint64_t index);

    /* Calculate all rank-intersecting summands that must be sent out because
     * their parent is non-local and located on another rank
     */
    vector<uint64_t> rankIntersectingSummands(void);

    /* Sum all numbers. Will return the total sum on rank 0
     */
    double accumulate(void);

    double allreduce(void);

protected:
    double accumulate(uint64_t index);

private:
    uint64_t size, globalSize, rank, n_ranks, begin, end;
    vector<uint64_t> n_summands;
    vector<double> summands;

};

