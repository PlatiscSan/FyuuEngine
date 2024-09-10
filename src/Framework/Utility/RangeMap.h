#ifndef RANGE_MAP_H
#define RANGE_MAP_H

#include <map>

namespace Fyuu::utility {

    template <class RangeType, class ValueType> class RangeMap {
    public:

        void AddRange(RangeType start, RangeType end, ValueType value) {

            auto range_pair = std::make_pair(start, end);

            auto iter = std::find_if(m_ranges.begin(), m_ranges.end(), 
                [&](std::pair<std::pair<RangeType, RangeType>, ValueType> const& p) {
                    return p.first == range_pair;
                }
            );

            if (iter != m_ranges.end()) {
                return;
            }

            m_ranges.emplace(std::make_pair(start, end), value);

        }

        ValueType GetValue(RangeType key) {

            auto iter = m_ranges.lower_bound(std::make_pair(key, RangeType()));

            if (iter != m_ranges.end() && key >= iter->first.first && key < iter->first.second) {
                return iter->second;
            }

            throw std::range_error("No range found for the given key");

        }

        ValueType operator[](RangeType key) {

            auto iter = m_ranges.lower_bound(std::make_pair(key, RangeType()));

            if (iter != m_ranges.end() && key >= iter->first.first && key < iter->first.second) {
                return iter->second;
            }

            throw std::range_error("No range found for the given key");

        }


    private:

        std::map<std::pair<RangeType, RangeType>, ValueType> m_ranges;

    };


}

#endif // !RANGE_MAP_H
