#ifndef _HOTSTUFF_ERA_H
#define _HOTSTUFF_ERA_H

#include "erasurecode/erasurecode.h"

#include "hotstuff/util.h"
#include "hotstuff/entity.h"


namespace hotstuff {

class ErasureCoding {
    struct ec_args args;
    int desc;

    public:
    ErasureCoding() {}


    void init(ReplicaConfig config){
        HOTSTUFF_LOG_INFO("Create erasure coding init");
        args = {
            .k = int(config.nmajority),
            .m = int(config.nreplicas - config.nmajority),
            .w = 8,
            .hd = int(config.nreplicas - config.nmajority),
            .ct = CHKSUM_NONE,
        };
        desc = liberasurecode_instance_create(EC_BACKEND_LIBERASURECODE_RS_VAND, &args);

        if (-EBACKENDNOTAVAIL == desc) {
            fprintf(stderr, "Backend library not available!\n");
            return;
        } else if ((args.k + args.m) > EC_MAX_FRAGMENTS) {
            assert(-EINVALIDPARAMS == desc);
            return;
        } else
            assert(desc > 0);
    }

    void createChunks(const block_t& blk, ReplicaConfig config, std::vector<blockChunk_t> &chunks);
    void reconstructBlock(std::unordered_map<const uint256_t, blockChunk_t> chunks, HotStuffCore* hsc, block_t blk);
};

}


#endif
