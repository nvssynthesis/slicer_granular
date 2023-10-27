/*
  ==============================================================================

    algo_util.h
    Created: 27 Oct 2023 1:00:41am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <numeric>
#include <vector>

namespace nvs {
namespace util {


template<typename T>
std::optional<std::pair<size_t, T>>
get_closest(T target, std::vector<T> V){
	assert(std::is_sorted(V.begin(), V.end()));
	if(V.empty()){
		return {};
	}
	size_t low = 0;
	size_t high = V.size() - 1;
	
	while (low < high){
		size_t mid = std::midpoint(low, high);
		
		if (V[mid] < target){
			low = mid + 1;
		}
		else {
			high = mid;
		}
	}
	if (low > 0){
		T const currDiff = std::abs(target - V[low]);
		T const prevDiff = std::abs(target - V[low - 1]);
		if (prevDiff <= currDiff){
			low -= 1;
		}
	}
	return std::make_pair(low, V[low]);
}

}	// namespace util
}	// namespace nvs
