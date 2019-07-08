#pragma once

// visual c++ :(
#ifdef min
#undef min
#endif

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

class SampleCache
{
public:
    typedef std::vector<uint8_t> Buffer;
    typedef std::pair<size_t, size_t> Range;
    typedef std::map<size_t, size_t> EndStarts;
    typedef std::function<Range (size_t pos, uint8_t *into_begin, size_t into_size)> LoadSamplesIncludingFn;

    SampleCache(size_t numFrames, size_t frameSize, const LoadSamplesIncludingFn& loadSamplesIncludingFn)
        : numFrames_(numFrames),
          loadSamplesIncludingFn_(loadSamplesIncludingFn),
          buffer_(numFrames * frameSize),
          frameSize_(frameSize)
    {    
        loaded_[numFrames_] = numFrames_;
    }

    const uint8_t *get(Range range) const
    {
        // ensure range.start is overlapped
        auto cursor = range.first;
        do {
            cursor = getEndOfLoadedRangeIncluding(cursor);
        } while (cursor < range.second);

        // return an iterator to the start of the requested range
        return &(*(buffer_.begin() + range.first * frameSize_));
    }

    size_t numFrames() const { return numFrames_; }

private:
    size_t getEndOfLoadedRangeIncluding(size_t cursor) const
    {
        auto it = loaded_.upper_bound(cursor);
        if (it->second > cursor)
        {
            auto loaded = loadSamplesIncludingFn_(cursor, &buffer_[0], buffer_.size());
            return amalgamateAndReturnEnd(loaded);
        }
        return it->first;
    }

    size_t amalgamateAndReturnEnd(Range range) const
    {
        auto earliest_possible_overlap = loaded_.lower_bound(range.first); // at start or above
        auto latest_possible_overlap = loaded_.lower_bound(range.second);  // at end or above

        // extend range to include overlapped segments
        auto begin = std::min(range.first, earliest_possible_overlap->second);
        auto end = (range.second >= latest_possible_overlap->second) ? latest_possible_overlap->first : range.second;

        // remove any that overlap the extended range
        loaded_.erase(loaded_.lower_bound(begin), loaded_.upper_bound(end));

        // add the expanded range
        loaded_[end] = begin;

        return end;
    }

    size_t numFrames_;
    LoadSamplesIncludingFn loadSamplesIncludingFn_;
    mutable Buffer buffer_;
    size_t frameSize_;
    mutable EndStarts loaded_;
};