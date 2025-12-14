#include<test.h>

#include<kit/bit.h>
#include<kit/macro.h>
#include<kit/types.h>

static bool __test_doExecList(TestList* list, void* ctx);

bool test_exec(TestGroup* group) {
    void* ctx = NULL;

    if (group->prepare != NULL) {
        ctx = group->prepare();
    }

    bool ret = __test_doExecList(group->rootList, ctx);
    
    if (group->clear != NULL) {
        group->clear(ctx);
    }

    return ret;
}

static bool __test_doExecList(TestList* list, void* ctx) {
    bool ret = true;
    
    for (int i = 0; ret && i < list->num; ++i) {
        if (TEST_FLAGS(list->isFuncBitmap, FLAG64(i))) {
            ret = list->entries[i].func(ctx);
        } else {
            ret = __test_doExecList(list->entries[i].subList, ctx);
        }
    }

    return ret;
}