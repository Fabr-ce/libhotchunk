#include "erasurecode/erasurecode.h"


#include <string> 
#include <sstream> 
#include <iostream>

#include "hotstuff/util.h"
#include "hotstuff/erasure.h"
#include "hotstuff/consensus.h"

namespace hotstuff {

    char* vectorToCharArray(const std::vector<uint8_t>& vec) {
        // Allocate memory with an extra byte for the null terminator
        char* charArray = new char[vec.size() + 1];

        // Copy vector data into the allocated memory
        std::memcpy(charArray, vec.data(), vec.size());

        // Add the null terminator at the end
        charArray[vec.size()] = '\0';

        return charArray;
    }

    std::string uint8_vector_to_hex_string(const std::vector<uint8_t>& vec) {
        std::ostringstream oss;
        oss << "{";
        for (size_t i = 0; i < vec.size(); ++i) {
            oss << static_cast<int>(vec[i]);
            if (i < vec.size() - 1) {
                oss << ", ";
            }
        }
        oss << "}";
        return oss.str();
    }

    void ErasureCoding::createChunks(const block_t& blk, ReplicaConfig config, std::vector<blockChunk_t> &chunks){
        DataStream stream;
        blk->serialize(stream);  // Serialize block into a data stream
        bytearray_t serialized_data = stream;  // Serialized data to be encoded

        std::string str = uint8_vector_to_hex_string(serialized_data);
        char * original_data = vectorToCharArray(serialized_data);
        
        struct ec_args args = {
            .k = config.nmajority,
            .m = config.nreplicas - config.nmajority,
            .w = 8,
            .hd = config.nreplicas - config.nmajority,
            .ct = CHKSUM_NONE,
        };

        int desc = liberasurecode_instance_create(EC_BACKEND_LIBERASURECODE_RS_VAND, &args);

        if (-EBACKENDNOTAVAIL == desc) {
            fprintf(stderr, "Backend library not available!\n");
            return;
        } else if ((args.k + args.m) > EC_MAX_FRAGMENTS) {
            assert(-EINVALIDPARAMS == desc);
            return;
        } else
            assert(desc > 0);

        char **encoded_data = NULL, **encoded_parity = NULL;
        uint64_t encoded_fragment_len = 0;


        int rc = liberasurecode_encode(desc, original_data, serialized_data.size(),
            &encoded_data, &encoded_parity, &encoded_fragment_len);
        assert(0 == rc);

        char** decode_test = (char**)malloc(config.nreplicas * sizeof(char *));

        // Create chunks from encoded data
        for (int i = 0; i < config.nreplicas; ++i) {
            char* encodedData;
            if (i < args.k) {
                encodedData = encoded_data[i];
            } else {
                encodedData = encoded_parity[i - args.k];
            }
            std::vector<uint8_t> fragment_data = {encodedData, encodedData + encoded_fragment_len};
            std::string str = uint8_vector_to_hex_string(fragment_data);
            DataStream fragment_stream(fragment_data);
            chunks.push_back(new BlockChunk(fragment_stream, blk->get_hash(), i));
        }

    }

    void ErasureCoding::reconstructBlock(std::unordered_map<const uint256_t, blockChunk_t> chunks, HotStuffCore* hsc, block_t blk){
        if(chunks.size() < hsc->get_config().nmajority) {
            throw std::runtime_error("cannot reconstruct block");
        }

        ReplicaConfig config = hsc->get_config();

        uint64_t decoded_data_len = 0;
        uint64_t encoded_fragment_len = 0;
        char *decoded_data = NULL;
        char **avail_frags = NULL;


        int chunkIndex = 0;
        for(auto& it: chunks){
            blockChunk_t chunk = it.second;
            bytearray_t buf = chunk->get_content();
            std::string str = uint8_vector_to_hex_string(buf);
            char* buf_data = vectorToCharArray(buf);
            
            if(encoded_fragment_len == 0){  
                encoded_fragment_len = buf.size();
                avail_frags = (char **)malloc(chunks.size() * sizeof(char *));
            }
            avail_frags[chunkIndex] = buf_data;
            chunkIndex++;
        }


        struct ec_args args = {
            .k = config.nmajority,
            .m = config.nreplicas - config.nmajority,
            .w = 8,
            .hd = config.nreplicas - config.nmajority,
            .ct = CHKSUM_NONE,
        };

        int desc = liberasurecode_instance_create(EC_BACKEND_LIBERASURECODE_RS_VAND, &args);
        if (-EBACKENDNOTAVAIL == desc) {
            fprintf(stderr, "Backend library not available!\n");
            return;
        } else if ((args.k + args.m) > EC_MAX_FRAGMENTS) {
            assert(-EINVALIDPARAMS == desc);
            return;
        } else
            assert(desc > 0);


        int num_avail_frags = chunks.size();
        int rc = liberasurecode_decode(desc, avail_frags, num_avail_frags,
                               encoded_fragment_len, 1,
                               &decoded_data, &decoded_data_len);
        assert(0 == rc);

        bytearray_t block_data = {decoded_data, decoded_data + decoded_data_len};
        DataStream stream = DataStream(block_data);
        blk->unserialize(stream, hsc);

        return;
    }


}


