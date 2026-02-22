#ifndef SRC_SERVER_FILE_UTILS_H_
#define SRC_SERVER_FILE_UTILS_H_

#include <filesystem>

namespace server {

// Returns true if the file extension is a recognized image format.
bool IsImageFile(const std::filesystem::directory_entry& entry);

// Returns true if the file extension is a recognized video format.
bool IsVideoFile(const std::filesystem::directory_entry& entry);

// Returns true if the file is either an image or video file.
bool IsMediaFile(const std::filesystem::directory_entry& entry);

}  // namespace server

#endif  // SRC_SERVER_FILE_UTILS_H_
