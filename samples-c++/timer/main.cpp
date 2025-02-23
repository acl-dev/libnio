#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <ctime>

#include "nio/nio_event.hpp"
#include "nio/event_timer.hpp"

class mytimer : public nio::event_timer {
public:
    mytimer(nio::nio_event& ev, int id, int cnt)
    : ev_(ev), id_(id), cnt_(cnt)
    {
        begin_ = time(nullptr);
    }

protected:
    void on_timer() override {
        loop_++;
        printf("Timer-%d called, tc: %ld, loop=%d\r\n",
               id_, time(nullptr) - begin_, loop_);
        if (loop_ >= cnt_) {
            delete this;
        } else {
            ev_.add_timer(this, 1000);
        }
    }

private:
    ~mytimer() override = default;

    nio::nio_event& ev_;
    int id_;
    int cnt_;
    int loop_ = 0;
    time_t begin_;
};

static void usage(const char *procname) {
    printf("usage: %s -h [help] -n count\r\n", procname);
}

int main(int argc, char *argv[]) {
    int ch, cnt = 1;

    while ((ch = getopt(argc, argv, "hn:")) > 0) {
        switch (ch) {
            case 'h':
                usage(argv[0]);
                return 0;
            case 'n':
                cnt = std::atoi(optarg);
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }

    nio::nio_event ev(10240);

    for (int i = 1; i <= 10; i++) {
        auto* timer = new mytimer(ev, i, cnt);
        ev.add_timer(timer, 1000);
    }

    while (true) {
        ev.wait(1000);
    }

    return 0;
}
