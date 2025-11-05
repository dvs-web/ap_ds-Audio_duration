// audio_parser.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// OGG相关定义
#define OGG_PAGE_HEADER "OggS"

typedef struct {
    long long file_size;
    unsigned int sample_rate;
    unsigned int total_pages;
    long long first_granule_position;
    long long last_granule_position;
} OGGInfo;

typedef struct {
    unsigned char capture_pattern[4];
    unsigned char version;
    unsigned char header_type;
    unsigned long long granule_position;
    unsigned int bitstream_serial;
    unsigned int page_sequence;
    unsigned int checksum;
    unsigned char page_segments;
} OGGPageHeader;

// FLAC相关定义
#define FLAC_SIGNATURE "fLaC"

typedef struct {
    unsigned int sample_rate;
    unsigned int channels;
    unsigned int bits_per_sample;
    unsigned long long total_samples;
    double duration;
} FLACInfo;

// MP3相关定义
typedef struct {
    double mpeg_version;
    int layer;
    int bitrate;
    int sample_rate;
    int frame_size;
    int padding;
    int protection;
} MP3FrameHeader;

typedef struct {
    double duration;
    int sample_rate;
    int bitrate;
    int is_vbr;
} MP3Info;

// WAV相关定义
#define WAV_RIFF_HEADER "RIFF"
#define WAV_WAVE_HEADER "WAVE"
#define WAV_FMT_HEADER  "fmt "
#define WAV_DATA_HEADER "data"

typedef struct {
    unsigned int sample_rate;
    unsigned int channels;
    unsigned int bits_per_sample;
    unsigned int data_size;
    double duration;
} WAVInfo;

// WAV函数
static int parse_wav_file(const char* filename, WAVInfo* info) {
    FILE* file = fopen(filename, "rb");
    if (!file) return 0;
    
    memset(info, 0, sizeof(WAVInfo));
    
    // 读取RIFF头
    char riff_header[4];
    if (fread(riff_header, 1, 4, file) != 4) {
        fclose(file);
        return 0;
    }
    
    if (memcmp(riff_header, WAV_RIFF_HEADER, 4) != 0) {
        fclose(file);
        return 0;
    }
    
    // 跳过文件大小
    fseek(file, 4, SEEK_CUR);
    
    // 读取WAVE头
    char wave_header[4];
    if (fread(wave_header, 1, 4, file) != 4) {
        fclose(file);
        return 0;
    }
    
    if (memcmp(wave_header, WAV_WAVE_HEADER, 4) != 0) {
        fclose(file);
        return 0;
    }
    
    // 查找fmt块
    int found_fmt = 0;
    char chunk_header[4];
    unsigned int chunk_size;
    
    while (fread(chunk_header, 1, 4, file) == 4) {
        if (fread(&chunk_size, 4, 1, file) != 1) {
            fclose(file);
            return 0;
        }
        
        if (memcmp(chunk_header, WAV_FMT_HEADER, 4) == 0) {
            found_fmt = 1;
            break;
        }
        
        // 跳过当前块
        fseek(file, chunk_size, SEEK_CUR);
    }
    
    if (!found_fmt) {
        fclose(file);
        return 0;
    }
    
    // 读取fmt块数据
    unsigned short audio_format;
    if (fread(&audio_format, 2, 1, file) != 1) {
        fclose(file);
        return 0;
    }
    
    // 只支持PCM格式
    if (audio_format != 1) {
        fclose(file);
        return 0;
    }
    
    unsigned short num_channels;
    if (fread(&num_channels, 2, 1, file) != 1) {
        fclose(file);
        return 0;
    }
    
    unsigned int sample_rate;
    if (fread(&sample_rate, 4, 1, file) != 1) {
        fclose(file);
        return 0;
    }
    
    // 跳过字节率和块对齐
    fseek(file, 6, SEEK_CUR);
    
    unsigned short bits_per_sample;
    if (fread(&bits_per_sample, 2, 1, file) != 1) {
        fclose(file);
        return 0;
    }
    
    // 查找data块
    fseek(file, chunk_size - 16, SEEK_CUR); // 跳过fmt块的剩余部分
    
    int found_data = 0;
    unsigned int data_size = 0;
    
    while (fread(chunk_header, 1, 4, file) == 4) {
        if (fread(&chunk_size, 4, 1, file) != 1) {
            fclose(file);
            return 0;
        }
        
        if (memcmp(chunk_header, WAV_DATA_HEADER, 4) == 0) {
            found_data = 1;
            data_size = chunk_size;
            break;
        }
        
        // 跳过当前块
        fseek(file, chunk_size, SEEK_CUR);
    }
    
    fclose(file);
    
    if (!found_data || data_size == 0) {
        return 0;
    }
    
    // 计算时长
    info->sample_rate = sample_rate;
    info->channels = num_channels;
    info->bits_per_sample = bits_per_sample;
    info->data_size = data_size;
    
    if (sample_rate > 0 && num_channels > 0 && bits_per_sample > 0) {
        unsigned int bytes_per_sample = bits_per_sample / 8;
        unsigned long long total_samples = data_size / (bytes_per_sample * num_channels);
        info->duration = (double)total_samples / sample_rate;
        return 1;
    }
    
    return 0;
}

// OGG函数
static int read_ogg_page_header(FILE* file, OGGPageHeader* header, long long* data_size) {
    size_t bytes_read;
    unsigned char header_data[27];
    
    bytes_read = fread(header_data, 1, 27, file);
    if (bytes_read < 27) return 0;
    
    if (memcmp(header_data, OGG_PAGE_HEADER, 4) != 0) return 0;
    
    memcpy(header->capture_pattern, header_data, 4);
    header->version = header_data[4];
    header->header_type = header_data[5];
    
    header->granule_position = 0;
    for (int i = 0; i < 8; i++) {
        header->granule_position |= ((unsigned long long)header_data[6 + i]) << (i * 8);
    }
    
    header->bitstream_serial = 0;
    header->page_sequence = 0;
    header->checksum = 0;
    
    for (int i = 0; i < 4; i++) {
        header->bitstream_serial |= ((unsigned int)header_data[14 + i]) << (i * 8);
        header->page_sequence |= ((unsigned int)header_data[18 + i]) << (i * 8);
        header->checksum |= ((unsigned int)header_data[22 + i]) << (i * 8);
    }
    
    header->page_segments = header_data[26];
    
    unsigned char* segment_table = (unsigned char*)malloc(header->page_segments);
    if (!segment_table) return 0;
    
    bytes_read = fread(segment_table, 1, header->page_segments, file);
    if (bytes_read < header->page_segments) {
        free(segment_table);
        return 0;
    }
    
    *data_size = 0;
    for (int i = 0; i < header->page_segments; i++) {
        *data_size += segment_table[i];
    }
    
    free(segment_table);
    return 1;
}

static int find_first_audio_page(FILE* file, unsigned int* sample_rate) {
    long original_pos = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    OGGPageHeader header;
    long long data_size;
    int found = 0;
    
    for (int i = 0; i < 10; i++) {
        if (!read_ogg_page_header(file, &header, &data_size)) break;
        
        long current_pos = ftell(file);
        unsigned char page_data[100];
        size_t read_size = data_size < 100 ? (size_t)data_size : 100;
        
        if (fread(page_data, 1, read_size, file) == read_size) {
            if (memcmp(page_data, "\x01vorbis", 7) == 0 && read_size >= 23) {
                *sample_rate = (unsigned int)page_data[12] | 
                              ((unsigned int)page_data[13] << 8) |
                              ((unsigned int)page_data[14] << 16) |
                              ((unsigned int)page_data[15] << 24);
                found = 1;
                break;
            }
            else if (memcmp(page_data, "OpusHead", 8) == 0 && read_size >= 12) {
                *sample_rate = (unsigned int)page_data[8] | 
                              ((unsigned int)page_data[9] << 8) |
                              ((unsigned int)page_data[10] << 16) |
                              ((unsigned int)page_data[11] << 24);
                found = 1;
                break;
            }
        }
        
        if (data_size > read_size) {
            fseek(file, (long)(data_size - read_size), SEEK_CUR);
        }
    }
    
    fseek(file, original_pos, SEEK_SET);
    return found;
}

// FLAC函数
static int check_flac_signature(FILE* file) {
    unsigned char signature[4];
    size_t bytes_read = fread(signature, 1, 4, file);
    if (bytes_read < 4) return 0;
    return memcmp(signature, FLAC_SIGNATURE, 4) == 0;
}

static void parse_streaminfo_block(FILE* file, FLACInfo* info, unsigned int block_length) {
    if (block_length != 34) return;
    
    unsigned char block_data[34];
    if (fread(block_data, 1, 34, file) != 34) return;
    
    // 解析采样率 (20 bits, 位置 10-12 字节的高20位)
    unsigned long long sample_rate_channels_bps = 0;
    for (int i = 0; i < 8; i++) {
        sample_rate_channels_bps |= ((unsigned long long)block_data[10 + i]) << ((7 - i) * 8);
    }
    
    info->sample_rate = (unsigned int)((sample_rate_channels_bps >> 44) & 0xFFFFF);
    
    // 解析声道数 (3 bits)
    info->channels = (unsigned int)(((sample_rate_channels_bps >> 41) & 0x07) + 1);
    
    // 解析位深度 (5 bits)
    info->bits_per_sample = (unsigned int)(((sample_rate_channels_bps >> 36) & 0x1F) + 1);
    
    // 解析总样本数 (36 bits)
    unsigned long long total_samples_high = (sample_rate_channels_bps >> 4) & 0xFFFFFFFF;
    unsigned long long total_samples_low = sample_rate_channels_bps & 0x0F;
    info->total_samples = (total_samples_high << 4) | total_samples_low;
    
    if (info->sample_rate > 0 && info->total_samples > 0) {
        info->duration = (double)info->total_samples / info->sample_rate;
    }
}

static int parse_flac_metadata(FILE* file, FLACInfo* info) {
    int last_block = 0;
    
    while (!last_block) {
        unsigned char block_header[4];
        if (fread(block_header, 1, 4, file) < 4) break;
        
        unsigned int block_info = 0;
        for (int i = 0; i < 4; i++) {
            block_info = (block_info << 8) | block_header[i];
        }
        
        int is_last = (block_info >> 31) & 0x01;
        int block_type = (block_info >> 24) & 0x7F;
        unsigned int block_length = block_info & 0xFFFFFF;
        
        last_block = (is_last == 1);
        
        if (block_type == 0) {  // STREAMINFO块
            parse_streaminfo_block(file, info, block_length);
            break;  // 找到STREAMINFO后就可以返回了
        } else {
            fseek(file, block_length, SEEK_CUR);
        }
    }
    
    return info->duration > 0;
}

static int parse_flac_file(const char* filename, FLACInfo* info) {
    FILE* file = fopen(filename, "rb");
    if (!file) return 0;
    
    memset(info, 0, sizeof(FLACInfo));
    
    if (!check_flac_signature(file)) {
        fclose(file);
        return 0;
    }
    
    int result = parse_flac_metadata(file, info);
    fclose(file);
    return result;
}

// MP3函数 - 优化版本
static int get_mp3_bitrate(int version, int layer, int index) {
    // MP3比特率表 (单位: kbps)
    int bitrate_table[2][3][16] = {
        { // MPEG Version 1
            {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0}, // Layer 1
            {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},    // Layer 2
            {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}      // Layer 3
        },
        { // MPEG Version 2 & 2.5
            {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0}, // Layer 1
            {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},      // Layer 2
            {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}       // Layer 3
        }
    };
    
    int version_key = (version == 1) ? 0 : 1;
    int layer_key;
    
    if (layer == 1) layer_key = 0;
    else if (layer == 2) layer_key = 1;
    else if (layer == 3) layer_key = 2;
    else return 0;
    
    if (index >= 0 && index < 16) {
        return bitrate_table[version_key][layer_key][index] * 1000; // 转换为bps
    }
    return 0;
}

static int get_mp3_sample_rate(double version, int index) {
    // MP3采样率表
    int sample_rate_table[3][4] = {
        {44100, 48000, 32000, 0},  // MPEG 1
        {22050, 24000, 16000, 0},   // MPEG 2
        {11025, 12000, 8000, 0}     // MPEG 2.5
    };
    
    int version_key;
    if (version == 1.0) version_key = 0;
    else if (version == 2.0) version_key = 1;
    else if (version == 2.5) version_key = 2;
    else return 0;
    
    if (index >= 0 && index < 4) {
        return sample_rate_table[version_key][index];
    }
    return 0;
}

static int get_mp3_frame_size(int layer, int bitrate, int sample_rate, int padding) {
    if (sample_rate == 0) return 0;
    
    if (layer == 1) { // Layer I
        return (12 * bitrate / sample_rate + padding) * 4;
    } else { // Layer II & III
        return 144 * bitrate / sample_rate + padding;
    }
}

static int parse_mp3_header(unsigned char* header_bytes, MP3FrameHeader* header) {
    if (!header_bytes || !header) return 0;
    
    unsigned int header_val = (header_bytes[0] << 24) | (header_bytes[1] << 16) | 
                             (header_bytes[2] << 8) | header_bytes[3];
    
    // 检查同步字
    if ((header_val & 0xFFE00000) != 0xFFE00000) return 0;
    
    // 提取MPEG版本
    int version_bits = (header_val >> 19) & 0x3;
    if (version_bits == 0) header->mpeg_version = 2.5;
    else if (version_bits == 2) header->mpeg_version = 2.0;
    else if (version_bits == 3) header->mpeg_version = 1.0;
    else return 0;
    
    // 提取层
    int layer_bits = (header_val >> 17) & 0x3;
    if (layer_bits == 1) header->layer = 3;
    else if (layer_bits == 2) header->layer = 2;
    else if (layer_bits == 3) header->layer = 1;
    else return 0;
    
    // 保护位
    header->protection = (header_val >> 16) & 0x1;
    
    // 提取比特率
    int bitrate_index = (header_val >> 12) & 0xF;
    header->bitrate = get_mp3_bitrate((int)header->mpeg_version, header->layer, bitrate_index);
    if (header->bitrate == 0) return 0;
    
    // 提取采样率
    int sample_rate_index = (header_val >> 10) & 0x3;
    header->sample_rate = get_mp3_sample_rate(header->mpeg_version, sample_rate_index);
    if (header->sample_rate == 0) return 0;
    
    // 填充位
    header->padding = (header_val >> 9) & 0x1;
    
    // 计算帧大小
    header->frame_size = get_mp3_frame_size(header->layer, header->bitrate, 
                                          header->sample_rate, header->padding);
    
    return header->frame_size > 0;
}

static int skip_id3v2_tag(FILE* file) {
    long original_pos = ftell(file);
    unsigned char header[10];
    
    if (fread(header, 1, 10, file) != 10) {
        fseek(file, original_pos, SEEK_SET);
        return 0;
    }
    
    if (memcmp(header, "ID3", 3) == 0) {
        // 计算ID3v2标签大小
        int size = 0;
        for (int i = 6; i < 10; i++) {
            size = size * 128 + (header[i] & 0x7F);
        }
        fseek(file, size, SEEK_CUR);  // 跳过标签内容
        return 1;
    } else {
        fseek(file, original_pos, SEEK_SET);
        return 0;
    }
}

static int skip_xing_header(FILE* file, int frame_size) {
    long original_pos = ftell(file);
    unsigned char header[12];
    
    if (fread(header, 1, 12, file) != 12) {
        fseek(file, original_pos, SEEK_SET);
        return 0;
    }
    
    // 检查Xing/VBRI头部
    if (memcmp(header, "Xing", 4) == 0 || memcmp(header, "Info", 4) == 0) {
        // 跳过Xing头部剩余部分
        fseek(file, frame_size - 12 - 4, SEEK_CUR);
        return 1;
    } else if (memcmp(header, "VBRI", 4) == 0) {
        // 跳过VBRI头部
        fseek(file, frame_size - 12 - 4, SEEK_CUR);
        return 1;
    }
    
    fseek(file, original_pos, SEEK_SET);
    return 0;
}

static int get_mp3_duration_optimized(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return 0;
    
    // 跳过ID3v2标签
    skip_id3v2_tag(file);
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return 0;
    }
    
    unsigned char buffer[4];
    int total_frames = 0;
    long long total_samples = 0;
    int first_valid_frame = 1;
    int sample_rate = 0;
    int is_vbr = 0;
    int first_bitrate = 0;
    int different_bitrates = 0;
    
    // 采样多个帧来检测VBR
    while (ftell(file) < file_size - 4) {
        long current_pos = ftell(file);
        
        if (fread(buffer, 1, 4, file) != 4) break;
        
        MP3FrameHeader header;
        if (parse_mp3_header(buffer, &header)) {
            total_frames++;
            
            // 记录第一帧的采样率
            if (first_valid_frame) {
                sample_rate = header.sample_rate;
                first_bitrate = header.bitrate;
                first_valid_frame = 0;
            } else {
                // 检查比特率是否变化
                if (header.bitrate != first_bitrate) {
                    different_bitrates = 1;
                    is_vbr = 1;
                }
            }
            
            // 跳过Xing/VBRI头部（VBR文件）
            if (total_frames == 1) {
                skip_xing_header(file, header.frame_size);
            }
            
            // 计算样本数
            int samples_per_frame;
            if (header.layer == 1) {
                samples_per_frame = 384;
            } else if (header.mpeg_version == 1.0) { // MPEG 1
                samples_per_frame = 1152;
            } else { // MPEG 2, 2.5
                samples_per_frame = 576;
            }
            
            total_samples += samples_per_frame;
            
            // 跳过当前帧的剩余部分
            if (header.frame_size > 4) {
                fseek(file, current_pos + header.frame_size, SEEK_SET);
            }
            
            // 采样足够的帧用于分析
            if (total_frames >= 10 && !is_vbr) {
                // 对于CBR文件，我们可以估算总帧数
                if (sample_rate > 0 && first_bitrate > 0) {
                    double avg_frame_size = (double)header.frame_size;
                    double estimated_total_frames = file_size / avg_frame_size;
                    total_samples = (long long)(estimated_total_frames * samples_per_frame);
                }
                break;
            }
        } else {
            // 如果不是有效的帧头，向前移动1字节继续搜索
            fseek(file, current_pos + 1, SEEK_SET);
        }
    }
    
    fclose(file);
    
    // 计算时长
    if (sample_rate > 0 && total_samples > 0) {
        return (int)(total_samples / sample_rate);
    }
    
    return 0;
}

// 统一的音频解析函数
int get_audio_duration(const char* filename) {
    if (!filename) return 0;
    
    // 首先检查文件扩展名来确定类型
    const char* ext = strrchr(filename, '.');
    if (!ext) return 0;
    
    // 尝试MP3格式
    if (strcasecmp(ext, ".mp3") == 0) {
        return get_mp3_duration_optimized(filename);
    }
    // 尝试FLAC格式
    else if (strcasecmp(ext, ".flac") == 0) {
        FLACInfo info;
        if (parse_flac_file(filename, &info)) {
            return (int)info.duration;
        }
    }
    // 尝试OGG格式
    else if (strcasecmp(ext, ".ogg") == 0) {
        FILE* file = fopen(filename, "rb");
        if (!file) return 0;
        
        unsigned int sample_rate = 0;
        if (!find_first_audio_page(file, &sample_rate)) {
            fclose(file);
            return 0;
        }
        
        if (sample_rate == 0) {
            fclose(file);
            return 0;
        }
        
        fseek(file, 0, SEEK_SET);
        OGGPageHeader header;
        long long data_size;
        int first_granule_found = 0;
        long long first_granule = 0;
        long long last_granule = 0;
        
        while (read_ogg_page_header(file, &header, &data_size)) {
            if (!first_granule_found && header.granule_position > 0) {
                first_granule = header.granule_position;
                first_granule_found = 1;
            }
            
            if (header.granule_position > 0) {
                last_granule = header.granule_position;
            }
            
            fseek(file, (long)data_size, SEEK_CUR);
        }
        
        fclose(file);
        
        if (!first_granule_found || last_granule == 0) return 0;
        
        long long total_samples = last_granule - first_granule;
        return (int)(total_samples / sample_rate);
    }
    // 尝试WAV格式
    else if (strcasecmp(ext, ".wav") == 0) {
        WAVInfo info;
        if (parse_wav_file(filename, &info)) {
            return (int)info.duration;
        }
    }
    
    return 0;
}

// 单独的格式检测函数
int get_ogg_duration(const char* filename) {
    if (!filename) return 0;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return 0;
    
    unsigned int sample_rate = 0;
    if (!find_first_audio_page(file, &sample_rate)) {
        fclose(file);
        return 0;
    }
    
    if (sample_rate == 0) {
        fclose(file);
        return 0;
    }
    
    fseek(file, 0, SEEK_SET);
    OGGPageHeader header;
    long long data_size;
    int first_granule_found = 0;
    long long first_granule = 0;
    long long last_granule = 0;
    
    while (read_ogg_page_header(file, &header, &data_size)) {
        if (!first_granule_found && header.granule_position > 0) {
            first_granule = header.granule_position;
            first_granule_found = 1;
        }
        
        if (header.granule_position > 0) {
            last_granule = header.granule_position;
        }
        
        fseek(file, (long)data_size, SEEK_CUR);
    }
    
    fclose(file);
    
    if (!first_granule_found || last_granule == 0) return 0;
    
    long long total_samples = last_granule - first_granule;
    return (int)(total_samples / sample_rate);
}

int get_flac_duration(const char* filename) {
    if (!filename) return 0;
    
    FLACInfo info;
    if (parse_flac_file(filename, &info)) {
        return (int)info.duration;
    }
    return 0;
}

int get_mp3_duration_export(const char* filename) {
    return get_mp3_duration_optimized(filename);
}

int get_wav_duration(const char* filename) {
    if (!filename) return 0;
    
    WAVInfo info;
    if (parse_wav_file(filename, &info)) {
        return (int)info.duration;
    }
    return 0;
}

// 导出所有函数供Python使用
__declspec(dllexport) int GetAudioDuration(const char* filename) {
    return get_audio_duration(filename);
}

__declspec(dllexport) int GetOggDuration(const char* filename) {
    return get_ogg_duration(filename);
}

__declspec(dllexport) int GetFlacDuration(const char* filename) {
    return get_flac_duration(filename);
}

__declspec(dllexport) int GetMp3Duration(const char* filename) {
    return get_mp3_duration_export(filename);
}

__declspec(dllexport) int GetWavDuration(const char* filename) {
    return get_wav_duration(filename);
}