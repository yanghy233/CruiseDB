// Author:RUC liang_jk

#pragma once

#include <algorithm>
#include <atomic>
#include <queue>
#include <sys/time.h>
#include "util/mutexlock.h"

namespace rocksdb {

#define DEFAULT_REFILL_PERIOD (100 * 1000)

class ColumnFamilyData;
class RateEstimater;
class DBImpl;

class TokenBucket {
    public:
        explicit TokenBucket(ColumnFamilyData* cfd, long long refill_period_us = DEFAULT_REFILL_PERIOD);
        explicit TokenBucket(ColumnFamilyData* cfd, long long rate_bytes_per_sec, long long refill_period_us, bool auto_tune = false);

        ~TokenBucket();

        void Begin(int code, DBImpl*);

        void Request(); 
        void Request(long long bytes);

        void Record(int type, long long bytes, long long begin_time);
        void Record(int type, long long begin_time);

        long long GetSingleBurstBytes() const {
            MutexLock mu(&request_mutex_);
            return refill_bytes_per_period_;
        };

        long long GetTotalBytesThrough() const {
            MutexLock mu(&request_mutex_);
            return total_bytes_through_;
        };

        long long GetTotalRequests() const{
            MutexLock mu(&request_mutex_);
            return total_requests_;
        };

        long long GetBytesPerSecond () const {
            MutexLock mu(&request_mutex_);
            return rate_bytes_per_sec_;
        };

        long long GetTime() const {
            return NowTime();
        };

    private:
        void Refill();

        long long NowTime() const {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return (long long)(tv.tv_sec)*1000000 + tv.tv_usec;
        };

        long long CalculateRefillBytesPerPeriod(long long rate_bytes_per_sec);

        void SetBytesPerSecond(long long bytes_per_second);

        ColumnFamilyData* cfd_;

        int valid_;

        // 令牌桶的填充速率(字节/秒)
        long long rate_bytes_per_sec_;

        // 令牌桶的填充周期(微秒)
        const long long refill_period_us_;

        // 填充周期内填充的总字节数
        long long refill_bytes_per_period_;

        // 填充周期内最小填充字节数
        const long long kMinRefillBytesPerPeriod = 100;

        bool stop_;
        port::CondVar exit_cv_;

        // 表示需要等待的请求数量
        int requests_to_wait_;

        // 当前可用的字节数
        long long available_bytes_;

        // 下次填充的时间（微秒）
        long long next_refill_us_;

        // 互斥锁，用于保护对令牌桶状态的访问
        mutable port::Mutex request_mutex_;

        // 总请求数
        long long total_requests_;

        // 总通过的字节数
        long long total_bytes_through_;

        // 存储等待的条件变量
        std::queue<port::CondVar*> queue_;

        // 调整周期
        int tune_period_;

        // 上次调整的时间
        long long tune_time_;

        // 上次调整时的字节数
        long long tune_bytes_;

        //用于估算速率
        RateEstimater* rate_estimater_;
        bool ShouldTune();

};

// extern TokenBucket token_bucket;

}
