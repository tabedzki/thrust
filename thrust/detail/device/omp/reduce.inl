/*
 *  Copyright 2008-2009 NVIDIA Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#include <omp.h>
#include <thrust/iterator/iterator_traits.h>
#include <thrust/distance.h>
#include <thrust/detail/raw_buffer.h>

namespace thrust
{
namespace detail
{
namespace device
{
namespace omp
{

#if defined(__CUDACC__) && (CUDA_VERSION < 3000)

// CUDA v2.3 and earlier strip OpenMP #pragmas from template functions

template <typename InputIterator,
          typename OutputType,
          typename BinaryFunction>
OutputType reduce(InputIterator first,
                  InputIterator last,
                  OutputType init,
                  BinaryFunction binary_op)
{
    // initialize the result
    OutputType result = init;

    while(first != last)
    {
        result = binary_op(result, *first);
        first++;
    } // end while

    return result;
}

#else

template <typename InputIterator,
          typename OutputType,
          typename BinaryFunction>
OutputType reduce(InputIterator first,
                  InputIterator last,
                  OutputType init,
                  BinaryFunction binary_op)
{
    typedef typename thrust::iterator_difference<InputIterator>::type difference_type;

    difference_type N = thrust::distance(first, last);

    int num_threads = std::min<difference_type>(omp_get_max_threads(), N);

    thrust::detail::raw_host_buffer<OutputType> thread_results(first, first + num_threads);

#   pragma omp parallel num_threads(num_threads)
    {

        int thread_id = omp_get_thread_num();

        OutputType thread_sum = thread_results[thread_id];

#      pragma omp for 
        for (difference_type i = num_threads; i < N; i++)
            thread_sum = binary_op(thread_sum, first[i]);

        thread_results[thread_id] = thread_sum;
    }

    OutputType total_sum = init;
    for (int i = 0; i < num_threads; ++i)
        total_sum = binary_op(total_sum, thread_results[i]);

    return total_sum;
}

#endif //#if defined(__CUDACC__) && (CUDA_VERSION < 3000)

} // end namespace omp
} // end namespace device
} // end namespace detail
} // end namespace thrust

