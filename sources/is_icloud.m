#include "is_icloud.h"
#import <Foundation/Foundation.h>

bool is_icloud_file(const char *filePath) {
    @autoreleasepool {
        NSURL *fileURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:filePath]];
        NSError *error = nil;
        
        NSNumber *isUbiquitous = nil;
        [fileURL getResourceValue:&isUbiquitous forKey:NSURLIsUbiquitousItemKey error:&error];

        if (error) {
            NSLog(@"Error getting file attributes: %@", error);
            return false;
        }

        if (isUbiquitous != nil && [isUbiquitous boolValue]) {
            NSString *downloadingStatus = nil;
            [fileURL getResourceValue:&downloadingStatus forKey:NSURLUbiquitousItemDownloadingStatusKey error:&error];

            if (error) {
                NSLog(@"Error getting file attributes: %@", error);
                return false;
            }

            return ![downloadingStatus isEqualToString:NSURLUbiquitousItemDownloadingStatusCurrent];
        }
        
        return false;
    }
}
