#include "erasurecode/erasurecode.h"


#include "hotstuff/util.h"
#include "hotstuff/erasure.h"
#include "hotstuff/consensus.h"

namespace hotstuff {
    void ErasureCoding::createChunks(const block_t& blk, ReplicaConfig config, std::vector<blockChunk_t> &chunks){
        DataStream stream;
        blk->serialize(stream);  // Serialize block into a data stream
        bytearray_t serialized_data = stream;  // Serialized data to be encoded
        
        struct ec_args args;
        args.k = config.nmajority; 
        args.m = config.nreplicas - config.nmajority;  
        args.w = 8;  // Word size

        int ec_handle = liberasurecode_instance_create(EC_BACKEND_JERASURE_RS_VAND, &args);

        char** encoded_data = (char **)malloc(2 * serialized_data.size() * sizeof(char*));
        char** encoded_parity = (char **)malloc(2 * serialized_data.size() * sizeof(char*));

        uint64_t fragment_len = 0;

        if (liberasurecode_encode(ec_handle, reinterpret_cast<char *>(serialized_data.data()), serialized_data.size(), reinterpret_cast<char ***>(&encoded_data), reinterpret_cast<char ***>(&encoded_parity), &fragment_len) != 0) {
            throw std::runtime_error("Failed to encode data");
        }

        // Create chunks from encoded data
        for (int i = 0; i < config.nreplicas; ++i) {
            std::vector<uint8_t> fragment_data(fragment_len);
            if (i < args.k) {
                std::memcpy(fragment_data.data(), encoded_data[i], fragment_len);
            } else {
                std::memcpy(fragment_data.data(), encoded_parity[i - args.k], fragment_len);
            }
            DataStream fragment_stream(fragment_data);
            chunks.push_back(new BlockChunk(fragment_stream, blk->get_hash()));
        }

        // Free encoded data memory
        free(encoded_data);
        free(encoded_parity);

        
        /*
        for (uint32_t i = 0; i < config.nreplicas; ++i) {
            DataStream stream;
            blk->serialize(stream);
            chunks.push_back(new BlockChunk(bytearray_t(stream), blk->get_hash()));
        }
        */
    }

    void ErasureCoding::reconstructBlock(std::unordered_map<const uint256_t, blockChunk_t> chunks, HotStuffCore* hsc, block_t blk){
        if (chunks.size() >= hsc->get_config().nmajority){
            blockChunk_t blkChunk = chunks.begin()->second;

            DataStream stream = DataStream(std::move(blkChunk->get_content()));
            blk->unserialize(stream, hsc);

            return;
        }


        throw std::runtime_error("cannot reconstruct block");
    }
}