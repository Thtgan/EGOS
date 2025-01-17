#include<carrier.h>

#include<kit/util.h>
#include<memory/memory.h>

OldResult carrier_carry(void* base, void* carryTo, Size n, CarrierMovMetadata** carryList) {
    memory_memcpy(carryTo, base, n);

    for (int i = 0; carryList[i] != NULL; ++i) {
        CarrierMovMetadata* metadata = carryList[i];

        if (metadata->magic != CARRIER_MOV_METADATA_MAGIC || !VALUE_WITHIN((Uintptr)base, (Uintptr)base + n, (Uintptr)metadata, <=, <)) {
            return RESULT_ERROR;
        }

        void* carriedMetadataPtr = (void*)metadata - base + carryTo;

        switch(metadata->length) {
            case 16: {
                void* replacePtr = (carriedMetadataPtr + sizeof(CarrierMovMetadata) + metadata->instructionOffset);
                if (PTR_TO_VALUE(16, replacePtr) != CARRIER_MOV_DUMMY_ADDRESS_16) {
                    return RESULT_ERROR;
                }

                PTR_TO_VALUE(16, replacePtr) = (Uint16)(carryTo + metadata->offset);
                break;
            }
            case 32: {
                void* replacePtr = (carriedMetadataPtr + sizeof(CarrierMovMetadata) + metadata->instructionOffset);
                if (PTR_TO_VALUE(32, replacePtr) != CARRIER_MOV_DUMMY_ADDRESS_32) {
                    return RESULT_ERROR;
                }

                PTR_TO_VALUE(32, replacePtr) = (Uint32)(carryTo + metadata->offset);
                break;
            }
            case 64: {
                void* replacePtr = (carriedMetadataPtr + sizeof(CarrierMovMetadata) + metadata->instructionOffset);
                if (PTR_TO_VALUE(64, replacePtr) != CARRIER_MOV_DUMMY_ADDRESS_64) {
                    return RESULT_ERROR;
                }

                PTR_TO_VALUE(64, replacePtr) = (Uint64)(carryTo + metadata->offset);
                break;
            }
            default: {
                return RESULT_ERROR;
            }
        }
    }

    return RESULT_SUCCESS;
}