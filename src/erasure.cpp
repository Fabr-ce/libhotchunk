#include "leopard/LeopardCommon.h"

#include "hotstuff/util.h"
#include "hotstuff/erasure.h"
#include "hotstuff/consensus.h"

#include <string> 
#include <sstream> 
#include <iostream>

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

        uint64_t total_bytes = serialized_data.size();
        uint64_t buffer_bytes = ((total_bytes + original_count - 1) / original_count);
        if(buffer_bytes % 64 != 0){
            buffer_bytes += 64 - (buffer_bytes % 64);
        }
        // HOTSTUFF_LOG_INFO("Encoding len:%d bufferB:%d %s", total_bytes, buffer_bytes, uint8_vector_to_hex_string(serialized_data).c_str());
        
        std::vector<uint8_t*> original_data(original_count);
        std::vector<uint8_t*> encode_work_data(work_count);
        
        uint64_t offset = 0;
        for (unsigned i = 0, count = original_count; i < count; ++i){
            original_data[i] = leopard::SIMDSafeAllocate(buffer_bytes);
            uint64_t bytes_to_copy = std::min(buffer_bytes, total_bytes - offset);
            std::memset(original_data[i], 0, buffer_bytes);
            std::memcpy(original_data[i], &serialized_data[offset], bytes_to_copy);
            offset += bytes_to_copy;
        }
        for (unsigned i = 0, count = work_count; i < count; ++i) {
            encode_work_data[i] = leopard::SIMDSafeAllocate(buffer_bytes);
            std::memset(encode_work_data[i], 0, buffer_bytes);
        }


        LeopardResult encodeResult = leo_encode(
            buffer_bytes,                    // Number of bytes in each data buffer
            original_count,                  // Number of original_data[] buffer pointers
            parity_count,                   // Number of recovery_data[] buffer pointers
            work_count,                      // Number of work_data[] buffer pointers, from leo_encode_work_count()
            (void**)&original_data[0],      // Array of pointers to original data buffers
            (void**)&encode_work_data[0]);   // Array of work buffers

        if (encodeResult != Leopard_Success)
        {
            if (encodeResult == Leopard_TooMuchData)
            {
                HOTSTUFF_LOG_WARN("Skipping this test: Parameters are unsupported by the codec");
                return;
            }
            HOTSTUFF_LOG_WARN("Error: Leopard encode failed with result=%d: %s",encodeResult,leo_result_string(encodeResult));
            return;
        }  

        // create chunks
        for (int i = 0; i < config.nreplicas; ++i) {
            uint8_t * encodedData;
            if (i < original_count) {
                encodedData = original_data[i];
            } else {
                encodedData = encode_work_data[i - original_count];
            }
            std::vector<uint8_t> fragment_data = {encodedData, encodedData + buffer_bytes};
            //fragment_data.push_back(0);
            // HOTSTUFF_LOG_INFO("CREATE CHUNK %d: %s", i, uint8_vector_to_hex_string(fragment_data).c_str());
            DataStream fragment_stream(fragment_data);
            chunks.push_back(new BlockChunk(fragment_stream, blk->get_hash(), i));
        }
        

        // decode check
        /*
        std::vector<uint8_t> reconstructed_data;


        original_data[1] = nullptr;
        uint decode_work_count = leo_decode_work_count(original_count, parity_count);
        std::vector<uint8_t*> decode_work_data(decode_work_count);

        for (unsigned i = 0, count = decode_work_count; i < count; ++i)
            decode_work_data[i] = leopard::SIMDSafeAllocate(buffer_bytes);

        LeopardResult decodeResult = leo_decode(
            buffer_bytes,
            original_count,
            parity_count,
            decode_work_count,
            (void**)&original_data[0],
            (void**)&encode_work_data[0],
            (void**)&decode_work_data[0]);  
        
        if (decodeResult != Leopard_Success)
        {
            HOTSTUFF_LOG_WARN("Error: Leopard decode failed with result=%d: %s",decodeResult,leo_result_string(decodeResult));
            return;
        }
        
        for (unsigned i = 0; i < original_count; ++i)
        {
            if (original_data[i]){
                reconstructed_data.insert(reconstructed_data.end(), original_data[i], original_data[i] + buffer_bytes);
            } else {
                reconstructed_data.insert(reconstructed_data.end(), decode_work_data[i], decode_work_data[i] + buffer_bytes);
            }
        } 

        reconstructed_data.resize(total_bytes);

        std::string stream_str = uint8_vector_to_hex_string(serialized_data);
        std::string recon_str = uint8_vector_to_hex_string(reconstructed_data);


        if(reconstructed_data != serialized_data){
            HOTSTUFF_LOG_WARN("COMPARISON Failed %s, %s", std::string(stream_str).c_str(), std::string(recon_str).c_str());
        }else{
            HOTSTUFF_LOG_INFO("COMPARISON Success %s", std::string(stream_str).c_str());
        }
        for (unsigned i = 0; i < decode_work_count; ++i)
            leopard::SIMDSafeFree(decode_work_data[i]);
        
        */

        for (unsigned i = 0; i < original_count; ++i)
            leopard::SIMDSafeFree(original_data[i]);
        for (unsigned i = 0; i < work_count; ++i)
            leopard::SIMDSafeFree(encode_work_data[i]);
        
    }

    void ErasureCoding::reconstructBlock(std::unordered_map<const uint256_t, blockChunk_t> chunks, HotStuffCore* hsc, block_t blk){
        ReplicaConfig config = hsc->get_config();

        std::vector<uint8_t*> original_data(original_count);
        std::vector<uint8_t*> parity_data(parity_count);
        std::vector<uint8_t> reconstructed_data;

        uint64_t buffer_bytes = 0;


        uint decode_work_count = leo_decode_work_count(original_count, parity_count);
        std::vector<uint8_t*> decode_work_data(decode_work_count);


        for(auto& it: chunks){
            blockChunk_t chunk = it.second;
            bytearray_t buf = chunk->get_content();
            char* buf_data = vectorToCharArray(buf);
            
            if(buffer_bytes == 0){  
                buffer_bytes = buf.size();
            }

            uint16_t id = chunk->get_index();
            if(id < original_count){
                original_data[id] = (uint8_t*)buf_data;
            }else{
                parity_data[id-original_count] = (uint8_t*)buf_data;
            }
        }

        for (unsigned i = 0, count = decode_work_count; i < count; ++i)
            decode_work_data[i] = leopard::SIMDSafeAllocate(buffer_bytes);


        LeopardResult decodeResult = leo_decode(
            buffer_bytes,
            original_count,
            parity_count,
            decode_work_count,
            (void**)&original_data[0],
            (void**)&parity_data[0],
            (void**)&decode_work_data[0]);  
        
        if (decodeResult != Leopard_Success)
        {
            HOTSTUFF_LOG_WARN("Error: Leopard decode failed with result=%d: %s",decodeResult,leo_result_string(decodeResult));
            return;
        }
        
        for (unsigned i = 0; i < original_count; ++i)
        {
            if (original_data[i]){
                reconstructed_data.insert(reconstructed_data.end(), original_data[i], original_data[i] + buffer_bytes);
            } else {
                reconstructed_data.insert(reconstructed_data.end(), decode_work_data[i], decode_work_data[i] + buffer_bytes);
            }
        } 


        DataStream stream = DataStream(reconstructed_data);
        blk->unserialize(stream, hsc);


        for (unsigned i = 0; i < decode_work_count; ++i)
            leopard::SIMDSafeFree(decode_work_data[i]);
    }


}


