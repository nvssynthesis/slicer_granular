/*
  ==============================================================================

    DataStructures.h
    Created: 17 Aug 2023 5:24:30am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <iterator>
/*
 A map structure that is able to be used in constexpr context.
 Taken from https://xuhuisun.com/post/c++-weekly-2-constexpr-map/
 Modified according to need with guidance from https://github.com/serge-sans-paille/frozen/blob/master/include/frozen/map.h
 Use only for small, non-dynamic maps.
 Used here for param mapping.
 TODO: add iterators
 */
template <typename Key, typename Value, std::size_t Size>
struct StaticMap {
	using container_type = std::array<std::pair<const Key, Value>, Size>;

	container_type data;

	[[nodiscard]] constexpr Value at(const Key &key) const {
		const auto itr =
			std::find_if(data.cbegin(), data.cend(),
						 [&key](const auto &v) { return v.first == key; });
		if (itr != data.cend()) {
			return itr->second;
		} else {
			throw std::range_error("Not Found");
		}
	}
	
	using iterator = typename container_type::iterator;
	using const_iterator = typename container_type::const_iterator;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	
	
	/* iterators */
	constexpr iterator begin() { return data.begin(); }
	constexpr const_iterator begin() const { return data.begin(); }
	constexpr const_iterator cbegin() const { return data.cbegin(); }
	constexpr iterator end() { return data.end(); }
	constexpr const_iterator end() const { return data.end(); }
	constexpr const_iterator cend() const { return data.cend(); }

	constexpr reverse_iterator rbegin() { return reverse_iterator{data.end()}; }
	constexpr const_reverse_iterator rbegin() const { return const_reverse_iterator{data.end()}; }
	constexpr const_reverse_iterator crbegin() const { return const_reverse_iterator{data.end()}; }
	constexpr reverse_iterator rend() { return reverse_iterator{data.begin()}; }
	constexpr const_reverse_iterator rend() const { return const_reverse_iterator{data.begin()}; }
	constexpr const_reverse_iterator crend() const { return const_reverse_iterator{data.begin()}; }

};


