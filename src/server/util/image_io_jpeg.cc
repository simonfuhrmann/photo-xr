#include "src/server/util/image_io_jpeg.h"

#include <jerror.h>
#include <jpeglib.h>

#include <algorithm>
#include <csetjmp>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <string_view>

#include "src/server/util/status.h"
#include "src/server/util/status_or.h"

namespace util {
namespace {

constexpr size_t kMaxXmpSize = 100 * 1024 * 1024;  // 100 MiB

struct JpegErrorManager {
  jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

struct JpegIStreamSource {
  jpeg_source_mgr pub;
  std::istream* stream = nullptr;
  JOCTET buffer[4096];
  bool start_of_file = true;
};

void JpegErrorExit(j_common_ptr cinfo) {
  JpegErrorManager* err = reinterpret_cast<JpegErrorManager*>(cinfo->err);
  longjmp(err->setjmp_buffer, 1);
}

void IstreamSourceInit(j_decompress_ptr cinfo) {
  JpegIStreamSource* src = reinterpret_cast<JpegIStreamSource*>(cinfo->src);
  src->start_of_file = true;
}

boolean IstreamFillInputBuffer(j_decompress_ptr cinfo) {
  JpegIStreamSource* src = reinterpret_cast<JpegIStreamSource*>(cinfo->src);

  src->stream->read(reinterpret_cast<char*>(src->buffer), sizeof(src->buffer));
  if (!src->stream->good() && !src->stream->eof()) {
    ERREXIT(cinfo, JERR_FILE_READ);
  }
  std::streamsize bytes_read = src->stream->gcount();

  // On istream read error, insert EOI marker to allow partial decoding.
  if (bytes_read <= 0) {
    if (src->start_of_file) {
      ERREXIT(cinfo, JERR_INPUT_EMPTY);
    }
    src->buffer[0] = (JOCTET)0xFF;
    src->buffer[1] = (JOCTET)JPEG_EOI;
    bytes_read = 2;
  }

  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = bytes_read;
  src->start_of_file = false;
  return TRUE;
}

void IstreamSkipInputData(j_decompress_ptr cinfo, long num_bytes) {
  if (num_bytes <= 0) return;
  JpegIStreamSource* src = reinterpret_cast<JpegIStreamSource*>(cinfo->src);

  // Skip what's left in the buffer first.
  if (num_bytes <= (long)src->pub.bytes_in_buffer) {
    src->pub.next_input_byte += num_bytes;
    src->pub.bytes_in_buffer -= num_bytes;
  } else {
    // Skip remaining bytes directly from stream.
    long to_skip = num_bytes - src->pub.bytes_in_buffer;
    src->stream->ignore(static_cast<std::streamsize>(to_skip));
    src->pub.next_input_byte = nullptr;
    src->pub.bytes_in_buffer = 0;
  }
}

void TermSource(j_decompress_ptr cinfo) {
  // Nothing to do.
}

// Sets up a JPEG source for std::istream.
void JpegSrcIstream(j_decompress_ptr cinfo, std::istream& input) {
  if (cinfo->src == nullptr) {
    cinfo->src = (jpeg_source_mgr*)(*cinfo->mem->alloc_small)(
        (j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(JpegIStreamSource));
  }

  JpegIStreamSource* src = reinterpret_cast<JpegIStreamSource*>(cinfo->src);
  src->stream = &input;
  src->pub.init_source = IstreamSourceInit;
  src->pub.fill_input_buffer = IstreamFillInputBuffer;
  src->pub.skip_input_data = IstreamSkipInputData;
  src->pub.resync_to_restart = jpeg_resync_to_restart;  // default
  src->pub.term_source = TermSource;
  src->pub.bytes_in_buffer = 0;
  src->pub.next_input_byte = nullptr;
}

uint32_t ReadBigEndian(const uint8_t* data) {
  return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

// Reads XMP metadata from the JPEG APP1 markers. Returns both the base XMP
// and the concatenated exteended XMP. There are a few known flaws:
//
// - Extended XMP is assembled purely from offset and length, but it does not
//   check for gaps or overlaps. Gaps can lead to null data (\0 bytes), and
//   overlaps to last-writer-wins corruption.
// - If multiple base XMP packets exist, they overwrite each other. The XMP spec
//   says there should be only one. But validation is not implemented.
// - The base XMP is supposed to declare xmpNote:HasExtendedXMP="GUID".
//   Instead, the GUID is read from the first extended XMP chunk. This is
//   robust, but lacks validation.
// - If the extended XMP is supposed to _continue_ the base XMP, i.e., starts
//   with an offset >0, this is not handled, and the extended XMP will have
//   null data (\0 bytes) in the beginning.
util::StatusOr<ImageData> ReadXmpMetadata(jpeg_saved_marker_ptr marker_list) {
  // The XMP prefixes. Note: The char arrays include the null terminator, and
  // the sizeof() operator INCLUDES the null terminator as well. This is
  // intentional, to allow memcmp() to check the null terminator as well.
  constexpr char kXmpPrefix[] = "http://ns.adobe.com/xap/1.0/";
  constexpr size_t kXmpPrefixLen = sizeof(kXmpPrefix);
  constexpr char kXmpExtPrefix[] = "http://ns.adobe.com/xmp/extension/";
  constexpr size_t kXmpExtPrefixLen = sizeof(kXmpExtPrefix);

  struct ExtendedChunk {
    uint32_t offset;
    std::vector<uint8_t> data;
  };

  std::string base_xmp;
  std::string extended_guid;
  uint32_t extended_full_length = 0;
  std::vector<ExtendedChunk> extended_chunks;

  for (jpeg_saved_marker_ptr marker = marker_list; marker != nullptr;
       marker = marker->next) {
    // Skip all markers that are not APP1.
    if (marker->marker != JPEG_APP0 + 1) continue;

    // Check if this is the base XMP segment.
    if (marker->data_length >= kXmpPrefixLen &&
        memcmp(marker->data, kXmpPrefix, kXmpPrefixLen) == 0) {
      const char* xmp_data =
          reinterpret_cast<const char*>(marker->data + kXmpPrefixLen);

      const size_t xmp_len = marker->data_length - kXmpPrefixLen;
      base_xmp.assign(xmp_data, xmp_len);
    }

    // Check if this is an extended XMP segment.
    if (marker->data_length >= kXmpExtPrefixLen + 32 + 8 &&
        memcmp(marker->data, kXmpExtPrefix, kXmpExtPrefixLen) == 0) {
      // On the first extended XMP segment, reserve the chunk vector.
      if (extended_chunks.empty()) {
        extended_chunks.reserve(64);
      }

      // Read the GUID.
      const uint8_t* ptr = marker->data + kXmpExtPrefixLen;
      std::string guid(reinterpret_cast<const char*>(ptr), 32);
      ptr += 32;

      // Read the length and offset (stored in big endian).
      const size_t marker_len = static_cast<size_t>(marker->data_length);
      const size_t chunk_size = marker_len - kXmpExtPrefixLen - 32 - 8;
      const uint32_t full_len = ReadBigEndian(ptr + 0);
      const uint32_t offset = ReadBigEndian(ptr + 4);
      ptr += 8;

      if (full_len > kMaxXmpSize) {
        return util::FailedPreconditionError("XMP is too large");
      }

      if (extended_guid.empty()) {
        extended_guid = guid;
        extended_full_length = full_len;
      }

      if (guid == extended_guid) {
        // Each extended chunk repeats the same full extended length.
        // Detect inconsistencies.
        if (full_len != extended_full_length) {
          return util::FailedPreconditionError("Inconsistent full length");
        }
        ExtendedChunk chunk;
        chunk.offset = offset;
        chunk.data.assign(ptr, ptr + chunk_size);
        extended_chunks.push_back(std::move(chunk));
      }
    }
  }

  // Return the base XMP and the extened XMP separately.
  // The spec says: "Extended XMP replaces the base XMP if too large."
  // This is often disregarded, and the base XMP contains data not present
  // in the extended XMP.
  ImageData image_data;
  image_data.xmp_metadata = std::move(base_xmp);

  // Concatenate all extended chunks.
  // TODO: Add more checks for gaps and overlaps.
  image_data.xmp_ext_metadata.resize(extended_full_length, '\0');
  char* ptr = image_data.xmp_ext_metadata.data();
  for (const ExtendedChunk& chunk : extended_chunks) {
    if (chunk.offset + chunk.data.size() > extended_full_length) {
      return util::FailedPreconditionError("Chunk offset + size too large");
    }
    std::copy_n(chunk.data.data(), chunk.data.size(), ptr + chunk.offset);
  }

  return image_data;
}

util::StatusOr<ImageData> JpegReadInternal(
    const JpegReadOptions& options,
    std::function<void(jpeg_decompress_struct*)> set_jpeg_src) {
  // Declare C++ structs before `setjmp` is called to ensure destruction in
  // case of jpeglib errors.
  ImageData xmp_metadata;
  ImageData image_data;

  // Create JPEG structs and set up error handling.
  jpeg_decompress_struct cinfo{};
  JpegErrorManager jerr;
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = JpegErrorExit;

  if (setjmp(jerr.setjmp_buffer)) {
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo.err->format_message)((j_common_ptr)&cinfo, buffer);
    jpeg_destroy_decompress(&cinfo);
    return util::InternalError(buffer);
  }

  jpeg_create_decompress(&cinfo);
  set_jpeg_src(&cinfo);

  // Request to save APP1 (XMP) metadata if requested.
  if (options.include_xmp_data) {
    jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xFFFF);
  }

  // Read the JPEG header.
  jpeg_read_header(&cinfo, TRUE);

  // Read the XMP data if requested. Iterate all APP1 markers. XMP can be split
  // across multiple APP1 segments. The first segment starts with kXmpPrefix,
  // and may contain "extended chunks" identified using a GUID. Consecutive
  // segments use kXmpExtPrefix with the same GUID.
  if (options.include_xmp_data) {
    util::StatusOr<ImageData> xmp = ReadXmpMetadata(cinfo.marker_list);
    if (xmp.ok()) {
      xmp_metadata = std::move(*xmp);
    }
  }

  // Start JPEG decompression and initialize the output struct.
  // This may discard markers and data. Read XMP before this.
  jpeg_start_decompress(&cinfo);

  image_data.width = cinfo.output_width;
  image_data.height = cinfo.output_height;
  image_data.channels = cinfo.output_components;
  const int row_stride = cinfo.output_width * cinfo.output_components;
  if (options.include_image_data) {
    image_data.data.resize(row_stride * cinfo.output_height);
  }
  image_data.xmp_metadata = std::move(xmp_metadata.xmp_metadata);
  image_data.xmp_ext_metadata = std::move(xmp_metadata.xmp_ext_metadata);

  // Read scanlines.
  if (options.include_image_data) {
    while (cinfo.output_scanline < cinfo.output_height) {
      JSAMPROW row = &image_data.data[cinfo.output_scanline * row_stride];
      jpeg_read_scanlines(&cinfo, &row, 1);
    }
    jpeg_finish_decompress(&cinfo);
  } else {
    jpeg_abort_decompress(&cinfo);
  }
  jpeg_destroy_decompress(&cinfo);

  return image_data;
}

}  // namespace

util::StatusOr<ImageData> JpegRead(const JpegReadOptions& options,
                                   std::string_view filename) {
  std::ifstream in(std::string(filename), std::ios::binary);
  if (!in.good()) {
    return util::UnavailableError(util::StrCat("Cannot open ", filename));
  }
  return JpegRead(options, in);
}

util::StatusOr<ImageData> JpegRead(const JpegReadOptions& options,
                                   const char* data, size_t size) {
  const auto set_jpeg_src = [&](jpeg_decompress_struct* cinfo) {
    jpeg_mem_src(cinfo,
                 reinterpret_cast<const unsigned char*>(data),
                 static_cast<unsigned long>(size));
  };
  return JpegReadInternal(options, set_jpeg_src);
}

util::StatusOr<ImageData> JpegRead(const JpegReadOptions& options,
                                   std::istream& input) {
  const auto set_jpeg_src = [&](jpeg_decompress_struct* cinfo) {
    JpegSrcIstream(cinfo, input);
  };
  return JpegReadInternal(options, set_jpeg_src);
}

}  // namespace util
