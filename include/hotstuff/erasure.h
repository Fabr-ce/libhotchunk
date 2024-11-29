#ifndef _HOTSTUFF_ERA_H
#define _HOTSTUFF_ERA_H

#include "leopard/leopard.h"

#include "hotstuff/util.h"
#include "hotstuff/entity.h"


namespace hotstuff {

class ErasureCoding {
    uint original_count;
    uint parity_count;
    uint work_count;

    public:
    ErasureCoding() {}


    void init(ReplicaConfig config){
        HOTSTUFF_LOG_INFO("Create erasure coding init");
        if (0 != leo_init())
        {
            HOTSTUFF_LOG_WARN("Failed to initialize");
        }
        original_count = config.nmajority;
        parity_count = config.nreplicas - config.nmajority;
        work_count = leo_encode_work_count(original_count, parity_count);
    }

    void createChunks(const block_t& blk, ReplicaConfig config, std::vector<blockChunk_t> &chunks);
    void reconstructBlock(std::unordered_map<const uint256_t, blockChunk_t> chunks, HotStuffCore* hsc, block_t blk);
};

}


#endif
