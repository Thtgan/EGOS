#include<memory/memoryOperations.h>

#include<memory/defaultOperations/anon.h>
#include<memory/defaultOperations/copy.h>
#include<memory/defaultOperations/cow.h>
#include<memory/defaultOperations/file.h>
#include<memory/defaultOperations/mixed.h>
#include<memory/defaultOperations/share.h>
#include<memory/extendedPageTable.h>
#include<memory/mm.h>
#include<debug.h>
#include<error.h>

static MemoryOperations* __memoryOperations_defaultOperations[DEFAULT_MEMORY_OPERATIONS_TYPE_NUM] = {
    [DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE]          = &defaultMemoryOperations_share,
    [DEFAULT_MEMORY_OPERATIONS_TYPE_COPY]           = &defaultMemoryOperations_copy,
    [DEFAULT_MEMORY_OPERATIONS_TYPE_MIXED]          = &defaultMemoryOperations_mixed,
    [DEFAULT_MEMORY_OPERATIONS_TYPE_COW]            = &defaultMemoryOperations_cow,
    [DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_PRIVATE]   = &defaultMemoryOperations_anon_private,
    [DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_SHARED]    = &defaultMemoryOperations_anon_shared,
    [DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE]   = &defaultMemoryOperations_file_private,
    [DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED]    = &defaultMemoryOperations_file_shared
};

void memoryOperations_registerDefault(ExtraPageTableContext* context) {
    for (int i = 0; i < DEFAULT_MEMORY_OPERATIONS_TYPE_NUM; ++i) {
        MemoryOperations* operations = __memoryOperations_defaultOperations[i];
        Index8 index = extraPageTableContext_registerMemoryOperations(&mm->extraPageTableContext, operations);
        DEBUG_ASSERT_SILENT(index == i && extraPageTableContext_getMemoryOperations(context, i) == __memoryOperations_defaultOperations[i]);
        ERROR_GOTO_IF_ERROR(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}