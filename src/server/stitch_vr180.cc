
// File format documentation is here:
// https://developers.google.com/streetview/spherical-metadata
//
// Main take-aways from the documentation:
// - FullWidth and FullHeight are the full 360 degree panorama dimensions.
// - CroppedWidth and CroppedHeight is the actual image content.
// - To get the horizontal FOV, divide CroppedWidth / FullWidth * 360°.
// - To get the vertical FOV, divide CroppedHeight / FullHeight * 180°.
// - The default GPano:ProjectionType is "equirectangular".
//
// The algorithm to stitch a VR180 image:
// - out_width = FullWidth / 2, out_height = FullHeight
// - row_padding: Number of left/right padding pixels if the h-FOV is <180°.
// - row_length: Number of pixels to copy for each row.
// - col_padding: Number of padding rows if the v-FOV is <180°.
//
// Then for stitching, perform:
// - add col_padding rows of black pixels at the top.
// - For each row:
//   - add row_padding pixels left
//   - copy row_length pixels from the input
//   - add row_padding pixels right
// - add col_padding rows of black pixels at the bottom.
//
// The Lenovo Mirage XMP metadata looks like this, 3016x3016 pixels per eye:
//
//   <x:xmpmeta xmlns:x="adobe:ns:meta/" x:xmptk="Adobe XMP">
//     <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
//       <rdf:Description
//           xmlns:GPano="http://ns.google.com/photos/1.0/panorama/"
//           xmlns:GImage="http://ns.google.com/photos/1.0/image/"
//           xmlns:xmpNote="http://ns.adobe.com/xmp/note/"
//           rdf:about=""
//           GPano:CroppedAreaLeftPixels="754"
//           GPano:CroppedAreaTopPixels="0"
//           GPano:CroppedAreaImageWidthPixels="3016"
//           GPano:CroppedAreaImageHeightPixels="3016"
//           GPano:FullPanoWidthPixels="6032"
//           GPano:FullPanoHeightPixels="3016"
//           GPano:InitialViewHeadingDegrees="135"
//           GImage:Mime="image/jpeg"
//           xmpNote:HasExtendedXMP="2964b37b0cca86f1fc96cb89d3b91476"/>
//     </rdf:RDF>
//   </x:xmpmeta>
//
// The Yi Horizon XMP metadata looks like this, 3200x2656 pixels per eye:
//
//   <x:xmpmeta xmlns:x="adobe:ns:meta/" xmptk="Adobe XMP">
//     <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
//       <rdf:Description
//           xmlns:GPano="http://ns.google.com/photos/1.0/panorama/"
//           xmlns:GImage="http://ns.google.com/photos/1.0/image/"
//           xmlns:xmpNote="http://ns.adobe.com/xmp/note/"
//           rdf:about=""
//           GPano:CroppedAreaLeftPixels="1828"
//           GPano:CroppedAreaTopPixels="386"
//           GPano:CroppedAreaImageWidthPixels="3200"
//           GPano:CroppedAreaImageHeightPixels="2656"
//           GPano:FullPanoWidthPixels="6856"
//           GPano:FullPanoHeightPixels="3428"
//           GPano:InitialViewHeadingDegrees="180"
//           GImage:Mime="image/jpeg"
//           xmpNote:HasExtendedXMP="D574B7EB3926B2F1A721D409F07BE4EE"/>
//     </rdf:RDF>
//   </x:xmpmeta>
//
// The Google VR180 XMP metadata (taken with the pano mode) looks like this:
//
//   <x:xmpmeta xmlns:x="adobe:ns:meta/" x:xmptk="Adobe XMP">
//     <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
//       <rdf:Description
//           xmlns:GPano="http://ns.google.com/photos/1.0/panorama/"
//           xmlns:GImage="http://ns.google.com/photos/1.0/image/"
//           xmlns:xmpNote="http://ns.adobe.com/xmp/note/"
//           rdf:about=""
//           GPano:CroppedAreaLeftPixels="0"
//           GPano:CroppedAreaTopPixels="1775"
//           GPano:CroppedAreaImageWidthPixels="3222"
//           GPano:CroppedAreaImageHeightPixels="1700"
//           GPano:FullPanoWidthPixels="9698"
//           GPano:FullPanoHeightPixels="4849"
//           GPano:InitialViewHeadingDegrees="59"
//           GPano:InitialViewPitchDegrees="0"
//           GPano:InitialViewRollDegrees="0"
//           GPano:ProjectionType="equirectangular"
//           GImage:Mime="image/jpeg"
//           xmpNote:HasExtendedXMP="ed88c80b094621ffd6d52cbb990872ca"/>
//     </rdf:RDF>
// </x:xmpmeta>

#include "src/server/stitch_vr180.h"

#include <algorithm>
#include <string_view>

#include "src/server/util/base64.h"
#include "src/server/util/image_io_jpeg.h"
#include "src/server/util/status.h"
#include "src/server/util/status_or.h"
#include "src/server/util/string_utils.h"

namespace server {
namespace {

constexpr std::string_view kCropWidth = "GPano:CroppedAreaImageWidthPixels";
constexpr std::string_view kCropHeight = "GPano:CroppedAreaImageHeightPixels";
constexpr std::string_view kFullWidth = "GPano:FullPanoWidthPixels";
constexpr std::string_view kFullHeight = "GPano:FullPanoHeightPixels";
constexpr std::string_view kRightEyeData = "GImage:Data";

util::StatusOr<std::string_view> GetXmlField(std::string_view xml,
                                             std::string_view field) {
  const size_t pos = xml.find(field);
  if (pos == std::string_view::npos) return util::NotFoundError("field");
  if (xml.size() < pos + field.size() + 2) return util::NotFoundError("size");
  if (xml[pos + field.size()] != '=') return util::NotFoundError("eq");
  if (xml[pos + field.size() + 1] != '"') return util::NotFoundError("q1");
  const size_t start_pos = pos + field.size() + 2;
  const size_t end_pos = xml.find('"', start_pos);
  if (end_pos == std::string_view::npos) return util::NotFoundError("q2");
  return xml.substr(start_pos, end_pos - start_pos);
}

util::StatusOr<int> GetXmlFieldInt(std::string_view xml,
                                   std::string_view field) {
  ASSIGN_OR_RETURN(std::string_view value, GetXmlField(xml, field));
  int int_value;
  if (!util::StrToInt(value, &int_value)) {
    return util::FailedPreconditionError("Invalid integer value");
  }
  return int_value;
}

}  // namespace

util::StatusOr<PhotoSphereMeta> ExtractPhotoSphereMeta(std::string_view xml) {
  PhotoSphereMeta meta;
  ASSIGN_OR_RETURN(meta.crop_width, GetXmlFieldInt(xml, kCropWidth));
  ASSIGN_OR_RETURN(meta.crop_height, GetXmlFieldInt(xml, kCropHeight));
  ASSIGN_OR_RETURN(meta.full_width, GetXmlFieldInt(xml, kFullWidth));
  ASSIGN_OR_RETURN(meta.full_height, GetXmlFieldInt(xml, kFullHeight));
  return meta;
}

util::StatusOr<util::ImageData> StitchVr180(const util::ImageData& image_data) {
  // Extract the relevant XMP metadata.
  ASSIGN_OR_RETURN(const PhotoSphereMeta& meta,
                   ExtractPhotoSphereMeta(image_data.xmp_metadata));

  // Check the image has the expected width/height.
  if (image_data.width != meta.crop_width ||
      image_data.height != meta.crop_height) {
    return util::FailedPreconditionError(
        "Left eye image dimensions do not match XMP metadata");
  }

  // Extract the right eye base64 encoded JPEG from the XMP.
  ASSIGN_OR_RETURN(const std::string_view re_base64,
                   GetXmlField(image_data.xmp_ext_metadata, kRightEyeData));

  // Some JPEG/XMP encoders do not properly pad the base64 data to multiples
  // of 4 bytes. It would be cheaper to ignore invalid padding in the base64
  // decoder. But copying the data here is easier for now.
  size_t new_len = re_base64.size() + (4 - re_base64.size() % 4) % 4;
  std::string re_padded_base64(new_len, '=');
  std::copy_n(re_base64.data(), re_base64.size(), re_padded_base64.data());

  // Decode the right eye base64 data.
  ASSIGN_OR_RETURN(const std::string re_jpeg,
                   util::Base64Decode(re_padded_base64));

  // Decode the right eye JPEG data.
  ASSIGN_OR_RETURN(
      const util::ImageData re_image,
      util::JpegRead(util::JpegReadOptions(), re_jpeg.data(), re_jpeg.size()));

  // Check the right eye image has the expected width/height.
  if (re_image.width != meta.crop_width ||
      re_image.height != meta.crop_height) {
    return util::FailedPreconditionError(
        "Right eye image dimensions do not match XMP metadata");
  }

  // Compute the output image dimensions.
  const int channels = image_data.channels;
  const int eye_width = meta.full_width / 2;
  const int eye_height = meta.full_height;
  const int img_width = eye_width * 2;
  const int img_height = eye_height;
  const int row_padding = std::max(0, (eye_width - meta.crop_width) / 2);
  const int col_padding = std::max(0, (eye_height - meta.crop_height) / 2);
  const int row_width = std::min(meta.crop_width, eye_width);
  const int row_offset = std::max(0, (meta.crop_width - eye_width) / 2);

  const int img_row_stride = img_width * channels;
  const int eye_row_stride = eye_width * channels;
  const int in_row_stride = image_data.width * channels;

  util::ImageData output;
  output.width = img_width;
  output.height = img_height;
  output.channels = channels;
  output.data.resize(img_width * img_height * channels);

  // First, add top padding rows of black pixels.
  for (int y = 0; y < col_padding; ++y) {
    uint8_t* row = &output.data[y * img_row_stride];
    std::fill_n(row, img_row_stride, 0);
  }

  // Copy the left eye image data.
  for (int y = 0; y < image_data.height; ++y) {
    uint8_t* out_row = &output.data[(col_padding + y) * img_row_stride];
    std::fill_n(out_row, row_padding * channels, 0);
    std::copy_n(&image_data.data[y * in_row_stride + row_offset * channels],
                row_width * channels, out_row + row_padding * channels);
    std::fill_n(out_row + (row_padding + row_width) * channels,
                row_padding * channels, 0);
  }

  // Copy the right eye image data.
  for (int y = 0; y < re_image.height; ++y) {
    uint8_t* out_row =
        &output.data[(col_padding + y) * img_row_stride + eye_row_stride];
    std::fill_n(out_row, row_padding * channels, 0);
    std::copy_n(&re_image.data[y * in_row_stride + row_offset * channels],
                row_width * channels, out_row + row_padding * channels);
    std::fill_n(out_row + (row_padding + row_width) * channels,
                row_padding * channels, 0);
  }

  // Finally, add bottom padding rows of black pixels.
  for (int y = col_padding + meta.crop_height; y < img_height; ++y) {
    uint8_t* row = &output.data[y * img_row_stride];
    std::fill_n(row, img_row_stride, 0);
  }

  return output;
}

}  // namespace server
