#ifndef _HOTSTUFF_ERA_H
#define _HOTSTUFF_ERA_H


#include "hotstuff/entity.h"


namespace hotstuff {

class ErasureCoding {
    public:
    static void createChunks(const block_t& blk, ReplicaConfig config, std::vector<blockChunk_t> &chunks);
    static void reconstructBlock(std::unordered_map<const uint256_t, blockChunk_t> chunks, HotStuffCore* hsc, block_t blk);
};

}


#endif
