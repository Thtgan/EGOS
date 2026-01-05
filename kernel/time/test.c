#include<kit/config.h>

#if defined(CONFIG_UNIT_TEST_TIME)

#include<time/timer.h>
#include<test.h>

typedef struct __TimeTestContext {
    Timer timer1, timer2;
    int val1, val2;
    bool success;
} __TimeTestContext;

static __TimeTestContext _time_test_context;

static void __time_test_timerFunc1(Timer* timer);

static void __time_test_timerFunc2(Timer* timer);

void* __time_timer_testGroupPrepare() {
    Timer* timer1 = &_time_test_context.timer1, * timer2 = &_time_test_context.timer2;

    timer_initStruct(timer1, 1, TIME_UNIT_SECOND);
    timer_initStruct(timer2, 500, TIME_UNIT_MILLISECOND);
    SET_FLAG_BACK(timer2->flags, TIMER_FLAGS_SYNCHRONIZE | TIMER_FLAGS_REPEAT);

    timer1->handler = __time_test_timerFunc1;
    timer1->data = (Object)&_time_test_context;
    timer2->handler = __time_test_timerFunc2;
    timer2->data = (Object)&_time_test_context;

    _time_test_context.val1 = _time_test_context.val2 = 0;
    _time_test_context.success = true;

    return &_time_test_context;
}

static bool __time_test_timer_run(void* arg) {
    __TimeTestContext* ctx = (__TimeTestContext*)arg;

    Timer* timer1 = &ctx->timer1, * timer2 = &ctx->timer2;

    timer_start(timer1);
    timer_start(timer2);

    if (!(ctx->val1 == 1 && ctx->val2 == 5)) {
        ctx->success = false;
    }

    return ctx->success;
}

TEST_SETUP_LIST(
    TIME_TIMER,
    (1, __time_test_timer_run)
);

TEST_SETUP_LIST(    //TODO: Add test for clocks
    TIME,
    (0, &TEST_LIST_FULL_NAME(TIME_TIMER))
);

TEST_SETUP_GROUP(time_testGroup, EMPTY_FLAGS, __time_timer_testGroupPrepare, TIME, NULL);

static void __time_test_timerFunc1(Timer* timer) {
    __TimeTestContext* ctx = (__TimeTestContext*)timer->data;
    ctx->val1 = 1;
}

static void __time_test_timerFunc2(Timer* timer) {
    __TimeTestContext* ctx = (__TimeTestContext*)timer->data;
    ++ctx->val2;
    
    if (ctx->val2 == 3) {
        ctx->success = (ctx->val1 > 0);
    } else if (ctx->val2 == 5) {
        CLEAR_FLAG_BACK(timer->flags, TIMER_FLAGS_REPEAT);
    }
}

#endif