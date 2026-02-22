
#include "src/server/stitch_vr180.h"

#include <string_view>

#include "src/server/test/tinytest.h"
#include "src/server/util/status_or.h"

namespace server {

TEST(StitchVr180, ExtractPhotoSphereMeta) {
  const std::string_view xml = R"(
      <x:xmpmeta xmlns:x="adobe:ns:meta/" xmptk="Adobe XMP">
        <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
          <rdf:Description
              xmlns:GPano="http://ns.google.com/photos/1.0/panorama/"
              xmlns:GImage="http://ns.google.com/photos/1.0/image/"
              xmlns:xmpNote="http://ns.adobe.com/xmp/note/"
              rdf:about=""
              GPano:CroppedAreaLeftPixels="1828"
              GPano:CroppedAreaTopPixels="386"
              GPano:CroppedAreaImageWidthPixels="3200"
              GPano:CroppedAreaImageHeightPixels="2656"
              GPano:FullPanoWidthPixels="6856"
              GPano:FullPanoHeightPixels="3428"
              GPano:InitialViewHeadingDegrees="180"
              GImage:Mime="image/jpeg"
              xmpNote:HasExtendedXMP="D574B7EB3926B2F1A721D409F07BE4EE"/>
        </rdf:RDF>
      </x:xmpmeta>
  )";

  util::StatusOr<PhotoSphereMeta> meta = ExtractPhotoSphereMeta(xml);
  ASSERT_TRUE(meta.ok());
  EXPECT_EQ(meta->crop_width, 3200);
  EXPECT_EQ(meta->crop_height, 2656);
  EXPECT_EQ(meta->full_width, 6856);
  EXPECT_EQ(meta->full_height, 3428);
}

}  // namespace server
