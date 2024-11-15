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

     /*

    void ErasureCoding::createChunks(const block_t& blk, ReplicaConfig config, std::vector<blockChunk_t> &chunks){
    
        for (uint32_t i = 0; i < config.nreplicas; ++i) {
            DataStream stream;
            blk->serialize(stream);
            chunks.push_back(new BlockChunk(bytearray_t(stream), blk->get_hash(), i));
        }
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
    */

   
    
    void ErasureCoding::createChunks(const block_t& blk, ReplicaConfig config, std::vector<blockChunk_t> &chunks){
        DataStream stream;
        blk->serialize(stream);  // Serialize block into a data stream
        bytearray_t serialized_data = stream;  // Serialized data to be encoded

        char * original_data = vectorToCharArray(serialized_data);

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
            DataStream fragment_stream(fragment_data);
            chunks.push_back(new BlockChunk(fragment_stream, blk->get_hash(), i));
        }


    }

    void ErasureCoding::reconstructBlock(std::unordered_map<const uint256_t, blockChunk_t> chunks, HotStuffCore* hsc, block_t blk){
        if(chunks.size() < hsc->get_config().nmajority) {
            throw std::runtime_error("cannot reconstruct block");
        }


        uint64_t decoded_data_len = 0;
        uint64_t encoded_fragment_len = 0;
        char *decoded_data = NULL;
        char **avail_frags = NULL;


        int chunkIndex = 0;
        for(auto& it: chunks){
            blockChunk_t chunk = it.second;
            bytearray_t buf = chunk->get_content();
            char* buf_data = vectorToCharArray(buf);
            
            if(encoded_fragment_len == 0){  
                encoded_fragment_len = buf.size();
                avail_frags = (char **)malloc(chunks.size() * sizeof(char *));
            }
            avail_frags[chunkIndex] = buf_data;
            chunkIndex++;
        }

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


