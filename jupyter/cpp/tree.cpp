#define PY_SSIZE_T_CLEAN

#include <cassert>
#include <deque>
#include <algorithm>
#include <cstdint>
#include <vector>
#include <Python.h>
#include <iostream>

using std::cout;
using std::endl;



const uint64_t parent(const uint64_t i) {
    return i & (i - 1);
}

const uint64_t subtree_size(const uint64_t i) {
    const uint64_t largest_child_idx = i | (i - 1);
    return largest_child_idx + 1 - i;
}

static PyObject *RADTree_message_count_remainder_at_end(PyObject *self, PyObject *args) {
    PyObject *n_list;
    uint64_t m;


    if(!PyArg_ParseTuple(args, "Ok", &n_list, &m))
        return NULL;

    if(!PyList_Check(n_list))
        return NULL;

    const size_t length = PyList_Size(n_list);

    std::vector<uint64_t> results(length);
    std::vector<uint64_t> n_vec(length);

    for (size_t i = 0; i < length; i++) {
        n_vec[i] = PyLong_AsLong(PyList_GetItem(n_list, i));
    }


    #pragma omp parallel for
    for(int64_t j = 0; j < PyList_Size(n_list); j++) {
        const uint64_t n = n_vec[j];
        if (m <= 1) {
            results[j] = 0;
            continue;
        }

        if (n <= m) {
            results[j] = n - 1;
            continue;
        }

        std::deque<uint64_t> startIndices;
        const uint64_t equalShare = n / m;
        const uint64_t remainder = n % m;

        // Add m + 1 elements (last one is the guard)
        startIndices.push_back(0);
        //cout << "startIndices = 0 ";
        for (uint64_t i = 1; i <= m; i++) {
            const auto lastIdx = startIndices.back();
            if (m - i < remainder) {
                startIndices.push_back(lastIdx + equalShare + 1);
            } else {
                startIndices.push_back(lastIdx + equalShare);
            }
            //cout << startIndices.back() << " ";
        }
        //cout << endl;
        assert(startIndices.size() == m + 1);


        startIndices.pop_front();
        const uint64_t guard = startIndices.back();

        uint64_t messageCount = 0;
        uint64_t idx = startIndices.front();

        while (idx < guard) {
            //const auto begin = startIndices.front();
            //assert(parent(idx) < begin);
            const auto end = startIndices.at(1);

            messageCount++;

            idx = std::min<uint64_t>(idx + subtree_size(idx), end);

            if (idx == end) {
                startIndices.pop_front();
            }
        }

        results[j] = messageCount;
    }

    PyObject *resultList = PyList_New(results.size());
    for (size_t i = 0; i < results.size(); i++) {
        PyList_SetItem(resultList, i, PyLong_FromLong(results[i]));
    }

    return resultList;
}

static PyObject *RADTree_message_count(PyObject *self, PyObject *args) {
    PyObject *n_list;
    uint64_t m;


    if(!PyArg_ParseTuple(args, "Ok", &n_list, &m))
        return NULL;

    if(!PyList_Check(n_list))
        return NULL;

    const size_t length = PyList_Size(n_list);

    std::vector<uint64_t> results(length);
    std::vector<uint64_t> n_vec(length);

    for (size_t i = 0; i < length; i++) {
        n_vec[i] = PyLong_AsLong(PyList_GetItem(n_list, i));
    }


    #pragma omp parallel for
    for(int64_t j = 0; j < PyList_Size(n_list); j++) {
        const uint64_t n = n_vec[j];
        if (m <= 1) {
            results[j] = 0;
            continue;
        }

        if (n <= m) {
            results[j] = n - 1;
            continue;
        }

        std::deque<uint64_t> startIndices;
        const uint64_t equalShare = n / m;
        const uint64_t remainder = n % m;

        // Add m + 1 elements (last one is the guard)
        startIndices.push_back(0);
        //cout << "startIndices = 0 ";
        for (uint64_t i = 1; i <= m; i++) {
            const auto lastIdx = startIndices.back();
            if (i <= remainder) {
                startIndices.push_back(lastIdx + equalShare + 1);
            } else {
                startIndices.push_back(lastIdx + equalShare);
            }
            //cout << startIndices.back() << " ";
        }
        //cout << endl;
        assert(startIndices.size() == m + 1);


        startIndices.pop_front();
        const uint64_t guard = startIndices.back();

        uint64_t messageCount = 0;
        uint64_t idx = startIndices.front();

        while (idx < guard) {
            //const auto begin = startIndices.front();
            //assert(parent(idx) < begin);
            const auto end = startIndices.at(1);

            messageCount++;

            idx = std::min<uint64_t>(idx + subtree_size(idx), end);

            if (idx == end) {
                startIndices.pop_front();
            }
        }

        results[j] = messageCount;
    }

    PyObject *resultList = PyList_New(results.size());
    for (size_t i = 0; i < results.size(); i++) {
        PyList_SetItem(resultList, i, PyLong_FromLong(results[i]));
    }

    return resultList;
}


static PyMethodDef RADTreeMethods[] = {
    {"message_count", RADTree_message_count, METH_VARARGS,
        "Get the number of messages"},
    {"message_count_remainder_at_end", RADTree_message_count_remainder_at_end, METH_VARARGS,
        "Get the number of messages but put the remainder on the last few ranks."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef radtreemodule = {
    PyModuleDef_HEAD_INIT,
    "RADTree",
    NULL,
    -1,
    RADTreeMethods
};

PyMODINIT_FUNC
PyInit_radtree(void) {
    return PyModule_Create(&radtreemodule);
}
