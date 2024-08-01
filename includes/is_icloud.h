#ifndef IS_ICLOUD_FILE_H
#define IS_ICLOUD_FILE_H

#include <stdbool.h>

/**
 * @brief Check if a file is stored in iCloud. If yes the file is not fully downloaded on the device.
 * In this case, if the download has not started yet, the file will be downloaded on the device and the
 * process will be blocked until the download is complete.
 *
 * @param filePath The path to the file.
 * @return true if the file is stored in iCloud, false otherwise.
 */
bool is_icloud_file(const char *path);

#endif // IS_ICLOUD_FILE_Ha