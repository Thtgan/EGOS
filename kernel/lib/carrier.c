#include<carrier.h>

#include<kit/util.h>
#include<memory/memory.h>

void carrier_carry(void* base, void* carryTo, Size n, CarrierMovMetadata** carryList) {
    memory_memcpy(carryTo, base, n);

    for (int i = 0; carryList[i] != NULL; ++i) {
        CarrierMovMetadata* metadata = carryList[i];

        if (metadata->magic != CARRIER_MOV_METADATA_MAGIC || !VALUE_WITHIN((Uintptr)base, (Uintptr)base + n, (Uintptr)metadata, <=, <)) {
            ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
        }

        void* carriedMetadataPtr = (void*)metadata - base + carryTo;

        switch(metadata->length) {
            case 16: {
                void* replacePtr = (carriedMetadataPtr + sizeof(CarrierMovMetadata) + metadata->instructionOffset);
                if (PTR_TO_VALUE(16, replacePtr) != CARRIER_MOV_DUMMY_ADDRESS_16) {
                    ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
                }

                PTR_TO_VALUE(16, replacePtr) = (Uint16)(carryTo + metadata->offset);
                break;
            }
            case 32: {
                void* replacePtr = (carriedMetadataPtr + sizeof(CarrierMovMetadata) + metadata->instructionOffset);
                if (PTR_TO_VALUE(32, replacePtr) != CARRIER_MOV_DUMMY_ADDRESS_32) {
                    ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
                }

                PTR_TO_VALUE(32, replacePtr) = (Uint32)(carryTo + metadata->offset);
                break;
            }
            case 64: {
                void* replacePtr = (carriedMetadataPtr + sizeof(CarrierMovMetadata) + metadata->instructionOffset);
                if (PTR_TO_VALUE(64, replacePtr) != CARRIER_MOV_DUMMY_ADDRESS_64) {
                    ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
                }

                PTR_TO_VALUE(64, replacePtr) = (Uint64)(carryTo + metadata->offset);
                break;
            }
            default: {
                ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
            }
        }
    }

    return;
    ERROR_FINAL_BEGIN(0);
}