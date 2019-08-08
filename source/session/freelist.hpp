#ifndef FREELIST_HPP
#define FREELIST_HPP

// thread-safe freelist. T must be unique_ptr<>
template<class T>
class FreeList
{
public:
    FreeList(std::function<T ()> factory) : factory_(factory) {}

    T allocate()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if (!freeList_.empty())
        {
            T t = std::move(freeList_.back());
            freeList_.pop_back();
            return t;
        }
        else
            return factory_();
    }
    void free(T t)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        freeList_.push_back(std::move(t));
    }

private:
    std::mutex mutex_;
    std::vector<T> freeList_;
    std::function<T ()> factory_;
};

#endif