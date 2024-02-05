#include "utils.h"
#include "aes.h"

namespace esphome {
namespace wmbus {

  static const char *TAG = "utils";

  uint16_t byteSize(uint16_t t_packetSize) {
    // In T-mode data is 3 out of 6 coded.
    uint16_t size = (( 3 * t_packetSize) / 2);

    // If packetSize is a odd number 1 extra byte   
    // that includes the 4-postamble sequence must be
    // read.    
    if (t_packetSize % 2) {
      return (size + 1);
    }
    else {
      return (size);
    }
  }

  uint16_t packetSize(uint8_t t_L) {
    uint16_t nrBytes;
    uint8_t  nrBlocks;

    // The 2 first blocks contains 25 bytes when excluding CRC and the L-field
    // The other blocks contains 16 bytes when excluding the CRC-fields
    // Less than 26 (15 + 10) 
    if ( t_L < 26 ) {
      nrBlocks = 2;
    }
    else { 
      nrBlocks = (((t_L - 26) / 16) + 3);
    }

    // Add all extra fields, excluding the CRC fields
    nrBytes = t_L + 1;

    // Add the CRC fields, each block is contains 2 CRC bytes
    nrBytes += (2 * nrBlocks);

    return nrBytes;
  }

  unsigned char *safeButUnsafeVectorPtr(std::vector<unsigned char> &v) {
    if (v.size() == 0) {
      return NULL;
    }
    else {
      return &v[0];
    }
  }

  void xorit(unsigned char *srca, unsigned char *srcb, unsigned char *dest, int len) {
    for (int i=0; i<len; ++i) {
      dest[i] = srca[i]^srcb[i];
    }
  }

  void incrementIV(unsigned char *iv, int len) {
    unsigned char *p = iv+len-1;
    while (p >= iv) {
      int pp = *p;
      (*p)++;
      if (pp+1 <= 255) {
        // Nice, no overflow. We are done here!
        break;
      }
      // Move left add add one.
      p--;
    }
  }

  bool decrypt_ELL_AES_CTR(std::vector<unsigned char> &frame,
                           std::vector<unsigned char> &aeskey) {
    unsigned char iv[16]{0};
    int i{0};
    int offset{0};
    int ci_field = frame[10];

    switch(ci_field) {
      case 0x8D: // ELL
        offset = 17;
        // tpl-mfct + tpl-id + tpl-version + tpl-type
        for (int j=0; j<8; ++j) {
          iv[i++] = frame[2+j];
        }
        // ell-cc
        for (int j=0; j<1; ++j) {
          iv[i++] = frame[11+j];
        }
        // aes-ctr
        for (int j=0; j<4; ++j) {
          iv[i++] = frame[13+j];
        }
        break;    
      default:
        ESP_LOGE(TAG, "(ELL) unknown CI field [%02X]", ci_field);
        return false;
        break;
    }

    if (aeskey.size() == 0) {
      return true;
    }

    ESP_LOGV(TAG, "(ELL)  CI: %02X  offset: %d", ci_field, offset);
    ESP_LOGV(TAG, "(ELL)  IV: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                   iv[0], iv[1], iv[2],  iv[3],  iv[4],  iv[5],  iv[6],  iv[7],
                   iv[8], iv[9], iv[10], iv[11], iv[12], iv[13], iv[14], iv[15]);
    std::string key = format_hex_pretty(aeskey);
    key.erase(std::remove(key.begin(), key.end(), '.'), key.end());
    ESP_LOGV(TAG, "(ELL) KEY: %s", key.c_str());

    std::vector<unsigned char>::iterator pos = frame.begin() + offset;
    std::vector<unsigned char> encrypted_bytes;
    std::vector<unsigned char> decrypted_bytes;
    encrypted_bytes.insert(encrypted_bytes.end(), pos, frame.end());
    
    std::string en_bytes = format_hex_pretty(encrypted_bytes);
    en_bytes.erase(std::remove(en_bytes.begin(), en_bytes.end(), '.'), en_bytes.end());
    ESP_LOGD(TAG, "(ELL) AES_CTR decrypting: %s", en_bytes.c_str());

    int block = 0;
    for (size_t offset = 0; offset < encrypted_bytes.size(); offset += 16) {
      size_t block_size = 16;
      if (offset + block_size > encrypted_bytes.size()) {
        block_size = encrypted_bytes.size() - offset;
      }

      assert(block_size > 0 && block_size <= 16);

      // Generate the pseudo-random bits from the IV and the key.
      unsigned char xordata[16];
      AES_ECB_encrypt(iv, safeButUnsafeVectorPtr(aeskey), xordata, 16);

      // Xor the data with the pseudo-random bits to decrypt into tmp.
      unsigned char tmp[block_size];
      xorit(xordata, &encrypted_bytes[offset], tmp, block_size);

      block++;

      std::vector<unsigned char> tmpv(tmp, tmp+block_size);
      decrypted_bytes.insert(decrypted_bytes.end(), tmpv.begin(), tmpv.end());
      incrementIV(iv, sizeof(iv));
    }

      std::string dec_bytes = format_hex_pretty(decrypted_bytes);
      dec_bytes.erase(std::remove(dec_bytes.begin(), dec_bytes.end(), '.'), dec_bytes.end());
      ESP_LOGD(TAG, "(ELL) AES_CTR  decrypted: %s", dec_bytes.c_str());

    // Remove the encrypted bytes.
    frame.erase(pos, frame.end());
    // Insert the decrypted bytes.
    frame.insert(frame.end(), decrypted_bytes.begin(), decrypted_bytes.end());
    return true;
  }


  bool decrypt_TPL_AES_CBC_IV(std::vector<unsigned char> &frame,
                              std::vector<unsigned char> &aeskey) {
    unsigned char iv[16]{0};
    int i{0};
    int offset{0};
    int ci_field = frame[10];

    switch(ci_field) {
      case 0x67: // short TPL
      case 0x6E:
      case 0x74:
      case 0x7A:
      case 0x7D:
      case 0x7F:
      case 0x9E:
        offset = 15;
        // dll-mfct + dll-id + dll-version + dll-type
        for (int j=0; j<8; ++j) {
          iv[i++] = frame[2+j];
        }
        // tpl-acc
        for (int j=0; j<8; ++j) {
          iv[i++] = frame[11];
        }
        break;
      case 0x68: // long TPL
      case 0x6F:
      case 0x72:
      case 0x75:
      case 0x7C:
      case 0x7E:
      case 0x9F:
        offset = 23;
        // tpl-mfct
        for (int j=0; j<2; ++j) {
          iv[i++] = frame[15+j];
        }
        // tpl-id
        for (int j=0; j<4; ++j) {
          iv[i++] = frame[11+j];
        }
        // tpl-version + tpl-type
        for (int j=0; j<2; ++j) {
          iv[i++] = frame[17+j];
        }
        // tpl-acc
        for (int j=0; j<8; ++j) {
          iv[i++] = frame[19];
        }
        break;
      
      default:
        ESP_LOGE(TAG, "(TPL) unknown CI field [%02X]", ci_field);
        return false;
        break;
    }

    if (aeskey.size() == 0) {
      return true;
    }

    ESP_LOGV(TAG, "(TPL)  CI: %02X  offset: %d", ci_field, offset);
    ESP_LOGV(TAG, "(TPL)  IV: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                   iv[0], iv[1], iv[2],  iv[3],  iv[4],  iv[5],  iv[6],  iv[7],
                   iv[8], iv[9], iv[10], iv[11], iv[12], iv[13], iv[14], iv[15]);
    std::string key = format_hex_pretty(aeskey);
    key.erase(std::remove(key.begin(), key.end(), '.'), key.end());
    ESP_LOGV(TAG, "(TPL) KEY: %s", key.c_str());

    std::vector<unsigned char>::iterator pos = frame.begin() + offset;
    std::vector<unsigned char> buffer;
    buffer.insert(buffer.end(), pos, frame.end());

    size_t num_bytes_to_decrypt = frame.end()-pos;

    uint8_t tpl_num_encr_blocks = ((uint8_t)frame[13] >> 4) & 0x0f; // check if true for both short and long
    if (tpl_num_encr_blocks) {
      num_bytes_to_decrypt = tpl_num_encr_blocks*16;
    }

    if (buffer.size() < num_bytes_to_decrypt) {
      num_bytes_to_decrypt = buffer.size();
      // We must have at least 16 bytes to decrypt. Give up otherwise.
      if (num_bytes_to_decrypt < 16) {
        return false;
      }
    }

    std::string dec_buffer = format_hex_pretty(buffer);
    dec_buffer.erase(std::remove(dec_buffer.begin(), dec_buffer.end(), '.'), dec_buffer.end());
    ESP_LOGD(TAG, "(TPL) AES CBC IV decrypting: %s", dec_buffer.c_str());

    // The content should be a multiple of 16 since we are using AES CBC mode.
    if (num_bytes_to_decrypt % 16 != 0) {
      num_bytes_to_decrypt -= num_bytes_to_decrypt % 16;
      assert (num_bytes_to_decrypt % 16 == 0);
      // There must be at least 16 bytes remaining.
      if (num_bytes_to_decrypt < 16) {
        return false;
      }
    }

    unsigned char buffer_data[num_bytes_to_decrypt];
    memcpy(buffer_data, safeButUnsafeVectorPtr(buffer), num_bytes_to_decrypt);
    unsigned char decrypted_data[num_bytes_to_decrypt];

    AES_CBC_decrypt_buffer(decrypted_data,
                          buffer_data,
                          num_bytes_to_decrypt,
                          safeButUnsafeVectorPtr(aeskey),
                          iv);

    // Remove the encrypted bytes.
    frame.erase(pos, frame.end());
    // Insert the decrypted bytes.
    frame.insert(frame.end(), decrypted_data, decrypted_data+num_bytes_to_decrypt);

    std::vector<unsigned char> dec_data(decrypted_data, decrypted_data + num_bytes_to_decrypt);
    std::string dec_bytes = format_hex_pretty(dec_data);
    dec_bytes.erase(std::remove(dec_bytes.begin(), dec_bytes.end(), '.'), dec_bytes.end());
    ESP_LOGD(TAG, "(TPL) AES CBC IV  decrypted: %s", dec_bytes.c_str());

    if (num_bytes_to_decrypt < buffer.size()) {
      frame.insert(frame.end(), buffer.begin()+num_bytes_to_decrypt, buffer.end());
    }

    uint32_t decrypt_check = 0x2F2F;
    uint32_t dc = (((uint16_t)frame[offset] << 8) | (frame[offset+1]));
    if ( dc == decrypt_check) {
      ESP_LOGI(TAG, "2F2f check after decrypting - OK");
    }
    else {
      ESP_LOGE(TAG, "2F2f check after decrypting - NOT OK");
      return false;
    }
    return true;
  }

}
}