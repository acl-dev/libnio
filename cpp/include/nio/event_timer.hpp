//
// Created by shuxin zheng on 2025/2/14.
//

#pragma once

namespace nio {

class nio_event;

/**
 * @brief The event timer used in the nio event loop.
 */
class event_timer {
public:
    event_timer() = default;
    virtual ~event_timer() = default;

    /**
     * @brief The timer callback function to be called when the timer expires,
     *       the derived class should implement this function.
     */
    virtual void on_timer() = 0;

    /**
     * @brief Get the expire time of the timer.
     * @return long long 
     */
    long long get_expire() const {
        return stamp_;
    }

private:
    friend class nio_event;

    /**
     * @brief Set the expire time of the timer called in nio_event object.
     * @param when The expire time in milliseconds.
     */
    void set_expire(long long when);

private:
    long long stamp_ = 0;
};

} // namespace
