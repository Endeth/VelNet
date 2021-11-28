#pragma once
#include "netCommon.h"
#include <mutex>

namespace Vel
{
    namespace Net
    {
        template<typename T>
        class ThreadSafeQueue
        {
        public:
            ThreadSafeQueue() = default;
            ThreadSafeQueue( const ThreadSafeQueue<T>& ) = delete;
            virtual ~ThreadSafeQueue()
            {
                clear();
            }

            const T& front()
            {
                std::scoped_lock lock( mutex );
                return deque.front();
            }

            const T& back()
            {
                std::scoped_lock lock( mutex );
                return deque.back();
            }

            void push_back( const T& item )
            {
                std::scoped_lock lock( mutex );
                deque.emplace_back( std::move( item ) );
            }

            void push_front( const T& item )
            {
                std::scoped_lock lock( mutex );
                deque.emplace_back( std::move( item ) );
            }

            T pop_front()
            {
                std::scoped_lock lock( mutex );
                auto t = std::move( deque.front() );
                deque.pop_front();
                return t;
            }

            T pop_back()
            {
                std::scoped_lock lock( mutex );
                auto t = std::move( deque.back() );
                deque.pop_back();
                return t;
            }

            void clear()
            {
                std::scoped_lock lock( mutex );
                deque.clear();
            }

            bool empty()
            {
                std::scoped_lock lock( mutex );
                return deque.empty();
            }

            size_t count()
            {
                std::scoped_lock lock( mutex );
                return deque.count();
            }
            
        protected:
            std::mutex mutex;
            std::deque<T> deque;
        };
    }
}